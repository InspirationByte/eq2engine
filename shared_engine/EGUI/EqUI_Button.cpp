//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI button
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Button.h"
#include "in_keys_ident.h"

#include "EQUI_Manager.h"
#include "IFont.h"

namespace equi
{

void DrawAlphaFilledRectangle(const IRectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2);

Button::Button() : m_state(false)
{

}

void Button::DrawSelf( const IRectangle& rect )
{
	eqFontStyleParam_t style;
	ColorRGBA btnColor(0.8,0.8,0.8,0.8);

	if(m_state && equi::Manager->GetMouseOver() == this)
		btnColor = ColorRGBA(0.6,0.6,0.6,0.8);

	DrawAlphaFilledRectangle(rect, btnColor, ColorRGBA(0.5,0.5,0.5,0.5));

	// TODO: GetFontStyle
	
	style.styleFlag |= TEXT_STYLE_SHADOW;

	IVector2D pos = rect.GetLeftTop() + IVector2D(0,25);

	// draw label
	GetFont()->RenderText(m_label.c_str(), pos, style);
}

// events
bool Button::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{
	if(flags & UIEVENT_MOUSE_OUT)
		m_state = false;

	if(nMouseButtons == MOU_B1)
	{
		if(flags & UIEVENT_DOWN)
		{
			m_state = true;
		}
		else if(flags & UIEVENT_UP)
		{
			if(m_state)
				ProcessCommand( m_commandEvent.args );

			m_state = false;
		}
	}

	return true;
}

bool Button::ProcessKeyboardEvents(int nKeyButtons, int flags)
{
	if(nKeyButtons == KEY_ENTER)
	{
		// ExecuteButtonCommand()
	}

	return true;
}

};

DECLARE_EQUI_CONTROL(button, Button)