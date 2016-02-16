//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Compiler functions to write compiled world data
//////////////////////////////////////////////////////////////////////////////////

#include "eqwc.h"

void AddLump(int nLump, ubyte *pData, int nDataSize, eqworldhdr_t* pHdr, DKFILE* pFile)
{
	eqworldlump_t lump;
	lump.data_type = nLump;
	lump.data_size = nDataSize;

	// write lump hdr
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	// write lump data
	g_fileSystem->Write(pData, 1, nDataSize, pFile);

	pHdr->num_lumps++;
}


// writes key-values section.
void KV_WriteToFile_r(kvkeybase_t* pKeyBase, DKFILE* pFile, int nTabs, bool bOldFormat);

void AddKVLump(int nLump, KeyValues &kv, eqworldhdr_t* pHdr, DKFILE* pFile)
{
	int begin_offset = g_fileSystem->Tell(pFile);

	eqworldlump_t lump;
	lump.data_type = nLump;
	lump.data_size = 0;

	// write lump hdr
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	int data_pos = g_fileSystem->Tell(pFile);

	// write, without tabs
	KV_WriteToFile_r(kv.GetRootSection(), pFile, 0, false);

	int cur_pos = g_fileSystem->Tell(pFile);

	g_fileSystem->Seek(pFile, begin_offset, SEEK_SET);

	lump.data_size = cur_pos - data_pos;

	// write lump hdr agian
	g_fileSystem->Write(&lump, 1, sizeof(eqworldlump_t), pFile);

	// move forward
	g_fileSystem->Seek(pFile, cur_pos, SEEK_SET);

	pHdr->num_lumps++;
}

void WCOccSurfaceToFile(cwsurface_t* pCompilerSurf, eqocclusionsurf_t* pEngineSurf, DkList<Vector3D> &verts, DkList<int> &indices)
{
	pEngineSurf->firstindex = indices.numElem();
	pEngineSurf->firstvertex = verts.numElem();

	indices.resize(indices.numElem()+pCompilerSurf->numIndices);

	for(int i = 0; i < pCompilerSurf->numIndices; i++)
		indices.append(pCompilerSurf->pIndices[i] + pEngineSurf->firstvertex);

	verts.resize(verts.numElem()+pCompilerSurf->numVertices);

	for(int i = 0; i < pCompilerSurf->numVertices; i++)
		verts.append( pCompilerSurf->pVerts[i].position );

	ComputeSurfaceBBOX(pCompilerSurf);

	pEngineSurf->mins = pCompilerSurf->bbox.minPoint;
	pEngineSurf->maxs = pCompilerSurf->bbox.maxPoint;
	pEngineSurf->numindices = pCompilerSurf->numIndices;
	pEngineSurf->numvertices = pCompilerSurf->numVertices;
}

// writes rendering geometry geometry
void WriteOcclusionGeometry()
{
	EqString world_geom_path(varargs("worlds/%s/occlusion.build", worldGlobals.worldName.GetData()));

	if(g_occlusionsurfaces.numElem() == 0)
	{
		g_fileSystem->RemoveFile(world_geom_path.GetData(), SP_MOD);
		Msg("No occlusion surfaces found\n");
		return;
	}

	eqworldhdr_t geomhdr;
	geomhdr.ident = EQWF_IDENT;
	geomhdr.version = EQWF_VERSION;

	geomhdr.num_lumps = 0;

	DKFILE* pFile = g_fileSystem->Open(world_geom_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_geom_path.GetData());

		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);

		DkList<eqocclusionsurf_t>	occ_surfs;
		DkList<Vector3D>			vertices;
		DkList<int>					indices;

		for(int i = 0; i < g_occlusionsurfaces.numElem(); i++)
		{
			eqocclusionsurf_t surf;
			WCOccSurfaceToFile(g_occlusionsurfaces[i], &surf, vertices, indices);

			occ_surfs.append(surf);
		}

		AddLump(EQWLUMP_OCCLUSION_SURFACES, (ubyte*)occ_surfs.ptr(), occ_surfs.numElem()*sizeof(eqocclusionsurf_t), &geomhdr, pFile);
		AddLump(EQWLUMP_OCCLUSION_INDICES, (ubyte*)indices.ptr(), indices.numElem()*sizeof(int), &geomhdr, pFile);
		AddLump(EQWLUMP_OCCLUSION_VERTS, (ubyte*)vertices.ptr(), vertices.numElem()*sizeof(Vector3D), &geomhdr, pFile);

		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);
		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
	{
		MsgError("ERROR! Cannot open file %s for writing\n", world_geom_path.GetData());
	}
}

void WCSurfaceToFile(cwlitsurface_t* pCompilerSurf, eqlevelsurf_t* pEngineSurf, DkList<eqlevelvertexlm_t> &verts, DkList<int> &indices)
{
	pEngineSurf->firstindex = indices.numElem();
	pEngineSurf->firstvertex = verts.numElem();

	pEngineSurf->flags = 0;

	// apply nonlightmap flag
	if(pCompilerSurf->lightmap_id == -1)
		pEngineSurf->flags |= EQSURF_FLAG_NOLIGHTMAP;

	bool bTranslucent = false;

	// material translucency flag for for better rendering
	if(pCompilerSurf->pMaterial)
	{
		// this is a translucent surface
		if(	(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_TRANSPARENT) ||
			(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_ADDITIVE) || 
			(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_MODULATE))
		{
			pEngineSurf->flags |= EQSURF_FLAG_TRANSLUCENT;
			bTranslucent = true;
		}

		if(	(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_ISSKY))
			pEngineSurf->flags |= EQSURF_FLAG_SKY;

		if(	(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_BLOCKLIGHT))
			pEngineSurf->flags |= EQSURF_FLAG_BLOCKLIGHT;

		if(	(pCompilerSurf->pMaterial->GetFlags() & MATERIAL_FLAG_WATER))
			pEngineSurf->flags |= EQSURF_FLAG_WATER;
	}

	indices.resize(indices.numElem()+pCompilerSurf->numIndices);

	for(int i = 0; i < pCompilerSurf->numIndices; i++)
		indices.append(pCompilerSurf->pIndices[i] + pEngineSurf->firstvertex);

	verts.resize(verts.numElem()+pCompilerSurf->numVertices);

	for(int i = 0; i < pCompilerSurf->numVertices; i++)
		verts.append( pCompilerSurf->pVerts[i] );

	pEngineSurf->mins = pCompilerSurf->bbox.minPoint;
	pEngineSurf->maxs = pCompilerSurf->bbox.maxPoint;

	pEngineSurf->numindices = pCompilerSurf->numIndices;
	pEngineSurf->numvertices = pCompilerSurf->numVertices;
	pEngineSurf->lightmap_id = pCompilerSurf->lightmap_id;

	pEngineSurf->material_id = g_materials.addUnique(pCompilerSurf->pMaterial);
}

extern int sort_by_material( cwlitsurface_t* const &a, cwlitsurface_t* const &b );

void GenerateVolumeGeomLumps(cwroomvolume_t* roomvolume, eqworldhdr_t* pHdr, DKFILE* pFile, DkList<eqlevelvertexlm_t> &vertices, DkList<int> &indices)
{
	DkList<eqlevelsurf_t>	surfaces;

	roomvolume->surfaces.sort(sort_by_material);

	for(int i = 0; i < roomvolume->surfaces.numElem(); i++)
	{
		eqlevelsurf_t surf;
		WCSurfaceToFile(roomvolume->surfaces[i], &surf, vertices, indices);

		surfaces.append(surf);
	}

	// add four lumps
	AddLump(EQWLUMP_SURFACES, (ubyte*)surfaces.ptr(), surfaces.numElem()*sizeof(eqlevelsurf_t), pHdr, pFile);
}

void WriteRooms(eqworldhdr_t* pHdr, DKFILE* pFile);
void WriteWater(eqworldhdr_t* pHdr, DKFILE* pFile);

void WriteDecals()
{
	// remove old decal file
	g_fileSystem->RemoveFile(varargs("worlds/%s/decals.build", worldGlobals.worldName.GetData()), SP_MOD);

	eqworldhdr_t geomhdr;
	geomhdr.ident = EQWF_IDENT;
	geomhdr.version = EQWF_VERSION;

	geomhdr.num_lumps = 0;

	EqString world_geom_path(varargs("worlds/%s/geometry.build", worldGlobals.worldName.GetData()));

	DKFILE* pFile = g_fileSystem->Open(world_geom_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_geom_path.GetData());

		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);

		DkList<eqlevelvertexlm_t>	vertices;
		DkList<int>					indices;

		for(int i = 0; i < g_roomList.numElem(); i++)
		{
			for(int j = 0; j < g_roomList[i]->volumes.numElem(); j++)
				GenerateVolumeGeomLumps(g_roomList[i]->volumes[j], &geomhdr, pFile, vertices, indices);
		}

		AddLump(EQWLUMP_DETAIL_INDICES, (ubyte*)indices.ptr(), indices.numElem()*sizeof(int), &geomhdr, pFile);
		AddLump(EQWLUMP_DETAIL_VERTICES, (ubyte*)vertices.ptr(), vertices.numElem()*sizeof(eqlevelvertexlm_t), &geomhdr, pFile);

		WriteRooms(&geomhdr, pFile);

		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);
		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
	{
		MsgError("ERROR! Cannot open file %s for writing\n", world_geom_path.GetData());
	}
}

// writes rendering geometry geometry
void WriteRenderGeometry()
{
	// remove all lightmaps because we rebuilded the level
	for(int i = 0; i < 32; i++)
		g_fileSystem->RemoveFile(varargs("worlds/%s/lm#%d.dds", worldGlobals.worldName.GetData(), i), SP_MOD);

	eqworldhdr_t geomhdr;
	geomhdr.ident = EQWF_IDENT;
	geomhdr.version = EQWF_VERSION;

	geomhdr.num_lumps = 0;

	EqString world_geom_path(varargs("worlds/%s/geometry.build", worldGlobals.worldName.GetData()));

	DKFILE* pFile = g_fileSystem->Open(world_geom_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_geom_path.GetData());

		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);

		DkList<eqlevelvertexlm_t>	vertices;
		DkList<int>					indices;

		for(int i = 0; i < g_roomList.numElem(); i++)
		{
			for(int j = 0; j < g_roomList[i]->volumes.numElem(); j++)
				GenerateVolumeGeomLumps(g_roomList[i]->volumes[j], &geomhdr, pFile, vertices, indices);
		}

		AddLump(EQWLUMP_INDICES, (ubyte*)indices.ptr(), indices.numElem()*sizeof(int), &geomhdr, pFile);
		AddLump(EQWLUMP_VERTICES, (ubyte*)vertices.ptr(), vertices.numElem()*sizeof(eqlevelvertexlm_t), &geomhdr, pFile);

		// write room info
		WriteRooms(&geomhdr, pFile);

		// write water info
		WriteWater(&geomhdr, pFile);

		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&geomhdr, 1, sizeof(geomhdr),pFile);
		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
	{
		MsgError("ERROR! Cannot open file %s for writing\n", world_geom_path.GetData());
	}
}

// writes physics geometry
void WritePhysics()
{
	eqworldhdr_t physhdr;
	physhdr.ident = EQWF_IDENT;
	physhdr.version = EQWF_VERSION;

	physhdr.num_lumps = 0;

	EqString world_geom_path(varargs("worlds/%s/physics.build", worldGlobals.worldName.GetData()));

	DKFILE* pFile = g_fileSystem->Open(world_geom_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_geom_path.GetData());

		g_fileSystem->Write(&physhdr, 1, sizeof(physhdr),pFile);

		AddLump( EQWLUMP_PHYS_VERTICES, (ubyte*)g_physicsverts.ptr(), g_physicsverts.numElem()*sizeof(eqphysicsvertex_t), &physhdr, pFile);
		AddLump( EQWLUMP_PHYS_INDICES, (ubyte*)g_physicsindices.ptr(), g_physicsindices.numElem()*sizeof(int), &physhdr, pFile);
		AddLump( EQWLUMP_PHYS_SURFACES, (ubyte*)g_physicssurfs.ptr(), g_physicssurfs.numElem()*sizeof(eqlevelphyssurf_t), &physhdr, pFile);

		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&physhdr, 1, sizeof(physhdr),pFile);
		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
	{
		MsgError("ERROR! Cannot open file %s for writing\n", world_geom_path.GetData());
	}	
}

void WriteEntities(DKFILE* pFile, eqworldhdr_t* pHdr)
{
	KeyValues entity_data;

	//kvkeyvaluepair_t
	for(int i = 0; i < g_entities.numElem(); i++)
	{
		if(worldGlobals.bNoLightmap)
		{
			ekeyvalue_t* pClassName = g_entities[i]->FindKey("classname");

			// check the lights
			if(		!stricmp(pClassName->value, "light_point") 
				||	!stricmp(pClassName->value, "light_spot") 
				||	!stricmp(pClassName->value, "light_sun") )
			{
				ekeyvalue_t* pDynamic = g_entities[i]->FindKey("dynamic");

				if( pDynamic && atoi(pDynamic->value) == 0 )
					continue;
			}
		}
		
		kvkeybase_t* newSec = entity_data.GetRootSection()->AddKeyBase("entitydesc");

		for(int j = 0; j < g_entities[i]->keys.numElem(); j++)
			newSec->AddKeyBase(g_entities[i]->keys[j].name, g_entities[i]->keys[j].value);
	}

	AddKVLump(EQWLUMP_ENTITIES, entity_data, pHdr, pFile);
}

void WriteNavMesh()
{
	EqString navmesh_geom_path(varargs("worlds/%s/navmesh.ai", worldGlobals.worldName.GetData()));

	if(!worldGlobals.bOnlyEnts)
	{
		g_fileSystem->RemoveFile(navmesh_geom_path.GetData(), SP_MOD);
		return;
	}
}

void WriteRooms(eqworldhdr_t* pHdr, DKFILE* pFile)
{
	/*
	DkList<eqroom_t> sectors_infos;
	for(int i = 0; i < g_sectors.numElem(); i++)
	{
		eqroom_t sectorinfo;

		sectorinfo.mins			= g_sectors[i].bbox.minPoint;
		sectorinfo.maxs			= g_sectors[i].bbox.maxPoint;

		//MsgInfo("Sector %d dimensions [%g %g %g] [%g %g %g]\n", sectorinfo.sector_id, 
		//	sectorinfo.mins.x, sectorinfo.mins.y, sectorinfo.mins.z,
		//	sectorinfo.maxs.x, sectorinfo.maxs.y, sectorinfo.maxs.z);

		sectors_infos.append(sectorinfo);
	}

	AddLump( EQWLUMP_SECTORINFO, (ubyte*)sectors_infos.ptr(),sectors_infos.numElem()*sizeof(eqsectorinfo_t),pHdr, pFile);
	*/

	DkList<eqroom_t>		rooms;
	DkList<eqareaportal_t>	portals;
	DkList<eqroomvolume_t>	volumes;
	DkList<Vector3D>		portalWindingVerts;
	DkList<Plane>			planes;

	// this is a primary size and most useful
	portalWindingVerts.resize(g_portalList.numElem()*8);

	int surfaceCounter = 0;

	for(int p = 0; p < g_roomList.numElem(); p++)
	{
		eqroom_t room;
		room.mins = g_roomList[p]->bounds.minPoint;
		room.maxs = g_roomList[p]->bounds.maxPoint;

		room.firstVolume = volumes.numElem();
		room.numVolumes = g_roomList[p]->volumes.numElem();

		room.numPortals = 0;

		// add volumes
		for(int i = 0; i < room.numVolumes; i++)
		{
			cwroomvolume_t* pVolume = g_roomList[p]->volumes[i];

			eqroomvolume_t volume;
			volume.mins = pVolume->bounds.minPoint;
			volume.maxs = pVolume->bounds.maxPoint;
			
			volume.firstPlane = planes.numElem();
			volume.numPlanes = pVolume->volumePlanes.numElem();

			for(int j = 0; j < pVolume->volumePlanes.numElem(); j++)
				planes.append(pVolume->volumePlanes[j]);

			volume.firstsurface = surfaceCounter;
			volume.numSurfaces = pVolume->surfaces.numElem();

			surfaceCounter += volume.numSurfaces;

			// add volume
			volumes.append(volume);
		}

		for(int i = 0; i < g_roomList[p]->numPortals; i++)
		{
			cwareaportal_t* pPortal = g_roomList[p]->portals[i];

			if(pPortal->numRooms < 2)
			{
				/*
				if(pPortal->rooms[0])
				{
					for(int j = 0; j < pPortal->rooms[0]->numPortals; j++)
					{
						if(pPortal->rooms[0]->portals[j] == pPortal)
						{
							room.numPortals--;

							pPortal->rooms[0]->portals[j] = pPortal->rooms[0]->portals[room.numPortals];
							pPortal->rooms[0]->portals[room.numPortals] = NULL;
						}
					}
				}
				*/

				MsgError("There is a unassigned to room portal side found!\n");
				MsgError("  It means that the portal is half or fully empty and will be removed!\n");

				continue;
			}

			eqareaportal_t portal;
			portal.mins = pPortal->bounds.minPoint;
			portal.maxs = pPortal->bounds.maxPoint;

			// first side
			brushwinding_t* pWinding = pPortal->windings[0];

			portal.firstVertex[0] = portalWindingVerts.numElem();
			portal.numVerts[0] = pWinding->numVerts;
			portal.portalPlanes[0] = planes.append(pWinding->face->Plane);

			for(int j = 0; j < pWinding->numVerts; j++)
				portalWindingVerts.append(pWinding->vertices[j].position);

			// second size
			pWinding = pPortal->windings[1];

			portal.firstVertex[1] = portalWindingVerts.numElem();
			portal.numVerts[1] = pWinding->numVerts;
			portal.portalPlanes[1] = planes.append(pWinding->face->Plane);

			for(int j = 0; j < pWinding->numVerts; j++)
				portalWindingVerts.append(pWinding->vertices[j].position);

			// assign room IDs
			portal.rooms[0] = pPortal->rooms[0]->room_idx;
			portal.rooms[1] = pPortal->rooms[1]->room_idx;

			// add new portal
			room.portals[room.numPortals] = portals.append( portal );

			room.numPortals++;
		}

		rooms.append(room);
	}

	AddLump( EQWLUMP_AREAPORTALS, (ubyte*)portals.ptr(),portals.numElem()*sizeof(eqareaportal_t),pHdr, pFile);
	AddLump( EQWLUMP_PLANES, (ubyte*)planes.ptr(),planes.numElem()*sizeof(Plane),pHdr, pFile);
	AddLump( EQWLUMP_AREAPORTALVERTS, (ubyte*)portalWindingVerts.ptr(), portalWindingVerts.numElem()*sizeof(Vector3D),pHdr, pFile);
	AddLump( EQWLUMP_ROOMS, (ubyte*)rooms.ptr(), rooms.numElem()*sizeof(eqroom_t),pHdr, pFile);
	AddLump( EQWLUMP_ROOMVOLUMES, (ubyte*)volumes.ptr(), volumes.numElem()*sizeof(eqroomvolume_t),pHdr, pFile);
}

void WriteWater(eqworldhdr_t* pHdr, DKFILE* pFile)
{
	if(g_waterInfos.numElem() == 0)
		return;

	Msg("Writing water info to geometry file...\n");

	DkList<eqwaterinfo_t>		waterInfos;
	DkList<eqroomvolume_t>		volumes;
	DkList<Plane>				planes;

	int surfaceCounter = 0;

	for(int p = 0; p < g_waterInfos.numElem(); p++)
	{
		eqwaterinfo_t water;
		water.mins = g_waterInfos[p]->bounds.minPoint;
		water.maxs = g_waterInfos[p]->bounds.maxPoint;

		water.firstVolume = volumes.numElem();
		water.numVolumes = g_waterInfos[p]->volumes.numElem();

		water.waterHeight = g_waterInfos[p]->waterHeight;
		water.waterMaterialId = g_materials.findIndex(g_waterInfos[p]->pMaterial);

		Msg("water material id: %d\n", water.waterMaterialId);
		Msg("water height: %g\n", water.waterHeight);

		// add volumes
		for(int i = 0; i < water.numVolumes; i++)
		{
			cwwatervolume_t* pVolume = g_waterInfos[p]->volumes[i];

			eqroomvolume_t volume;
			volume.mins = pVolume->bounds.minPoint;
			volume.maxs = pVolume->bounds.maxPoint;
			
			volume.firstPlane = planes.numElem();
			volume.numPlanes = pVolume->volumePlanes.numElem();

			for(int j = 0; j < pVolume->volumePlanes.numElem(); j++)
				planes.append(pVolume->volumePlanes[j]);

			volume.firstsurface = surfaceCounter;
			volume.numSurfaces = pVolume->surfaces.numElem();

			surfaceCounter += volume.numSurfaces;

			// add volume
			volumes.append(volume);
		}

		waterInfos.append(water);
	}

	AddLump( EQWLUMP_WATER_PLANES, (ubyte*)planes.ptr(),planes.numElem()*sizeof(Plane),pHdr, pFile);
	AddLump( EQWLUMP_WATER_INFOS, (ubyte*)waterInfos.ptr(), waterInfos.numElem()*sizeof(eqwaterinfo_t),pHdr, pFile);
	AddLump( EQWLUMP_WATER_VOLUMES, (ubyte*)volumes.ptr(), volumes.numElem()*sizeof(eqroomvolume_t),pHdr, pFile);
}

// writes world and entities
void WriteWorld()
{
	if(worldGlobals.bOnlyPhysics)
	{
		// write physics geometry
		WritePhysics();

		return;
	}

	if(!worldGlobals.bOnlyEnts)
	{
		// write render buffers
		WriteRenderGeometry();

		// write physics geometry
		WritePhysics();
	}

	WriteOcclusionGeometry();

	WriteNavMesh();

	// write base file
	eqworldhdr_t worldhdr;
	worldhdr.ident = EQWF_IDENT;
	worldhdr.version = EQWF_VERSION;

	worldhdr.num_lumps = 0;

	EqString world_data_path(varargs("worlds/%s/world.build", worldGlobals.worldName.GetData()));

	if(worldGlobals.bOnlyEnts)
	{
		ubyte* pLevelData = (ubyte*)g_fileSystem->GetFileBuffer(world_data_path.GetData(), 0, -1, true);

		if(!pLevelData)
			return;

		eqworldhdr_t* pHdr = (eqworldhdr_t*)pLevelData;
		if(pHdr->ident != EQWF_IDENT)
		{
			MsgError("ERROR: Invalid %s world file\n", worldGlobals.worldName.GetData());
			PPFree(pLevelData);
			return;
		}

		if(pHdr->version < EQWF_VERSION)
		{
			MsgError("ERROR: Invalid %s world version, please rebuild fully\n", worldGlobals.worldName.GetData());
			PPFree(pLevelData);
			return;
		}

		if(pHdr->version > EQWF_VERSION)
		{
			MsgError("ERROR: Invalid %s world version, please update SDK\n", worldGlobals.worldName.GetData());
			PPFree(pLevelData);
			return;
		}

		ubyte* pLump = ((ubyte*)pHdr + sizeof(eqworldhdr_t));

		DkList<eqworldlump_t*> lumps;

		// rescue all lumps excepting the entity lump
		for(int i = 0; i < pHdr->num_lumps; i++)
		{
			eqworldlump_t* lump_data = (eqworldlump_t*)pLump;

			int data_size = lump_data->data_size;

			if(lump_data->data_type != EQWLUMP_ENTITIES)
			{	
				lumps.append(lump_data);
			}

			// go to next lump
			pLump += sizeof(eqworldlump_t)+data_size;
		}

		DKFILE* pFile = g_fileSystem->Open(world_data_path.GetData(), "wb", SP_MOD);

		if(pFile)
		{
			MsgWarning("Writing %s... ", world_data_path.GetData());

			g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);

			for(int i = 0; i < lumps.numElem(); i++)
				AddLump( lumps[i]->data_type, ((ubyte*)lumps[i])+sizeof(eqworldlump_t),lumps[i]->data_size,&worldhdr, pFile);

			// write new entities
			WriteEntities(pFile, &worldhdr);

			g_fileSystem->Seek(pFile, 0, SEEK_SET);

			// rewrite hdr
			g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);
			g_fileSystem->Close(pFile);

			MsgWarning("Done\n");
		}
		else
		{
			MsgError("ERROR! Cannot open file %s for writing\n", world_data_path.GetData());
		}

		PPFree(pLevelData);

		return;
	}

	DKFILE* pFile = g_fileSystem->Open(world_data_path.GetData(), "wb", SP_MOD);

	if(pFile)
	{
		MsgWarning("Writing %s... ", world_data_path.GetData());

		g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);

		DkList<eqlevelmaterial_t> level_materials;
		for(int i = 0; i < g_materials.numElem(); i++)
		{
			eqlevelmaterial_t mat;
			memset(mat.material_path, 0, sizeof(mat.material_path));

			if(g_materials[i])
				strcpy(mat.material_path, g_materials[i]->GetName());
			else
				strcpy(mat.material_path, "ErrorMaterial");

			level_materials.append(mat);
		}

		// add four lumps
		AddLump(EQWLUMP_MATERIALS, (ubyte*)level_materials.ptr(), level_materials.numElem()*sizeof(eqlevelmaterial_t), &worldhdr, pFile);

		eqlevellightmapinfo_t lm_info;
		lm_info.lightmapsize = worldGlobals.lightmapSize;
		lm_info.numlightmaps = worldGlobals.numLightmaps;

		AddLump(EQWLUMP_LIGHTMAPINFO, (ubyte*)&lm_info, sizeof(lm_info), &worldhdr, pFile);

		WriteEntities(pFile, &worldhdr);

		g_fileSystem->Seek(pFile, 0, SEEK_SET);

		// rewrite hdr
		g_fileSystem->Write(&worldhdr, 1, sizeof(worldhdr),pFile);
		g_fileSystem->Close(pFile);

		MsgWarning("Done\n");
	}
	else
	{
		MsgError("ERROR! Cannot open file %s for writing\n", world_data_path.GetData());
	}
}