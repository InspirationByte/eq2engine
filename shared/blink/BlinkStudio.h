//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: BLINK meshes of EGF models
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterial.h"

struct BlinkMesh;
typedef struct studioModelHeader_s studioHdr_t;

// The meshes of the body groups in bodyGroupFlags at lod (a missing LOD falls back to a lower one, as
// the renderer does) in the bind pose and model space, added to mesh: a batch per EGF mesh, the strips
// as triangles without the degenerate ones, zero normals for a mesh without them (Blender computes
// them). The batch material is the name of materials[materialIndex] (CEqStudioGeom::GetMaterials of a
// skin), else the material name stored in the EGF.
void BlinkMakeStudioMesh(const studioHdr_t& studio, ArrayCRef<IMaterialPtr> materials, int bodyGroupFlags, int lod, BlinkMesh& mesh);
