//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Label.h"

#include "EqUI_Manager.h"
#include "font/IFont.h"
#include "font/FontLayoutBuilders.h"
#include "materialsystem1/IMaterialSystem.h"

namespace equi
{

// drawn rectangle
IRectangle Label::GetClientScissorRectangle() const
{
	eqFontStyleParam_t style;
	GetCalcFontStyle(style);

	float lineHeight = GetFont()->GetLineHeight(style);

	IRectangle rect = BaseClass::GetClientRectangle();
	rect.vleftTop.y -= lineHeight * 0.5f;

	return rect;
}

void Label::DrawSelf( const IRectangle& rect, bool scissorOn)
{
	CRectangleTextLayoutBuilder rectLayout;
	rectLayout.SetRectangle(Rectangle_t(rect));

	IEqFont* font = GetFont();
	eqFontStyleParam_t style;
	GetCalcFontStyle(style);

	style.layoutBuilder = &rectLayout;

	if(!scissorOn)
		style.styleFlag &= ~TEXT_STYLE_SCISSOR;

	IVector2D pos = rect.GetLeftTop() + IVector2D(0, font->GetLineHeight(style)*0.5f);

	// draw label
	font->RenderText(m_label.ToCString(), pos, style);
}

};

DECLARE_EQUI_CONTROL(label, Label)
