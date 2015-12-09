//******************* Copyright (C) Illusion Way, L.L.C 2011 *********************
//
// Description: DarkTech Engine decal builder
//
// Thanks to donor's (Tenebrae) developers
//
//****************************************************************************

#include "DebugInterface.h"
#include "Platform.h"
#include "Math/DkMath.h"
#include "EngineBSPLoader.h"
#include "scene_def.h"

#ifndef DECALBUILDER_H
#define DECALBUILDER_H

decal_t *R_SpawnDecal(Vector3D &center, Vector3D &normal, Vector3D &tangent, enginebspnode_t *nodes, bsphwleaf_t* leaves, float radius);
decal_t *R_SpawnCustomDecal(Vector3D &center, Vector3D &mins, Vector3D &maxs, enginebspnode_t *nodes, bsphwleaf_t* leaves);

#endif // DECALBUILDER_H