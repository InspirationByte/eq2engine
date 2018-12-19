//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Editor configuration structure
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

#include "Math/DkMath.h"

struct editor_user_prefs_t
{
	bool		matsystem_threaded;

	int			default_grid_size;			// default grid size
	bool		zoom_2d_gridsnap;			// snap to grid when zoomed out in 2D views?
	EqString	default_material;		// startup material

	ColorRGB	background_color;			// background color
	ColorRGBA	grid1_color;				// small grid color
	ColorRGBA	grid2_color;				// big highlighted grid color
	ColorRGB	level_center_color;			// center axis color
	ColorRGB	selection_color;
};

extern editor_user_prefs_t* g_editorCfg;

void LoadEditorConfig();
void SaveEditorConfig();

#endif // EDITOR_CONFIG_H