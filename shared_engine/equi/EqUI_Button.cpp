//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI button
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "EqUI_Button.h"

#include "EqUI_Manager.h"

namespace equi
{

void DrawWindowRectangle(const AARectangle &rect, const ColorRGBA &color1, const ColorRGBA &color2);

Button::Button() : m_state(false)
{

}

void Button::DrawSelf( const IAARectangle& rect, bool scissorOn)
{
	ColorRGBA btnColor(0.8f);

	if(m_state && equi::Manager->GetMouseOver() == this)
		btnColor = ColorRGBA(0.6f,0.6f,0.6f,0.8f);

	DrawWindowRectangle(rect, btnColor, ColorRGBA(0.5f));

	eqFontStyleParam_t style;
	GetCalcFontStyle(style);

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
				RaiseEvent( "click", nullptr ); // TODO: ButtonClickEventData

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
