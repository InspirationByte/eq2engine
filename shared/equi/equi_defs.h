//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace equi
{
// attaching to the parent box sides
enum EBorders
{
	UI_BORDER_LEFT		= (1 << 0),
	UI_BORDER_TOP		= (1 << 1),
	UI_BORDER_RIGHT		= (1 << 2),
	UI_BORDER_BOTTOM	= (1 << 3),
};

enum EAlignment
{
	UI_ALIGN_LEFT		= (1 << 0),
	UI_ALIGN_TOP		= (1 << 1),
	UI_ALIGN_RIGHT		= (1 << 2),
	UI_ALIGN_BOTTOM		= (1 << 3),
	UI_ALIGN_HCENTER	= (1 << 4),
	UI_ALIGN_VCENTER	= (1 << 5),
};

enum EScalingMode
{
	UI_SCALING_NONE		= 0,

	UI_SCALING_INHERIT,
	UI_SCALING_INHERIT_MIN,
	UI_SCALING_INHERIT_MAX,

	UI_SCALING_ASPECT_MIN,
	UI_SCALING_ASPECT_MAX,
	UI_SCALING_ASPECT_W,
	UI_SCALING_ASPECT_H,
};

enum EEventFlags
{
	UI_EVENT_DOWN			= (1 << 0),
	UI_EVENT_UP				= (1 << 1),

	UI_EVENT_MOUSE_MOVE		= (1 << 2),

	UI_EVENT_MOUSE_IN		= (1 << 3),
	UI_EVENT_MOUSE_OUT		= (1 << 4),

	UI_EVENT_MOD_CTRL		= (1 << 5),
	UI_EVENT_MOD_SHIFT		= (1 << 6),
};

};
