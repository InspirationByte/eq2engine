//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics layer
//////////////////////////////////////////////////////////////////////////////////


#include "GameObject.h"
#include "eqGlobalMutex.h"

#include "physics.h"

#include "heightfield.h"
#include "IDebugOverlay.h"

#include "utils/GeomTools.h"

#include "eqPhysics/eqBulletIndexedMesh.h"
#include "../shared_engine/physics/BulletConvert.h"

#include "Shiny.h"

ConVar ph_debugrender("ph_debugrender", "0", NULL, CV_CHEAT);
ConVar ph_singleiter("ph_singleiter", "0");

using namespace EqBulletUtils;

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
	PROFILE_FUNC();

	if (!m_object->IsCanIterate(true))
		return;

	if (m_owner->m_state == GO_STATE_IDLE)
	{
		Vector3D angles = eulers(m_object->GetOrientation());
		m_owner->m_vecAngles = VRAD2DEG(angles);
		m_owner->m_vecOrigin = m_object->GetPosition();

		float lastdt = m_object->GetLastFrameTime();
		m_owner->OnPrePhysicsFrame(lastdt);
	}
}

void CPhysicsHFObject::PostSimulate( float fDt )
{
	PROFILE_FUNC();

	if (!m_object->IsCanIterate(true))
		return;

	if (m_owner->m_state == GO_STATE_IDLE)
	{
		Vector3D angles = eulers(m_object->GetOrientation());
		m_owner->m_vecAngles = VRAD2DEG(angles);
		m_owner->m_vecOrigin = m_object->GetPosition();

		//float lastdt = m_object->GetLastFrameTime();
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

#ifndef EDITOR
	m_physics.InitGrid();
#endif // m_physics

	m_dtAccumulator = 0.0f;

//	m_nextPhysicsUpdate = 0.0f;
//	m_curTime = 0.0f;
}

#ifdef EDITOR
void CPhysicsEngine::SceneInitBroadphase()
{
	m_physics.InitGrid();
}

void CPhysicsEngine::SceneDestroyBroadphase()
{
	m_physics.DestroyGrid();
}

#endif

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

void PhysDebugRender(void* arg)
{
	CPhysicsEngine* physEngine = (CPhysicsEngine*)arg;

	physEngine->m_physics.DebugDrawBodies( ph_debugrender.GetInt() );
}

const float PHYSICS_FRAME_INTERVAL = (1.0f / 60.0f);

void CPhysicsEngine::Simulate( float fDt, int numIterations, FNSIMULATECALLBACK preIntegrateFn )
{
	PROFILE_FUNC();

	double timestep = PHYSICS_FRAME_INTERVAL / numIterations;

	m_dtAccumulator += fDt;

	// increase fixed frames if we're going low
	while(m_dtAccumulator > PHYSICS_FRAME_INTERVAL)
	{
		if(ph_singleiter.GetBool())
			numIterations = 1;

		// do real iteration count
		for(int i = 0; i < numIterations; i++)
		{
			m_physics.PrepareSimulateStep();
			m_physics.SimulateStep(timestep, i, preIntegrateFn);
		}

		m_dtAccumulator -= PHYSICS_FRAME_INTERVAL;
	}

	// debug rendering
	if(ph_debugrender.GetBool())
		debugoverlay->Draw3DFunc( PhysDebugRender, this );
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

bool physHFieldVertexComparator(const Vector3D &a, const Vector3D &b)
{
	return compare_epsilon(a, b, 0.001f);
}

CEqBulletIndexedMesh* CreateBulletTriangleMeshFromBatch(hfieldbatch_t* batch)
{
	// generate optimized/reduced set
	DkList<uint32>		batchindices;
	batchindices.resize(batch->indices.numElem());

	for(int i = 0; i < batch->indices.numElem(); i++)
	{
		int vtxId = batch->indices[i];

		int idx = batch->physicsVerts.addUnique( batch->verts[vtxId].position, physHFieldVertexComparator );

		batchindices.append(idx);
	}

	batch->verts.clear();
	batch->indices.clear();

	batch->indices.append(batchindices);

	CEqBulletIndexedMesh* pTriMesh = new CEqBulletIndexedMesh( (ubyte*)batch->physicsVerts.ptr(), sizeof(Vector3D),
																(ubyte*)batch->indices.ptr(), sizeof(uint32), batch->physicsVerts.numElem(), batch->indices.numElem() );

	return pTriMesh;
}

void CPhysicsEngine::AddHeightField( CHeightTileField* pField )
{
	if(pField->m_physData != NULL)
	{
		ASSERTMSG(false, "AddHeightField: please remove first");
		return;
	}

	hfieldPhysicsData_t* fieldInfo = new hfieldPhysicsData_t;

	pField->Generate(false, fieldInfo->m_batches);

	if(fieldInfo->m_batches.numElem() == 0)
	{
		delete fieldInfo;
		return;
	}

	// add collision object
    Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_LEVEL_LOADER);

	for(int i = 0; i < fieldInfo->m_batches.numElem(); i++)
	{
		hfieldbatch_t* batch = fieldInfo->m_batches[i];
		IMaterial* material = batch->materialBundle->material;

		IMatVar* pVar = material->GetMaterialVar("surfaceprops", "default");

		const char* surfParamName = pVar->GetString();

		eqPhysSurfParam_t* param = FindSurfaceParam( surfParamName );

		if(!param)
		{
			MsgError("FindSurfaceParam: invalid material '%s' in material '%s'\n", surfParamName, material->GetName());
			param = FindSurfaceParam( "default" );
		}

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

		mutex.Lock();

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

		mutex.Unlock();
	}

	// set user data and add
	pField->m_physData = fieldInfo;

	mutex.Lock();
	m_heightFields.append(pField);
	mutex.Unlock();
}

void CPhysicsEngine::RemoveObject( CPhysicsHFObject* pPhysObject )
{
	if(!pPhysObject)
		return;

	pPhysObject->m_object->SetUserData(NULL);

	CEqRigidBody* body = pPhysObject->m_object;

	// detach HFObject from body and remove from list
	delete pPhysObject;
	m_pObjects.fastRemove( pPhysObject );

	// destroy body
	m_physics.DestroyBody( body );
}

void CPhysicsEngine::RemoveHeightField( CHeightTileField* pPhysObject )
{
    Threading::CEqMutex& mutex = Threading::GetGlobalMutex(Threading::MUTEXPURPOSE_LEVEL_LOADER);

	if( pPhysObject->m_physData )
	{
		hfieldPhysicsData_t* fieldInfo = pPhysObject->m_physData;

		mutex.Lock();

		for(int i = 0; i < fieldInfo->m_collisionObjects.numElem(); i++)
		{
			CEqCollisionObject* obj = fieldInfo->m_collisionObjects[i];

			// add it as ghost object
			if( obj->GetContents() & OBJECTCONTENTS_WATER )
				m_physics.DestroyGhostObject( obj );
			else
				m_physics.DestroyStaticObject( obj );
		}

		mutex.Unlock();

		for(int i = 0; i < fieldInfo->m_meshes.numElem(); i++)
			delete fieldInfo->m_meshes[i];

		for(int i = 0; i < fieldInfo->m_batches.numElem(); i++)
			delete fieldInfo->m_batches[i];

		delete fieldInfo;
		pPhysObject->m_physData = NULL;
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
