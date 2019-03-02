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

void Label::DrawSelf( const IRectangle& rect )
{
	Vector2D scaling = CalcScaling();

	CRectangleTextLayoutBuilder rectLayout;
	rectLayout.SetRectangle(Rectangle_t(rect));

	// TODO: GetFontStyle
	eqFontStyleParam_t style;
	style.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_SCISSOR | TEXT_STYLE_USE_TAGS;
	style.align = m_textAlignment;
	style.scale = m_fontScale * scaling;
	style.layoutBuilder = &rectLayout;
	style.textColor = m_textColor;

	if(equi::Manager->GetFocus() == this)
	{
		style.textColor = ColorRGBA(1,1,0,1);	// TODO: selected text color
	}

	IVector2D pos = rect.GetLeftTop() + IVector2D(0, GetFont()->GetLineHeight(style)*0.5f + 4);

	// draw label
	GetFont()->RenderText(m_label.c_str(), pos, style);
}

};

DECLARE_EQUI_CONTROL(label, Label)
