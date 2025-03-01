//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI panel
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IFileSystem.h"
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
/*
DECLARE_KEYVALUES_FLAGS_DESC(EBordersFlagsDesc)
BEGIN_KEYVALUES_FLAGS_DESC(EBordersFlagsDesc)
	KV_FLAG_DESC(UI_BORDER_LEFT, "left")
	KV_FLAG_DESC(UI_BORDER_TOP, "top")
	KV_FLAG_DESC(UI_BORDER_RIGHT, "right")
	KV_FLAG_DESC(UI_BORDER_BOTTOM, "bottom")
	KV_FLAG_DESC(UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM, "all")
END_KEYVALUES_FLAGS_DESC

DECLARE_KEYVALUES_FLAGS_DESC(EAlignmentFlagsDesc)
BEGIN_KEYVALUES_FLAGS_DESC(EAlignmentFlagsDesc)
	KV_FLAG_DESC(UI_ALIGN_LEFT, "left")
	KV_FLAG_DESC(UI_ALIGN_TOP, "top")
	KV_FLAG_DESC(UI_ALIGN_RIGHT, "right")
	KV_FLAG_DESC(UI_ALIGN_BOTTOM, "bottom")
	KV_FLAG_DESC(UI_ALIGN_HCENTER, "hcenter")
	KV_FLAG_DESC(UI_ALIGN_VCENTER, "vcenter")
END_KEYVALUES_FLAGS_DESC

DECLARE_KEYVALUES_FLAGS_DESC(EScalingModeFlagsDesc)
BEGIN_KEYVALUES_FLAGS_DESC(EScalingModeFlagsDesc)
	KV_FLAG_DESC(UI_SCALING_NONE, "none")
	KV_FLAG_DESC(UI_SCALING_INHERIT, "inherit")
	KV_FLAG_DESC(UI_SCALING_INHERIT_MIN, "inherit_min")
	KV_FLAG_DESC(UI_SCALING_INHERIT_MAX, "inherit_max")
	KV_FLAG_DESC(UI_SCALING_ASPECT_MIN, "aspect_min")
	KV_FLAG_DESC(UI_SCALING_ASPECT_MAX, "aspect_max")
	KV_FLAG_DESC(UI_SCALING_ASPECT_W, "aspectw")
	KV_FLAG_DESC(UI_SCALING_ASPECT_H, "aspecth")
	KV_FLAG_DESC(UI_SCALING_ASPECT_H, "uniform")
END_KEYVALUES_FLAGS_DESC
*/

IUIControl::IUIControl()
{
}

IUIControl::~IUIControl()
{
	ClearChilds(true);
}

const char*	IUIControl::GetLabel() const
{
	return m_label;
}

void IUIControl::SetLabel(const char* pszLabel)
{
	if (!pszLabel)
		return;

	if (!m_label.Compare(pszLabel))
		return;

	if (*pszLabel == '#')
	{
		AnsiUnicodeConverter(m_labelTextValue, pszLabel + 1);
		m_labelText = g_localizer->GetTokenString(pszLabel + 1, m_labelTextValue);
	}
	else
	{
		AnsiUnicodeConverter(m_labelTextValue, pszLabel);
		m_labelText = m_labelTextValue;
	}
	m_label = pszLabel;
}

const wchar_t* IUIControl::GetLabelText() const
{
	return m_labelText;
}

void IUIControl::SetLabelText(const wchar_t* pszLabel)
{
	if (!m_labelText.Compare(pszLabel))
		return;

	m_labelTextValue = pszLabel;
	m_labelText = m_labelTextValue;
	AnsiUnicodeConverter(m_label, m_labelText);
}

void IUIControl::InitFonts(const KVSection* sec)
{
	auto ParseFontDef = [this](FontProps& fontProps, const KVSection* fontSec, const KVSection* textParamsSec) {
		if (fontSec)
		{
			EqStringRef fontName;
			int fontSize = 20;
			const int valueCnt = fontSec->GetValues(fontName, fontSize);

			const FontProps* foundFontProps = (valueCnt == 1) ? FindFont(fontName, m_name) : nullptr;

			if (foundFontProps)
			{
				fontProps = *foundFontProps;
			}
			else
			{
				int styleFlags = 0;
				for (EqStringRef fontFlag : fontSec->Values<EqStringRef>(1))
				{
					if (!fontFlag.CompareCaseIns("bold"))
						styleFlags |= TEXT_STYLE_BOLD;
					else if (!fontFlag.CompareCaseIns("italic"))
						styleFlags |= TEXT_STYLE_ITALIC;
				}

				fontProps.font = g_fontCache->GetFont(fontName, fontSize, styleFlags, false);
			}
		}

		if (textParamsSec)
		{
			EqStringRef textCase;
			textParamsSec->Get("textCase").GetValues(textCase);

			if (!textCase.CompareCaseIns("upper") || !textCase.CompareCaseIns("upperCase"))
				fontProps.textCase = FontProps::UPPER_CASE;
			else if (!textCase.CompareCaseIns("lower") || !textCase.CompareCaseIns("lowerCase"))
				fontProps.textCase = FontProps::LOWER_CASE;

			textParamsSec->Get("fontScale").GetValues(fontProps.fontScale);
			textParamsSec->Get("textColor").GetValues(fontProps.textColor);
			textParamsSec->Get("textMonospace").GetValues(fontProps.monoSpace);
			textParamsSec->Get("textWeight").GetValues(fontProps.textWeight);
			textParamsSec->Get("textShadowColor").GetValues(fontProps.shadowColor);
			textParamsSec->Get("textShadowOffset").GetValues(fontProps.shadowOffset);
			textParamsSec->Get("textShadowWeight").GetValues(fontProps.shadowWeight);
		}
	};

	// parse fonts if any
	const KVSection* fontsSec = sec->FindSection("fonts");
	if (fontsSec)
	{
		for (const KVSection* fontSec : fontsSec->Keys())
		{
			FontProps fontProps;
			ParseFontDef(fontProps, fontSec, fontSec);

			const uint fontId = StringId(fontSec->GetName(), true);
			m_fontCollection.insert(fontId, fontProps);
		}
	}

	ParseFontDef(m_font, sec->FindSection("font"), sec);

	const KVSection* textAlignSec = sec->FindSection("textAlign");
	if (textAlignSec)
	{
		m_font.textAlignment = 0;
		for (EqStringRef alignVal : textAlignSec->Values<EqStringRef>())
		{
			if (!alignVal.CompareCaseIns("left"))
				m_font.textAlignment |= TEXT_ALIGN_LEFT;
			else if (!alignVal.CompareCaseIns("top"))
				m_font.textAlignment |= TEXT_ALIGN_TOP;
			else if (!alignVal.CompareCaseIns("right"))
				m_font.textAlignment |= TEXT_ALIGN_RIGHT;
			else if (!alignVal.CompareCaseIns("bottom"))
				m_font.textAlignment |= TEXT_ALIGN_BOTTOM;
			else if (!alignVal.CompareCaseIns("vcenter"))
				m_font.textAlignment |= TEXT_ALIGN_VCENTER;
			else if (!alignVal.CompareCaseIns("hcenter"))
				m_font.textAlignment |= TEXT_ALIGN_HCENTER;
			else if (!alignVal.CompareCaseIns("center"))
				m_font.textAlignment |= TEXT_ALIGN_HCENTER | TEXT_ALIGN_VCENTER;
		}
	}
}

void IUIControl::InitFromKeyValues(const KVSection* sec, bool keepElements)
{
	if (!keepElements)
		ClearChilds(true);

	Parse(sec);
	InitChildItems(sec, keepElements);
}

void IUIControl::Parse(const KVSection* sec)
{
	EqStringRef elementName;
	if (!CString::CompareCaseIns(sec->GetName(), "child"))
		sec->GetValuesAt(1, elementName);
	else
		sec->GetValuesAt(0, elementName);

	if (elementName)
		SetName(elementName);

	EqStringRef label;
	if (sec->Get("label").GetValues(label))
		SetLabel(label);

	m_clipChilds = m_parent ? m_parent->m_clipChilds : m_clipChilds;
	m_clipTransform = m_parent ? m_parent->m_clipTransform : m_clipTransform;

	sec->Get("position").GetValues(m_position);
	sec->Get("size").GetValues(m_size);
	sec->Get("visible").GetValues(m_visible);
	sec->Get("selfvisible").GetValues(m_selfVisible);
	sec->Get("clipChilds").GetValues(m_clipChilds);
	sec->Get("clipTransform").GetValues(m_clipTransform);

	m_sizeReal = m_size;

	const KVSection* commandSec = sec->FindSection("command");
	if (commandSec)
	{
		// NOTE: command event always have UID == 0
		EvtHandler& evt = m_eventCallbacks.append();
		evt.callback = CommandCb;
		evt.name = "command";
		evt.uid = 0;

		for (EqStringRef command : commandSec->Values<EqStringRef>())
			evt.args.append(command);
	}

	//------------------------------------------------------------------------------

	const KVSection* anchorsSec = sec->FindSection("anchors");
	if (anchorsSec)
	{
		m_anchors = 0;
		for (EqStringRef anchorVal : anchorsSec->Values<EqStringRef>())
		{
			if (!anchorVal.CompareCaseIns("left"))
				m_anchors |= UI_BORDER_LEFT;
			else if (!anchorVal.CompareCaseIns("top"))
				m_anchors |= UI_BORDER_TOP;
			else if (!anchorVal.CompareCaseIns("right"))
				m_anchors |= UI_BORDER_RIGHT;
			else if (!anchorVal.CompareCaseIns("bottom"))
				m_anchors |= UI_BORDER_BOTTOM;
			else if (!anchorVal.CompareCaseIns("all"))
				m_anchors = (UI_BORDER_LEFT | UI_BORDER_TOP | UI_BORDER_RIGHT | UI_BORDER_BOTTOM);
		}
	}

	//------------------------------------------------------------------------------
	const KVSection* alignSec = sec->FindSection("align");
	if (alignSec)
	{
		m_alignment = 0;
		for (EqStringRef alignVal : alignSec->Values<EqStringRef>())
		{
			if (!alignVal.CompareCaseIns("left"))
				m_alignment |= UI_ALIGN_LEFT;
			else if (!alignVal.CompareCaseIns("top"))
				m_alignment |= UI_ALIGN_TOP;
			else if (!alignVal.CompareCaseIns("right"))
				m_alignment |= UI_ALIGN_RIGHT;
			else if (!alignVal.CompareCaseIns("bottom"))
				m_alignment |= UI_ALIGN_BOTTOM;
			else if (!alignVal.CompareCaseIns("hcenter"))
				m_alignment |= UI_ALIGN_HCENTER;
			else if (!alignVal.CompareCaseIns("vcenter"))
				m_alignment |= UI_ALIGN_VCENTER;
			else if (!alignVal.CompareCaseIns("center"))
				m_alignment = UI_ALIGN_HCENTER | UI_ALIGN_VCENTER;
		}
	}

	//------------------------------------------------------------------------------
	const KVSection* transformSec = sec->FindSection("transform");
	if (transformSec)
	{
		const float rotateVal = KV_GetValueFloat(transformSec->FindSection("rotate"), 0.0f);
		const Vector2D scaleVal = KV_GetVector2D(transformSec->FindSection("elementScale"), 0, 1.0f);
		const Vector2D translate = KV_GetVector2D(transformSec->FindSection("translate"), 0, 0.0f);

		SetTransform(translate, scaleVal, rotateVal);
	}

	//------------------------------------------------------------------------------

	InitFonts(sec);

	//------------------------------------------------------------------------------

	const KVSection* scalingSec = sec->FindSection("scaling");
	if (scalingSec)
	{
		m_scaling = UI_SCALING_NONE;

		EqStringRef scalingValue = KV_GetValueString(scalingSec, 0, "none");

		if (!scalingValue.CompareCaseIns("aspectw"))
			m_scaling = UI_SCALING_ASPECT_W;
		else if (!scalingValue.CompareCaseIns("aspecth") || !scalingValue.CompareCaseIns("uniform"))
			m_scaling = UI_SCALING_ASPECT_H;
		else if (!scalingValue.CompareCaseIns("inherit"))
			m_scaling = UI_SCALING_INHERIT;
		else if (!scalingValue.CompareCaseIns("inherit_min"))
			m_scaling = UI_SCALING_INHERIT_MIN;
		else if (!scalingValue.CompareCaseIns("inherit_max"))
			m_scaling = UI_SCALING_INHERIT_MAX;
		else if (!scalingValue.CompareCaseIns("aspect_min"))
			m_scaling = UI_SCALING_ASPECT_MIN;
		else if (!scalingValue.CompareCaseIns("aspect_max"))
			m_scaling = UI_SCALING_ASPECT_MAX;
	}

}

void IUIControl::InitChildItems(const KVSection* sec, bool keepElements)
{
	// walk for childs
	for (const KVSection* childSec : sec->Keys("child"))
	{
		EqStringRef childClass;
		EqStringRef childName;
		if (childSec->GetValues(childClass, childName) < 1)
		{
			MsgError("eqUI error: Can't create child without class name");
			continue;
		}

		// add childs from file
		if (!childClass.CompareCaseIns("file"))
		{
			KVSection section;
			if (!KV_LoadFromFile(childName, SP_MOD, &section))
			{
				MsgWarning("EqUI warning: file %s requested by element %s not found\n", childName.ToCString(), m_name.ToCString());
				continue;
			}

			InitFonts(&section);
			InitChildItems(&section, true);
			continue;
		}

		bool isNewControl = false;
		IUIControl* control = nullptr;
		if (childName)
		{
			// try find existing child and override it
			control = FindChild(childName);

			// replace is class is not matching
			if (control && childClass.CompareCaseIns(control->GetClassname()))
			{
				if (control) // replace children if it has different class
					RemoveChild(control);

				control = equi::Manager->CreateElement(childClass);
				isNewControl = true;
			}
		}
		
		if(!control)
		{
			control = equi::Manager->CreateElement(childClass);
			isNewControl = true;
		}

		if (!control)
			continue;

		if(isNewControl)
		{
			AddChild(control);
		}

		control->InitFromKeyValues(childSec, keepElements);
	}
}

const IUIControl::FontProps* IUIControl::FindFont(const char* name, const char* requestedBy) const
{
	const uint fontId = StringId(name, true);
	auto fontIt = m_fontCollection.find(fontId);
	if (!fontIt.atEnd())
		return &(*fontIt);

	if (m_parent)
		return m_parent->FindFont(name, requestedBy);

	MsgWarning("EqUI warning: Font %s requested by element %s not found\n", name, requestedBy);

	return nullptr;
}

void IUIControl::SetSize(const IVector2D &size)
{
	if (m_size == size)
		return;
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

	if(Manager->GetRootPanel() == m_parent)
		return Vector2D(1.0f, 1.0f);

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

IAARectangle IUIControl::GetClientRectangle() const
{
	const Vector2D scale = CalcScaling();

	const IVector2D scaledSize(m_size * scale);
	const IVector2D scaledPos(m_position * scale);

	IAARectangle thisRect(scaledPos, scaledPos + scaledSize);

	if (!m_parent)
		return thisRect;

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

	return thisRect;
}

IAARectangle IUIControl::TransformScissorRectangle(const IAARectangle& rect, const Matrix4x4& transform)
{
	// transform scissor rectangle accordingly
	IAARectangle ret;
	for (int i = 0; i < 4; ++i)
	{
		const Vector2D vertex = rect.GetVertex(i);
		ret.AddVertex(transformPointTransposed(Vector3D(vertex, 0.0f), transform).xy());
	}
	return ret;
}

IAARectangle IUIControl::ClipScissorRectangle(const IAARectangle& rect, const IAARectangle& parentRect)
{
	IAARectangle newRect;
	newRect.Reset();
	for (int i = 0; i < 4; ++i)
	{
		const IVector2D clippedVertex = parentRect.ClampPointInRectangle(rect.GetVertex(i));
		newRect.AddVertex(clippedVertex);
	}
	return newRect;
}

IAARectangle IUIControl::GetClientScissorRectangle(int depth, const RenderContextAbstract& context) const
{
	IAARectangle clientRect = GetClientRectangle();
	if (m_clipTransform)
		clientRect = TransformScissorRectangle(clientRect, context.transformStack[depth]);

	if (!m_parent || !m_parent->m_clipChilds)
	{
		return clientRect;
	}

	const IAARectangle parentRect = m_parent->GetClientScissorRectangle(depth - 1, context);
	return ClipScissorRectangle(clientRect, parentRect);
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

void IUIControl::GetCalcFontStyle(FontStyleParam& style) const
{
	style.styleFlag |= TEXT_STYLE_SCISSOR | TEXT_STYLE_USE_TAGS | (m_font.monoSpace ? TEXT_STYLE_MONOSPACE : 0);

	switch (m_font.textCase)
	{
	case FontProps::UPPER_CASE:
		style.styleFlag |= TEXT_STYLE_UPPERCASE;
		break;
	case FontProps::LOWER_CASE:
		style.styleFlag |= TEXT_STYLE_LOWERCASE;
		break;
	}

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

#ifdef ENABLE_DEBUG_DRAWING
inline void DebugDrawRectangle(const AARectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2, IGPURenderPassRecorder* rendPassRecorder)
{
	const Vector2D r0[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.leftTop.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r1[] = { MAKEQUAD(rect.rightBottom.x, rect.leftTop.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r2[] = { MAKEQUAD(rect.leftTop.x, rect.rightBottom.y,rect.rightBottom.x, rect.rightBottom.y, -0.5f) };
	const Vector2D r3[] = { MAKEQUAD(rect.leftTop.x, rect.leftTop.y,rect.rightBottom.x, rect.leftTop.y, -0.5f) };

	// draw all rectangles with just single draw call
	CMeshBuilder meshBuilder(g_matSystem->GetDynamicMesh());
	RenderDrawCmd drawCmd;
	drawCmd.SetMaterial(g_matSystem->GetDefaultMaterial());

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
#endif // ENABLE_DEBUG_DRAWING

// rendering function
void IUIControl::Render(int depth, RenderContextAbstract& context)
{
	if(!m_visible)
		return;

	IGPURenderPassRecorder* rendPassRecorder = context.rendPassRecorder;

	static const IAARectangle defaultScissorRect(IVector2D(COM_INT_MIN), IVector2D(COM_INT_MAX));

	g_matSystem->SetFogInfo(FogInfo());			// disable fog

	// calculate absolute transformation using previous matrix
	Matrix4x4 prevTransform = context.transformStack.back();

	// we apply scaling to our transform to match the units of the elements
	const Vector2D elementScale = CalcScaling();

	const IAARectangle clientRectRender = GetClientRectangle();
	{
		const Vector2D clientRectCenter = clientRectRender.GetCenter();
		const Matrix4x4 clientPosMat = translate(clientRectCenter.x, clientRectCenter.y, 0.0f);
		Matrix4x4 rotationScale = clientPosMat * scale4(m_transform.scale.x, m_transform.scale.y, 1.0f) * rotateZ4(DEG2RAD(m_transform.rotation));
		rotationScale = rotationScale * !clientPosMat;

		const Matrix4x4 localTransform = rotationScale * translate(m_transform.translation.x * elementScale.x, m_transform.translation.y * elementScale.y, 0.0f);
		const Matrix4x4 newTransform = (prevTransform * localTransform);
		context.transformStack.append(newTransform);
	}

	IAARectangle scissorRect = GetClientScissorRectangle(depth, context);

#ifdef ENABLE_DEBUG_DRAWING
	HOOK_TO_CVAR(equi_debug);
	if (equi_debug->GetInt() == -1 || equi_debug->GetInt() == depth)
	{
		rendPassRecorder->SetScissorRectangle(defaultScissorRect);
		g_matSystem->SetMatrix(MATRIXMODE_WORLD2, identity4);

		DebugDrawRectangle(scissorRect, ColorRGBA(1, 1, 0, 0.05), ColorRGBA(1, 0, 1, 0.8), rendPassRecorder);

		FontStyleParam params;
		debugoverlay->GetFont()->SetupRenderText(
			EqString::Format("%s x=%d y=%d w=%d h=%d (v=%d)", m_name.ToCString(), m_position.x, m_position.y, m_size.x, m_size.y, m_visible).ToCString(),
			clientRectRender.GetLeftBottom(), params, rendPassRecorder);
	}
#endif

	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, context.transformStack.back());

	// FIXME: currently scissor is only needed for text
	// should we instead contain elements inside of parent scissor rectangle?
	rendPassRecorder->SetScissorRectangle(scissorRect);

	if( m_parent && m_selfVisible )
	{
		// paint control itself
		DrawSelf(clientRectRender, rendPassRecorder);
	}
	RenderChilds(depth + 1, context);

	context.transformStack.popBack();
	g_matSystem->SetMatrix(MATRIXMODE_WORLD2, context.transformStack.back());

	// reset scissor after drawing equi
	if (depth <= 1)
		rendPassRecorder->SetScissorRectangle(defaultScissorRect);
}

void IUIControl::RenderChilds(int depth, RenderContextAbstract& context)
{
	// render all childs from last to first
	for (auto it = m_childs.last(); !it.atEnd(); --it)
		(*it)->Render(depth, context);
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
		if (!CString::Compare(child->GetName(), pszName))
			return child;
	}

	return nullptr;
}

IUIControl* IUIControl::FindChildRecursive(const char* pszName)
{
	// find nearest child
	for (IUIControl* child : m_childs)
	{
		if (!CString::Compare(child->GetName(), pszName))
			return child;

		IUIControl* foundChild = child->FindChildRecursive(pszName);
		if (foundChild)
			return foundChild;
	}

	return nullptr;
}

IUIControl* IUIControl::Get(const char* pathToElem)
{
	IUIControl* currentElement = this;

	EqString path = pathToElem;
	char* iter = path.GetData();
	while (iter && *iter)
	{
		char* nextStart = (char*)strchr(iter, '.');
		if(nextStart)
			*nextStart = '\0';

		currentElement = currentElement->FindChild(iter);
		if (!currentElement)
			return nullptr;

		iter = nextStart ? nextStart + 1 : nullptr;
	}
	return currentElement == this ? nullptr : currentElement;
}

void IUIControl::ClearAll(bool destroy)
{
	ClearChilds(destroy);
	m_fontCollection.clear(destroy);
	m_eventCallbacks.clear(destroy);
}

void IUIControl::ForEachChild(const EqFunction<bool(IUIControl* child)>& childFunc)
{
	for (IUIControl* child : m_childs)
	{
		if (!childFunc(child))
			break;
	}
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
	if(!pControl->m_childs.getCount())
		pControl->m_font = m_font;

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

int IUIControl::CommandCb(IUIControl* control, const EvtHandler& event, void* userData)
{
	if (UICMD_ARGC == 0)
		return 1;

	if (!UICMD_ARGV(0).CompareCaseIns("hideparent"))
	{
		if (control->m_parent)
			control->m_parent->Hide();
	}
	else if (!UICMD_ARGV(0).CompareCaseIns("engine"))
	{
		// execute console commands
		g_consoleCommands->SetCommandBuffer(UICMD_ARGV(1).ToCString());
		g_consoleCommands->ExecuteCommandBuffer();
		g_consoleCommands->ClearCommandBuffer();
	}
	else if (!UICMD_ARGV(0).CompareCaseIns("showpanel"))
	{
		// show panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).ToCString());
		panel->Show();
	}
	else if (!UICMD_ARGV(0).CompareCaseIns("hidepanel"))
	{
		// hide panel
		equi::Panel* panel = equi::Manager->FindPanel(UICMD_ARGV(1).ToCString());
		panel->Hide();
	}

	// TODO: findChild/hideChild etc

	return 1;
}

int	IUIControl::AddEventHandler(const char* pszName, EvtCallback&& cb)
{
	EvtHandler& evt = m_eventCallbacks.append();
	evt.uid = StringId24(pszName, true);
	evt.name = pszName;
	evt.callback = std::move(cb);

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
	for (const EvtHandler& handler : m_eventCallbacks)
	{
		if (handler.name.CompareCaseIns(name))
			continue;

		result = handler.callback(this, handler, userData);
		break;
	}

	return result;
}

int IUIControl::RaiseEventUid(int uid, void* userData)
{
	int result = -1;
	for (const EvtHandler& handler : m_eventCallbacks)
	{
		if (handler.uid != uid)
			continue;

		result = handler.callback(this, handler, userData);
		break;
	}

	return result;
}

};
