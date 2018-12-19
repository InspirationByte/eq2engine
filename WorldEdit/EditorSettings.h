//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Options dialog
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORSETTINGS_H
#define EDITORSETTINGS_H

#include "EditorHeader.h"

struct editorsettings_t
{
	void		Load();
	void		Save();

	// visual settings
	ColorRGB	background2Dcolor;
	ColorRGB	background3Dcolor;

	ColorRGB	gridcolor1;
	ColorRGB	gridcolor2;

	ColorRGB	selectioncolor1;	// selection preparation color
	ColorRGB	selectioncolor2;	// selection selected color
	ColorRGB	selectioncolor3;	// selection handles color

	bool		onlycenterhandleselection;	// select by center handles only
};

#endif // EDITORSETTINGS_H