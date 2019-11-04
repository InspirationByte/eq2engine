//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Level region
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "heightfield.h"

#ifdef EDITOR
#include "../DriversEditor/EditorLevel.h"
#endif // EDITOR

#define AI_NAVIGATION_ROAD_PRIORITY (1)

extern ConVar r_enableLevelInstancing;
ConVar w_noCollide("w_noCollide", "0");
ConVar nav_debug_map("nav_debug_map", "0", nullptr, CV_CHEAT);

//-----------------------------------------------------------------------------------------

regionObject_t::~regionObject_t()
{
	// no def = fake object
	if(!def)
		return;

	if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
		CLevelModel* mod = def->m_model;
		mod->Ref_Drop();

		// the model cannot be removed if it's not loaded with region
		if(mod->Ref_Count() <= 0)
			delete mod;

#ifndef EDITOR
		g_pPhysics->m_physics.DestroyStaticObject(physObject);
		physObject = NULL;
#endif // EDITOR
	}
	else
	{
		// delete associated object from game world
		if( g_pGameWorld->IsValidObject( game_object ) )
		{
			game_object->SetUserData(NULL);
			g_pGameWorld->RemoveObject( game_object );
		}

		game_object = NULL;
	}
}

#ifdef EDITOR
// writing object
bool regionObject_t::Undoable_WriteObjectData(IVirtualStream* stream)
{
	stream->Write(&tile_x, 1, sizeof(tile_x));
	stream->Write(&tile_y, 1, sizeof(tile_y));
	stream->Write(&position, 1, sizeof(position));
	stream->Write(&rotation, 1, sizeof(rotation));

	// store length and short string
	int nameLen = name.Length();

	stream->Write(&nameLen, 1, sizeof(nameLen));
	stream->Write(name.c_str(), 1, nameLen);

	return true;
}

// reading object
void regionObject_t::Undoable_ReadObjectData(IVirtualStream* stream)
{
	stream->Read(&tile_x, 1, sizeof(tile_x));
	stream->Read(&tile_y, 1, sizeof(tile_y));
	stream->Read(&position, 1, sizeof(position));
	stream->Read(&rotation, 1, sizeof(rotation));

	// read length
	int nameLen = 0;
	stream->Read(&nameLen, 1, sizeof(nameLen));

	char* nameStr = (char*)stackalloc(nameLen+1);
	stream->Read(nameStr, 1, nameLen);
	nameStr[nameLen] = '\0';

	// assign name
	name = nameStr;
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

	m_hasTransparentSubsets = false;

	m_render = true;
	m_isLoaded = false;
	m_scriptEventCallbackCalled = true;
	m_queryTimes.SetValue(0);

	memset(m_heightfield, 0, sizeof(m_heightfield));
}

CLevelRegion::~CLevelRegion()
{
	Cleanup();

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			delete m_heightfield[i];
	}
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

Vector3D GetModelRefPosition(CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Vector3D addHeight(0.0f);

	CLevObjectDef* objectDef = ref->def;

	if(objectDef != NULL && objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		addHeight.y = -objectDef->m_defModel->GetAABB().minPoint.y;

	if(ref->tile_x != 0xFFFF)
	{
		hfieldtile_t* tile = defField.GetTile( ref->tile_x, ref->tile_y );

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, ref->tile_y*HFIELD_POINT_SIZE);

		return defField.m_position + tilePosition + addHeight;
	}

	return ref->position;
}

Quaternion GetModelRefRotation(CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];
	CLevObjectDef* objectDef = ref->def;

	Matrix3x3 m = rotateXYZ3(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	if(ref->tile_x!= 0xFFFF)
	{
		if( objectDef != NULL && (objectDef->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			objectDef->m_info.type != LOBJ_TYPE_OBJECT_CFG )
		{
			Vector3D t,b,n;
			defField.GetTileTBN( ref->tile_x, ref->tile_y, t,b,n );

			Matrix3x3 tileAngle(b,n,t);

			tileAngle = (!tileAngle)*m;
			//Matrix4x4 tileShear(Vector4D(b, 0), Vector4D(0,1,0,0), Vector4D(t, 0), Vector4D(0,0,0,1));

			//Matrix4x4 tilemat = shearY(cosf(atan2f(b.x, b.y)))*tileTBN;

			return Quaternion(tileAngle);
		}
	}

	return Quaternion(m);
}

Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Matrix4x4 m = rotateXYZ4(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	Vector3D addHeight(0.0f);

	CLevObjectDef* objectDef = ref->def;

	if(objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG && objectDef->m_defModel != NULL)
		addHeight.y = -objectDef->m_defModel->GetAABB().minPoint.y;

	if(ref->tile_x!= 0xFFFF)
	{
		hfieldtile_t* tile = defField.GetTile( ref->tile_x, ref->tile_y );

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, ref->tile_y*HFIELD_POINT_SIZE);
		Vector3D modelPosition = defField.m_position + tilePosition + addHeight;

		if( objectDef != NULL && (objectDef->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			objectDef->m_info.type != LOBJ_TYPE_OBJECT_CFG )
		{
			Vector3D t,b,n;
			defField.GetTileTBN( ref->tile_x, ref->tile_y, t,b,n );

			Matrix4x4 tileAngle(Vector4D(b, 0), Vector4D(n, 0), Vector4D(t, 0), Vector4D(0,0,0,1));

			tileAngle = (!tileAngle)*m;
			//Matrix4x4 tileShear(Vector4D(b, 0), Vector4D(0,1,0,0), Vector4D(t, 0), Vector4D(0,0,0,1));

			//Matrix4x4 tilemat = shearY(cosf(atan2f(b.x, b.y)))*tileTBN;

			m = translate(modelPosition)/* * tileShear*/ * tileAngle;
		}
		else
			m = translate(modelPosition)*m;
	}
	else
	{
		m = translate(ref->position + addHeight)*m;
	}

	return m;
}

static Vector3D s_sortCamPos;

int occluderComparator(levOccluderLine_t* const& a, levOccluderLine_t* const& b)
{
	Vector3D am = lerp(a->start, a->end, 0.5f);
	Vector3D bm = lerp(b->start, b->end, 0.5f);
	float ad = lengthSqr(s_sortCamPos-am);
	float bd = lengthSqr(s_sortCamPos-bm);

	return ad>bd;
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

		bool basicVisibility = frustumOccluders.frustum.IsBoxInside(bbox.minPoint,bbox.maxPoint);

		bool visibleThruOccl = (frustumOccluders.IsSphereVisibleThruOcc(verts[0], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[1], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[2], 0.0f) ||
								frustumOccluders.IsSphereVisibleThruOcc(verts[3], 0.0f));

		if( basicVisibility && visibleThruOccl)
		{
			frustumOccluders.occluderSets.append( new occludingVolume_t(occl, cameraPosition) );
		}
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

	bool renderTranslucency = (nRenderFlags & RFLAG_TRANSLUCENCY) > 0;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];
		CLevObjectDef* cont = ref->def;

#ifdef EDITOR
		if(ref->hide)
			continue;

//----------------------------------------------------------------
// IN-EDITOR RENDERER
//----------------------------------------------------------------

		float fDist = length(cameraPosition - ref->position);

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);

		if(cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
			if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
			{
				if (caps.isInstancingSupported && !(cont->m_info.modelflags & LMODEL_FLAG_UNIQUE))
				{
					if(!cont->m_instData)	// make new instancing data
						cont->m_instData = new levObjInstanceData_t;

					regObjectInstance_t& inst = cont->m_instData->instances[cont->m_instData->numInstances++];

					inst.position = Vector4D(GetModelRefPosition(this, ref), 1.0f);
					inst.rotation = GetModelRefRotation(this, ref);
				}
				else
				{
					materials->SetMatrix(MATRIXMODE_WORLD, mat);
					cont->Render(fDist, ref->bbox, false, nRenderFlags);
				}
			}
		}

		else
		{
			materials->SetMatrix(MATRIXMODE_WORLD, mat);

			const BoundingBox& bbox = cont->m_defModel->GetAABB();

			if( occlFrustum.IsSphereVisible( ref->position, length(bbox.GetSize())) )
				cont->Render(fDist, ref->bbox, false, nRenderFlags);
		}
#else
//----------------------------------------------------------------
// IN-GAME RENDERER
//----------------------------------------------------------------

		if(cont->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
			continue;

		if(renderTranslucency && !cont->m_model->m_hasTransparentSubsets)
			continue;

		if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
		{
			if(	caps.isInstancingSupported &&
				r_enableLevelInstancing.GetBool() && cont->m_instData)
			{
				regObjectInstance_t& inst = cont->m_instData->instances[cont->m_instData->numInstances++];

				inst.position = Vector4D(GetModelRefPosition(this, ref), 1.0f);
				inst.rotation = GetModelRefRotation(this, ref);
			}
			else
			{
				float fDist = length(cameraPosition - ref->position);
				materials->SetMatrix(MATRIXMODE_WORLD, GetModelRefRenderMatrix(this, ref));

				cont->Render(fDist, ref->bbox, false, nRenderFlags);
			}
		}
#endif	// EDITOR
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	if(nav_debug_map.GetBool())
	{
		for(int i = 0; i < GetNumHFields(); i++)
		{
			if(m_heightfield[i])
				m_heightfield[i]->RenderDebug(m_navGrid[0].debugObstacleMap, nRenderFlags, occlFrustum );
		}
	}
	else
	{
		for(int i = 0; i < GetNumHFields(); i++)
		{
			if(m_heightfield[i])
				m_heightfield[i]->Render( nRenderFlags, occlFrustum );
		}
	}
}

void CLevelRegion::Init()
{
	if(!m_heightfield[0])
		m_heightfield[0] = new CHeightTileFieldRenderable();

#ifdef EDITOR
	// init all hfields
	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		if(!m_heightfield[i])
			m_heightfield[i] = new CHeightTileFieldRenderable();

		m_heightfield[i]->m_fieldIdx = i;
	}
#endif // EDITOR
}

void CLevelRegion::InitRoads()
{
	CHeightTileField& defField = *m_heightfield[0];

	// also init navigation grid
	for (int grid = 0; grid < 2; grid++)
		m_navGrid[grid].Init(defField.m_sizew*s_navGridScales[grid], defField.m_sizeh*s_navGridScales[grid]);

	if(!m_roads)
	{
		int numRoadCells = defField.m_sizew * defField.m_sizeh;

		m_roads = new levroadcell_t[ numRoadCells ];
	}

	// we're initializing roads and then going to initialize
	// navgrid data since our heightfield tiles can be detached as well
	for (int x = 0; x < defField.m_sizew; x++)
	{
		for (int y = 0; y < defField.m_sizeh; y++)
		{
			int idx = y * defField.m_sizew + x;

			m_roads[idx].posX = x;
			m_roads[idx].posY = y;

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

	// init debug maps
	if (nav_debug_map.GetInt() > 0)
	{
		m_navGrid[0].debugObstacleMap = g_pShaderAPI->CreateProceduralTexture(varargs("navgrid_%d", m_regionIndex), FORMAT_RGBA8, m_navGrid[0].wide, m_navGrid[0].tall, 1, 1, TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		m_navGrid[0].debugObstacleMap->Ref_Grab();
	}
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

		if((!def || def && def == obj->def) && !obj->name.CompareCaseIns(name))
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

	for(int i = 0; i < m_zones.numElem(); i++)
		delete [] m_zones[i].zoneName;
	m_zones.clear();

	for(int i = 0; i < m_regionDefs.numElem(); i++)
		delete m_regionDefs[i];
	m_regionDefs.clear();

	m_occluders.clear();

	delete [] m_roads;
	m_roads = nullptr;

	g_pShaderAPI->FreeTexture(m_navGrid[0].debugObstacleMap);
	m_navGrid[0].debugObstacleMap = NULL;

	for (int i = 0; i < 2; i++)
	{
		m_navGrid[i].Cleanup();
	}

#ifndef EDITOR
	for (int i = 0; i < GetNumHFields(); i++)
	{
		if (m_heightfield[i])
		{
			g_pPhysics->RemoveHeightField(m_heightfield[i]);
			m_heightfield[i]->CleanRenderData();
		}
	}
#endif // EDITOR

	m_hasTransparentSubsets = false;

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

void CLevelRegion::ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& levelmodels)
{
	if(m_isLoaded)
		return;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(!m_heightfield[i])
			continue;

		m_heightfield[i]->GenereateRenderData( nav_debug_map.GetBool() );

		if(m_heightfield[i]->m_hasTransparentSubsets)
			m_hasTransparentSubsets = true;
	}

	levRegionDataInfo_t	regdatahdr;
	stream->Read(&regdatahdr, 1, sizeof(levRegionDataInfo_t));

	//
	// Load models (object definitions)
	//
	for(int i = 0; i < regdatahdr.numObjectDefs; i++)
	{
		levObjectDefInfo_t defInfo;
		stream->Read(&defInfo, 1, sizeof(levObjectDefInfo_t));

#ifdef EDITOR
		// don't use regenerated models in editor.
		if( defInfo.modelflags & LMODEL_FLAG_GENERATED )
		{
			// skip model data and continue
			stream->Seek(defInfo.size, VS_SEEK_CUR);
			continue;
		}
#endif // EDITOR

		CLevelModel* modelRef = new CLevelModel();
		modelRef->Load( stream );
		modelRef->PreloadTextures();
		modelRef->Ref_Grab();

		CLevObjectDef* newDef = new CLevObjectDef();
		newDef->m_info = defInfo;
		newDef->m_model = modelRef;

		modelRef->GeneratePhysicsData(false);

		m_regionDefs.append(newDef);
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

		//
		// pick from region or global list
		//
		if(cellObj.flags & CELLOBJ_REGION_DEF)
			ref->def = m_regionDefs[cellObj.objectDefId];
		else
			ref->def = levelmodels[cellObj.objectDefId];

		// calculate the transformation
		ref->transform = GetModelRefRenderMatrix( this, ref );

		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// create collision objects and translate them
			CLevelModel* model = ref->def->m_model;
			model->Ref_Grab();

#ifndef EDITOR
			bool noCollide = !(ref->def->m_info.modelflags & LMODEL_FLAG_ISGROUND) && w_noCollide.GetBool();

			if(!noCollide)
			{
				model->CreateCollisionObject( ref );

				// add physics objects
				g_pPhysics->m_physics.AddStaticObject( ref->physObject );
			}
#endif // EDITOR

			if(model->m_hasTransparentSubsets)
				m_hasTransparentSubsets = true;
		}
#ifndef EDITOR
		else
		{
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

				ref->game_object = newObj;

				// let the game objects to be spawned on game frame
				{
					CScopedMutex m(m_level->m_mutex);
					g_pGameWorld->AddObject(newObj);
				}
			}
		}
#endif

		// calculate ref aabb
		ref->CalcBoundingBox();

		// finally add that object
		{
			CScopedMutex m(m_level->m_mutex);

			ASSERT(ref);
			m_objects.append(ref);
		}
	}

#ifndef EDITOR
	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			g_pPhysics->AddHeightField( m_heightfield[i] );
	}
#endif //EDITOR

	Platform_Sleep(1);

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

		// delete associated object from game world
		if(ref->game_object)
		{
			//g_pGameWorld->RemoveObject(ref->object);

			if(g_pGameWorld->m_gameObjects.remove(ref->game_object))
			{
				ref->game_object->SetUserData(NULL);
				ref->game_object->OnRemove();
				delete ref->game_object;
			}

		}

		ref->game_object = NULL;

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

			ref->game_object = newObj;

			// let the game objects to be spawned on game frame
			g_pGameWorld->AddObject( newObj );
		}
	}
#endif // EDITOR
}

void CLevelRegion::ReadLoadRoads(IVirtualStream* stream)
{
	if(m_heightfield[0]->IsEmpty()) // don't init roads, navigation if there is no main heightfield
		return;

	InitRoads();

	int numRoadCells = 0;

	stream->Read(&numRoadCells, 1, sizeof(int));

	if( numRoadCells )
	{
		for(int i = 0; i < numRoadCells; i++)
		{
			levroadcell_t tmpCell;
			stream->Read(&tmpCell, 1, sizeof(levroadcell_t));

			int idx = tmpCell.posY*m_heightfield[0]->m_sizew + tmpCell.posX;
			m_roads[idx] = tmpCell;

			if(tmpCell.type == ROADTYPE_PARKINGLOT)
				continue;

			// fast navgrid uses same resolution
			m_navGrid[1].staticObst[idx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;

			// higher the priority of road nodes
			for(int j = 0; j < AI_NAV_DETAILED_SCALE; j++)
			{
				for(int k = 0; k < AI_NAV_DETAILED_SCALE; k++)
				{
					int ofsX = tmpCell.posX*AI_NAV_DETAILED_SCALE +j;
					int ofsY = tmpCell.posY*AI_NAV_DETAILED_SCALE +k;

					int navCellIdx = ofsY*m_navGrid[0].tall + ofsX;
					m_navGrid[0].staticObst[navCellIdx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;
				}
			}
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
