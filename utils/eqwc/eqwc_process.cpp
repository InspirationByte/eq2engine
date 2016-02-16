//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Loader and processor
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"

// data at load.
DkList<cwsurface_t*>	g_surfacemodels;	// surface models
//DkList<cwbrush_t*>		g_brushes;			// brush models that will be casted to surface models automatically

// pre-final data
DkList<cwsurface_t*>	g_surfaces;			// surface models, to be here may be tesselated if terrain patch
DkList<cwentity_t*>		g_entities;			// world entities
DkList<cwsector_t>		g_sectors;			// sectors

DkList<IMaterial*>		g_materials;		// materials

DkList<Vector3D>		g_navmeshverts;
DkList<int>				g_navmeshindices;

DkList<cwsurface_t*>	g_occlusionsurfaces;

DkList<eqworldlayer_t*>	g_layers;


Vector2D UTIL_StringToVector2(const char *str)
{
	Vector2D vec(0,0);

	sscanf(str, "%f %f", &vec.x, &vec.y);

	return vec;
}

ColorRGB UTIL_StringToColor3(const char *str)
{
	ColorRGB col(0,0.5f,0);

	sscanf(str, "%f %f %f", &col.x, &col.y, &col.z);

	return col;
}

ColorRGBA UTIL_StringToColor4(const char *str)
{
	ColorRGBA col(0,0.5f,0,1);

	sscanf(str, "%f %f %f %f", &col.x, &col.y, &col.z, &col.w);

	return col;
}

int	ComputeCompileSurfaceFlags(IMaterial* pMaterial)
{
	// preload it first
	if(pMaterial && pMaterial->GetShader())
		pMaterial->GetShader()->InitParams();

	int nFlags = 0;

	IMatVar* pNodraw = pMaterial->FindMaterialVar("nodraw");
	IMatVar* pNoCollide = pMaterial->FindMaterialVar("nocollide");
	IMatVar* pOcclusion = pMaterial->FindMaterialVar("occlusion");
	IMatVar* pNoLightmap = pMaterial->FindMaterialVar("nolightmap");
		
	if(pNodraw && pNodraw->GetInt() > 0)
		nFlags |= CS_FLAG_NODRAW;

	if(pNoCollide && pNoCollide->GetInt() > 0)
		nFlags |= CS_FLAG_NOCOLLIDE;

	if(pOcclusion && pOcclusion->GetInt() > 0)
		nFlags |= (CS_FLAG_OCCLUDER | CS_FLAG_NOCOLLIDE | CS_FLAG_NODRAW);

	if(pMaterial->GetShader() && (pMaterial->GetShader()->GetFlags() & MATERIAL_FLAG_ISTEXTRANSITION))
		nFlags |= CS_FLAG_MULTIMATERIAL;

	if((pNoLightmap && pNoLightmap->GetInt() > 0))
		nFlags |= CS_FLAG_NOLIGHTMAP;

	if(pMaterial->GetShader() && (pMaterial->GetShader()->GetFlags() & MATERIAL_FLAG_ISSKY))
		nFlags |= (CS_FLAG_NOLIGHTMAP | CS_FLAG_DONTSUBDIVIDE | CS_FLAG_NOCOLLIDE);

	return nFlags;
}

// parses entity
void ParseEntity(kvkeybase_t* pSection)
{
	cwentity_t* pEntity = new cwentity_t;

	kvkeybase_t* pParamsSec = pSection->FindKeyBase("params", KV_FLAG_SECTION);

	for(int i = 0; i < pParamsSec->keys.numElem(); i++)
	{
		ekeyvalue_t pair;
		strcpy(pair.name, pParamsSec->keys[i]->name);
		strcpy(pair.value, pParamsSec->keys[i]->values[0]);

		pEntity->keys.append(pair);
	}
	
	g_entities.append(pEntity);
};

// parses brush
cwbrush_t* ParseBrush(kvkeybase_t* pSection)
{
	cwbrush_t* pBrush = new cwbrush_t;

	kvkeybase_t* pFacesSec = pSection->FindKeyBase("faces", KV_FLAG_SECTION);

	int numFaces = pFacesSec->keys.numElem();

	pBrush->flags = 0;

	// alloc faces
	pBrush->faces = new cwbrushface_t[numFaces];
	pBrush->numfaces = numFaces;
	
	for(int i = 0; i < numFaces; i++)
	{
		kvkeybase_t* pThisFace = pFacesSec->keys[i];
		if(!pThisFace->keys.numElem())
			continue;

		cwbrushface_t *face = &pBrush->faces[i];
		face->flags = 0;
		face->winding = NULL;
		face->visibleHull = NULL;
		face->brush = pBrush;

		kvkeybase_t* pPair = pThisFace->FindKeyBase("nplane");

		Vector4D plane = UTIL_StringToColor4(pPair->values[0]);

		face->Plane.normal = plane.xyz();
		face->Plane.offset = plane.w;

		pPair = pThisFace->FindKeyBase("uplane");
		plane = UTIL_StringToColor4(pPair->values[0]);

		face->UAxis.normal = plane.xyz();
		face->UAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("vplane");
		plane = UTIL_StringToColor4(pPair->values[0]);

		face->VAxis.normal = plane.xyz();
		face->VAxis.offset = plane.w;

		pPair = pThisFace->FindKeyBase("texscale");
		face->vScale = UTIL_StringToVector2(pPair->values[0]);

		/*
		pPair = pThisFace->FindKey("rotation");
		face.fRotation = atof(pPair->value);
		*/

		pPair = pThisFace->FindKeyBase("material");
		face->pMaterial = materials->FindMaterial(pPair->values[0], true);

		pPair = pThisFace->FindKeyBase("smoothinggroup");
		face->nSmoothingGroup = atoi(pPair->values[0]);

		face->flags = ComputeCompileSurfaceFlags(face->pMaterial);

		pPair = pThisFace->FindKeyBase("nocollide");
		face->flags |= (KV_GetValueBool(pPair, false) ? CS_FLAG_NOCOLLIDE : 0);

		pPair = pThisFace->FindKeyBase("nosubdivision");
		face->flags |= (KV_GetValueBool(pPair, false) ? CS_FLAG_DONTSUBDIVIDE : 0);

		IMatVar* pVar = face->pMaterial->FindMaterialVar("roomfiller");
		if(pVar)
			pBrush->flags |= (pVar->GetInt() ? BRUSH_FLAG_ROOMFILLER : 0);

		pVar = face->pMaterial->FindMaterialVar("areaportal");
		if(pVar)
			pBrush->flags |= (pVar->GetInt() ? BRUSH_FLAG_AREAPORTAL : 0);

		if(	(face->pMaterial->GetFlags() & MATERIAL_FLAG_WATER))
			pBrush->flags |= BRUSH_FLAG_WATER;
	}

	return pBrush;

	//g_brushes.append(pBrush);
}

struct ReadWriteSurfaceTexture_t
{
	char		material[256];	// material and texture
	Plane		UAxis;
	Plane		VAxis;
	Plane		Plane;

	float		rotate;
	Vector2D	scale;

	int			nFlags;
	int			nTesseleation;
};

// parses surface and loads data
void ParseAndLoadSurface(kvkeybase_t* pSection)
{
	// get model name
	EqString leveldir(_Es("worlds/")+worldGlobals.worldName.GetData());
	kvkeybase_t* pPair = pSection->FindKeyBase("meshfile");

	EqString object_full_filename(leveldir + pPair->values[0]);

	IFile* pStream = g_fileSystem->Open(object_full_filename.GetData(), "rb");

	if(!pStream)
	{
		MsgError("Cannot open file %s\n", object_full_filename.GetData());
		exit(0);
	}

	cwsurface_t* surf = new cwsurface_t;

	// skip editable type
	int nType;
	pStream->Read(&nType, 1, sizeof(int));

	// read surface type
	int nSurfType;
	pStream->Read(&nSurfType, 1, sizeof(int));

	surf->type = (SurfaceType_e)nSurfType;

	// read vertex count and index count
	pStream->Read(&surf->numVertices, 1, sizeof(int));
	pStream->Read(&surf->numIndices, 1, sizeof(int));

	// allocate buffers
	surf->pVerts = new eqlevelvertex_t[surf->numVertices];
	surf->pIndices = new int[surf->numIndices];

	// read vertex and index data
	pStream->Read(surf->pVerts, 1, sizeof(eqlevelvertex_t) * surf->numVertices);
	pStream->Read(surf->pIndices, 1, sizeof(int) * surf->numIndices);

	Vector3D vNormal;

	// read surface shared normal
	pStream->Read(&vNormal, 1, sizeof(Vector3D));

	// read texture projection info
	ReadWriteSurfaceTexture_t surfData;

	pStream->Read(&surfData, 1, sizeof(ReadWriteSurfaceTexture_t));

	surf->surfflags = surfData.nFlags;
	surf->nTesselation = surfData.nTesseleation;

	surf->pMaterial = materials->FindMaterial(surfData.material, true);

	surf->flags = ComputeCompileSurfaceFlags(surf->pMaterial);

	if(surf->surfflags & STFL_NOCOLLIDE)
		surf->flags |= CS_FLAG_NOCOLLIDE;

	if(surf->surfflags & STFL_NOSUBDIVISION)
		surf->flags |= CS_FLAG_DONTSUBDIVIDE;

	// read terrain patch sizes
	pStream->Read(&surf->nWide, 1, sizeof(int));
	pStream->Read(&surf->nTall, 1, sizeof(int));

	g_surfacemodels.append(surf);

	// before filesystem was changed I have forgotten to destroy the file handle :D
	g_fileSystem->Close(pStream);
}

void ParseAndLoadDecal(kvkeybase_t* pSection)
{
	cwdecal_t* decal = new cwdecal_t;

	kvkeybase_t* pair = pSection->FindKeyBase("origin");
	if(pair)
		decal->origin = UTIL_StringToColor3(pair->values[0]);

	pair = pSection->FindKeyBase("size");
	if(pair)
		decal->size = UTIL_StringToColor3(pair->values[0]);

	pair = pSection->FindKeyBase("material");
	if(pair)
		decal->pMaterial = materials->FindMaterial(pair->values[0], true);

	pair = pSection->FindKeyBase("nplane");

	Vector4D plane;

	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		decal->projaxis = plane.xyz();
	}

	pair = pSection->FindKeyBase("uplane");
	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		decal->uaxis = plane.xyz();
	}

	pair = pSection->FindKeyBase("vplane");
	if(pair)
	{
		plane = UTIL_StringToColor4(pair->values[0]);

		decal->vaxis = plane.xyz();
	}

	pair = pSection->FindKeyBase("texscale");
	if(pair)
		decal->texscale = UTIL_StringToVector2(pair->values[0]);

	pair = pSection->FindKeyBase("rotation");
	if(pair)
		decal->rotate = atof(pair->values[0]);

	pair = pSection->FindKeyBase("customtex");
	if(pair)
		decal->projectcoord = atoi(pair->values[0]) > 0;

	g_decals.append(decal);
}

void FreeSurface(cwsurface_t* pSurf)
{
	// free the surface
	delete [] pSurf->pIndices;
	delete [] pSurf->pVerts;
	delete pSurf;
}

void FreeLitSurface(cwlitsurface_t* pSurf)
{
	// free the surface
	delete [] pSurf->pIndices;
	delete [] pSurf->pVerts;
	delete pSurf;
}

// parses world data
void ParseWorldData(kvkeybase_t* pLevelSection)
{
	//g_brushes.resize(pLevelSection->sections.numElem());
	g_surfacemodels.resize(pLevelSection->keys.numElem());

	g_sectors.resize(pLevelSection->keys.numElem());

	MsgInfo("Parsing world soruce file...\n");

	g_pBrushList = NULL;

	g_numBrushes = 0;

	// load world
	for(int i = 0; i < pLevelSection->keys.numElem(); i++)
	{	
		if(!pLevelSection->keys[i]->keys.numElem())
			continue;

		if(( !stricmp("static", pLevelSection->keys[i]->name) || 
			 !stricmp("terrainpath", pLevelSection->keys[i]->name)))
		{
			kvkeybase_t* pObjectSection = pLevelSection->keys[i];

			// parse and load tesselated surface
			ParseAndLoadSurface(pObjectSection);
		}		
		else if(!stricmp("brush", pLevelSection->keys[i]->name))// && !g_bOnlyEnts)
		{
			kvkeybase_t* pObjectSection = pLevelSection->keys[i];

			// parse brush
			cwbrush_t* pBrush = ParseBrush(pObjectSection);
			pBrush->next = g_pBrushList;
			g_pBrushList = pBrush;

			g_numBrushes++;
		}
		else if(!stricmp("entity", pLevelSection->keys[i]->name))
		{
			kvkeybase_t* pObjectSection = pLevelSection->keys[i];

			// parse entity
			ParseEntity(pObjectSection);
		}
		else if(!stricmp("decal", pLevelSection->keys[i]->name))
		{
			kvkeybase_t* pObjectSection = pLevelSection->keys[i];

			// parse entity
			ParseAndLoadDecal(pObjectSection);
		}
	}

	Msg("Brushes: %d\n", g_numBrushes);
	Msg("Surfaces: %d\n", g_surfacemodels.numElem());
	Msg("Entities: %d\n", g_entities.numElem());
	Msg("Decals: %d\n", g_decals.numElem());

	Msg("\n");
}

void MergeLitSurfaceIntoList(cwlitsurface_t* pSurface, DkList<cwlitsurface_t*> &surfaces, bool bCheckTranslucents)
{
	int nSurfaceIndex = -1;
	for(int i = 0; i < surfaces.numElem(); i++)
	{
		int surfFlags = pSurface->pMaterial->GetFlags();
		bool isTranslucent = false;

		if(bCheckTranslucents)
		{
			if(	(surfFlags & MATERIAL_FLAG_TRANSPARENT) ||
				(surfFlags & MATERIAL_FLAG_ADDITIVE) || 
				(surfFlags & MATERIAL_FLAG_MODULATE))
			{
				isTranslucent = true;
			}
		}

		if(surfaces[i]->pMaterial == pSurface->pMaterial && !isTranslucent && (surfaces[i]->lightmap_id == pSurface->lightmap_id))
		{
			nSurfaceIndex = i;
			break;
		}
	}

	if(nSurfaceIndex == -1)
	{
		cwlitsurface_t* pNewSurface = new cwlitsurface_t;
		surfaces.append(pNewSurface);

		*pNewSurface = *pSurface;

		pNewSurface->pVerts = new eqlevelvertexlm_t[pSurface->numVertices];
		pNewSurface->pIndices = new int[pSurface->numIndices];

		// copy data
		memcpy(pNewSurface->pVerts, pSurface->pVerts, sizeof(eqlevelvertexlm_t)*pSurface->numVertices);
		memcpy(pNewSurface->pIndices, pSurface->pIndices, sizeof(int)*pSurface->numIndices);
	}
	else
	{
		// reallocate index and vertex buffers
		cwlitsurface_t* pNewSurface = surfaces[nSurfaceIndex];

		eqlevelvertexlm_t*	pVerts	= new eqlevelvertexlm_t[pNewSurface->numVertices + pSurface->numVertices];
		int* pIndices				= new int[pNewSurface->numIndices + pSurface->numIndices];

		// copy old contents to new buffers
		memcpy(pVerts, pNewSurface->pVerts, sizeof(eqlevelvertexlm_t)*pNewSurface->numVertices);
		memcpy(pIndices, pNewSurface->pIndices, sizeof(int)*pNewSurface->numIndices);

		// copy new contents
		memcpy(pVerts + pNewSurface->numVertices, pSurface->pVerts, sizeof(eqlevelvertexlm_t)*pSurface->numVertices);

		// indices is always remapped
		for(int i = 0; i < pSurface->numIndices; i++)
			pIndices[pNewSurface->numIndices+i] = (pSurface->pIndices[i] + pNewSurface->numVertices);

		pNewSurface->numVertices += pSurface->numVertices;
		pNewSurface->numIndices += pSurface->numIndices;

		delete [] pNewSurface->pVerts;
		delete [] pNewSurface->pIndices;

		pNewSurface->pVerts = pVerts;
		pNewSurface->pIndices = pIndices;
	}
}

int sort_by_material( cwlitsurface_t* const &a, cwlitsurface_t* const &b )
{
	// HACK: Using pointers for qsort
	int ptr_diff = (b->pMaterial - a->pMaterial);

	return ptr_diff;
}

void OptimizeLitGeometry()
{
	MsgInfo("Optimizing %d lit surfaces\n", g_litsurfaces.numElem());

	DkList<cwlitsurface_t*> new_list;

	for(int i = 0; i < g_litsurfaces.numElem(); i++)
	{
		MergeLitSurfaceIntoList(g_litsurfaces[i], new_list, true);
		FreeLitSurface( g_litsurfaces[i] );
	}

	g_litsurfaces.clear();

	// now let's collapse the whole geometry on the boxes


	MsgInfo("Got %d lit surfaces after optimization\n", new_list.numElem());

	g_litsurfaces.append(new_list);

	g_litsurfaces.sort(sort_by_material);
}

// builds world
void BuildWorld()
{
	if(worldGlobals.bOnlyPhysics)
	{
		// convert brushes to the surfaces with CSG option if specified
		ProcessBrushes();

		// surface processing
		ProcessSurfaces();

		BuildPhysicsGeometry();

		return;
	}

	if(worldGlobals.bOnlyEnts)
	{
		// convert brushes to the surfaces with CSG option if specified
		ProcessBrushes();

		// surface processing
		ProcessSurfaces();

		// generate decals from our surfaces
		GenerateDecals();

		return;
	}

	// convert brushes to the surfaces with CSG option if specified
	ProcessBrushes();

	// surface processing
	ProcessSurfaces();

	// generate decals from our surfaces
	GenerateDecals();

	MsgWarning("\nLoaded and generated %d surfaces...\n", g_surfaces.numElem());

	// lightmap uv generation
	ParametrizeLightmapUVs();

	// Optimize geometry
	OptimizeLitGeometry();

	// Subdivide on sectors
	MakeSectors();

	// create water info
	MakeWaterInfosFromVolumes();

	// build a special file that contains occluders
	// BuildOcclusionGeometry();

	// Make physics geometry
	BuildPhysicsGeometry();
}