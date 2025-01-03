//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI button
//////////////////////////////////////////////////////////////////////////////////

#pragma once
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
	bool			ProcessMouseEvents(const IVector2D& mousePos, const IVector2D& mouseDelta, int nMouseButtons, int flags) override;
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags) override;

	void			DrawSelf(const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder) override;

protected:
	bool			m_state;
};

};