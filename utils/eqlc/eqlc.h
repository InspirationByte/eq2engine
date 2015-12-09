//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Lightmap compiler main header
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQLC_H
#define EQLC_H

#include "eqlevel.h"
#include "core_base_header.h"
#include "DebugInterface.h"
#include "materialsystem/IMaterialSystem.h"
#include "EqGameLevel.h"
#include "IViewRenderer.h"
#include "cmd_pacifier.h"
#include "idkphysics.h"


extern CEqLevel*						g_pLevel;
extern int								g_lightmapSize;

extern DkList<dlight_t*>				g_lights;

//extern DkList<eqlevellightvolume_t>		g_lightvolumes;
extern EqString							g_worldName;

void GenerateLightmapTextures();

#endif // EQLC_H