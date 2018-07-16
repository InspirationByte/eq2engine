//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "IEqUI_Control.h"

#include "EqUI_Manager.h"
#include "EqUI_Panel.h"

#include "IFileSystem.h"
#include "FontCache.h"

#include "IConCommandFactory.h"

#include "ILocalize.h"

#include "materialsystem/MeshBuilder.h"
#include "IDebugOverlay.h"

namespace equi
{

IUIControl::IUIControl()
	: m_visible(true), m_selfVisible(true), m_enabled(true), m_parent(NULL), 
	m_font(nullptr), m_fontScale(1.0f), m_sizeDiff(0), m_sizeDiffPerc(1.0f), 
	m_position(0),m_size(25),
	m_scaling(UI_SCALING_NONE), m_anchors(0), m_alignment(UI_BORDER_LEFT | UI_BORDER_TOP)
{
	m_label = "Control";
}

IUIControl::~IUIControl()
{
	ClearChilds(true);
}

const char*	IUIControl::GetLabel() const
{
	return _Es(m_label).c_str();
}

void IUIControl::SetLabel(const char* pszLabel)
{
	m_label = LocalizedString(pszLabel);
}

void IUIControl::InitFromKeyValues( kvkeybase_t* sec, bool noClear )
{
	if (!noClear)
		ClearChilds(true);

	if(!stricmp(sec->GetName(), "child"))
		SetName(KV_GetValueString(sec, 1, ""));
	else
		SetName(KV_GetValueString(sec, 0, ""));

	SetLabel(KV_GetValueString(sec->FindKeyBase("label"), 0, GetLabel()));

	m_position = KV_GetIVector2D(sec->FindKeyBase("position"), 0, m_position);
	m_size = KV_GetIVector2D(sec->FindKeyBase("size"), 0, m_size);

	m_visible = KV_GetValueBool(sec->FindKeyBase("visible"), 0, m_visible);
	m_selfVisible = KV_GetValueBool(sec->FindKeyBase("selfvisible"), 0, m_selfVisible);

	m_font = nullptr;
	m_sizeDiff = 0;
	m_sizeDiffPerc = 1.0f;
	m_anchors = 0;
	m_alignment = (UI_BORDER_LEFT | UI_BORDER_TOP);

	kvkeybase_t* font = sec->FindKeyBase("font");

	if(font)
	{
		// TODO: styles
		m_font = g_fontCache->GetFont(KV_GetValueString(font), KV_GetValueInt(font, 1, 20), 0, false);
	}

	m_fontScale = KV_GetVector2D(sec->FindKeyBase("fontScale"), 0, m_fontScale);

	kvkeybase_t* command = sec->FindKeyBase("command");

	if(command)
	{
		for(int i = 0; i < command->values.numElem(); i++)
		{
			m_commandEvent.args.append( KV_GetValueString(command, i) );
		}
	}

	//------------------------------------------------------------------------------
	kvkeybase_t* anchors = sec->FindKeyBase("anchors");

	if(anchors)
	{
		m_anchors = 0;

		for(int i = 0; i < anchors->values.numElem(); i++)
		{
			const char* anchorVal = KV_GetValueString(anchors, i);

			//if(!stricmp("left", anchorVal))
			//	m_anchors |= UI_ANCHOR_LEFT;
			//else if(!stricmp("top", anchorVal))
			//	m_anchors |= UI_ANCHOR_TOP;
			if(!stricmp("right", anchorVal))
				m_anchors |= UI_BORDER_RIGHT;
			else if(!stricmp("bottom", anchorVal))
				m_anchors |= UI_BORDER_BOTTOM;
		}
	}

	//------------------------------------------------------------------------------
	kvkeybase_t* align = sec->FindKeyBase("align");

	if(align)
	{
		m_alignment = 0;

		for(int i = 0; i < align->values.numElem(); i++)
		{
			const char* alignVal = KV_GetValueString(align, i);

			if(!stricmp("left", alignVal))
				m_alignment |= UI_BORDER_LEFT;
			else if(!stricmp("top", alignVal))
				m_alignment |= UI_BORDER_TOP;
			else if(!stricmp("right", alignVal))
				m_alignment |= UI_BORDER_RIGHT;
			else if(!stricmp("bottom", alignVal))
				m_alignment |= UI_BORDER_BOTTOM;
		}
	}

	//------------------------------------------------------------------------------
	kvkeybase_t* scaling = sec->FindKeyBase("scaling");
	const char* scalingValue = KV_GetValueString(scaling, 0, "none");

	m_scaling = UI_SCALING_NONE;

	if(!stricmp("width", scalingValue))
		m_scaling = UI_SCALING_WIDTH;
	else if(!stricmp("height", scalingValue))
		m_scaling = UI_SCALING_HEIGHT;
	else if(!stricmp("uniform", scalingValue))
		m_scaling = UI_SCALING_BOTH_UNIFORM;
	else if(!stricmp("inherit", scalingValue))
		m_scaling = UI_SCALING_BOTH_INHERIT;

	// walk for childs
	for(int i = 0; i < sec->keys.numElem(); i++)
	{
		kvkeybase_t* csec = sec->keys[i];

		if(!csec->IsSection())
			continue;

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
	IVector2D oldSize = m_size;
	m_size = size;

	m_sizeDiff += m_size-oldSize;
	m_sizeDiffPerc *= Vector2D(m_size)/Vector2D(oldSize);
}

void IUIControl::SetPosition(const IVector2D &pos)
{
	m_position = pos;
}

void IUIControl::SetRectangle(const IRectangle& rect)
{
	SetPosition(rect.vleftTop);
	SetSize(rect.vrightBottom - m_position);
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

IRectangle IUIControl::GetRectangle() const
{
	return IRectangle(m_position, m_position + m_size);
}

void IUIControl::ResetSizeDiffs()
{
	m_sizeDiff = 0;
	m_sizeDiffPerc = 1.0f;
}

Vector2D IUIControl::CalcScaling() const
{
	if(!m_parent || m_scaling == UI_SCALING_NONE)
		return Vector2D(1.0f, 1.0f);

	Vector2D scale(m_parent->m_sizeDiffPerc);

	if(m_scaling >= UI_SCALING_BOTH_INHERIT)
	{
		if(Manager->GetRootPanel() != m_parent)
		{
			if(m_scaling == UI_SCALING_BOTH_UNIFORM)
			{
				float aspectCorrection = scale.y / scale.x;
				scale.x *= aspectCorrection;
			}

			return scale * m_parent->CalcScaling();
		}
		else
			return Vector2D(1.0f, 1.0f);
	}
	else
	{
		
	}

	return Vector2D(1.0f, 1.0f);
}

IRectangle IUIControl::GetClientRectangle() const
{
	Vector2D scale = CalcScaling();

	IVector2D scaledSize(m_size*scale);
	IVector2D scaledPos(m_position*scale);

	IRectangle thisRect(scaledPos, scaledPos + scaledSize);

	if(m_parent)
	{
		IRectangle parentRect = m_parent->GetClientRectangle();

		// align
		if(m_alignment & UI_BORDER_LEFT)
		{
			thisRect.vleftTop.x += parentRect.vleftTop.x;
			thisRect.vrightBottom.x += parentRect.vleftTop.x;
		}

		if(m_alignment & UI_BORDER_TOP)
		{
			thisRect.vleftTop.y += parentRect.vleftTop.y;
			thisRect.vrightBottom.y += parentRect.vleftTop.y;
		}

		if(m_alignment & UI_BORDER_RIGHT)
		{
			thisRect.vleftTop.x += parentRect.vrightBottom.x - scaledSize.x - scaledPos.x*2;
			thisRect.vrightBottom.x += parentRect.vrightBottom.x - scaledSize.x - scaledPos.x*2;
		}

		if(m_alignment & UI_BORDER_BOTTOM)
		{
			thisRect.vleftTop.y += parentRect.vrightBottom.y - scaledSize.y - scaledPos.y*2;
			thisRect.vrightBottom.y += parentRect.vrightBottom.y - scaledSize.y - scaledPos.y*2;
		}

		// move by anchor border
		if(m_anchors > 0)
		{
			IVector2D parentSizeDiff = m_parent->m_sizeDiff;
			IVector2D offsetAnchors((m_anchors & UI_BORDER_RIGHT) ? 1 : 0, (m_anchors & UI_BORDER_BOTTOM) ? 1 : 0);

			// apply offset
			thisRect.vleftTop += parentSizeDiff*offsetAnchors;
			thisRect.vrightBottom += parentSizeDiff*offsetAnchors;
		}
	}

	return thisRect;
}

IEqFont* IUIControl::GetFont() const
{
	if(m_font == NULL)
	{
		if( m_parent )
			return m_parent->GetFont();
		else
			return equi::Manager->GetDefaultFont();
	}

	return m_font;
}

inline void DebugDrawRectangle(const Rectangle_t &rect, const ColorRGBA &color1, const ColorRGBA &color2)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	g_pShaderAPI->SetTexture(NULL, 0, 0);
	materials->SetBlendingStates(blending);
	materials->SetRasterizerStates(CULL_FRONT, FILL_SOLID);
	materials->SetDepthStates(false, false);

	materials->BindMaterial(materials->GetDefaultMaterial());

	Vector2D r0[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vleftTop.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r1[] = { MAKEQUAD(rect.vrightBottom.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r2[] = { MAKEQUAD(rect.vleftTop.x, rect.vrightBottom.y,rect.vrightBottom.x, rect.vrightBottom.y, -0.5f) };
	Vector2D r3[] = { MAKEQUAD(rect.vleftTop.x, rect.vleftTop.y,rect.vrightBottom.x, rect.vleftTop.y, -0.5f) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(materials->GetDynamicMesh());
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
	meshBuilder.End();
}

// rendering function
void IUIControl::Render()
{
	if(!m_visible)
		return;

	RasterizerStateParams_t rasterState;
	//rasterState.fillMode = FILL_SOLID;
	//rasterState.cullMode = CULL_NONE;
	rasterState.scissor = true;

	IRectangle clientRectRender = GetClientRectangle();

	if( m_parent && m_selfVisible )
	{
		// force rasterizer state
		// other states are pretty useless
		materials->SetRasterizerStates(rasterState);
		materials->SetShaderParameterOverriden(SHADERPARAM_RASTERSETUP, true);

		// set scissor rect before childs are rendered
		g_pShaderAPI->SetScissorRectangle(clientRectRender);

		// paint control itself
		DrawSelf( clientRectRender );

		HOOK_TO_CVAR(equi_debug)

		if (equi_debug->GetBool())
		{
			DebugDrawRectangle(clientRectRender, ColorRGBA(1,1,0,0.15), ColorRGBA(1, 1, 1, 0.15));

			eqFontStyleParam_t params;
			debugoverlay->GetFont()->RenderText(varargs("%s x=%d y=%d w=%d h=%d (v=%d)", m_name.c_str(), m_position.x, m_position.y, m_size.x, m_size.y, m_visible), clientRectRender.GetLeftBottom(), params);
		}
	}

	// render from last
	if(m_childs.goToLast())
	{
		do
		{
			m_childs.getCurrent()->Render();
		}
		while(m_childs.goToPrev());
	}
}

IUIControl* IUIControl::HitTest(const IVector2D& point)
{
	if(!m_visible)
		return NULL;

	IUIControl* bestControl = this;

	IRectangle clientRect = GetClientRectangle();

	if(!clientRect.IsInRectangle(point))
		return NULL;

	if(m_childs.goToFirst())
	{
		do
		{
			IUIControl* hit = m_childs.getCurrent()->HitTest(point);

			if(hit)
			{
				bestControl = hit;
				break;
			}
		}
		while(m_childs.goToNext());
	}

	return bestControl;
}

// returns child control
IUIControl* IUIControl::FindChild(const char* pszName)
{
	DkLinkedListIterator<IUIControl*> iter(m_childs);

	if(iter.goToFirst())
	{
		do
		{
			if(!strcmp(iter.getCurrent()->GetName(), pszName))
				return iter.getCurrent();
		}
		while(iter.goToNext());
	}

	return NULL;
}

void IUIControl::ClearChilds(bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			m_childs.getCurrent()->m_parent = NULL;

			if(destroy)
				delete m_childs.getCurrent();

			m_childs.setCurrent(NULL);
		}
		while(m_childs.goToNext());
	}

	m_childs.clear();
}

void IUIControl::AddChild(IUIControl* pControl)
{
	m_childs.addFirst(pControl);
	pControl->m_parent = this;
}

void IUIControl::RemoveChild(IUIControl* pControl, bool destroy)
{
	if(m_childs.goToFirst())
	{
		do
		{
			if(m_childs.getCurrent() == pControl)
			{
				pControl->m_parent = NULL;

				if(destroy)
					delete pControl;

				m_childs.removeCurrent();
				return;
			}
		}
		while(m_childs.goToNext());
	}
}

bool IUIControl::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{

	///Msg("ProcessMouseEvents on %s\n", m_name.c_str());
	return true;
}

bool IUIControl::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	//Msg("ProcessKeyboardEvents on %s\n", m_name.c_str());
	return true;
}

bool IUIControl::ProcessCommand(DkList<EqString>& args)
{
	if(UICMD_ARGC == 0)
		return true;

	if(!stricmp("hideparent", UICMD_ARGV(0).c_str()))
	{
		if(m_parent)
			m_parent->Hide();
	}
	else if(!stricmp("engine", UICMD_ARGV(0).c_str()))
	{
		// execute console commands
		g_sysConsole->SetCommandBuffer(UICMD_ARGV(1).c_str());
		g_sysConsole->ExecuteCommandBuffer();
		g_sysConsole->ClearCommandBuffer();
	}
	else if(!stricmp("showpanel", UICMD_ARGV(0).c_str()))
	{
		// show panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).c_str());
		panel->Show();
	}
	else if(!stricmp("hidepanel", UICMD_ARGV(0).c_str()))
	{
		// hide panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).c_str());
		panel->Hide();
	}

	return true;
}

};
