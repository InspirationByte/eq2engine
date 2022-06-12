//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI
//////////////////////////////////////////////////////////////////////////////////

#pragma once

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
enum EUIBorders
{
	UI_BORDER_LEFT				= (1 << 0),
	UI_BORDER_TOP				= (1 << 1),
	UI_BORDER_RIGHT				= (1 << 2),
	UI_BORDER_BOTTOM			= (1 << 3),
};

enum EUIAlignment
{
	UI_ALIGN_LEFT = (1 << 0),
	UI_ALIGN_TOP = (1 << 1),
	UI_ALIGN_RIGHT = (1 << 2),
	UI_ALIGN_BOTTOM = (1 << 3),
	UI_ALIGN_HCENTER = (1 << 4),
	UI_ALIGN_VCENTER = (1 << 5),
};

enum EUIScalingMode
{
	UI_SCALING_NONE = 0,

	UI_SCALING_WIDTH,
	UI_SCALING_HEIGHT,

	UI_SCALING_INHERIT,

	UI_SCALING_ASPECT_W,
	UI_SCALING_ASPECT_H,
};

class IEvent
{
public:
	virtual ~IEvent() {}
};

};
