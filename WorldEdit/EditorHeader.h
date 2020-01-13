//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Editor main cycle
//////////////////////////////////////////////////////////////////////////////////

#ifndef EDITORHDR_H
#define EDITORHDR_H

#include "DebugInterface.h"
#include "EditorMainFrame.h"

#include "ILocalize.h"
#include "materialsystem/IMaterialSystem.h"

#include "EditorLevel.h"
#include "in_keys_ident.h"
#include "EntityDef.h"
#include "editor_config.h"
#include "utils/VirtualStream.h"
#include "eqParallelJobs.h"

extern float	g_realtime;
extern float	g_oldrealtime;
extern float	g_frametime;

extern int		g_gridsize;

extern CEditorLevel *g_pLevel;

// Tools setup
enum ToolType_e
{
	Tool_Selection = 0,
	Tool_Brush,
	Tool_Surface,
	Tool_Patch,
	Tool_Entity,
	Tool_PathTool,
	Tool_Clipper,
	Tool_VertexManip,
	Tool_Decals,

	Tool_Count
};

enum VertexPainterType_e
{
	PAINTER_NONE = 0,

	PAINTER_VERTEX_ADVANCE,		// advances vertex position
	PAINTER_VERTEX_SMOOTH,		// sets height level related to current painting position, with smooth
	PAINTER_VERTEX_SETLEVEL,	// sets height level related to current painting position, hard
	PAINTER_TEXTURETRANSITION,	// texture transition editor
};

void Editor_Init();

#endif // EDITORHDR_H