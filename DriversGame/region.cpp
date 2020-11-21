//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Level region
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "heightfield.h"
#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"

#ifdef EDITOR
#include "../DriversEditor/EditorLevel.h"
#else
#include "state_game.h"
#endif // EDITOR

#include "Shiny.h"

#define AI_NAVIGATION_ROAD_PRIORITY (1)

extern ConVar r_enableLevelInstancing;
ConVar w_noCollide("w_noCollide", "0");
ConVar nav_debug_map("nav_debug_map", "0", nullptr, CV_CHEAT);

//-----------------------------------------------------------------------------------------

regionObject_t::~regionObject_t()
{
	regionIdx = -1;

	// no def = fake object
	if(!def)
		return;

	RemoveGameObject();

	{
		CScopedMutex m(GetGlobalMutex(MUTEXPURPOSE_LEVEL_LOADER));
		def->Ref_Drop();
	}
}

void regionObject_t::RemoveGameObject()
{
	if (def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
#ifndef EDITOR
		g_pPhysics->m_physics.DestroyStaticObject(static_phys_object);
		static_phys_object = NULL;
#endif // EDITOR
	}
	else
	{
		// delete associated object from game world
		g_pGameWorld->RemoveObjectById(game_objectId);
		game_objectId = -1;
	}
}

#ifdef EDITOR
CUndoableObject* regionObject_t::_regionObjectFactory(IVirtualStream* stream)
{
	regionObject_t* obj = new regionObject_t();
	obj->Undoable_ReadObjectData(stream);

	ASSERT(obj->def != nullptr);

	obj->def->Ref_Grab();

	return obj;
}

UndoableFactoryFunc	regionObject_t::Undoable_GetFactoryFunc()
{
	return _regionObjectFactory;
}

void regionObject_t::Undoable_Remove()
{
	def->Ref_Drop();

	CEditorLevelRegion& region = g_pGameWorld->m_level.m_regions[regionIdx];
	region.m_objects.fastRemove(this);
	delete this;
}

// writing object
bool regionObject_t::Undoable_WriteObjectData(IVirtualStream* stream)
{
	stream->Write(&def->m_id, 1, sizeof(int));

	stream->Write(&tile_x, 1, sizeof(tile_x));
	stream->Write(&tile_y, 1, sizeof(tile_y));
	stream->Write(&position, 1, sizeof(position));
	stream->Write(&rotation, 1, sizeof(rotation));

	stream->Write(&regionIdx, 1, sizeof(regionIdx));

	// store length and short string
	int nameLen = name.Length();

	stream->Write(&nameLen, 1, sizeof(nameLen));
	stream->Write(name.c_str(), 1, nameLen);

	return true;
}

// reading object
void regionObject_t::Undoable_ReadObjectData(IVirtualStream* stream)
{
	int defId = 0;
	stream->Read(&defId, 1, sizeof(int));

	CLevObjectDef** defPtr = g_pGameWorld->m_level.m_objectDefs.findFirst([defId](CLevObjectDef* sdef) {
		return (sdef->m_id == defId);
	});

	if (defPtr && !def)
		def = *defPtr;

	int prevRegionIdx = regionIdx;

	stream->Read(&tile_x, 1, sizeof(tile_x));
	stream->Read(&tile_y, 1, sizeof(tile_y));
	stream->Read(&position, 1, sizeof(position));
	stream->Read(&rotation, 1, sizeof(rotation));

	stream->Read(&regionIdx, 1, sizeof(regionIdx));

	// read length
	int nameLen = 0;
	stream->Read(&nameLen, 1, sizeof(nameLen));

	char* nameStr = (char*)stackalloc(nameLen+1);
	stream->Read(nameStr, 1, nameLen);
	nameStr[nameLen] = '\0';

	// assign name
	name = nameStr;

	// move to new region if needed
	if (prevRegionIdx != regionIdx)
	{
		if (prevRegionIdx >= 0)
			g_pGameWorld->m_level.m_regions[prevRegionIdx].m_objects.fastRemove(this);

		if (regionIdx >= 0)
			g_pGameWorld->m_level.m_regions[regionIdx].m_objects.append(this);
	}
}
#endif // EDITOR

void regionObject_t::CalcBoundingBox()
{
	BoundingBox tbbox;
	BoundingBox modelBBox;

	if(this->def->m_model)
		modelBBox = this->def->m_model->GetAABB();
	else if(this->def->m_defModel)
		modelBBox = this->def->m_defModel->GetAABB();

	for(int i = 0; i < 8; i++)
	{
		tbbox.AddVertex((transform*Vector4D(modelBBox.GetVertex(i), 1.0f)).xyz());
	}

	// set reference bbox for light testing
	bbox = tbbox;
}

//-----------------------------------------------------------------------------------------

CLevelRegion::CLevelRegion()
{
	m_level = nullptr;
	m_roads = nullptr;
	m_regionIndex = -1;

	m_isLoaded = false;
	m_scriptEventCallbackCalled = true;
	m_queryTimes.SetValue(0);

	memset(m_heightfield, 0, sizeof(m_heightfield));
}

CLevelRegion::~CLevelRegion()
{
	Cleanup();

	// cleanup of permanent data
	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			delete m_heightfield[i];
		m_heightfield[i] = nullptr;
	}

	delete[] m_roads;
	m_roads = nullptr;
}

Vector3D CLevelRegion::CellToPosition(int x, int y) const
{
	CHeightTileField& defField = *m_heightfield[0];

	hfieldtile_t* tile = defField.GetTile(x, y);

	Vector3D tile_position;

	if(tile)
		tile_position = defField.m_position + Vector3D(x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, y*HFIELD_POINT_SIZE);
	else
		tile_position = defField.m_position + Vector3D(x*HFIELD_POINT_SIZE, 0, y*HFIELD_POINT_SIZE);

	return tile_position;
}

IVector2D CLevelRegion::PositionToCell(const Vector3D& position) const
{
	CHeightTileField& defField = *m_heightfield[0];

	IVector2D point;
	defField.PointAtPos(position, point.x, point.y);

	return point;
}

IVector2D CLevelRegion::GetTileAndNeighbourRegion(int x, int y, CLevelRegion** reg) const
{
	CHeightTileField& defField = *m_heightfield[0];

	// if we're out of bounds - try to find neightbour tile
	if(	(x >= defField.m_sizew || y >= defField.m_sizeh) ||
		(x < 0 || y < 0))
	{
		if(defField.m_regionPos < 0)
			return IVector2D(0);

		// only -1/+1, no more
		int ofs_x = (x < 0) ? -1 : ((x >= defField.m_sizew) ? 1 : 0 );
		int ofs_y = (y < 0) ? -1 : ((y >= defField.m_sizeh) ? 1 : 0 );

		// достать соседа
		(*reg) = m_level->GetRegionAt(IVector2D(defField.m_regionPos.x + ofs_x, defField.m_regionPos.y + ofs_y));

		if(*reg)
		{
			// rolling
			int tofs_x = ROLLING_VALUE(x, (*reg)->m_heightfield[0]->m_sizew);
			int tofs_y = ROLLING_VALUE(y, (*reg)->m_heightfield[0]->m_sizeh);

			return IVector2D(tofs_x, tofs_y);
		}
		else
			return IVector2D(0);
	}

	(*reg) = (CLevelRegion*)this;

	return IVector2D(x,y);
}

void GetModelRefRenderQuaternionPosition(Quaternion& rotation, Vector3D& position, CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Matrix3x3 m = rotateXYZ3(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	float addh = 0.0f;

	CLevObjectDef* objectDef = ref->def;

	if (objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG && objectDef->m_defModel != NULL)
		addh = -objectDef->m_defModel->GetAABB().minPoint.y;

	if (ref->tile_x != 0xFFFF)
	{
		hfieldtile_t* tile = defField.GetTile(ref->tile_x, ref->tile_y);

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, (tile ? tile->height*HFIELD_HEIGHT_STEP : 0) + addh, ref->tile_y*HFIELD_POINT_SIZE);

		position = defField.m_position + tilePosition;

		if (objectDef != NULL && (objectDef->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			objectDef->m_info.type != LOBJ_TYPE_OBJECT_CFG)
		{
			Matrix3x3 tileAngle;
			defField.GetTileTBN(ref->tile_x, ref->tile_y, tileAngle.rows[2], tileAngle.rows[0], tileAngle.rows[1]);

			rotation = Quaternion((!tileAngle)*m);
		}
		else
			rotation = Quaternion(m);

		return;
	}

	rotation = Quaternion(m);
	position = ref->position;
}

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Matrix4x4 m = rotateXYZ4(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	float addh = 0.0f;

	CLevObjectDef* objectDef = ref->def;

	if(objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG && objectDef->m_defModel != NULL)
		addh = -objectDef->m_defModel->GetAABB().minPoint.y;

	if(ref->tile_x!= 0xFFFF)
	{
		hfieldtile_t* tile = defField.GetTile( ref->tile_x, ref->tile_y );

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, (tile ? tile->height*HFIELD_HEIGHT_STEP : 0) + addh, ref->tile_y*HFIELD_POINT_SIZE);
		Vector3D modelPosition = defField.m_position + tilePosition;

		if( objectDef != NULL && (objectDef->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			objectDef->m_info.type != LOBJ_TYPE_OBJECT_CFG )
		{
			Matrix4x4 tileAngle;
			tileAngle.rows[0].w = 0.0f;
			tileAngle.rows[1].w = 0.0f;
			tileAngle.rows[2].w = 0.0f;
			tileAngle.rows[3] = Vector4D(0.0f, 0.0f, 0.0f, 1.0f);
			
			defField.GetTileTBN(ref->tile_x, ref->tile_y, *(Vector3D*)(float*)tileAngle.rows[2], *(Vector3D*)(float*)tileAngle.rows[0], *(Vector3D*)(float*)tileAngle.rows[1]);

			m = translate(modelPosition) *(!tileAngle)*m;
		}
		else
			m = translate(modelPosition)*m;
	}
	else
	{
		m = translate(ref->position + vec3_up*addh)*m; // maybe I should add addh
	}

	return m;
}

static Vector3D s_sortCamPos;

int occluderComparator(levOccluderLine_t* const& a, levOccluderLine_t* const& b)
{
	Vector3D am = (a->start+a->end)*0.5f;
	Vector3D bm = (b->start, b->end)*0.5f;
	float ad = lengthSqr(s_sortCamPos-am);
	float bd = lengthSqr(s_sortCamPos-bm);

	float diff = (ad - bd);

	return sign(diff);
}

void CLevelRegion::CollectOccluders(occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition)
{
	{
		CScopedMutex m(m_level->m_mutex);

		if (!m_isLoaded)
			return;
	}

	static DkList<levOccluderLine_t*> occluders;
	occluders.clear(false);

	for(int i = 0; i < m_occluders.numElem(); i++)
		occluders.append( &m_occluders[i] );

	s_sortCamPos = cameraPosition;

	// sort occluders
	occluders.sort( occluderComparator );

	// go from closest to furthest
	for(int i = 0; i < occluders.numElem(); i++)
	{
		levOccluderLine_t* occl = occluders[i];

		Vector3D verts[4] = {
			occl->start,
			occl->end,
			occl->start + Vector3D(0, occl->height, 0),
			occl->end + Vector3D(0, occl->height, 0)
		};

		BoundingBox bbox;
		bbox.AddVertices(verts,4);

		frustumOccluders.occluderSets.append( new occludingVolume_t(cameraPosition, this, occl) );
	}
}

void CLevelRegion::CollectVisibleOccluders( occludingFrustum_t& frustumOccluders, const Vector3D& cameraPosition )
{
	{
		CScopedMutex m(m_level->m_mutex);

		if (!m_isLoaded)
			return;
	}

	static DkList<levOccluderLine_t*> occluders;
	occluders.clear(false);

	for(int i = 0; i < m_occluders.numElem(); i++)
		occluders.append( &m_occluders[i] );

	s_sortCamPos = cameraPosition;

	// sort occluders
	occluders.sort( occluderComparator );

	// go from closest to furthest
	for(int i = 0; i < occluders.numElem(); i++)
	{
		levOccluderLine_t* occl = occluders[i];

		Vector3D verts[4] = {
			occl->start,
			occl->end,
			occl->start + Vector3D(0, occl->height, 0),
			occl->end + Vector3D(0, occl->height, 0)
		};

		BoundingBox bbox;
		bbox.AddVertices(verts,4);

		if( frustumOccluders.IsBoxVisible(bbox) )
			frustumOccluders.occluderSets.append( new occludingVolume_t(cameraPosition, this, occl) );
	}
}

void CLevelRegion::Render(const Vector3D& cameraPosition, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
#ifndef EDITOR
	{
		CScopedMutex m(m_level->m_mutex);
		if (!m_isLoaded)
			return;
	}
#endif // EDITOR

	PROFILE_FUNC();

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

#ifndef EDITOR
	int numObjects = m_staticObjects.numElem();
#else
	int numObjects = m_objects.numElem();
#endif // EDITOR

	for(int i = 0; i < numObjects; i++)
	{
#ifdef EDITOR
		regionObject_t* ref = m_objects[i];
		CLevObjectDef* cont = ref->def;

		if (ref->hide)
			continue;

//----------------------------------------------------------------
// IN-EDITOR RENDERER
//----------------------------------------------------------------

		float fDist = length(cameraPosition - ref->position);

		if(cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			if (occlFrustum.IsBoxVisible(ref->bbox))
			{
				if (caps.isInstancingSupported && !(cont->m_info.modelflags & LMODEL_FLAG_UNIQUE))
				{
					if(!cont->m_instData)	// make new instancing data
						cont->m_instData = new levObjInstanceData_t;

					regObjectInstance_t inst;

					Vector3D refPos(0.0f);
					GetModelRefRenderQuaternionPosition(inst.rotation, refPos, this, ref);

					inst.position = Vector4D(refPos, 1.0f);

					cont->m_instData->instances[cont->m_instData->numInstances++] = inst;
				}
				else
				{
					Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);

					materials->SetMatrix(MATRIXMODE_WORLD, mat);
					cont->Render(fDist, ref->bbox, false, nRenderFlags);
				}
			}
		}

		else
		{
			Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);

			materials->SetMatrix(MATRIXMODE_WORLD, mat);

			BoundingBox bbox;
			if (cont->m_defModel)
				bbox = cont->m_defModel->GetAABB();

			if (occlFrustum.IsBoxVisible(ref->bbox)) //occlFrustum.IsSphereVisible( ref->position, length(bbox.GetSize())) )
				cont->Render(fDist, ref->bbox, false, nRenderFlags);
		}
#else
//----------------------------------------------------------------
// IN-GAME RENDERER
//----------------------------------------------------------------

		regionObject_t* ref = m_staticObjects[i];
		CLevObjectDef* cont = ref->def;

		if (!occlFrustum.IsBoxVisible(ref->bbox)) //ref->position, length(ref->bbox.GetSize())) )
			continue;

		levObjInstanceData_t* instData = cont->m_instData;

		PROFILE_BEGIN(DrawOrInstance);

		if(	caps.isInstancingSupported &&
			r_enableLevelInstancing.GetBool() && 
			instData)
		{
			Matrix4x4 transform = ref->transform;

			regObjectInstance_t inst;
			inst.rotation = transform.getRotationComponent();
			inst.position = Vector4D(transform.getTranslationComponentTransposed(), 1.0f);

			//GetModelRefRenderQuaternionPosition(inst.rotation, *(Vector3D*)(float*)inst.position, this, ref);

			instData->instances[instData->numInstances++] = inst;
		}
		else
		{
			float fDist = length(cameraPosition - ref->position);
			materials->SetMatrix(MATRIXMODE_WORLD, GetModelRefRenderMatrix(this, ref));

			cont->Render(fDist, ref->bbox, false, nRenderFlags);
		}

		PROFILE_END();

#endif	// EDITOR
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	int numHFields = GetNumHFields();

	PROFILE_BEGIN(DrawHFields);

	if(nav_debug_map.GetBool())
	{
		for(int i = 0; i < numHFields; i++)
		{
			if(m_heightfield[i])
				m_heightfield[i]->RenderDebug(m_navGrid[0].debugObstacleMap, nRenderFlags, occlFrustum );
		}
	}
	else
	{
		for(int i = 0; i < numHFields; i++)
		{
			if(m_heightfield[i])
				m_heightfield[i]->Render( nRenderFlags, occlFrustum );
		}
	}

	PROFILE_END();
}

void CLevelRegion::Init(int cellsSize, const IVector2D& regPos, const Vector3D& hfieldPos)
{
#ifndef EDITOR
	if(!m_heightfield[0])
		m_heightfield[0] = new CHeightTileFieldRenderable();
#endif // EDITOR
	// init all hfields
	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
#ifdef EDITOR
		if (!m_heightfield[i])
		{
			m_heightfield[i] = new CHeightTileFieldRenderable();
			m_heightfield[i]->m_regionPos = regPos;
			m_heightfield[i]->m_position = hfieldPos;
		}
#else
		if (!m_heightfield[i])
			continue;

		m_heightfield[i]->m_regionPos = regPos;
		m_heightfield[i]->m_position = hfieldPos;
#endif // EDITOR

		m_heightfield[i]->m_fieldIdx = i;

#ifdef EDITOR
		// init other things like road data
		m_heightfield[i]->Init(cellsSize, regPos);
#endif // EDITOR
	}

#ifdef EDITOR
	if (!m_roads)
		m_roads = new levroadcell_t[cellsSize * cellsSize];
#endif // EDITOR
}



void CLevelRegion::InitNavigationGrid()
{
	if (!m_roads)
		return;

	CHeightTileField& defField = *m_heightfield[0];

	// also init navigation grid
	for (int grid = 0; grid < 2; grid++)
		m_navGrid[grid].Init(defField.m_sizew*s_navGridScales[grid], defField.m_sizeh*s_navGridScales[grid]);

	// we're initializing roads and then going to initialize
	// navgrid data since our heightfield tiles can be detached as well
	for (int x = 0; x < defField.m_sizew; x++)
	{
		for (int y = 0; y < defField.m_sizeh; y++)
		{
			int idx = y * defField.m_sizew + x;

			levroadcell_t& road = m_roads[idx];

			// adjust priority by road
			if (road.type != ROADTYPE_PARKINGLOT && 
				road.type != ROADTYPE_PAVEMENT &&
				road.type != ROADTYPE_NOROAD)
			{
				// fast navgrid uses same resolution
				m_navGrid[1].staticObst[idx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;

				// higher the priority of road nodes
				for (int xx = 0; xx < AI_NAV_DETAILED_SCALE; xx++)
				{
					for (int yy = 0; yy < AI_NAV_DETAILED_SCALE; yy++)
					{
						int ofsX = road.x*AI_NAV_DETAILED_SCALE + xx;
						int ofsY = road.y*AI_NAV_DETAILED_SCALE + yy;

						int navCellIdx = ofsY * m_navGrid[0].tall + ofsX;
						m_navGrid[0].staticObst[navCellIdx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;
					}
				}
			}

			hfieldtile_t* tile = defField.GetTile(x, y);

			bool isBlockingTile = false; /*(tile->flags & EHTILE_EMPTY) || tile->texture == -1*/;

			// check for water
			for (int hIdx = 0; hIdx < ENGINE_REGION_MAX_HFIELDS; hIdx++)
			{
				if (!m_heightfield[hIdx])
					continue;

				hfieldtile_t* checkWaterTile = m_heightfield[hIdx]->GetTile(x, y);

				if (checkWaterTile == NULL || checkWaterTile && !m_heightfield[hIdx]->m_materials.inRange(checkWaterTile->texture))
					continue;

				hfieldmaterial_t* mat = m_heightfield[hIdx]->m_materials[checkWaterTile->texture];
				if (!mat->material)
					continue;

				if (mat->material->GetFlags() & MATERIAL_FLAG_WATER)
				{
					int heightDiff = checkWaterTile->height - tile->height;

					if ( heightDiff > 3)
					{
						isBlockingTile = true;
						break;
					}
				}
			}

			// detect detached tiles and place obstacle at NAV grid
			if (!isBlockingTile && tile->flags & EHTILE_DETACHED)
			{
				int nx[] = NEIGHBOR_OFFS_X(x);
				int ny[] = NEIGHBOR_OFFS_Y(y);

				// check the non-diagonal neighbour heights
				for (int n = 0; n < 4; n++)
				{
					hfieldtile_t* nTile = defField.GetTile(nx[n], ny[n]);

					if (!nTile)
						continue;

					int heightDiff = abs(tile->height - nTile->height);

					// don't check the same flags... it must be linked betweet detached and attached
					// if there's too much, we're blocking NAV roads
					isBlockingTile = (heightDiff > 2) && !(nTile->flags & EHTILE_DETACHED);

					if (isBlockingTile)
						break;
				}
				
			}

			if (isBlockingTile)
			{
				m_navGrid[1].staticObst[idx] = 0;

				// set the high detail navgrid point too
				for (int j = 0; j < AI_NAV_DETAILED_SCALE; j++)
				{
					for (int k = 0; k < AI_NAV_DETAILED_SCALE; k++)
					{
						int ofsX = x * AI_NAV_DETAILED_SCALE + j;
						int ofsY = y * AI_NAV_DETAILED_SCALE + k;

						int navCellIdx = ofsY * m_navGrid[0].tall + ofsX;
						m_navGrid[0].staticObst[navCellIdx] = 0;
					}
				}
			}
		}
	}
	// THIS DOES NOT WORK WELL, BUT SHOULD...
	if (m_navGrid[0].dirty || m_navGrid[1].dirty)
	{
		//for (int i = 0; i < m_objects.numElem(); i++)
		//	m_level->Nav_AddObstacle(this, m_objects[i]);

		
		g_parallelJobs->AddJob(NavAddObstacleJob, this, m_objects.numElem());
		g_parallelJobs->Submit();
		

		m_navGrid[0].dirty = false;
		m_navGrid[1].dirty = false;
	}

	

	// init debug maps
	if (nav_debug_map.GetInt() > 0)
	{
		m_navGrid[0].debugObstacleMap = g_pShaderAPI->CreateProceduralTexture(varargs("navgrid_%d", m_regionIndex), 
			FORMAT_RGBA8, m_navGrid[0].wide, m_navGrid[0].tall, 1, 1, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);

		m_navGrid[0].debugObstacleMap->Ref_Grab();
	}
}

void CLevelRegion::NavAddObstacleJob(void *data, int i)
{
	CLevelRegion* thisReg = (CLevelRegion*)data;
	regionObject_t* regObj = thisReg->m_objects[i];

	thisReg->m_level->Nav_AddObstacle(thisReg, regObj);
}

void CLevelRegion::UpdateDebugMaps()
{
	// update navigation debug map
	if(m_navGrid[0].debugObstacleMap != NULL)
	{
		texlockdata_t lockData;

		m_navGrid[0].debugObstacleMap->Lock(&lockData, NULL, true);
		if(lockData.pData)
		{
			memset(lockData.pData, 0, m_navGrid[0].wide*m_navGrid[0].tall*sizeof(TVec4D<ubyte>));

			TVec4D<ubyte>* imgData = (TVec4D<ubyte>*)lockData.pData;

			for(int y = 0; y < m_navGrid[0].tall; y++)
			{
				for(int x = 0; x < m_navGrid[0].wide; x++)
				{
					int pixIdx = y*m_navGrid[0].tall+x;

					TVec4D<ubyte> color(0);

					if(nav_debug_map.GetInt() > 1)
					{
						color.z = 255-m_navGrid[0].dynamicObst[pixIdx] * 48;
					}
					else
					{
						color.z = 255-m_navGrid[0].staticObst[pixIdx] * 48;
					}

					imgData[pixIdx] = color;
				}
			}

			m_navGrid[0].debugObstacleMap->Unlock();
		}
	}
}

bool CLevelRegion::FindObject(levCellObject_t& objectInfo, const char* name, CLevObjectDef* def) const
{
	{
		CScopedMutex m(m_level->m_mutex);

		if (!m_isLoaded)
			return false;
	}

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* obj = m_objects[i];

		if(obj->name.Length() == 0)
			continue;

		if((!def || def == obj->def) && !obj->name.CompareCaseIns(name))
		{
			strcpy(objectInfo.name, obj->name.c_str());

			objectInfo.position = obj->position;
			objectInfo.rotation = obj->rotation;

			objectInfo.tile_x = obj->tile_x;
			objectInfo.tile_y = obj->tile_y;

			return true;
		}
	}

	return false;
}

void CLevelRegion::Cleanup()
{
	{
		CScopedMutex m(m_level->m_mutex);

		if (!m_isLoaded)
			return;

		m_isLoaded = false;
	}

	for(int i = 0; i < m_objects.numElem(); i++)
		delete m_objects[i];
	m_objects.clear();

#ifndef EDITOR
	m_staticObjects.clear();
#endif // EDITOR

	for(int i = 0; i < m_zones.numElem(); i++)
		delete [] m_zones[i].zoneName;
	m_zones.clear();

	for(int i = 0; i < m_regionDefs.numElem(); i++)
		delete m_regionDefs[i];
	m_regionDefs.clear();

	m_occluders.clear();

	g_pShaderAPI->FreeTexture(m_navGrid[0].debugObstacleMap);
	m_navGrid[0].debugObstacleMap = NULL;

	for (int i = 0; i < 2; i++)
	{
		m_navGrid[i].Cleanup();
	}

#ifdef EDITOR
	for (int i = 0; i < GetNumHFields(); i++)
	{
		if (m_heightfield[i])
		{
			m_heightfield[i]->CleanRenderData();
			m_heightfield[i]->Destroy();
		}
	}
#else
	for (int i = 0; i < GetNumHFields(); i++)
	{
		if (m_heightfield[i])
		{
			g_pPhysics->RemoveHeightField(m_heightfield[i]);
			m_heightfield[i]->CleanRenderData();
		}
	}
#endif // EDITOR

	m_queryTimes.SetValue(0);

	DevMsg(DEVMSG_CORE, "Region %d freed\n", m_regionIndex);
}

bool CLevelRegion::IsRegionEmpty()
{
	if(m_objects.numElem() == 0 &&
		(m_heightfield[0] ? m_heightfield[0]->IsEmpty() : true) &&
		(m_heightfield[1] ? m_heightfield[1]->IsEmpty() : true))
		return true;

	return false;
}

CHeightTileFieldRenderable* CLevelRegion::GetHField( int idx ) const
{
	return m_heightfield[idx];
}

int CLevelRegion::GetNumHFields() const
{
	return ENGINE_REGION_MAX_HFIELDS;
}

int	CLevelRegion::GetNumNomEmptyHFields() const
{
	int nCount = 0;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i] && !m_heightfield[i]->IsEmpty()  )
			nCount++;
	}

	return nCount;
}

void CLevelRegion::GetDecalPolygons(decalPrimitives_t& polys, occludingFrustum_t* frustum)
{
	for (int i = 0; i < GetNumHFields(); i++)
	{
		CHeightTileField* hfield = GetHField(i);

		if(!hfield)
			continue;

		hfield->GetDecalPolygons(polys, frustum);
	}

	// get model polygons
	for (int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];
		CLevObjectDef* def = m_objects[i]->def;

		if(!def)
			continue;

		if(def->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
			continue;

		
		if( !polys.settings.clipVolume.IsBoxInside(ref->bbox.minPoint, ref->bbox.maxPoint) )
			continue;

		if(frustum && !frustum->IsBoxVisible(ref->bbox))
			continue;

		def->m_model->GetDecalPolygons(polys, ref->transform);
	}
}

void CLevelRegion::InitRegionHeightfieldsJob(void *data, int i)
{
	CLevelRegion* reg = (CLevelRegion*)data;

	if (!reg->m_heightfield[i])
		return;

	if (reg->m_heightfield[i]->m_levOffset == 0)
		return;

	reg->m_heightfield[i]->GenerateRenderData(nav_debug_map.GetBool());

#ifndef EDITOR
	g_pPhysics->AddHeightField(reg->m_heightfield[i]);
#endif //EDITOR
}

ConVar w_nospawn("w_nospawn", "0", nullptr, CV_CHEAT);

void CLevelRegion::ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& levelmodels)
{
	if(m_isLoaded)
		return;

#ifndef EDITOR
	if (!g_State_Game->IsGameRunning())
	{
#endif
		g_parallelJobs->AddJob(InitRegionHeightfieldsJob, this, GetNumHFields());
		g_parallelJobs->Submit();
#ifndef EDITOR
	}
	else
	{
		for (int i = 0; i < GetNumHFields(); i++)
		{
			InitRegionHeightfieldsJob(this, i);
		}
	}
#endif

	levRegionDataInfo_t	regdatahdr;
	stream->Read(&regdatahdr, 1, sizeof(levRegionDataInfo_t));

	levObjectDefInfo_t defInfo;

	//
	// Load models (object definitions)
	//
	for(int i = 0; i < regdatahdr.numObjectDefs; i++)
	{
		stream->Read(&defInfo, 1, sizeof(levObjectDefInfo_t));

#ifdef EDITOR
		// don't use regenerated models in editor.
		if( defInfo.modelflags & LMODEL_FLAG_GENERATED )
		{
			// skip model data and continue
			stream->Seek(defInfo.size, VS_SEEK_CUR);
			continue;
		}
#else
		CLevObjectDef* def = new CLevObjectDef();
		def->m_info = defInfo;
		def->m_modelOffset = stream->Tell();

		// FIXME: add to task manager?
		def->PreloadModel(stream);

		m_regionDefs.append(def);
#endif // EDITOR
	}

	levCellObject_t cellObj;

	//
	// READ cell objects
	//
	for(int i = 0; i < regdatahdr.numCellObjects; i++)
	{
		stream->Read(&cellObj, 1, sizeof(levCellObject_t));

#ifdef EDITOR
		if(cellObj.flags & CELLOBJ_GENERATED)
			continue;
#endif // EDITOR

		// Init basics
		regionObject_t* ref = new regionObject_t;

		ref->name = cellObj.name;

		ref->tile_x = cellObj.tile_x;
		ref->tile_y = cellObj.tile_y;

		ref->position = cellObj.position;
		ref->rotation = cellObj.rotation;

		ref->regionIdx = m_regionIndex;

		//
		// pick from region or global list
		//
		if(cellObj.flags & CELLOBJ_REGION_DEF)
			ref->def = m_regionDefs[cellObj.objectDefId];
		else
			ref->def = levelmodels[cellObj.objectDefId];

		// reference object
		{
			CScopedMutex m(m_level->m_mutex);
			ref->def->Ref_Grab();
		}
		

		// calculate the transformation
		ref->transform = GetModelRefRenderMatrix( this, ref );

		// def model has to be loaded
		// also it will be loaded for m_regionDefs
		ref->def->PreloadModel(stream);

		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			CLevelModel* model = ref->def->m_model;

#ifndef EDITOR
			bool noCollide = (ref->def->m_info.modelflags & LMODEL_FLAG_NOCOLLIDE) || 
							!(ref->def->m_info.modelflags & LMODEL_FLAG_ISGROUND) && w_noCollide.GetBool();

			if(!noCollide)
			{
				// create collision objects
				model->CreateCollisionObjectFor( ref );

				// add physics objects
				g_pPhysics->m_physics.AddStaticObject( ref->static_phys_object );
			}
#endif // EDITOR
		}
#ifndef EDITOR
		else if(!w_nospawn.GetBool())
		{
			// create object, spawn in game cycle
			CGameObject* newObj = g_pGameWorld->CreateGameObject(ref->def->m_defType.c_str(), ref->def->m_defKeyvalues);

			if (newObj)
			{
				Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

				newObj->SetOrigin(transpose(ref->transform).getTranslationComponent());
				newObj->SetAngles(VRAD2DEG(eulAngles));
				newObj->SetUserData(ref);

				// game name of this object
				if (ref->name.Length() > 0)
					newObj->SetName(ref->name.c_str());
				else if (newObj->ObjType() != GO_SCRIPTED)
					newObj->SetName(varargs("_reg%d_ref%d", m_regionIndex, i));

				// let the game objects to be spawned on game frame
				ref->game_objectId = g_pGameWorld->AddObject(newObj);
			}
		}
#endif

		// calculate ref aabb
		ref->CalcBoundingBox();

		// finally add that object
		ASSERT(ref);
		m_objects.append(ref);

#ifndef EDITOR
		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
			m_staticObjects.append(ref);
#endif // EDITOR
	}

	// init navigation grid
	InitNavigationGrid();

	{
		CScopedMutex m(m_level->m_mutex);
		m_isLoaded = true;
	}
}

void CLevelRegion::RespawnObjects()
{
#ifndef EDITOR
	{
		CScopedMutex m(m_level->m_mutex);
		if (!m_isLoaded)
			return;
	}

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];

		if(ref->def->m_info.type != LOBJ_TYPE_OBJECT_CFG)
			continue;

		ref->RemoveGameObject();

		if (w_nospawn.GetBool())
			continue;

		ref->transform = GetModelRefRenderMatrix( this, ref );

		// create object, spawn in game cycle
		CGameObject* newObj = g_pGameWorld->CreateGameObject( ref->def->m_defType.c_str(), ref->def->m_defKeyvalues );

		if(newObj)
		{
			Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

			newObj->SetOrigin( transpose(ref->transform).getTranslationComponent() );
			newObj->SetAngles( VRAD2DEG(eulAngles) );
			newObj->SetUserData( ref );

			// game name of this object
			if( ref->name.Length() > 0 )
				newObj->SetName( ref->name.c_str() );
			else if(newObj->ObjType() != GO_SCRIPTED)
				newObj->SetName( varargs("_reg%d_ref%d", m_regionIndex, i) );

			// let the game objects to be spawned on game frame
			ref->game_objectId = g_pGameWorld->AddObject(newObj);
		}
	}
#endif // EDITOR
}

void CLevelRegion::ReadLoadRoads(IVirtualStream* stream, bool fix)
{
	CHeightTileField* field = GetHField(0);

	if(field->IsEmpty()) // don't init roads if there is no main heightfield
		return;

	if(!m_roads)
		m_roads = new levroadcell_t[field->m_sizew * field->m_sizeh];

	int numRoadCells = 0;
	stream->Read(&numRoadCells, 1, sizeof(int));

	if(fix)
	{ 
		for (int i = 0; i < numRoadCells; i++)
		{
			oldlevroadcell_t tmpCell;
			stream->Read(&tmpCell, 1, sizeof(oldlevroadcell_t));

			levroadcell_t newCell;
			newCell.direction = tmpCell.direction;
			newCell.flags = tmpCell.flags;
			newCell.type = tmpCell.type;
			newCell.x = tmpCell.posX;
			newCell.y = tmpCell.posY;

			int idx = newCell.y * field->m_sizew + newCell.x;
			m_roads[idx] = newCell;
		}
	}
	else
	{
		for (int i = 0; i < numRoadCells; i++)
		{
			levroadcell_t tmpCell;
			stream->Read(&tmpCell, 1, sizeof(levroadcell_t));

			int idx = tmpCell.y * field->m_sizew + tmpCell.x;
			m_roads[idx] = tmpCell;
		}
	}
}

void CLevelRegion::ReadLoadOccluders(IVirtualStream* stream)
{
	int numOccluders = 0;
	stream->Read(&numOccluders, 1, sizeof(int));

	m_occluders.setNum(numOccluders);
	stream->Read(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

//-----------------------------------------------------------------------------------------------------------
