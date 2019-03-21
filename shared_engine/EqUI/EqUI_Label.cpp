//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Label.h"

#include "EqUI_Manager.h"
#include "IFont.h"
#include "FontLayoutBuilders.h"

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

void Label::GetCalcFontStyle(eqFontStyleParam_t& style) const
{
	Vector2D scaling = CalcScaling();
	style.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_SCISSOR | TEXT_STYLE_USE_TAGS;
	style.align = m_textAlignment;
	style.scale = m_fontScale * scaling;
	
	style.textColor = m_textColor;
}

void Label::DrawSelf( const IRectangle& rect )
{
	CRectangleTextLayoutBuilder rectLayout;
	rectLayout.SetRectangle(Rectangle_t(rect));

	eqFontStyleParam_t style;
	GetCalcFontStyle(style);

	style.layoutBuilder = &rectLayout;

	if(equi::Manager->GetFocus() == this)
	{
		style.textColor = ColorRGBA(1,1,0,1);	// TODO: selected text color
	}

	IVector2D pos = rect.GetLeftTop() + IVector2D(0, GetFont()->GetLineHeight(style)*0.5f);

	// draw label
	GetFont()->RenderText(m_label.c_str(), pos, style);
}

};

DECLARE_EQUI_CONTROL(label, Label)
