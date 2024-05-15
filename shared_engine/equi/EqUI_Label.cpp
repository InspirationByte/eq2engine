//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "EqUI_Label.h"

#include "EqUI_Manager.h"
#include "font/FontLayoutBuilders.h"

#include "materialsystem1/IMaterialSystem.h"
#include "materialsystem1/MeshBuilder.h"

namespace equi
{

// drawn rectangle
IAARectangle Label::GetClientScissorRectangle() const
{
	FontStyleParam style;
	GetCalcFontStyle(style);

	float lineHeight = GetFont()->GetLineHeight(style);

	IAARectangle rect = BaseClass::GetClientRectangle();
	rect.leftTop.y -= lineHeight * 0.5f;

	return rect;
}

void Label::DrawSelf( const IAARectangle& rect, bool scissorOn, IGPURenderPassRecorder* rendPassRecorder)
{
	CRectangleTextLayoutBuilder rectLayout;
	rectLayout.SetRectangle(AARectangle(rect));

	IEqFont* font = GetFont();
	FontStyleParam style;
	GetCalcFontStyle(style);

	style.layoutBuilder = &rectLayout;

	if(!scissorOn)
		style.styleFlag &= ~TEXT_STYLE_SCISSOR;

	IVector2D pos = rect.GetLeftTop() + IVector2D(0, font->GetLineHeight(style)*0.5f);

	// draw label
	font->SetupRenderText(m_label.ToCString(), pos, style, rendPassRecorder);
}

};

DECLARE_EQUI_CONTROL(label, Label)
