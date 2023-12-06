//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ILocalize.h"
#include "core/IConsoleCommands.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"
#include "IEqUI_Control.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

#include "render/IDebugOverlay.h"
#include "font/IFontCache.h"
#include "EqUI_Manager.h"
#include "EqUI_Panel.h"


namespace equi
{

IUIControl::IUIControl()
	: m_visible(true), m_selfVisible(true), m_enabled(true), m_parent(nullptr),
	m_sizeDiff(0), m_sizeDiffPerc(1.0f),
	m_position(0),m_size(25),
	m_scaling(UI_SCALING_NONE), m_anchors(0), m_alignment(UI_ALIGN_LEFT | UI_ALIGN_TOP)
{
	m_label = "Control";

	m_transform.rotation = 0.0f;
	m_transform.translation = 0.0f;
	m_transform.scale = 1.0f;
}

IUIControl::~IUIControl()
{
	ClearChilds(true);
}

const char*	IUIControl::GetLabel() const
{
	return _Es(m_label).ToCString();
}

void IUIControl::SetLabel(const char* pszLabel)
{
	m_label = LocalizedString(pszLabel);
}

const wchar_t* IUIControl::GetLabelText() const
{
	return m_label;
}

void IUIControl::SetLabelText(const wchar_t* pszLabel)
{
	m_label = pszLabel;
}

void IUIControl::InitFromKeyValues( KVSection* sec, bool noClear )
{
	if (!noClear)
		ClearChilds(true);

	if(!stricmp(sec->GetName(), "child"))
		SetName(KV_GetValueString(sec, 1, ""));
	else
		SetName(KV_GetValueString(sec, 0, ""));

	KVSection* label = sec->FindSection("label");
	if (label)
		SetLabel(KV_GetValueString(sec->FindSection("label")));

	m_position = KV_GetIVector2D(sec->FindSection("position"), 0, m_position);
	m_size = KV_GetIVector2D(sec->FindSection("size"), 0, m_size);
	m_sizeReal = m_size;

	m_visible = KV_GetValueBool(sec->FindSection("visible"), 0, m_visible);
	m_selfVisible = KV_GetValueBool(sec->FindSection("selfvisible"), 0, m_selfVisible);
	
	m_sizeDiff = 0;
	m_sizeDiffPerc = 1.0f;
	m_anchors = 0;
	m_alignment = (UI_BORDER_LEFT | UI_BORDER_TOP);

	m_font.font = nullptr;
	KVSection* font = sec->FindSection("font");
	if(font)
	{
		int styleFlags = 0;

		for (int i = 1; i < font->values.numElem(); i++)
		{
			if (!stricmp(KV_GetValueString(font, i), "bold"))
				styleFlags |= TEXT_STYLE_BOLD;
			else if (!stricmp(KV_GetValueString(font, i), "italic"))
				styleFlags |= TEXT_STYLE_ITALIC;
		}

		m_font.font = g_fontCache->GetFont(KV_GetValueString(font), KV_GetValueInt(font, 1, 20), styleFlags, false);
	}

	

	m_font.fontScale = KV_GetVector2D(sec->FindSection("fontScale"), 0, m_parent ? m_parent->m_font.fontScale : m_font.fontScale);
	m_font.textColor = KV_GetVector4D(sec->FindSection("textColor"), 0, m_parent ? m_parent->m_font.textColor : m_font.textColor);
	m_font.monoSpace = KV_GetValueBool(sec->FindSection("textMonospace"), 0, m_parent ? m_parent->m_font.monoSpace : m_font.monoSpace);
	m_font.textWeight = KV_GetValueFloat(sec->FindSection("textWeight"), 0, m_parent ? m_parent->m_font.textWeight : m_font.textWeight);

	m_font.shadowColor = KV_GetVector4D(sec->FindSection("textShadowColor"), 0, m_parent ? m_parent->m_font.shadowColor : m_font.shadowColor);
	m_font.shadowOffset = KV_GetValueFloat(sec->FindSection("textShadowOffset"), 0, m_parent ? m_parent->m_font.shadowOffset : m_font.shadowOffset);
	m_font.shadowWeight = KV_GetValueFloat(sec->FindSection("textShadowWeight"), 0, m_parent ? m_parent->m_font.shadowWeight : m_font.shadowWeight);

	KVSection* command = sec->FindSection("command");

	if(command)
	{
		// NOTE: command event always have UID == 0
		ui_event evt("command", CommandCb);

		for(int i = 0; i < command->values.numElem(); i++)
			evt.args.append( KV_GetValueString(command, i) );

		m_eventCallbacks.append(evt);
	}

	//------------------------------------------------------------------------------
	KVSection* anchors = sec->FindSection("anchors");

	if(anchors)
	{
		m_anchors = 0;

		for(int i = 0; i < anchors->values.numElem(); i++)
		{
			const char* anchorVal = KV_GetValueString(anchors, i);

			if(!stricmp("left", anchorVal))
				m_anchors |= UI_BORDER_LEFT;
			else if(!stricmp("top", anchorVal))
				m_anchors |= UI_BORDER_TOP;
			else if(!stricmp("right", anchorVal))
				m_anchors |= UI_BORDER_RIGHT;
			else if(!stricmp("bottom", anchorVal))
				m_anchors |= UI_BORDER_BOTTOM;
			else if (!stricmp("all", anchorVal))
				m_anchors = (UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM);
		}
	}

	//------------------------------------------------------------------------------
	KVSection* align = sec->FindSection("align");

	if(align)
	{
		m_alignment = 0;

		for(int i = 0; i < align->values.numElem(); i++)
		{
			const char* alignVal = KV_GetValueString(align, i);

			if(!stricmp("left", alignVal))
				m_alignment |= UI_ALIGN_LEFT;
			else if(!stricmp("top", alignVal))
				m_alignment |= UI_ALIGN_TOP;
			else if(!stricmp("right", alignVal))
				m_alignment |= UI_ALIGN_RIGHT;
			else if(!stricmp("bottom", alignVal))
				m_alignment |= UI_ALIGN_BOTTOM;
			else if (!stricmp("hcenter", alignVal))
				m_alignment |= UI_ALIGN_HCENTER;
			else if (!stricmp("vcenter", alignVal))
				m_alignment |= UI_ALIGN_VCENTER;
		}
	}

	//------------------------------------------------------------------------------
	KVSection* transform = sec->FindSection("transform");

	if (transform)
	{
		float rotateVal = KV_GetValueFloat(transform->FindSection("rotate"), 0.0f);
		rotateVal = rotateVal;

		Vector2D scaleVal = KV_GetVector2D(transform->FindSection("scale"), 0, 1.0f);
		Vector2D translate = KV_GetVector2D(transform->FindSection("translate"), 0, 0.0f);

		SetTransform(translate, scaleVal, rotateVal);
	}

	//------------------------------------------------------------------------------
	KVSection* textAlign = sec->FindSection("textAlign");

	if (textAlign)
	{
		m_font.textAlignment = 0;

		for (int i = 0; i < textAlign->values.numElem(); i++)
		{
			const char* alignVal = KV_GetValueString(textAlign, i);

			if (!stricmp("left", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_LEFT;
			else if (!stricmp("top", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_TOP;
			else if (!stricmp("right", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_RIGHT;
			else if (!stricmp("bottom", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_BOTTOM;
			else if (!stricmp("vcenter", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_VCENTER;
			else if (!stricmp("hcenter", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_HCENTER;
			else if (!stricmp("center", alignVal))
				m_font.textAlignment |= TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER;
		}
	}

	//------------------------------------------------------------------------------


	KVSection* scaling = sec->FindSection("scaling");
	if (scaling)
	{
		m_scaling = UI_SCALING_NONE;

		const char* scalingValue = KV_GetValueString(scaling, 0, "none");

		if (!stricmp("aspectw", scalingValue))
			m_scaling = UI_SCALING_ASPECT_W;
		else if (!stricmp("aspecth", scalingValue) || !stricmp("uniform", scalingValue))
			m_scaling = UI_SCALING_ASPECT_H;
		else if (!stricmp("inherit", scalingValue))
			m_scaling = UI_SCALING_INHERIT;
		else if (!stricmp("inherit_min", scalingValue))
			m_scaling = UI_SCALING_INHERIT_MIN;
		else if (!stricmp("inheri_tmax", scalingValue))
			m_scaling = UI_SCALING_INHERIT_MAX;
		else if (!stricmp("aspect_min", scalingValue))
			m_scaling = UI_SCALING_ASPECT_MIN;
		else if (!stricmp("aspect_max", scalingValue))
			m_scaling = UI_SCALING_ASPECT_MAX;
	}

	// walk for childs
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		KVSection* csec = sec->keys[i];

		if (!csec->IsSection())
			continue;
	
		// INIT CHILD CONTROLS
		if(!stricmp(csec->GetName(), "child"))
		{
			const char* childClass = KV_GetValueString(csec, 0, "InvalidClass");
			const char* childName = KV_GetValueString(csec, 1, "Invalid");

			// try find existing child
			IUIControl* control = FindChild(childName);

			// if nothing, create new one
			if(!control || control && stricmp(control->GetClassname(), childClass))
			{
				if (control) // replace children if it has different class
					RemoveChild(control);

				control = equi::Manager->CreateElement( childClass );
				AddChild( control );
			}

			// if still no luck (wrong class name), we abort
			if(!control)
				continue;

			control->InitFromKeyValues(csec, noClear);
		}
	}
}

void IUIControl::SetSize(const IVector2D &size)
{
	m_size = size;

	m_sizeDiff = m_size - m_sizeReal;
	m_sizeDiffPerc = Vector2D(m_size) / Vector2D(m_sizeReal);
}

void IUIControl::SetPosition(const IVector2D &pos)
{
	m_position = pos;
}

void IUIControl::SetRectangle(const IAARectangle& rect)
{
	SetPosition(rect.leftTop);
	SetSize(rect.rightBottom - m_position);
}

// sets new transformation. Set all zeros to reset
void IUIControl::SetTransform(const Vector2D& translateVal, const Vector2D& scaleVal, float rotateVal)
{
	m_transform.translation = translateVal;
	m_transform.scale = scaleVal;
	m_transform.rotation = rotateVal;
}

bool IUIControl::IsVisible() const
{
	if(m_parent)
		return m_parent->IsVisible() && m_visible;

	return m_visible;
}

bool IUIControl::IsEnabled() const
{
	if(m_parent)
		return m_parent->IsEnabled() && m_enabled;

	return m_enabled;
}

const IVector2D& IUIControl::GetSize() const
{
	return m_size;
}

const IVector2D& IUIControl::GetPosition() const
{
	return m_position;
}

IAARectangle IUIControl::GetRectangle() const
{
	return IAARectangle(m_position, m_position + m_size);
}

void IUIControl::ResetSizeDiffs()
{
	m_sizeDiff = 0;
	m_sizeDiffPerc = 1.0f;
}

const Vector2D& IUIControl::GetSizeDiff() const 
{
	return m_sizeDiff;
}

const Vector2D& IUIControl::GetSizeDiffPerc() const
{
	return m_sizeDiffPerc;
}

Vector2D IUIControl::CalcScaling() const
{
	if(!m_parent)
		return Vector2D(1.0f, 1.0f);

	Vector2D scale(m_parent->m_sizeDiffPerc);

	if(Manager->GetRootPanel() != m_parent)
	{
		Vector2D parentScaling = m_parent->CalcScaling();

		switch (m_scaling)
		{
			case UI_SCALING_INHERIT_MIN:
			{
				parentScaling = Vector2D(min(parentScaling.x, parentScaling.y));
				break;
			}
			case UI_SCALING_INHERIT_MAX:
			{
				parentScaling = Vector2D(max(parentScaling.x, parentScaling.y));
				break;
			}
			case UI_SCALING_ASPECT_W:
			{
				const float aspectCorrection = scale.x / scale.y;
				scale.y *= aspectCorrection;
				break;
			}
			case UI_SCALING_ASPECT_H:
			{
				const float aspectCorrection = scale.y / scale.x;
				scale.x *= aspectCorrection;
				break;
			}
			case UI_SCALING_ASPECT_MIN:
			{
				const float aspectCorrectionW = scale.x / scale.y;
				const float aspectCorrectionH = scale.y / scale.x;

				if (aspectCorrectionW < aspectCorrectionH)
					scale.x *= aspectCorrectionH;
				else
					scale.y *= aspectCorrectionW;
				break;
			}
			case UI_SCALING_ASPECT_MAX:
			{
				const float aspectCorrectionW = scale.x / scale.y;
				const float aspectCorrectionH = scale.y / scale.x;

				if (aspectCorrectionW > aspectCorrectionH)
					scale.x *= aspectCorrectionH;
				else
					scale.y *= aspectCorrectionW;
				break;
			}
		}

		return scale * parentScaling;
	}

	return Vector2D(1.0f, 1.0f);
}

IAARectangle IUIControl::GetClientRectangle() const
{
	const Vector2D scale = CalcScaling();

	const IVector2D scaledSize(m_size * scale);
	const IVector2D scaledPos(m_position * scale);

	IAARectangle thisRect(scaledPos, scaledPos + scaledSize);

	if(m_parent)
	{
		// move by anchor border
		if (m_anchors > 0)
		{
			const IVector2D parentSizeDiff = m_parent->m_sizeDiff;

			const IVector2D offsetAnchorsLT((m_anchors & UI_BORDER_LEFT) > 0, (m_anchors & UI_BORDER_TOP) > 0);
			const IVector2D offsetAnchorsRB((m_anchors & UI_BORDER_RIGHT) > 0, (m_anchors & UI_BORDER_BOTTOM) > 0);

			const Vector2D anchorSizeLT = parentSizeDiff * offsetAnchorsLT;
			const Vector2D anchorSizeRB = parentSizeDiff * offsetAnchorsRB;

			// apply offset of each bound based on anchors
			// all anchors enabled will just stretch elements
			thisRect.leftTop += anchorSizeRB - anchorSizeLT;
			thisRect.rightBottom += anchorSizeRB;
		}

		const IAARectangle parentRect = m_parent->GetClientRectangle();

		// compute alignment to the parent client rectangle
		if(m_alignment & UI_ALIGN_LEFT)
		{
			thisRect.leftTop.x += parentRect.leftTop.x;
			thisRect.rightBottom.x += parentRect.leftTop.x;
		} 
		else if(m_alignment & UI_ALIGN_RIGHT)
		{
			thisRect.leftTop.x += parentRect.rightBottom.x - scaledSize.x - scaledPos.x * 2;
			thisRect.rightBottom.x += parentRect.rightBottom.x - scaledSize.x - scaledPos.x * 2;
		}
		else if (m_alignment & UI_ALIGN_HCENTER)
		{
			const IVector2D center = parentRect.GetCenter();
			thisRect.leftTop.x += center.x - scaledSize.x / 2;
			thisRect.rightBottom.x += center.x - scaledSize.x / 2;
		}

		if (m_alignment & UI_ALIGN_TOP)
		{
			thisRect.leftTop.y += parentRect.leftTop.y;
			thisRect.rightBottom.y += parentRect.leftTop.y;
		}
		else if(m_alignment & UI_ALIGN_BOTTOM)
		{
			thisRect.leftTop.y += parentRect.rightBottom.y - scaledSize.y - scaledPos.y * 2;
			thisRect.rightBottom.y += parentRect.rightBottom.y - scaledSize.y - scaledPos.y * 2;
		}
		else if (m_alignment & UI_ALIGN_VCENTER)
		{
			const IVector2D center = parentRect.GetCenter();
			thisRect.leftTop.y += center.y - scaledSize.y / 2;
			thisRect.rightBottom.y += center.y - scaledSize.y / 2;
		}
	}

	return thisRect;
}

IEqFont* IUIControl::GetFont() const
{
	if(!m_font.font)
	{
		if( m_parent )
			return m_parent->GetFont();
		else
			return equi::Manager->GetDefaultFont();
	}

	return m_font.font;
}

void IUIControl::GetCalcFontStyle(eqFontStyleParam_t& style) const
{
	style.styleFlag |= TEXT_STYLE_SCISSOR | TEXT_STYLE_USE_TAGS | (m_font.monoSpace ? TEXT_STYLE_MONOSPACE : 0);
	style.align = m_font.textAlignment;
	style.scale = m_font.fontScale * CalcScaling();
	style.textWeight = m_font.textWeight;
	style.shadowOffset = m_font.shadowOffset;
	style.shadowWeight = m_font.shadowWeight;

	style.shadowColor = m_font.shadowColor.xyz();
	style.shadowAlpha = m_font.shadowColor.w;

	if (style.shadowAlpha > 0.0f)
		style.styleFlag |= TEXT_STYLE_SHADOW;

	style.textColor = m_font.textColor;
}

inline void DebugDrawRectangle(const AARectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2, IGPURenderPassRecorder* rendPassRecorder)
{
	const Vector2D r0[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.leftTop.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r1[] = { MAKEQUAD(rect.rightBottom.x, rect.leftTop.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r2[] = { MAKEQUAD(rect.leftTop.x, rect.rightBottom.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r3[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.rightBottom.x, rect.leftTop.y, -0.5f) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.material = g_matSystem->GetDefaultMaterial();

	MatSysDefaultRenderPass defaultRenderPass;
	defaultRenderPass.blendMode = SHADER_BLEND_TRANSLUCENT;

	meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
		// put main rectangle
		meshBuilder.Color4fv(color1);
		meshBuilder.Quad2(rect.GetLeftBottom(), rect.GetRightBottom(), rect.GetLeftTop(), rect.GetRightTop());

		// put borders
		meshBuilder.Color4fv(color2);
		meshBuilder.Quad2(r0[0], r0[1], r0[2], r0[3]);
		meshBuilder.Quad2(r1[0], r1[1], r1[2], r1[3]);
		meshBuilder.Quad2(r2[0], r2[1], r2[2], r2[3]);
		meshBuilder.Quad2(r3[0], r3[1], r3[2], r3[3]);
	if (meshBuilder.End(drawCmd))
		g_matSystem->SetupDrawCommand(drawCmd, RenderPassContext(rendPassRecorder, &defaultRenderPass));
}

// rendering function
void IUIControl::Render(int depth, IGPURenderPassRecorder* rendPassRecorder)
{
	if(!m_visible)
		return;

	bool scissorOn = true;

	const IAARectangle clientRectRender = GetClientRectangle();
	g_matSystem->SetFogInfo(FogInfo());			// disable fog

	// calculate absolute transformation using previous matrix
	Matrix4x4 prevTransform;
	g_matSystem->GetMatrix(MATRIXMODE_WORLD2, prevTransform);

	// we apply scaling to our transform to match the units of the elements
	const Vector2D scale = CalcScaling();

	const Matrix4x4 clientPosMat = translate((float)clientRectRender.GetCenter().x, (float)clientRectRender.GetCenter().y, 0.0f);
	Matrix4x4 rotationScale = clientPosMat * scale4(m_transform.scale.x, m_transform.scale.y, 1.0f) * rotateZ4(DEG2RAD(m_transform.rotation));
	rotationScale = rotationScale * !clientPosMat;

	const Matrix4x4 localTransform = rotationScale * translate(m_transform.translation.x * scale.x, m_transform.translation.y * scale.y, 0.0f);
	const Matrix4x4 newTransform = (prevTransform * localTransform);

	// load new absolulte transformation
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, newTransform);

	if( m_parent && m_selfVisible )
	{
		// set scissor rect before childs are rendered
		// only if no transformation applied
		if (newTransform.rows[0].x != 1.0f)
		{
			IAARectangle scissorRect(IVector2D(0, 0), rendPassRecorder->GetRenderTargetDimensions());
			rendPassRecorder->SetScissorRectangle(scissorRect);
			scissorOn = false;
		}
		else
		{
			IAARectangle scissorRect = GetClientScissorRectangle();
			scissorRect.leftTop += m_transform.translation * scale;
			scissorRect.rightBottom += m_transform.translation * scale;
			scissorRect.leftTop = clamp(scissorRect.leftTop, IVector2D(0, 0), rendPassRecorder->GetRenderTargetDimensions());
			scissorRect.rightBottom = clamp(scissorRect.rightBottom, IVector2D(0, 0), rendPassRecorder->GetRenderTargetDimensions());

			rendPassRecorder->SetScissorRectangle(scissorRect);
		}

		// paint control itself
		DrawSelf( clientRectRender, scissorOn, rendPassRecorder);
	}

	HOOK_TO_CVAR(equi_debug);
	if (equi_debug->GetInt() > 0 && equi_debug->GetInt() <= depth)
	{
		DebugDrawRectangle(clientRectRender, ColorRGBA(1, 1, 0, 0.05), ColorRGBA(1, 0, 1, 0.8), rendPassRecorder);

		eqFontStyleParam_t params;
		debugoverlay->GetFont()->SetupRenderText(
			EqString::Format("%s x=%d y=%d w=%d h=%d (v=%d)", m_name.ToCString(), m_position.x, m_position.y, m_size.x, m_size.y, m_visible).ToCString(), 
			clientRectRender.GetLeftBottom(), params, rendPassRecorder);
	}

	// render from last
	for (auto it = m_childs.last(); !it.atEnd(); --it)
	{
		// load new absolulte transformation
		g_matSystem->SetMatrix(MATRIXMODE_WORLD2, newTransform);
		(*it)->Render(depth + 1, rendPassRecorder);
	}

	// always reset previous absolute transformation
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, prevTransform);

	// reset scissor after drawing equi
	if (depth <= 1)
	{
		IAARectangle scissorRect(IVector2D(0, 0), rendPassRecorder->GetRenderTargetDimensions());
		rendPassRecorder->SetScissorRectangle(scissorRect);
	}
}

IUIControl* IUIControl::HitTest(const IVector2D& point) const
{
	if(!m_visible)
		return nullptr;

	IUIControl* bestControl = const_cast<IUIControl*>(this);

	IAARectangle clientRect = GetClientRectangle();

	if(!clientRect.Contains(point))
		return nullptr;

	for (IUIControl* child : m_childs)
	{
		IUIControl* hit = child->HitTest(point);

		if (hit)
		{
			bestControl = hit;
			break;
		}
	}

	return bestControl;
}

// returns child control
IUIControl* IUIControl::FindChild(const char* pszName)
{
	for (IUIControl* child : m_childs)
	{
		if (!strcmp(child->GetName(), pszName))
			return child;
	}

	return nullptr;
}

IUIControl* IUIControl::FindChildRecursive(const char* pszName)
{
	// find nearest child
	for (IUIControl* child : m_childs)
	{
		if (!strcmp(child->GetName(), pszName))
			return child;

		IUIControl* foundChild = child->FindChildRecursive(pszName);
		if (foundChild)
			return foundChild;
	}

	return nullptr;
}

void IUIControl::ClearChilds(bool destroy)
{
	for (IUIControl* child : m_childs)
	{
		child->m_parent = nullptr;

		if (destroy)
			delete child;
	}

	m_childs.clear();
}

void IUIControl::AddChild(IUIControl* pControl)
{
	m_childs.prepend(pControl);
	pControl->m_parent = this;
}

void IUIControl::RemoveChild(IUIControl* pControl, bool destroy)
{
	auto it = m_childs.findBack(pControl);
	if (it.atEnd())
		return;

	(*it)->m_parent = nullptr;

	if (destroy)
		delete (*it);

	m_childs.remove(it);
}

bool IUIControl::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{

	///Msg("ProcessMouseEvents on %s\n", m_name.ToCString());
	return true;
}

bool IUIControl::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	//Msg("ProcessKeyboardEvents on %s\n", m_name.ToCString());
	return true;
}

int IUIControl::CommandCb(IUIControl* control, ui_event& event, void* userData)
{
	if (UICMD_ARGC == 0)
		return 1;

	if (!stricmp("hideparent", UICMD_ARGV(0).ToCString()))
	{
		if (control->m_parent)
			control->m_parent->Hide();
	}
	else if (!stricmp("engine", UICMD_ARGV(0).ToCString()))
	{
		// execute console commands
		g_consoleCommands->SetCommandBuffer(UICMD_ARGV(1).ToCString());
		g_consoleCommands->ExecuteCommandBuffer();
		g_consoleCommands->ClearCommandBuffer();
	}
	else if (!stricmp("showpanel", UICMD_ARGV(0).ToCString()))
	{
		// show panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).ToCString());
		panel->Show();
	}
	else if (!stricmp("hidepanel", UICMD_ARGV(0).ToCString()))
	{
		// hide panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).ToCString());
		panel->Hide();
	}

	// TODO: findChild/hideChild etc

	return 1;
}

static int s_uidCounter = 1;

int	IUIControl::AddEventHandler(const char* pszName, uiEventCallback_t cb)
{
	ui_event evt(pszName, cb);
	evt.uid = s_uidCounter++;

	m_eventCallbacks.append(evt);
	return evt.uid;
}

void IUIControl::RemoveEventHandler(int handlerId)
{
	for (int i = 0; i < m_eventCallbacks.numElem(); i++)
	{
		if (m_eventCallbacks[i].uid == handlerId)
		{
			m_eventCallbacks.fastRemoveIndex(i);
			break;
		}
	}
}

void IUIControl::RemoveEventHandlers(const char* name)
{
	for (int i = 0; i < m_eventCallbacks.numElem(); i++)
	{
		if (!m_eventCallbacks[i].name.CompareCaseIns(name))
		{
			m_eventCallbacks.removeIndex(i--);
		}
	}
}

int IUIControl::RaiseEvent(const char* name, void* userData)
{
	int result = -1;
	for (int i = 0; i < m_eventCallbacks.numElem(); i++)
	{
		if (!m_eventCallbacks[i].name.CompareCaseIns(name))
		{
			result = m_eventCallbacks[i].callback(this, m_eventCallbacks[i], userData);
			break;
		}
	}

	return result;
}

int IUIControl::RaiseEventUid(int uid, void* userData)
{
	int result = -1;
	for (int i = 0; i < m_eventCallbacks.numElem(); i++)
	{
		if (!m_eventCallbacks[i].uid == uid)
		{
			result = m_eventCallbacks[i].callback(this, m_eventCallbacks[i], userData);
			break;
		}
	}

	return result;
}

};
