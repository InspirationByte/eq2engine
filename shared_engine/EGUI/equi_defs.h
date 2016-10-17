//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq UI
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQUI_H
#define EQUI_H

#include "dktypes.h"

enum EEqUIEventFlags
{
	UIEVENT_DOWN			= (1 << 0),
	UIEVENT_UP				= (1 << 1),

	UIEVENT_MOUSE_MOVE		= (1 << 2),

	UIEVENT_MOD_CTRL		= (1 << 3),
	UIEVENT_MOD_SHIFT		= (1 << 4),
};

enum EUIElementType
{
	EQUI_INVALID	= -1,

	EQUI_PANEL		= 0,
	EQUI_BUTTON,
	EQUI_IMAGE,
	EQUI_TEXTINPUT,

	EQUI_TYPES,
};

static const char* s_equi_typestrings[] =
{
	"panel",
	"button",
	"image",
	"input",
	NULL
};

//--------------------------------------------------------

enum EUIAnchors
{
	UI_ANCHOR_LEFT		= (1 << 0),
	UI_ANCHOR_TOP		= (1 << 1),
	UI_ANCHOR_RIGHT		= (1 << 2),
	UI_ANCHOR_BOTTOM	= (1 << 3),
};

#endif // EQUI_H
