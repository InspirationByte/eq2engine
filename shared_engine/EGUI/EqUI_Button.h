//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI button
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_BUTTON_H
#define EQUI_BUTTON_H

#include "equi_defs.h"
#include "IEqUI_Control.h"

namespace equi
{

// eq label class
class Button : public IUIControl
{
public:
	EQUI_CLASS(Button, IUIControl)

	Button();
	~Button(){}

	// events
	bool			ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags);
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags);

	void			DrawSelf( const IRectangle& rect);

protected:
	bool			m_state;
};

};

#endif // EQUI_BUTTON_H