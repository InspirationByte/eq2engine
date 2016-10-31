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

//-----------------------------------------------------------------------------------------

regionObject_t::~regionObject_t()
{
	// no def = fake object
	if(!def)
		return;

	if(def->m_info.type == LOBJ_TYPE_INTERNAL_STATIC)
	{
#ifndef EDITOR
		g_pPhysics->m_physics.DestroyStaticObject( physObject );
		physObject = NULL;
#endif // EDITOR

		CLevelModel* mod = def->m_model;
		mod->Ref_Drop();

		// the model cannot be removed if it's not loaded with region
		if(mod->Ref_Count() <= 0)
			delete mod;
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

//-----------------------------------------------------------------------------------------

CLevelRegion::CLevelRegion()
{
	m_level = NULL;
	m_roads = NULL;

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

	CLevObjectDef* objectDef = ref->def;;

	if(objectDef != NULL && objectDef->m_info.type == LOBJ_TYPE_OBJECT_CFG)
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

	bool renderTranslucency = (nRenderFlags & RFLAG_TRANSLUCENCY) > 0;

	const ShaderAPICaps_t& caps = g_pShaderAPI->GetCaps();

	m_level->m_mutex.Lock();

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
		frustum.LoadAsFrustum(viewProj * mat);

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

		Matrix4x4 mat = GetModelRefRenderMatrix(this, ref);
		//frustum.LoadAsFrustum(viewProj * mat);

		//if( frustum.IsBoxInside(cont->m_model->m_bbox.minPoint, cont->m_model->m_bbox.maxPoint) )
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
				materials->SetMatrix(MATRIXMODE_WORLD, mat);

				cont->Render(fDist, ref->bbox, false, nRenderFlags);
			}
		}
#endif	// EDITOR
	}

	m_level->m_mutex.Unlock();

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

#ifndef EDITOR
	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
		{
			g_pPhysics->RemoveHeightField( m_heightfield[i] );
			m_heightfield[i]->CleanRenderData();
		}
	}
#endif // EDITOR

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
	m_roads = NULL;

	m_navGrid.Cleanup();

	m_isLoaded = false;
	m_hasTransparentSubsets = false;

	m_level->m_mutex.Unlock();

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

void CLevelRegion::GetDecalPolygons(decalprimitives_t& polys, occludingFrustum_t* frustum)
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

ConVar w_pregeneratedhfields("w_pregeneratedhfields", "1");

void CLevelRegion::ReadLoadRegion(IVirtualStream* stream, DkList<CLevObjectDef*>& levelmodels)
{
	if(m_isLoaded)
		return;

	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(!m_heightfield[i])
			continue;

        if(w_pregeneratedhfields.GetBool())
            m_heightfield[i]->GenereateRenderData();

		if(m_heightfield[i]->m_hasTransparentSubsets)
			m_hasTransparentSubsets = true;
	}

	levRegionDataInfo_t		regdatahdr;
	stream->Read(&regdatahdr, 1, sizeof(levRegionDataInfo_t));

	m_regionDefs.resize( regdatahdr.numObjectDefs );
	m_objects.resize( regdatahdr.numCellObjects );

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

		ASSERT(ref);

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
			model->CreateCollisionObject( ref );

			// add physics objects
			g_pPhysics->m_physics.AddStaticObject( ref->physObject );
#endif // EDITOR

			if(model->m_hasTransparentSubsets)
				m_hasTransparentSubsets = true;

			BoundingBox tbbox;

			for(int i = 0; i < 8; i++)
				tbbox.AddVertex((ref->transform*Vector4D(model->m_bbox.GetVertex(i), 1.0f)).xyz());

			// set reference bbox for light testing
			ref->bbox = tbbox;
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
				m_level->m_mutex.Lock();
				g_pGameWorld->AddObject( newObj, false );
				m_level->m_mutex.Unlock();
			}
		}
#endif

		// finally add that object
		m_level->m_mutex.Lock();

		m_objects.append(ref);

		m_level->m_mutex.Unlock();
	}

#ifndef EDITOR
	for(int i = 0; i < GetNumHFields(); i++)
	{
		if(m_heightfield[i])
			g_pPhysics->AddHeightField( m_heightfield[i] );
	}
#endif //EDITOR

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

			m_level->m_mutex.Lock();
			g_pGameWorld->AddObject( newObj, false );
			m_level->m_mutex.Unlock();
		}
	}
#endif // EDITOR
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

#ifdef EDITOR
			tmpCell.flags &= ~ROAD_FLAG_TRAFFICLIGHT;
#endif // EDITOR

			int idx = tmpCell.posY*m_heightfield[0]->m_sizew + tmpCell.posX;
			m_roads[idx] = tmpCell;

#define NAVGRIDSCALE_HALF	int(AI_NAVIGATION_GRID_SCALE/2)

			if(tmpCell.type == ROADTYPE_PARKINGLOT)
				continue;

			// higher the priority of road nodes
			for(int j = 0; j < AI_NAVIGATION_GRID_SCALE; j++)
			{
				for(int k = 0; k < AI_NAVIGATION_GRID_SCALE; k++)
				{
					int ofsX = tmpCell.posX*AI_NAVIGATION_GRID_SCALE+j;
					int ofsY = tmpCell.posY*AI_NAVIGATION_GRID_SCALE+k;

					int navCellIdx = ofsY*m_navGrid.tall + ofsX;
					m_navGrid.staticObst[navCellIdx] = 4 - AI_NAVIGATION_ROAD_PRIORITY;
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
