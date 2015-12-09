//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics layer
//////////////////////////////////////////////////////////////////////////////////

#include "physics.h"

#include "heightfield.h"
#include "IDebugOverlay.h"

#include "utils/GeomTools.h"

#include "world.h"

#include "../shared_engine/physics/BulletConvert.h"

#include "Shiny.h"

using namespace EqBulletUtils;

struct physicshfield_t
{
	DkList<CEqCollisionObject*>			m_collisionObjects;
	DkList<btStridingMeshInterface*>	m_meshes;
	DkList<hfieldbatch_t*>				m_batches;
};

#define PH_GRAVITY -9.81

// high frequency object wrapper
// used for vehicles
CPhysicsHFObject::CPhysicsHFObject( CEqRigidBody* pObj, CGameObject* owner ) : IEqPhysCallback(pObj)
{
	m_owner = owner;
}

CPhysicsHFObject::~CPhysicsHFObject()
{
}

void CPhysicsHFObject::PreSimulate( float fDt )
{
	PROFILE_FUNC()

	if (!m_object->IsCanIterate(true))
		return;

	if (m_owner->m_state == GO_STATE_IDLE)
	{
		float lastdt = m_object->GetLastFrameTime();
		m_owner->OnPrePhysicsFrame(lastdt);
	}
}

void CPhysicsHFObject::PostSimulate( float fDt )
{
	PROFILE_FUNC()

	if (!m_object->IsCanIterate(true))
		return;

	if (m_owner->m_state == GO_STATE_IDLE)
	{
		float lastdt = m_object->GetLastFrameTime();
		m_owner->OnPhysicsFrame(fDt);
	}
		
}

//-------------------------------------------------------------------------------------

CPhysicsEngine::CPhysicsEngine()
{
	//m_physics = NULL;
}

void CPhysicsEngine::SceneInit()
{
	m_physics.InitWorld();

//	m_nextPhysicsUpdate = 0.0f;
//	m_curTime = 0.0f;
}

void CPhysicsEngine::SceneShutdown()
{
	// Destruye el array de objetos dinámicos
	for(int i = 0; i < m_pObjects.numElem(); i++)
	{
		m_physics.DestroyBody(m_pObjects[i]->m_object);
	}

	m_pObjects.clear();

	m_heightFields.clear();


	m_physics.DestroyWorld();
}


//extern ConVar sys_timescale;

const int constIterationsFramerate = 60;

ConVar ph_singleiter("ph_singleiter", "0");

void CPhysicsEngine::Simulate( float fDt, int numIterations, FNSIMULATECALLBACK preIntegrateFn )
{
	PROFILE_FUNC()

	double timestep = fDt / numIterations;

	m_physics.DebugDrawBodies();

	if(ph_singleiter.GetBool())
		numIterations = 1;

	for(int i = 0; i < numIterations; i++)
	{
		m_physics.PrepareSimulateStep();
		m_physics.SimulateStep(timestep, i, preIntegrateFn);
	}
}

bool CPhysicsEngine::TestLine(const FVector3D& start, const FVector3D& end, CollisionData_t& coll, int rayMask, eqPhysCollisionFilter* filter)
{
	return m_physics.TestLineCollision(start, end, coll, rayMask, filter);
}

bool CPhysicsEngine::TestConvexSweep(btCollisionShape* shape, const Quaternion& rotation, const FVector3D& start, const FVector3D& end, CollisionData_t& coll, int rayMask, eqPhysCollisionFilter* filter)
{
	return m_physics.TestConvexSweepCollision(shape, rotation, start, end, coll, rayMask, filter);
}

// object add/remove functions
void CPhysicsEngine::AddObject( CPhysicsHFObject* pPhysObject )
{
	m_pObjects.append( pPhysObject );

	m_physics.AddToWorld(pPhysObject->m_object);
}

eqPhysSurfParam_t* CPhysicsEngine::FindSurfaceParam( const char* name )
{
	return m_physics.FindSurfaceParam(name);
}

eqPhysSurfParam_t* CPhysicsEngine::GetSurfaceParamByID( int id )
{
	return m_physics.GetSurfaceParamByID(id);
}

#include "eqPhysics/eqBulletIndexedMesh.h"

bool physHFieldVertexComparator(const hfielddrawvertex_t &a, const hfielddrawvertex_t &b)
{
	return compare_epsilon(a.position, b.position, 0.001f);
}

CEqBulletIndexedMesh* CreateBulletTriangleMeshFromBatch(hfieldbatch_t* batch)
{
	// generate optimized/reduced set
	DkList<hfielddrawvertex_t>	batchverts;
	DkList<uint32>				batchindices;

	batchverts.resize(batch->verts.numElem());
	batchindices.resize(batch->indices.numElem());

	for(int i = 0; i < batch->indices.numElem(); i++)
	{
		int vtxId = batch->indices[i];

		int idx = batchverts.addUnique( batch->verts[vtxId] , physHFieldVertexComparator );

		batchindices.append(idx);
	}

	batch->verts.clear();
	batch->indices.clear();

	batch->verts.append(batchverts);
	batch->indices.append(batchindices);

	CEqBulletIndexedMesh* pTriMesh = new CEqBulletIndexedMesh(	((ubyte*)batch->verts.ptr())+offsetOf(hfielddrawvertex_t, position), sizeof(hfielddrawvertex_t),
																(ubyte*)batch->indices.ptr(), sizeof(uint32), batch->verts.numElem(), batch->indices.numElem() );


	return pTriMesh;
}

void CPhysicsEngine::AddHeightField( CHeightTileField* pField )
{
	if(pField->m_userData != NULL)
	{
		ASSERTMSG(false, "AddHeightField: please remove first");
		return;
	}

	physicshfield_t* fieldInfo = new physicshfield_t;

	pField->Generate(false, fieldInfo->m_batches);

	if(fieldInfo->m_batches.numElem() == 0)
	{
		delete fieldInfo;
		return;
	}

	// add collision object

	int nVerts = 0;
	int nIndices = 0;

	for(int i = 0; i < fieldInfo->m_batches.numElem(); i++)
	{
		hfieldbatch_t* batch = fieldInfo->m_batches[i];

		nVerts += batch->verts.numElem();
		nIndices += batch->indices.numElem();
	}

	nVerts = 0;
	nIndices = 0;

	// TODO: array of static objects and transformation info since it's a fixed point physics...

	for(int i = 0; i < fieldInfo->m_batches.numElem(); i++)
	{
		hfieldbatch_t* batch = fieldInfo->m_batches[i];
		IMaterial* material = batch->materialBundle->material;

		IMatVar* pVar = material->GetMaterialVar("surfaceprops", "default");

		eqPhysSurfParam_t* param = FindSurfaceParam( pVar->GetString() );

		if(!param)
		{
			MsgError("FindSurfaceParam: invalid material '%s' in material '%s'\n", pVar->GetString(), material->GetName());
			param = FindSurfaceParam( "default" );
		}

		nVerts += batch->verts.numElem();
		nIndices += batch->indices.numElem();

		// add physics object
		CEqBulletIndexedMesh* mesh = CreateBulletTriangleMeshFromBatch( batch );

		CEqCollisionObject* staticObject = new CEqCollisionObject();

		if(param)
			staticObject->m_surfParam = param->id;

		staticObject->Initialize(mesh);
		staticObject->SetPosition(pField->m_position);

		fieldInfo->m_collisionObjects.append( staticObject );
		fieldInfo->m_meshes.append(mesh);
		
		staticObject->SetCollideMask(0); // since it's not a dynamic object
		staticObject->SetRestitution(0.0f);
		staticObject->SetFriction(1.0f);

		g_pGameWorld->m_level.m_mutex.Lock();
		
		if(param->word == 'W')	// water?
		{
			// add it as ghost object
			staticObject->SetContents( OBJECTCONTENTS_WATER );
			m_physics.AddGhostObject( staticObject );

			// raycast allowed
			staticObject->m_flags &= ~COLLOBJ_NO_RAYCAST;
		}
		else
		{
			// add normally
			staticObject->SetContents( OBJECTCONTENTS_SOLID_GROUND );
			m_physics.AddStaticObject( staticObject );
		}

		g_pGameWorld->m_level.m_mutex.Unlock();
	}

	// set user data and add
	pField->m_userData = fieldInfo;

	g_pGameWorld->m_level.m_mutex.Lock();
	m_heightFields.append(pField);
	g_pGameWorld->m_level.m_mutex.Unlock();
}

void CPhysicsEngine::RemoveObject( CPhysicsHFObject* pPhysObject )
{
	if(!pPhysObject)
		return;

	pPhysObject->m_object->SetUserData(NULL);

	m_physics.DestroyBody( pPhysObject->m_object );
	m_pObjects.fastRemove( pPhysObject );

	delete pPhysObject;
}

void CPhysicsEngine::RemoveHeightField( CHeightTileField* pPhysObject )
{
	if( pPhysObject->m_userData )
	{
		physicshfield_t* fieldInfo = (physicshfield_t*)pPhysObject->m_userData;

		g_pGameWorld->m_level.m_mutex.Lock();

		for(int i = 0; i < fieldInfo->m_collisionObjects.numElem(); i++)
		{
			CEqCollisionObject* obj = fieldInfo->m_collisionObjects[i];

			// add it as ghost object
			if( obj->GetContents() & OBJECTCONTENTS_WATER )
				m_physics.DestroyGhostObject( obj );
			else
				m_physics.DestroyStaticObject( obj );
		}

		g_pGameWorld->m_level.m_mutex.Unlock();

		for(int i = 0; i < fieldInfo->m_meshes.numElem(); i++)
			delete fieldInfo->m_meshes[i];

		for(int i = 0; i < fieldInfo->m_batches.numElem(); i++)
			delete fieldInfo->m_batches[i];

		delete fieldInfo;
		pPhysObject->m_userData = NULL;
	}

	m_heightFields.fastRemove( pPhysObject );
}


//-------------------------------------------------------------------------------------------------------

static CPhysicsEngine s_Physics;
CPhysicsEngine* g_pPhysics = &s_Physics;

//-------------------------------------------------------------------------------------------------------
/*
Vector3D TAtoEqVector3D(const TA::Vec3& vec)
{
	return Vector3D(vec.x, vec.y, vec.z);
}

TA::Vec3 EqtoTAVector3D(const Vector3D& vec)
{
	return TA::Vec3(vec.x, vec.y, vec.z);
}*/