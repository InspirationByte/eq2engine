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
	IAARectangle	GetClientScissorRectangle(int depth, const RenderContextAbstract& context) const override;
	void			DrawSelf( const IAARectangle& rect, IGPURenderPassRecorder* rendPassRecorder) override;
};

};
