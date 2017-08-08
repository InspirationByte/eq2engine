//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_H
#define EQUI_H

#include "dktypes.h"

namespace equi
{

enum EEqUIEventFlags
{
	UIEVENT_DOWN			= (1 << 0),
	UIEVENT_UP				= (1 << 1),

	UIEVENT_MOUSE_MOVE		= (1 << 2),

	UIEVENT_MOUSE_IN		= (1 << 3),
	UIEVENT_MOUSE_OUT		= (1 << 4),

	UIEVENT_MOD_CTRL		= (1 << 5),
	UIEVENT_MOD_SHIFT		= (1 << 6),
};

//--------------------------------------------------------

// attaching to the parent box sides
enum EUIAnchors
{
	//UI_ANCHOR_LEFT				= (1 << 0),
	//UI_ANCHOR_TOP				= (1 << 1),
	UI_ANCHOR_RIGHT				= (1 << 2),
	UI_ANCHOR_BOTTOM			= (1 << 3),

	UI_ANCHOR_SCALING_WIDE		= (1 << 4),
	UI_ANCHOR_SCALING_TALL		= (1 << 5),
	UI_ANCHOR_SCALING_ASPECT	= (1 << 6),		// controls are scaled with aspect ratio (box scaled as box)
};

};

#endif // EQUI_H
