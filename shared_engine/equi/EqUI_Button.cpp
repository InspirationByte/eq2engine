//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI button
//////////////////////////////////////////////////////////////////////////////////

#include "EqUI_Button.h"

#include "EqUI_Manager.h"
#include "font/IFont.h"

namespace equi
{

void DrawWindowRectangle(const Rectangle_t &rect, const ColorRGBA &color1, const ColorRGBA &color2);

Button::Button() : m_state(false)
{

}

void Button::DrawSelf( const IRectangle& rect )
{
	eqFontStyleParam_t style;
	ColorRGBA btnColor(0.8,0.8,0.8,0.8);

	if(m_state && equi::Manager->GetMouseOver() == this)
		btnColor = ColorRGBA(0.6,0.6,0.6,0.8);

	DrawWindowRectangle(rect, btnColor, ColorRGBA(0.5,0.5,0.5,0.5));

	// TODO: GetFontStyle

	style.styleFlag |= TEXT_STYLE_SHADOW | TEXT_STYLE_SCISSOR;
	style.scale = m_fontScale;

	IVector2D pos = rect.GetLeftTop() + IVector2D(0,25);

	// draw label
	GetFont()->RenderText(m_label.ToCString(), pos, style);
}

// events
bool Button::ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags)
{
	if(flags & UIEVENT_MOUSE_OUT)
		m_state = false;

	if(nMouseButtons & 1)
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
	return true;
}

};

DECLARE_EQUI_CONTROL(button, Button)
