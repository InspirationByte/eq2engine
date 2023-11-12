//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IEqUI_Control.h"

namespace equi
{

// eq label class
class Label : public IUIControl
{
public:
	EQUI_CLASS(Label, IUIControl)

	Label() : IUIControl() {}
	~Label(){}

	// drawn rectangle
	IAARectangle		GetClientScissorRectangle() const;

	// events
	bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) {return true;}
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags) {return true;}

	void			DrawSelf( const IAARectangle& rect, bool scissorOn, IGPURenderPassRecorder* rendPassRecorder);
};

};
