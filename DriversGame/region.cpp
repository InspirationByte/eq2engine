//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: Level region
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "heightfield.h"

#define AI_NAVIGATION_ROAD_PRIORITY (1)

extern ConVar r_enableLevelInstancing;

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

		for(int i = 0; i < collisionObjects.numElem(); i++)
			g_pPhysics->m_physics.DestroyStaticObject( collisionObjects[i] );
	}
	else
	{
		// delete associated object from game world
		if( g_pGameWorld->IsValidObject( game_object ) )
			g_pGameWorld->RemoveObject( game_object );

		game_object = NULL;
	}
}

//-----------------------------------------------------------------------------------------

CLevelRegion::CLevelRegion()
{
	m_level = NULL;
	m_roads = NULL;

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


Matrix4x4 GetModelRefRenderMatrix(CLevelRegion* reg, regionObject_t* ref)
{
	// models are usually placed at heightfield 0
	CHeightTileField& defField = *reg->m_heightfield[0];

	Matrix4x4 m = rotateXYZ4(DEG2RAD(ref->rotation.x), DEG2RAD(ref->rotation.y), DEG2RAD(ref->rotation.z));

	Vector3D addHeight(0.0f);

	CLevObjectDef* objectDef = ref->def;;

	if(objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG)
		addHeight.y = -objectDef->m_defModel->GetBBoxMins().y;

	if(ref->tile_dependent)
	{
		hfieldtile_t* tile = defField.GetTile( ref->tile_x, ref->tile_y );

		Vector3D tilePosition(ref->tile_x*HFIELD_POINT_SIZE, tile->height*HFIELD_HEIGHT_STEP, ref->tile_y*HFIELD_POINT_SIZE);
		Vector3D modelPosition = defField.m_position + tilePosition + addHeight;

		if( (ref->def->m_info.modelflags & LMODEL_FLAG_ALIGNTOCELL) &&
			ref->def->m_info.type != LOBJ_TYPE_OBJECT_CFG )
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

#ifdef EDITOR
void CLevelRegion::Ed_Prerender()
{
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		CLevObjectDef* cont = m_objects[i]->def;

		Matrix4x4 mat = GetModelRefRenderMatrix(this, m_objects[i]);

		DrawDefLightData(mat, cont->m_lightData, 1.0f);
	}
}
#endif // EDITOR


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
	static DkList<levOccluderLine_t*> occluders;
	occluders.clear(false);

	for(int i = 0; i < m_occluders.numElem(); i++)
		occluders.append( &m_occluders[i] );

	s_sortCamPos = cameraPosition;

	// sort occluders
	occluders.sort( occluderComparator );

	for(int i = 0; i < occluders.numElem(); i++)
	{
		Vector3D verts[4] = {
			occluders[i]->start,
			occluders[i]->end,
			occluders[i]->start + Vector3D(0, occluders[i]->height, 0),
			occluders[i]->end + Vector3D(0, occluders[i]->height, 0)
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
			frustumOccluders.occluderSets.append( new occludingVolume_t(occluders[i], cameraPosition) );
		}
	}
}

void CLevelRegion::Render(const Vector3D& cameraPosition, const Matrix4x4& viewProj, const occludingFrustum_t& occlFrustum, int nRenderFlags)
{
#ifndef EDITOR
	if(!m_isLoaded)
		return;
#else
	Volume frustum;
#endif // EDITOR

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];
		CLevObjectDef* cont = ref->def;


#ifdef EDITOR
//----------------------------------------------------------------
// IN-EDITOR RENDERER
//----------------------------------------------------------------

		float fDist = length(cameraPosition - ref->position);

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);
		frustum.LoadAsFrustum(viewProj * mat);

		if(cont->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
			if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
			{
				if (caps.isInstancingSupported)
				{
					if(!cont->m_instData)	// make new instancing data
						cont->m_instData = new levObjInstanceData_t;

					cont->m_instData->instances[cont->m_instData->numInstances++].worldTransform = mat;
				}
				else
				{
					float fDist = length(cameraPosition - ref->position);

					materials->SetMatrix(MATRIXMODE_WORLD, mat);
					cont->Render(fDist, ref->bbox, false, nRenderFlags);
				}
			}
		}

		else
		{
			materials->SetMatrix(MATRIXMODE_WORLD, mat);

			// render studio model
			//if( frustum.IsBoxInside(cont->m_defModel->GetBBoxMins(), cont->m_defModel->GetBBoxMaxs()) )

			BoundingBox bbox(cont->m_defModel->GetBBoxMins(), cont->m_defModel->GetBBoxMaxs());

			if( occlFrustum.IsSphereVisible( ref->position, length(bbox.GetSize())) )
				cont->Render(fDist, ref->bbox, false, nRenderFlags);
		}
#else
//----------------------------------------------------------------
// IN-GAME RENDERER
//----------------------------------------------------------------

		if(cont->m_info.type != LOBJ_TYPE_INTERNAL_STATIC)
			continue;

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);
		//frustum.LoadAsFrustum(viewProj * mat);

		//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
		if( occlFrustum.IsSphereVisible( ref->position, length(ref->bbox.GetSize())) )
		{
			if(	caps.isInstancingSupported &&
				r_enableLevelInstancing.GetBool())
			{
				cont->m_instData->instances[cont->m_instData->numInstances++].worldTransform = mat;
			}
			else
			{
			/*
				if( ref->object )
				{
					ref->object->Draw( nRenderFlags );
				}
				else
				{*/
					float fDist = length(cameraPosition - ref->position);
					materials->SetMatrix(MATRIXMODE_WORLD, mat);

					cont->Render(fDist, ref->bbox, false, nRenderFlags);
				//}
			}
		}
#endif	// EDITOR
	}

	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			m_heightfield[i]->Render( nRenderFlags, occlFrustum );
	}
}

void CLevelRegion::Init()
{
	if(!m_heightfield[0])
		m_heightfield[0] = new CHeightTileFieldRenderable();

#ifdef EDITOR
	// init all hfields
	for(int i = 1; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		m_heightfield[i] = new CHeightTileFieldRenderable();
		m_heightfield[i]->m_fieldIdx = i;
	}
#endif // EDITOR
}

void CLevelRegion::InitRoads()
{
	CHeightTileField& defField = *m_heightfield[0];

	int numRoadCells = defField.m_sizew * defField.m_sizeh;

	if(numRoadCells > 0 && !m_roads)
	{
		m_roads = new levroadcell_t[ numRoadCells ];

		for(int x = 0; x < defField.m_sizew; x++)
		{
			for(int y = 0; y < defField.m_sizeh; y++)
			{
				int idx = y * defField.m_sizew + x;

				m_roads[idx].type = ROADTYPE_NOROAD;
				m_roads[idx].flags = 0;
				m_roads[idx].posX = x;
				m_roads[idx].posY = y;
				m_roads[idx].direction = ROADDIR_NORTH;
			}
		}
	}

	m_navGrid.Init( defField.m_sizew*AI_NAVIGATION_GRID_SCALE,
					defField.m_sizeh*AI_NAVIGATION_GRID_SCALE);
}

void CLevelRegion::Cleanup()
{
	m_level->m_mutex.Lock();

	for(int i = 0; i < GetNumHFields(); i++)
		g_pPhysics->RemoveHeightField( m_heightfield[i] );

	for(int i = 0; i < m_objects.numElem(); i++)
		delete m_objects[i];

	m_objects.clear();

	m_level->m_mutex.Unlock();

	m_occluders.clear();

	delete [] m_roads;
	m_roads = NULL;

	m_navGrid.Cleanup();

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			m_heightfield[i]->CleanRenderData();
	}

	m_isLoaded = false;

	m_queryTimes.SetValue(0);
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
	for(int i = 0; i < ENGINE_REGION_MAX_HFIELDS; i++)
	{
		if(m_heightfield[i] == NULL)
			return i;
	}

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

#ifdef EDITOR

float CheckStudioRayIntersection(IEqModel* pModel, Vector3D& ray_start, Vector3D& ray_dir)
{
	BoundingBox bbox(pModel->GetBBoxMins(), pModel->GetBBoxMaxs());

	float f1,f2;
	if(!bbox.IntersectsRay(ray_start, ray_dir, f1,f2))
		return MAX_COORD_UNITS;

	float best_dist = MAX_COORD_UNITS;
	float fraction = 1.0f;

	studiohdr_t* pHdr = pModel->GetHWData()->pStudioHdr;
	int nLod = 0;

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];

		while(nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->lodmodels[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			modelgroupdesc_t* pGroup = pHdr->pModelDesc(nModDescId)->pGroup(j);

			uint32 *pIndices = pGroup->pVertexIdx(0);

			for(uint32 k = 0; k < pGroup->numindices; k+=3)
			{
				Vector3D v0,v1,v2;

				v0 = pGroup->pVertex(pIndices[k])->point;
				v1 = pGroup->pVertex(pIndices[k+1])->point;
				v2 = pGroup->pVertex(pIndices[k+2])->point;

				float dist = MAX_COORD_UNITS+1;

				if(IsRayIntersectsTriangle(v0,v1,v2, ray_start, ray_dir, dist))
				{
					if(dist < best_dist && dist > 0)
					{
						best_dist = dist;
						fraction = dist;

						//outPos = lerp(start, end, dist);
					}
				}
			}
		}
	}

	return fraction;
}

int CLevelRegion::Ed_SelectRef(const Vector3D& start, const Vector3D& dir, float& dist)
{
	int bestDistrefIdx = -1;
	float fMaxDist = MAX_COORD_UNITS;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		Matrix4x4 wmatrix = GetModelRefRenderMatrix(this, m_objects[i]);

		Vector3D tray_start = ((!wmatrix)*Vector4D(start, 1)).xyz();
		Vector3D tray_dir = (!wmatrix.getRotationComponent())*dir;

		float raydist = MAX_COORD_UNITS;

		CLevObjectDef* def = m_objects[i]->def;

		if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			raydist = def->m_model->Ed_TraceRayDist(tray_start, tray_dir);
		}
		else
		{
			raydist = CheckStudioRayIntersection(def->m_defModel, tray_start, tray_dir);
		}

		if(raydist < fMaxDist)
		{
			fMaxDist = raydist;
			bestDistrefIdx = i;
		}
	}

	dist = fMaxDist;

	return bestDistrefIdx;
}

int CLevelRegion::Ed_ReplaceDefs(CLevObjectDef* whichReplace, CLevObjectDef* replaceTo)
{
	int numReplaced = 0;

	for(int i = 0; i < m_objects.numElem(); i++)
	{
		if(m_objects[i]->def == whichReplace)
		{
			m_objects[i]->def = replaceTo;
			numReplaced++;
		}
	}

	return numReplaced;
}

#endif // EDITOR


int FindObjectContainer(DkList<CLevObjectDef*>& listObjects, CLevObjectDef* container)
{
	for(int i = 0; i < listObjects.numElem(); i++)
	{
		if( listObjects[i] == container )
			return i;
	}

	ASSERTMSG(false, "Programmer error, cannot find object definition (is editor cached it?)");

	return -1;
}

void CLevelRegion::WriteRegionData( IVirtualStream* stream, DkList<CLevObjectDef*>& listObjects, bool final )
{
	// create region model lists
	DkList<CLevelModel*>		modelList;
	DkList<levCellObject_t>		cellObjectsList;

	cellObjectsList.resize(m_objects.numElem());

	// collect models and cell objects
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		CLevObjectDef* def = m_objects[i]->def;

		int objectDefId = FindObjectContainer(listObjects, def);

		if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			 if( final && !(def->m_info.modelflags & LMODEL_FLAG_NONUNIQUE) )
				objectDefId = modelList.addUnique( def->m_model );
		}

		levCellObject_t object;

		memset(object.name, 0, LEV_OBJECT_NAME_LENGTH);

		object.objectDefId = objectDefId;

		object.tile_x = m_objects[i]->tile_dependent ? m_objects[i]->tile_x : -1;
		object.tile_y = m_objects[i]->tile_dependent ? m_objects[i]->tile_y : -1;
		object.position = m_objects[i]->position;
		object.rotation = m_objects[i]->rotation;

		// add
		cellObjectsList.append(object);
	}

	CMemoryStream regionData;
	regionData.Open(NULL, VS_OPEN_WRITE, 8192);

	// write model data
	for(int i = 0; i < modelList.numElem(); i++)
	{
		CMemoryStream modelData;
		modelData.Open(NULL, VS_OPEN_WRITE, 8192);

		modelList[i]->Save( &modelData );

		int modelSize = modelData.Tell();

		regionData.Write(&modelSize, 1, sizeof(int));
		regionData.Write(modelData.GetBasePointer(), 1, modelData.Tell());
	}

	// write cell objects list
	for(int i = 0; i < cellObjectsList.numElem(); i++)
	{
		regionData.Write(&cellObjectsList[i], 1, sizeof(levCellObject_t));
	}

	levRegionDataInfo_t regdatahdr;
	regdatahdr.numModelObjects = cellObjectsList.numElem();
	regdatahdr.numModels = modelList.numElem();
	regdatahdr.size = regionData.Tell();

	stream->Write(&regdatahdr, 1, sizeof(levRegionDataInfo_t));
	stream->Write(regionData.GetBasePointer(), 1, regionData.Tell());
}

void CLevelRegion::ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& levelmodels)
{
	if(m_isLoaded)
		return;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		m_heightfield[i]->GenereateRenderData();
	}

	DkList<CLevelModel*>	modelList;

	levRegionDataInfo_t		regdatahdr;

	stream->Read(&regdatahdr, 1, sizeof(levRegionDataInfo_t));

	// now read models
	for(int i = 0; i < regdatahdr.numModels; i++)
	{
		int modelSize = 0;
		stream->Read(&modelSize, 1, sizeof(int));

		CLevelModel* modelRef = new CLevelModel();
		modelRef->Load( stream );
		modelRef->PreloadTextures();

		modelList.append(modelRef);
	}

	m_objects.resize( regdatahdr.numModelObjects );

	// read model cells
	for(int i = 0; i < regdatahdr.numModelObjects; i++)
	{
		levCellObject_t cellObj;
		stream->Read(&cellObj, 1, sizeof(levCellObject_t));

		// TODO: add models
		regionObject_t* ref = new regionObject_t;

		if(regdatahdr.numModels == 0)
		{
			ref->def = levelmodels[cellObj.objectDefId];

			CLevelModel* model = ref->def->m_model;
			if(model)
				model->Ref_Grab();
		}
		else
		{
#pragma todo("model (object) containers in region data fix")
			ref->def = NULL;

			//ref->model = modelList[cellObj.objIndex];
			//if(ref->model)
			//	ref->model->Ref_Grab();
		}

		ref->tile_x = cellObj.tile_x;
		ref->tile_y = cellObj.tile_y;

		ref->tile_dependent = (ref->tile_x != -1 || ref->tile_y != -1);
		ref->position = cellObj.position;
		ref->rotation = cellObj.rotation;

		ref->transform = GetModelRefRenderMatrix( this, ref );

		if(ref->def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
		{
			// create collision objects and translate them
			CLevelModel* model = ref->def->m_model;
			model->CreateCollisionObjects( this, ref );

			for(int j = 0; j < ref->collisionObjects.numElem(); j++)
			{
				CScopedMutex m(m_level->m_mutex);

				// add physics objects
				g_pPhysics->m_physics.AddStaticObject( ref->collisionObjects[j] );
			}

			BoundingBox tbbox;

			for(int i = 0; i < 8; i++)
				tbbox.AddVertex((ref->transform*Vector4D(model->m_bbox.GetVertex(i), 1.0f)).xyz());

			// set reference bbox for light testing
			ref->bbox = tbbox;
		}
		else
		{
#ifndef EDITOR

			// create object, spawn in game cycle
			CGameObject* newObj = g_pGameWorld->CreateGameObject( ref->def->m_defType.c_str(), &ref->def->m_defKeyvalues );

			if(newObj)
			{
				Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

				newObj->SetOrigin( transpose(ref->transform).getTranslationComponent() );
				newObj->SetAngles( VRAD2DEG(eulAngles) );
				newObj->SetUserData( ref );

				// network name for this object
				newObj->SetName( varargs("_reg%d_ref%d", m_regionIndex, i) );

				ref->game_object = newObj;

				m_level->m_mutex.Lock();
				g_pGameWorld->AddObject( newObj, false );
				m_level->m_mutex.Unlock();
			}

#endif
		}

		m_level->m_mutex.Lock();
		m_objects.append(ref);
		m_level->m_mutex.Unlock();
	}

	for(int i = 0; i < GetNumHFields(); i++)
		g_pPhysics->AddHeightField( m_heightfield[i] );

	Platform_Sleep(1);

	m_isLoaded = true;
}

void CLevelRegion::RespawnObjects()
{
#ifndef EDITOR
	for(int i = 0; i < m_objects.numElem(); i++)
	{
		regionObject_t* ref = m_objects[i];

		if(ref->def->m_info.type != LOBJ_TYPE_OBJECT_CFG)
			continue;

		// delete associated object from game world
		if(ref->game_object)
		{
			//g_pGameWorld->RemoveObject(ref->object);

			if(g_pGameWorld->m_pGameObjects.remove(ref->game_object))
			{
				ref->game_object->OnRemove();
				delete ref->game_object;
			}

		}

		ref->game_object = NULL;

		ref->transform = GetModelRefRenderMatrix( this, ref );

		// create object, spawn in game cycle
		CGameObject* newObj = g_pGameWorld->CreateGameObject( ref->def->m_defType.c_str(), &ref->def->m_defKeyvalues );

		if(newObj)
		{
			Vector3D eulAngles = EulerMatrixXYZ(ref->transform.getRotationComponent());

			newObj->SetOrigin( transpose(ref->transform).getTranslationComponent() );
			newObj->SetAngles( VRAD2DEG(eulAngles) );
			newObj->SetUserData( ref );

			// network name for this object
			newObj->SetName( varargs("_reg%d_ref%d", m_regionIndex, i) );

			ref->game_object = newObj;

			m_level->m_mutex.Lock();
			g_pGameWorld->AddObject( newObj, false );
			m_level->m_mutex.Unlock();
		}
	}
#endif // EDITOR
}

void CLevelRegion::WriteRegionRoads( IVirtualStream* stream )
{
	CMemoryStream cells;
	cells.Open(NULL, VS_OPEN_WRITE, 2048);

	int numRoadCells = 0;

	for(int x = 0; x < m_heightfield[0]->m_sizew; x++)
	{
		for(int y = 0; y < m_heightfield[0]->m_sizeh; y++)
		{
			int idx = y*m_heightfield[0]->m_sizew + x;

			if(m_roads[idx].type == ROADTYPE_NOROAD)
				continue;

			m_roads[idx].posX = x;
			m_roads[idx].posY = y;

			cells.Write(&m_roads[idx], 1, sizeof(levroadcell_t));
			numRoadCells++;
		}
	}

	stream->Write(&numRoadCells, 1, sizeof(int));
	stream->Write(cells.GetBasePointer(), 1, cells.Tell());
}

void CLevelRegion::ReadLoadRoads(IVirtualStream* stream)
{
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

#define NAVGRIDSCALE_HALF	int(AI_NAVIGATION_GRID_SCALE/2)

			// higher the priority of road nodes
			for(int j = 0; j < AI_NAVIGATION_GRID_SCALE*AI_NAVIGATION_GRID_SCALE; j++)
			{
				int ofsX = tmpCell.posX*AI_NAVIGATION_GRID_SCALE + (j % AI_NAVIGATION_GRID_SCALE);
				int ofsY = tmpCell.posY*AI_NAVIGATION_GRID_SCALE + (j % NAVGRIDSCALE_HALF);

				int navCellIdx = ofsY*m_heightfield[0]->m_sizew + ofsX;
				m_navGrid.staticObst[navCellIdx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;
			}
		}

		for (int i = 0; i < m_objects.numElem(); i++)
			m_level->Nav_AddObstacle(this, m_objects[i]);
	}
}

void CLevelRegion::WriteRegionOccluders(IVirtualStream* stream)
{
	int numOccluders = m_occluders.numElem();

	stream->Write(&numOccluders, 1, sizeof(int));
	stream->Write(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

void CLevelRegion::ReadLoadOccluders(IVirtualStream* stream)
{
	int numOccluders = 0;
	stream->Read(&numOccluders, 1, sizeof(int));

	m_occluders.setNum(numOccluders);
	stream->Read(m_occluders.ptr(), 1, sizeof(levOccluderLine_t)*numOccluders);
}

//-----------------------------------------------------------------------------------------------------------