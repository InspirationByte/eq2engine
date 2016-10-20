//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Label.h"

#include "EQUI_Manager.h"
#include "IFont.h"

namespace equi
{

void Label::DrawSelf( const IRectangle& rect )
{
	// TODO: GetFontStyle
	eqFontStyleParam_t style;
	style.styleFlag |= TEXT_STYLE_SHADOW;

	if(equi::Manager->GetFocus() == this)
	{
		style.textColor = ColorRGBA(1,1,0,1);
	}

	IVector2D pos = rect.GetLeftTop() + IVector2D(0,25);

	// draw label
	GetFont()->RenderText(m_label.c_str(), pos, style);
}

};

DECLARE_EQUI_CONTROL(label, Label)