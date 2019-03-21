//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: EGUI label
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_LABEL_H
#define EQUI_LABEL_H

#include "equi_defs.h"
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
	IRectangle		GetClientScissorRectangle() const;

	// events
	bool			ProcessMouseEvents(float x, float y, int nMouseButtons, int flags) {return true;}
	bool			ProcessKeyboardEvents(int nKeyButtons, int flags) {return true;}

	void			DrawSelf( const IRectangle& rect);
};

};


#endif // EQUI_LABEL_H