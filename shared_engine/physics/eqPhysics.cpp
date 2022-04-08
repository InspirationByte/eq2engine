//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//
//	FEATURES:
//				Fixed point object positions
//				Best works with fixed timestep
//				Simple dynamics build from ground up
//				Raycasting along with sweep collision test
//				(BAD) Using Bullet Collision Library for fast collision detection
//
//////////////////////////////////////////////////////////////////////////////////

#include "core/ConVar.h"
#include "core/DebugInterface.h"
#include "core/IEqParallelJobs.h"

#include "utils/KeyValues.h"
#include "utils/global_mutex.h"

#include "render/IDebugOverlay.h"

#include "eqPhysics.h"
#include "eqPhysics_Body.h"
#include "eqPhysics_Contstraint.h"
#include "eqPhysics_Controller.h"

#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include "eqBulletIndexedMesh.h"

// #define PROFILE			// this has to be disabled when GamePhysics using worker thread
#define CONTACT_GROUPING

#ifdef PROFILE
#include "Shiny.h"
#endif

#include "../shared_engine/physics/BulletConvert.h"

using namespace EqBulletUtils;
using namespace Threading;

#define PHYSGRID_WORLD_SIZE			24	// compromised betwen memory usage and performance
#define PHYSICS_WORLD_MAX_UNITS		65535.0f

extern ConVar ph_margin;

ConVar ph_showcontacts("ph_showcontacts", "0", NULL, CV_CHEAT);
ConVar ph_erp("ph_erp", "0.15", "Collision correction", CV_CHEAT);

// cvar value mostly depends on velocity
ConVar ph_grid_tolerance("ph_grid_tolerance", "0.1", NULL, CV_CHEAT);

const float PHYSICS_DEFAULT_FRICTION = 0.5f;
const float PHYSICS_DEFAULT_RESTITUTION = 0.25f;
const float PHYSICS_DEFAULT_TIRE_FRICTION = 0.2f;
const float PHYSICS_DEFAULT_TIRE_TRACTION = 1.0f;

CEqCollisionObject* ContactPair_t::GetOppositeTo(CEqCollisionObject* obj) const
{
	return (obj == bodyA) ? bodyB : bodyA;
}

CEqCollisionObject* CollisionPairData_t::GetOppositeTo(CEqCollisionObject* obj) const
{
	return (obj == bodyA) ? bodyB : bodyA;
}

//------------------------------------------------------------------------------------------------------------

static inline int btInternalGetHash(int partId, int triangleIndex)
{
	return (partId << (31 - MAX_NUM_PARTS_IN_BITS)) | triangleIndex;
}

/// Adjusts collision for using single side, ignoring internal triangle edges
/// If this info map is missing, or the triangle is not store in this map, nothing will be done
void AdjustSingleSidedContact(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0)
{
	const btCollisionShape* shape = colObj0Wrap->getCollisionShape();

	if (shape->getShapeType() != TRIANGLE_SHAPE_PROXYTYPE)
		return;

	const btCollisionObject* obj = colObj0Wrap->getCollisionObject();
	const btCollisionShape* collObjShape = obj->getCollisionShape();

	btBvhTriangleMeshShape* trimesh = nullptr;

	if (shape->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE)
		trimesh = ((btScaledBvhTriangleMeshShape*)collObjShape)->getChildShape();
	else
		trimesh = (btBvhTriangleMeshShape*)collObjShape;

	btTriangleInfoMap* triangleInfoMapPtr = trimesh->getTriangleInfoMap();
	if (!triangleInfoMapPtr)
		return;

	if (triangleInfoMapPtr->findIndex(btInternalGetHash(partId0, index0)) == BT_HASH_NULL)
		return;

	const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(shape);

	btVector3 tri_normal;
	tri_shape->calcNormal(tri_normal);

	cp.m_normalWorldOnB = obj->getWorldTransform().getBasis() * tri_normal;
}

//----------------------------------------------------------------------------------------------

const float CONTACT_GROUPING_POSITION_TOLERANCE		= 0.05f;		// distance
const float CONTACT_GROUPING_NORMAL_TOLERANCE		= 0.85f;		// cosine

struct CEqManifoldResult : public btManifoldResult
{
	CEqManifoldResult(const btCollisionObjectWrapper* obj0Wrap, const btCollisionObjectWrapper* obj1Wrap, bool singleSided, const Vector3D& center)
		: btManifoldResult(obj0Wrap, obj1Wrap), m_center(center), m_singleSided(singleSided)
	{
		m_closestPointDistanceThreshold = 0.0f;
	}

	virtual void addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth)
	{
		bool isSwapped = m_manifoldPtr->getBody0() != m_body0Wrap->getCollisionObject();

		btVector3 pointA = pointInWorld + normalOnBInWorld * depth;
		btVector3 localA;
		btVector3 localB;

		if (isSwapped)
		{
			localA = m_body1Wrap->getWorldTransform().invXform(pointA);
			localB = m_body0Wrap->getWorldTransform().invXform(pointInWorld);
		}
		else
		{
			localA = m_body0Wrap->getWorldTransform().invXform(pointA);
			localB = m_body1Wrap->getWorldTransform().invXform(pointInWorld);
		}

		btManifoldPoint newPt(localA, localB, normalOnBInWorld, depth);
		newPt.m_positionWorldOnA = pointA;
		newPt.m_positionWorldOnB = pointInWorld;

		//BP mod, store contact triangles.
		if (isSwapped)
		{
			newPt.m_partId0 = m_partId1;
			newPt.m_partId1 = m_partId0;
			newPt.m_index0 = m_index1;
			newPt.m_index1 = m_index0;
		}
		else
		{
			newPt.m_partId0 = m_partId0;
			newPt.m_partId1 = m_partId1;
			newPt.m_index0 = m_index0;
			newPt.m_index1 = m_index1;
		}

		//experimental feature info, for per-triangle material etc.
		const btCollisionObjectWrapper* obj0Wrap = isSwapped ? m_body1Wrap : m_body0Wrap;
		const btCollisionObjectWrapper* obj1Wrap = isSwapped ? m_body0Wrap : m_body1Wrap;

		addSingleResult(newPt, obj0Wrap, newPt.m_partId0, newPt.m_index0, obj1Wrap);
	}

	void addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap)
	{
		if (m_singleSided)
			AdjustSingleSidedContact(cp, colObj1Wrap, cp.m_partId1, cp.m_index1);
		else
			btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, cp.m_partId1, cp.m_index1);

		float distance = cp.getDistance();

		// if something is a NaN we have to deny it
		if (cp.m_positionWorldOnA != cp.m_positionWorldOnA || 
			cp.m_normalWorldOnB != cp.m_normalWorldOnB)
			return;

		Vector3D position;
		Vector3D normal;

		ConvertBulletToDKVectors(position, cp.m_positionWorldOnA);
		position -= m_center;
		
		ConvertBulletToDKVectors(normal, cp.m_normalWorldOnB);
		int materialIndex = -1;

		const btCollisionShape* shape0 = colObj0Wrap->getCollisionShape();
		const btCollisionShape* shape1 = colObj1Wrap->getCollisionShape();

		if (shape1->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE)
		{
			CEqCollisionObject* obj = (CEqCollisionObject*)colObj1Wrap->getCollisionObject()->getUserPointer();

			if (obj && obj->GetMesh())
			{
				CEqBulletIndexedMesh* mesh = (CEqBulletIndexedMesh*)obj->GetMesh();
				materialIndex = mesh->getSubPartMaterialId(cp.m_partId1);
			}
		}

		float processingThreshold = m_manifoldPtr->getContactProcessingThreshold();

#ifdef CONTACT_GROUPING		
		for(int i = 0; i < m_collisions.numElem(); i++)
		{
			CollisionData_t& coll = m_collisions[i];

			if(	coll.materialIndex == materialIndex &&
				fsimilar(coll.position.x, position.x, CONTACT_GROUPING_POSITION_TOLERANCE) &&
				fsimilar(coll.position.y, position.y, CONTACT_GROUPING_POSITION_TOLERANCE) &&
				fsimilar(coll.position.z, position.z, CONTACT_GROUPING_POSITION_TOLERANCE))
				//dot(coll.normal, normal) > CONTACT_GROUPING_NORMAL_TOLERANCE)
			{
				coll.position += position;
				coll.normal += normal;
				coll.fract += distance;
				
				coll.position *= 0.5f;
				coll.normal *= 0.5f;
				coll.fract *= 0.5f;
				
				return;
			}
		}
#endif // CONTACT_GROUPING
		
		int numColls = m_collisions.numElem();
		m_collisions.setNum(numColls + 1);

		CollisionData_t& data = m_collisions[numColls];

		data.fract = distance;
		ConvertBulletToDKVectors(data.normal, cp.m_normalWorldOnB);
		ConvertBulletToDKVectors(position, cp.m_positionWorldOnA);
		data.position = position - m_center;
		data.materialIndex = materialIndex;
		data.pad = 1;
	}

	bool GetSingleContact(CollisionData_t& coll)
	{
		coll.normal = 0;
		coll.position = vec3_zero;
		coll.fract = 0;
		coll.materialIndex = -1;

		int count = m_collisions.numElem();

		if (!count)
			return false;
	
		for(int i = 0; i < m_collisions.numElem(); i++)
		{
			coll.position += m_collisions[i].position;
			coll.normal += m_collisions[i].normal;
			coll.fract += m_collisions[i].fract;

			coll.materialIndex = m_collisions[i].materialIndex;
		}

		coll.normal /= float(count);
		coll.position /= float(count);
		coll.fract /= float(count);

		return true;
	}

	DkList<CollisionData_t> m_collisions;
	Vector3D				m_center;
	bool					m_singleSided;
};

//------------------------------------------------------------------------------------------------------------

CEqPhysics::CEqPhysics() : m_mutex(GetGlobalMutex(MUTEXPURPOSE_PHYSICS))
{
	m_debugRaycast = false;
}

CEqPhysics::~CEqPhysics()
{

}

void InitSurfaceParams( DkList<eqPhysSurfParam_t*>& list )
{
	// parse physics file
	KeyValues kvs;
	if (!kvs.LoadFromFile("scripts/SurfaceParams.def"))
	{
		MsgError("ERROR! Failed to load 'scripts/SurfaceParams.def'!");
		return;
	}

	for (int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* pSec = kvs.GetRootSection()->keys[i];

		if (!stricmp(pSec->name, "#include"))
			continue;

		eqPhysSurfParam_t* pMaterial = new eqPhysSurfParam_t;
		int mat_idx = list.append(pMaterial);

		pMaterial->id = mat_idx;
		pMaterial->name = pSec->name;
		pMaterial->friction = KV_GetValueFloat(pSec->FindKeyBase("friction"), 0, PHYSICS_DEFAULT_FRICTION);
		pMaterial->restitution = KV_GetValueFloat(pSec->FindKeyBase("restitution"), 0, PHYSICS_DEFAULT_RESTITUTION);
		pMaterial->tirefriction = KV_GetValueFloat(pSec->FindKeyBase("tirefriction"), 0, PHYSICS_DEFAULT_TIRE_FRICTION);
		pMaterial->tirefriction_traction = KV_GetValueFloat(pSec->FindKeyBase("tirefriction_traction"), 0, PHYSICS_DEFAULT_TIRE_TRACTION);
		pMaterial->word = KV_GetValueString(pSec->FindKeyBase("surfaceword"), NULL, "C")[0];
	}
}

void CEqPhysics::InitWorld()
{
	// collision configuration contains default setup for memory, collision setup
	m_collConfig = new btDefaultCollisionConfiguration();

	// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_collDispatcher = new btCollisionDispatcher( m_collConfig );

	// still required for raycasts
	m_collisionWorld = new btCollisionWorld(m_collDispatcher, NULL, m_collConfig);

	m_dispatchInfo.m_enableSatConvex = true;
	m_dispatchInfo.m_useContinuous = true;
	m_dispatchInfo.m_stepCount = 1;
	//m_dispatchInfo.m_enableSPU = false;
	//m_dispatchInfo.m_useEpa = false;
	//m_dispatchInfo.m_useConvexConservativeDistanceUtil = false;

	InitSurfaceParams( m_physSurfaceParams );
}

void CEqPhysics::InitGrid()
{
	m_grid.Init(this, PHYSGRID_WORLD_SIZE, Vector3D(-EQPHYS_MAX_WORLDSIZE), Vector3D(EQPHYS_MAX_WORLDSIZE));

	// add all objects to the grid
	for(int i = 0; i < m_dynObjects.numElem(); i++)
	{
		SetupBodyOnCell(m_dynObjects[i]);
	}

	for(int i = 0; i < m_staticObjects.numElem(); i++)
	{
		m_grid.AddStaticObjectToGrid(m_staticObjects[i]);
	}

	for(int i = 0; i < m_ghostObjects.numElem(); i++)
	{
		SetupBodyOnCell(m_ghostObjects[i]);
	}
}

void CEqPhysics::DestroyWorld()
{
	for(int i = 0; i < m_dynObjects.numElem(); i++)
	{
		delete m_dynObjects[i];
	}
	m_dynObjects.clear();
	m_moveable.clear();

	for(int i = 0; i < m_staticObjects.numElem(); i++)
	{
		delete m_staticObjects[i];
	}
	m_staticObjects.clear();

	for(int i = 0; i < m_ghostObjects.numElem(); i++)
	{
		delete m_ghostObjects[i];
	}
	m_ghostObjects.clear();

	// update the controllers
	for (int i = 0; i < m_controllers.numElem(); i++)
	{
		m_controllers[i]->SetEnabled(false);
	}
	m_controllers.clear();
	m_constraints.clear();

	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
	{
		delete m_physSurfaceParams[i];
	}
	m_physSurfaceParams.clear();

	if (m_collisionWorld)
		delete m_collisionWorld;
	m_collisionWorld = NULL;

	if(m_collDispatcher)
		delete m_collDispatcher;
	m_collDispatcher = NULL;

	if(m_collConfig)
		delete m_collConfig;
	m_collConfig = NULL;
}

void CEqPhysics::DestroyGrid()
{
	m_grid.Destroy();
}

eqPhysSurfParam_t* CEqPhysics::FindSurfaceParam(const char* name)
{
	int count = m_physSurfaceParams.numElem();
	for (int i = 0; i < count; i++)
	{
		if (!m_physSurfaceParams[i]->name.CompareCaseIns(name))
			return m_physSurfaceParams[i];
	}

	return NULL;
}

eqPhysSurfParam_t* CEqPhysics::GetSurfaceParamByID(int id)
{
	if (id == -1)
		return NULL;

	return m_physSurfaceParams[id];
}

#ifdef DEBUG
#define CHECK_ALREADY_IN_LIST(list, obj) ASSERTMSG(list.findIndex(obj) == -1, "Object already added")
#else
#define CHECK_ALREADY_IN_LIST(list, obj)
#endif

void CEqPhysics::AddToMoveableList( CEqRigidBody* body )
{
	if(!body)
		return;	
	
	if (body->m_flags & BODY_MOVEABLE)
		return;

	//Threading::CScopedMutex m(m_mutex);

	body->m_flags |= BODY_MOVEABLE;

	CHECK_ALREADY_IN_LIST(m_moveable, body);
	m_moveable.append( body );
}

void CEqPhysics::AddToWorld( CEqRigidBody* body, bool moveable )
{
	if(!body)
		return;

	//Threading::CScopedMutex m(m_mutex);

	CHECK_ALREADY_IN_LIST(m_dynObjects, body);

	body->m_flags |= COLLOBJ_TRANSFORM_DIRTY;

	m_dynObjects.append(body);

	if(moveable)
		AddToMoveableList( body );
	else
		SetupBodyOnCell( body );
}

bool CEqPhysics::RemoveFromWorld( CEqRigidBody* body )
{
	if(!body)
		return false;

	//Threading::CScopedMutex m(m_mutex);

	collgridcell_t* cell = body->GetCell();

	if (cell)
	{
		Threading::CScopedMutex m(m_mutex);
		cell->m_dynamicObjs.fastRemove(body);
	}

	bool result = m_dynObjects.fastRemove(body);

	if (result)
	{
		body->m_flags &= ~BODY_MOVEABLE;
		m_moveable.fastRemove(body);
	}
		

	return result;
}

void CEqPhysics::DestroyBody( CEqRigidBody* body )
{
	if(!body)
		return;

	if(RemoveFromWorld(body))
		delete body;
}

void CEqPhysics::AddGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//Threading::CScopedMutex m(m_mutex);

	// add extra flags to objects
	object->m_flags = COLLOBJ_ISGHOST | COLLOBJ_DISABLE_RESPONSE | COLLOBJ_NO_RAYCAST;

	if (!object->m_callbacks)
		object->m_flags |= COLLOBJ_COLLISIONLIST;

	m_ghostObjects.append(object);

	if(m_grid.IsInit())
	{
		if(object->GetMesh() != NULL)
		{
			m_grid.AddStaticObjectToGrid( object );
		}
		else
			SetupBodyOnCell( object );
	}
}

void CEqPhysics::DestroyGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//Threading::CScopedMutex m(m_mutex);

	if(m_grid.IsInit())
	{
		if(object->GetMesh() != NULL)
		{
			m_grid.RemoveStaticObjectFromGrid(object);
		}
		else
		{
			collgridcell_t* cell = object->GetCell();

			if (cell)
			{
				Threading::CScopedMutex m(m_mutex);
				cell->m_dynamicObjs.fastRemove(object);
			}
		}
	}

	m_ghostObjects.fastRemove(object);
	delete object;
}

void CEqPhysics::AddStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//Threading::CScopedMutex m(m_mutex);

	m_staticObjects.append(object);

	if(m_grid.IsInit())
		m_grid.AddStaticObjectToGrid( object );
}

void CEqPhysics::RemoveStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	if (m_staticObjects.fastRemove(object))
	{
		if (m_grid.IsInit())
			m_grid.RemoveStaticObjectFromGrid(object);
	}
	else
		MsgError("CEqPhysics::RemoveStaticObject - INVALID\n");
}

void CEqPhysics::DestroyStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//Threading::CScopedMutex m(m_mutex);

	if (m_staticObjects.fastRemove(object))
	{
		if (m_grid.IsInit())
			m_grid.RemoveStaticObjectFromGrid(object);

		delete object;
	}
	else
		MsgError("CEqPhysics::DestroyStaticObject - INVALID\n");
}

bool CEqPhysics::IsValidStaticObject( CEqCollisionObject* obj ) const
{
    if(obj->IsDynamic())
        return false;

    return m_staticObjects.findIndex( obj ) != -1;
}

bool CEqPhysics::IsValidBody( CEqCollisionObject* body ) const
{
    if(!body->IsDynamic())
        return false;

    return m_dynObjects.findIndex( (CEqRigidBody*)body ) != -1;
}

void CEqPhysics::AddConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	//Threading::CScopedMutex m(m_mutex);
	m_constraints.append( constraint );
}

void CEqPhysics::RemoveConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	//Threading::CScopedMutex m(m_mutex);
	m_constraints.fastRemove( constraint );
}

void CEqPhysics::AddController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//Threading::CScopedMutex m(m_mutex);
	m_controllers.append( controller );

	controller->AddedToWorld( this );
}

void CEqPhysics::RemoveController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//Threading::CScopedMutex m(m_mutex);
	m_controllers.fastRemove( controller );

	controller->RemovedFromWorld( this );
}

void CEqPhysics::DestroyController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//Threading::CScopedMutex m(m_mutex);

	if(m_controllers.fastRemove(controller))
	{
		controller->RemovedFromWorld( this );
		delete controller;
	}
		
	else
		MsgError("CEqPhysics::DestroyController - INVALID\n");
}

//-----------------------------------------------------------------------------------------------

void CEqPhysics::DetectBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt)
{
#ifdef PROFILE
	PROFILE_FUNC();
#endif

	// apply filters
	if(!bodyA->CheckCanCollideWith(bodyB))
		return;

	// test radius between bodies
	const float lenA = lengthSqr(bodyA->m_aabb.GetSize());
	const float lenB = lengthSqr(bodyB->m_aabb.GetSize());

#pragma todo("SolveBodyCollisions - add speed vector for more 'swept' broadphase detection")

	const FVector3D center = (bodyA->GetPosition()-bodyB->GetPosition());

	const float distBetweenObjects = lengthSqr(center);

	// yep, center is a length also...
	if(distBetweenObjects > lenA+lenB)
		return;

	// check the contact pairs of bodyB (because it has been already processed by the order)
	// if we had any contact pair with bodyA we should discard this collision
	{
		DkList<ContactPair_t>& pairsB = bodyB->m_contactPairs;

		// don't process collisions again
		for (int i = 0; i < pairsB.numElem(); i++)
		{
			if (pairsB[i].bodyA == bodyB && pairsB[i].bodyB == bodyA)
				return;
		}
	}

	// trasform collision objects and test
	//PROFILE_BEGIN(shapeOperations);

	// prepare for testing...
	btCollisionObject* objA = bodyA->m_collObject;
	btCollisionObject* objB = bodyB->m_collObject;

	/*
	btBoxShape boxShapeA(ConvertDKToBulletVectors(bodyA->m_aabb.GetSize()*0.5f));
	boxShapeA.setMargin(ph_margin.GetFloat());

	btBoxShape boxShapeB(ConvertDKToBulletVectors(bodyB->m_aabb.GetSize()*0.5f));
	boxShapeB.setMargin(ph_margin.GetFloat());

	btCollisionObject boxObjectA;
	boxObjectA.setCollisionShape(&boxShapeA);

	btCollisionObject boxObjectB;
	boxObjectB.setCollisionShape(&boxShapeB);

	if(bodyA->m_flags & BODY_BOXVSDYNAMIC)
		objA = &boxObjectA;

	if(bodyB->m_flags & BODY_BOXVSDYNAMIC)
		objB = &boxObjectB;
	*/

	//PROFILE_END();

	//PROFILE_BEGIN(matrixOperations);

	// body a
	Matrix4x4 eqTransA = Matrix4x4( bodyA->GetOrientation() );
	eqTransA.translate(bodyA->GetShapeCenter());
	eqTransA = transpose(eqTransA);
	eqTransA.rows[3] += Vector4D(bodyA->GetPosition()+center, 1.0f);

	// body b
	Matrix4x4 eqTransB = Matrix4x4( bodyB->GetOrientation() );
	eqTransB.translate(bodyB->GetShapeCenter());
	eqTransB = transpose(eqTransB);
	eqTransB.rows[3] += Vector4D(bodyB->GetPosition()+center, 1.0f);

	btTransform transA;
	btTransform transB;

	ConvertMatrix4ToBullet(transA, eqTransA);
	ConvertMatrix4ToBullet(transB, eqTransB);

	//objA->setWorldTransform(transA);
	//objB->setWorldTransform(transB);

	//PROFILE_END();

	btCollisionObjectWrapper obA(nullptr, bodyA->m_shape, objA, transA, -1, -1);
	btCollisionObjectWrapper obB(nullptr, bodyB->m_shape, objB, transB, -1, -1);

	CEqManifoldResult cbResult(&obA, &obB, true, center);

	{
		btCollisionAlgorithm* algorithm = nullptr;

		const int numShapesA = bodyA->m_numShapes;
		const int numShapesB = bodyB->m_numShapes;
		const btCollisionShape* shapeA[] = { bodyA->m_shape };
		const btCollisionShape* shapeB[] = { bodyB->m_shape };
		const btCollisionShape** shapesA = numShapesA > 1 ? bodyA->m_shapeList : shapeA;
		const btCollisionShape** shapesB = numShapesB > 1 ? bodyB->m_shapeList : shapeB;

		// FIXME:
		// Due to btCompoundShape producing unreliable results, there is a really slow checks appear...
		for (int i = 0; i < numShapesB; i++)
		{
			for (int j = 0; j < numShapesA; j++)
			{
				btCollisionObjectWrapper obA(nullptr, shapesA[j], objA, transA, -1, -1);
				btCollisionObjectWrapper obB(nullptr, shapesB[i], objB, transB, -1, -1);

				if(!algorithm)
					algorithm = m_collDispatcher->findAlgorithm(&obA, &obB, 0, BT_CONTACT_POINT_ALGORITHMS);

				algorithm->processCollision(&obA, &obB, m_dispatchInfo, &cbResult);
			}
		}

		algorithm->~btCollisionAlgorithm();
		m_collDispatcher->freeCollisionAlgorithm(algorithm);
	}

	// so collision test were performed, get our results to contact pairs
	const int numCollResults = cbResult.m_collisions.numElem();

	const float iter_delta = 1.0f / numCollResults;

	for(int j = 0; j < numCollResults; j++)
	{
		CollisionData_t& coll = cbResult.m_collisions[j];

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		if(hitDepth < 0 && !(bodyA->m_flags & COLLOBJ_ISGHOST))
			continue;

		ContactPair_t newPair;
		newPair.normal = hitNormal;
		newPair.flags = 0;
		newPair.depth = hitDepth;
		newPair.position = hitPos;
		newPair.bodyA = bodyA;
		newPair.bodyB = bodyB;
		newPair.dt = iter_delta;

		newPair.restitutionA = bodyA->GetRestitution();
		newPair.frictionA = bodyA->GetFriction();
		
		newPair.restitutionB = bodyB->GetRestitution();
		newPair.frictionB = bodyB->GetFriction();

		bodyA->m_contactPairs.append(newPair);

		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), 1.0f, "penetration depth: %f", hitDepth);
		}
	}
}

void CEqPhysics::DetectStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt)
{
#ifdef PROFILE
	PROFILE_FUNC();
#endif

	if(staticObj == NULL || bodyB == NULL)
		return;

	if(!staticObj->CheckCanCollideWith(bodyB))
		return;

	if( !staticObj->m_aabb_transformed.Intersects(bodyB->m_aabb_transformed))
		return;

	Vector3D center = (staticObj->GetPosition()-bodyB->GetPosition());

	// prepare for testing...
	btCollisionObject* objA = staticObj->m_collObject;
	btCollisionObject* objB = bodyB->m_collObject;

	//PROFILE_BEGIN(matrixOperations);

	// body a
	Matrix4x4 eqTransA;
	{
		// body a
		eqTransA = Matrix4x4( staticObj->GetOrientation() );
		eqTransA.translate(staticObj->GetShapeCenter());
		eqTransA = transpose(eqTransA);
		eqTransA.rows[3] += Vector4D(staticObj->GetPosition()+center, 1.0f);
	}

	// body b
	Matrix4x4 eqTransB_orig;
	//Matrix4x4 eqTransB_vel;

	{
		// body a
		eqTransB_orig = Matrix4x4( bodyB->GetOrientation() );
		eqTransB_orig.translate(bodyB->GetShapeCenter());
		eqTransB_orig = transpose(eqTransB_orig);
		eqTransB_orig.rows[3] += Vector4D(bodyB->GetPosition()+center, 1.0f);
	}

	/*
	{
		FVector3D addVelToPos = normalize(bodyB->GetLinearVelocity()) * fDt * 200.5f;

		bodyB->ConstructRenderMatrix(eqTransB_vel, addVelToPos);

		eqTransB_vel.translate(bodyB->GetShapeCenter());

		eqTransB_vel = transpose(eqTransB_vel);
		eqTransB_vel.rows[3] += Vector4D(center, 0.0f);
	}*/

	btTransform transA; 
	btTransform transB;

	ConvertMatrix4ToBullet(transA, eqTransA);
	ConvertMatrix4ToBullet(transB, eqTransB_orig);
	//btTransform transB_vel = ConvertMatrix4ToBullet(eqTransB_vel);

	objA->setWorldTransform(transA);
	objB->setWorldTransform(transB);

	btCollisionObjectWrapper obA(nullptr, staticObj->m_shape, objA, transA, -1, -1);
	btCollisionObjectWrapper obB(nullptr, bodyB->m_shape, objB, transB, -1, -1);
	
	CEqManifoldResult cbResult(&obA, &obB, /*(bodyB->m_flags & BODY_ISCAR)*/true, center);

	{
#ifdef PROFILE
		PROFILE_BLOCK(CollisionAlgo);
#endif
		//discrete collision detection query
#ifdef PROFILE
		PROFILE_BLOCK(ProcessCollision);
#endif

#if 0
		btCollisionAlgorithm* algorithm = nullptr;

		const int numShapesB = bodyB->m_numShapes;
		const btCollisionShape* shapeB[] = { bodyB->m_shape };
		const btCollisionShape** shapesB = numShapesB > 1 ? bodyB->m_shapeList : shapeB;

		for (int i = 0; i < numShapesB; i++)
		{
			btCollisionObjectWrapper obB(nullptr, shapesB[i], objB, transB, -1, -1);

			if(!algorithm)
				algorithm = m_collDispatcher->findAlgorithm(&obA, &obB, 0, BT_CONTACT_POINT_ALGORITHMS);

			algorithm->processCollision(&obA, &obB, m_dispatchInfo, &cbResult);
		}
#else
		btCollisionAlgorithm* algorithm = m_collDispatcher->findAlgorithm(&obA, &obB, 0, BT_CONTACT_POINT_ALGORITHMS);

		algorithm->processCollision(&obA, &obB, m_dispatchInfo, &cbResult);
#endif

		algorithm->~btCollisionAlgorithm();
		m_collDispatcher->freeCollisionAlgorithm(algorithm);
	}

	const int numCollResults = cbResult.m_collisions.numElem();

	const float iter_delta = 1.0f / numCollResults;

	for(int j = 0; j < numCollResults; j++)
	{
		CollisionData_t& coll = cbResult.m_collisions[j];

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		if(hitDepth < 0 && !(staticObj->m_flags & COLLOBJ_ISGHOST))
			continue;

		if(hitDepth > 1.0f)
			hitDepth = 1.0f;

		ContactPair_t newPair;
		newPair.normal = hitNormal;
		newPair.flags = COLLPAIRFLAG_OBJECTA_STATIC;
		newPair.depth = hitDepth;
		newPair.position = hitPos;
		newPair.bodyA = staticObj;
		newPair.bodyB = bodyB;
		newPair.dt = iter_delta;

		eqPhysSurfParam_t* sparam = GetSurfaceParamByID(coll.materialIndex);

		if(sparam)
		{
			newPair.restitutionA = sparam->restitution;
			newPair.frictionA = sparam->friction;
		}
		else
		{
			newPair.restitutionA = 1.0f;
			newPair.frictionA = 1.0f;
		}

		newPair.restitutionA *= staticObj->m_restitution;
		newPair.frictionA *= staticObj->m_friction;

		newPair.restitutionB = bodyB->GetRestitution();
		newPair.frictionB = bodyB->GetFriction();

		bodyB->m_contactPairs.append(newPair);

		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), 1.0f, "penetration depth: %f", hitDepth);
		}
	}
}

void CEqPhysics::SetupBodyOnCell( CEqCollisionObject* body )
{
	// check body is in the world
	if(!m_grid.IsInit())
		return;

	collgridcell_t* oldCell = body->GetCell();

	// get new cell
	collgridcell_t* newCell = m_grid.GetPreallocatedCellAtPos( body->GetPosition() );

	// move object in grid
	if (newCell != oldCell)
	{
		Threading::CScopedMutex m(m_mutex);
		if (oldCell)
			oldCell->m_dynamicObjs.fastRemove(body);

		if (newCell)
			newCell->m_dynamicObjs.append(body);

		body->SetCell(newCell);
	}
}

void CEqPhysics::IntegrateSingle(CEqRigidBody* body)
{
#ifdef PROFILE
	PROFILE_FUNC();
#endif

	collgridcell_t* oldCell = body->GetCell();

	// move object
	body->Integrate( m_fDt );

	bool bodyFrozen = body->IsFrozen();
	bool forceSetCell = !oldCell && bodyFrozen;

	if(!bodyFrozen && body->IsCanIntegrate(true) || forceSetCell)
	{
		// get new cell
		collgridcell_t* newCell = m_grid.GetCellAtPos( body->GetPosition() );

		// move object in grid if it's a really new cell
		if (newCell != oldCell)
		{
			Threading::CScopedMutex m(m_mutex);
			if (oldCell)
				oldCell->m_dynamicObjs.fastRemove(body);

			if (newCell)
				newCell->m_dynamicObjs.append(body);

			body->SetCell(newCell);
		}
	}
}

void CEqPhysics::DetectCollisionsSingle(CEqRigidBody* body)
{
#ifdef PROFILE
	PROFILE_FUNC();
#endif

	// don't refresh frozen object, other will wake up us (or user)
	if (body->IsFrozen())
		return;

	// skip collision detection on iteration
	if (!body->IsCanIntegrate())
		return;

	bool disabledCollisionChecks = (body->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK);

	const BoundingBox& aabb = body->m_aabb_transformed;

	// get the grid box range for searching collision objects
	IVector2D crMin, crMax;
	m_grid.FindBoxRange(aabb, crMin, crMax, ph_grid_tolerance.GetFloat() );

	// in this range do all collision checks
	// might be slow
	for(int y = crMin.y; y < crMax.y+1; y++)
	{
		for(int x = crMin.x; x < crMax.x+1; x++)
		{
			collgridcell_t* ncell = m_grid.GetCellAt( x, y );

			if(!ncell)
				continue;

			const DkList<CEqCollisionObject*>& gridObjects = ncell->m_gridObjects;
			const DkList<CEqCollisionObject*>& dynamicObjects = ncell->m_dynamicObjs;
			
			// iterate over static objects in cell
			for (int i = 0; i < gridObjects.numElem(); i++)
				DetectStaticVsBodyCollision(gridObjects[i], body, body->GetLastFrameTime());

			// if object is only affected by other dynamic objects, don't waste my cycles!
			if (disabledCollisionChecks)
				continue;

			// iterate over dynamic objects in cell
			for (int i = 0; i < dynamicObjects.numElem(); i++)
			{
				CEqCollisionObject* collObj = dynamicObjects[i];

				if (collObj == body)
					continue;

				if (collObj->IsDynamic())
					DetectBodyCollisions(body, (CEqRigidBody*)collObj, body->GetLastFrameTime());
				else // purpose for triggers
					DetectStaticVsBodyCollision(collObj, body, body->GetLastFrameTime());
			}
		}
	}
}


ConVar ph_carVsCarErp("ph_carVsCarErp", "0.15", "Car versus car erp", CV_CHEAT);

void CEqPhysics::ProcessContactPair(ContactPair_t& pair)
{
#ifdef PROFILE
	PROFILE_FUNC();
#endif

	CollisionPairData_t collData;

	CEqRigidBody* bodyB = (CEqRigidBody*)pair.bodyB;
	int bodyAFlags = pair.bodyA->m_flags;
	int bodyBFlags = bodyB->m_flags;

	float appliedImpulse = 0.0f;
	float impactVelocity = 0.0f;

	bool bodyADisableResponse = false;


	IEqPhysCallback* callbacksA = pair.bodyA->m_callbacks;
	IEqPhysCallback* callbacksB = pair.bodyB->m_callbacks;

	//-----------------------------------------------
	// OBJECT A
	if (callbacksA)
		callbacksA->OnPreCollide(pair);

	//-----------------------------------------------
	// OBJECT B
	if (callbacksB)
		callbacksB->OnPreCollide(pair);

	if (pair.flags & COLLPAIRFLAG_OBJECTA_STATIC)
	{
		CEqCollisionObject* bodyA = pair.bodyA;

		// correct position
		if (!(pair.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE) && !(bodyAFlags & COLLOBJ_DISABLE_RESPONSE) && pair.depth > 0)
		{
			impactVelocity = fabs(dot(pair.normal, bodyB->GetVelocityAtWorldPoint(pair.position)));

			if (impactVelocity < 0)
				impactVelocity = 0.0f;

			// apply response
			pair.normal *= -1.0f;
			pair.depth *= -1.0f;

			float combinedErp = ph_erp.GetFloat() + pair.bodyA->m_erp + pair.bodyB->m_erp;
			float positionalError = pair.depth * pair.dt;

			combinedErp = max(combinedErp, ph_erp.GetFloat());

			bodyB->m_position += pair.normal * positionalError * combinedErp;
			bodyB->m_prevPosition += pair.normal * positionalError * combinedErp;
			
			appliedImpulse = CEqRigidBody::ApplyImpulseResponseTo(pair, positionalError * combinedErp * 2.0f);
			//appliedImpulse =  CEqRigidBody::ApplyImpulseResponseTo(bodyB, pair.position, pair.normal, 0.0, pair.restitutionA, pair.frictionA);
		}

		bodyADisableResponse = (bodyAFlags & COLLOBJ_DISABLE_RESPONSE);
	}
	else
	{
		CEqRigidBody* bodyA = (CEqRigidBody*)pair.bodyA;

		bool isCarCollidingWithCar = (bodyAFlags & BODY_ISCAR) && (bodyBFlags & BODY_ISCAR);
		float varyErp = (isCarCollidingWithCar ? ph_carVsCarErp.GetFloat() : ph_erp.GetFloat());
		
		int bodyAFlags = pair.bodyA->m_flags;
		int bodyBFlags = bodyB->m_flags;

		float combinedErp = varyErp + pair.bodyA->m_erp + pair.bodyB->m_erp;
		float positionalError = pair.depth * pair.dt;

		combinedErp = max(combinedErp, varyErp);

		impactVelocity = fabs( dot(pair.normal, bodyA->GetVelocityAtWorldPoint(pair.position) - bodyB->GetVelocityAtWorldPoint(pair.position)) );

		// correct position
		if (pair.depth > 0 &&
			!(pair.flags & COLLPAIRFLAG_OBJECTA_NO_RESPONSE) && 
			!(bodyAFlags & BODY_FORCE_FREEZE) && 
			!(bodyAFlags & BODY_INFINITEMASS) && 
			!(bodyBFlags & COLLOBJ_DISABLE_RESPONSE))
		{
			bodyA->m_position += pair.normal * positionalError * combinedErp;
			bodyA->m_prevPosition += pair.normal * positionalError * combinedErp;
		}

		if (pair.depth > 0 &&
			!(pair.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE) && 
			!(bodyBFlags & BODY_FORCE_FREEZE) && 
			!(bodyBFlags & BODY_INFINITEMASS) && 
			!(bodyAFlags & COLLOBJ_DISABLE_RESPONSE))
		{
			bodyB->m_position -= pair.normal * positionalError * combinedErp;
			bodyB->m_prevPosition -= pair.normal * positionalError * combinedErp;
		}

		// apply response
		//appliedImpulse = 2.0f * CEqRigidBody::ApplyImpulseResponseTo2(bodyA, bodyB, pair.position, pair.normal, 0.0, pair.flags);
		appliedImpulse = 2.0f * CEqRigidBody::ApplyImpulseResponseTo(pair, positionalError * combinedErp * 2.0f);
		bodyADisableResponse = (bodyAFlags & COLLOBJ_DISABLE_RESPONSE);
	}

	CollisionPairData_t tempPairData;

	//-----------------------------------------------
	// OBJECT A
	{
		DkList<CollisionPairData_t>& pairs = pair.bodyA->m_collisionList;

		collData.bodyA = pair.bodyA;
		collData.bodyB = pair.bodyB;
		collData.fract = pair.depth;
		collData.normal = pair.normal;
		collData.position = pair.position;
		collData.appliedImpulse = appliedImpulse; // because subtracted
		collData.impactVelocity = impactVelocity;
		collData.flags = 0;

		if (bodyAFlags & COLLOBJ_DISABLE_RESPONSE || (pair.flags & COLLPAIRFLAG_OBJECTA_NO_RESPONSE))
			collData.flags |= COLLPAIRFLAG_OBJECTA_NO_RESPONSE;

		if (bodyBFlags & COLLOBJ_DISABLE_RESPONSE || (pair.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE))
			collData.flags |= COLLPAIRFLAG_OBJECTB_NO_RESPONSE;

		if (callbacksA)
			callbacksA->OnCollide(collData);

		if((bodyAFlags & COLLOBJ_COLLISIONLIST) && pairs.numElem() < PHYSICS_COLLISION_LIST_MAX)
			pairs.append(collData);
	}

	//-----------------------------------------------
	// OBJECT B
	{
		DkList<CollisionPairData_t>& pairs = pair.bodyB->m_collisionList;

		collData.bodyA = pair.bodyB;
		collData.bodyB = pair.bodyA;
		collData.fract = pair.depth;
		collData.normal = pair.normal;
		collData.position = pair.position;
		collData.appliedImpulse = appliedImpulse; // because subtracted
		collData.impactVelocity = impactVelocity;
		collData.flags = 0;

		if ((bodyBFlags & BODY_ISCAR) && !(pair.flags & COLLPAIRFLAG_OBJECTA_STATIC))
			collData.flags = COLLPAIRFLAG_NO_SOUND;

		if (bodyADisableResponse || (pair.flags & COLLPAIRFLAG_OBJECTA_NO_RESPONSE))
			collData.flags |= COLLPAIRFLAG_OBJECTB_NO_RESPONSE;

		if (bodyBFlags & COLLOBJ_DISABLE_RESPONSE || (pair.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE))
			collData.flags |= COLLPAIRFLAG_OBJECTA_NO_RESPONSE;

		if (callbacksB)
			callbacksB->OnCollide(collData);

		if((bodyBFlags & COLLOBJ_COLLISIONLIST) && pairs.numElem() < PHYSICS_COLLISION_LIST_MAX)
			pairs.append(collData);
	}
}

//----------------------------------------------------------------------------------------------------
//
// Physics simulation step
//
//----------------------------------------------------------------------------------------------------

void CEqPhysics::SimulateStep(float deltaTime, int iteration, FNSIMULATECALLBACK preIntegrFunc)
{
	// don't let the physics simulate something is not init
	if(!m_grid.IsInit())
		return;

	// save delta
	m_fDt = deltaTime;

#ifdef PROFILE
	PROFILE_FUNC();
#endif
	
	// prepare all the constraints
	{
		for (int i = 0; i < m_constraints.numElem(); i++)
		{
			IEqPhysicsConstraint* constr = m_constraints[i];

			if(constr->IsEnabled())
				constr->PreApply( m_fDt );
		}
	}

	// update the controllers
	for (int i = 0; i < m_controllers.numElem(); i++)
	{
		IEqPhysicsController* contr = m_controllers[i];

		if(contr->IsEnabled())
			contr->Update( m_fDt );
	}
	
	// move all bodies
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		// execute pre-simulation callbacks
		IEqPhysCallback* callbacks = body->m_callbacks;

		if (callbacks) 	// execute pre-simulation callbacks
			callbacks->PreSimulate(m_fDt);

		// clear contact pairs and results
		body->ClearContacts();

		// apply velocities
		IntegrateSingle(body);
	}

	m_fDt = deltaTime;

	if(preIntegrFunc)
		preIntegrFunc(m_fDt, iteration);

	// calculate collisions
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		DetectCollisionsSingle(m_moveable[i]);
	}

	// TODO: job barrier

	// solve positions
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];
		body->Update(m_fDt);
	}
	
	// process generated contact pairs
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		DkList<ContactPair_t>& pairs = body->m_contactPairs;
		int numContactPairs = pairs.numElem();

		for (int j = 0; j < numContactPairs; j++)
			ProcessContactPair(pairs[j]);

		IEqPhysCallback* callbacks = body->m_callbacks;

		if (callbacks) // execute post simulation callbacks
			callbacks->PostSimulate(m_fDt);
	}

	// update all constraints
	for (int i = 0; i < m_constraints.numElem(); i++)
	{
		IEqPhysicsConstraint* constr = m_constraints[i];

		if (constr->IsEnabled())
			constr->Apply( m_fDt );
	}

	m_numRayQueries = 0;
}

void CEqPhysics::PerformCollisionDetectionJob(void* thisPhys, int i)
{
	CEqPhysics* thisPhysics = (CEqPhysics*)thisPhys;
	thisPhysics->DetectCollisionsSingle(thisPhysics->m_moveable[i]);
}

//----------------------------------------------------------------------------------------------------
//
// VOXEL TRACING FOR RAYS
//
//----------------------------------------------------------------------------------------------------
template <typename F>
void CEqPhysics::InternalTestLineCollisionCells(int y1, int x1, int y2, int x2,
	const FVector3D& start,
	const FVector3D& end,
	const BoundingBox& rayBox,
	CollisionData_t& coll,
	int rayMask,
	eqPhysCollisionFilter* filterParams,
	F func,
	void* args)
{
	//if( TestLineCollisionOnCell(y1, x1, start, end, rayBox, coll, rayMask, filterParams, func, args) )
	//	return;

    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;)
	{
		if( TestLineCollisionOnCell(y1, x1, start, end, rayBox, coll, rayMask, filterParams, func, args) )
			return; // if we just found nearest collision, stop.

        if (x1 == x2 && y1 == y2)
			break;

        e2 = 2 * err;

        // EITHER horizontal OR vertical step (but not both!)
        if (e2 > dy)
		{
            err += dy;
            x1 += sx;
        }
		else if (e2 < dx)
		{
            err += dx;
            y1 += sy;
        }
    }
}

//-----------------------------------------------------------------------------------------------------------------------------

template <typename F>
bool CEqPhysics::TestLineCollisionOnCell(int y, int x,
	const FVector3D& start,
	const FVector3D& end,
	const BoundingBox& rayBox,
	CollisionData_t& coll,
	int rayMask, eqPhysCollisionFilter* filterParams,
	F func,
	void* args)
{
	collgridcell_t* cell = m_grid.GetCellAt(x,y);

	if (!cell)
		return false;

	bool hasClosestCollision = false;
	float fMaxDist = coll.fract;

	int objectTypeTesting = 0x3;

	if(filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_DISALLOW_STATIC))
		objectTypeTesting &= ~0x1;

	if(filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC))
		objectTypeTesting &= ~0x2;

	bool staticInBoundTest = (rayBox.minPoint.y <= cell->cellBoundUsed);

	if(staticInBoundTest && m_debugRaycast)
	{
		Vector3D cellMin, cellMax;

		if(m_grid.GetCellBounds(x, y, cellMin, cellMax))
			debugoverlay->Box3D(cellMin, cellMax, ColorRGBA(1,0,0,0.25f));
	}

	const DkList<CEqCollisionObject*>& gridObjects = cell->m_gridObjects;
	const DkList<CEqCollisionObject*>& dynamicObjects = cell->m_dynamicObjs;
	
	// static objects are not checked if line is not in Y bound
	if(staticInBoundTest && (objectTypeTesting & 0x1))
	{
		Threading::CScopedMutex m(m_mutex);
		for (int i = 0; i < gridObjects.numElem(); i++)
		{
			CollisionData_t tempColl;

			if ((this->*func)(gridObjects[i], start, end, rayBox, tempColl, rayMask, filterParams, args) &&
				(tempColl.fract < fMaxDist))
			{
				fMaxDist = tempColl.fract;
				coll = tempColl;
				hasClosestCollision = true;
			}
		}
	}

	if(objectTypeTesting & 0x2)
	{
		Threading::CScopedMutex m(m_mutex);
		// test dynamic objects within cell
		for (int i = 0; i < dynamicObjects.numElem(); i++)
		{
			CollisionData_t tempColl;

			if ((this->*func)(dynamicObjects[i], start, end, rayBox, tempColl, rayMask, filterParams, args) &&
				(tempColl.fract < fMaxDist))
			{
				fMaxDist = tempColl.fract;
				coll = tempColl;
				hasClosestCollision = true;
			}
		}
	}

	return hasClosestCollision;
}

//-----------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//
//	TestConvexSweepCollision
//		- Casts line in the physics world
//
//----------------------------------------------------------------------------------------------------
bool CEqPhysics::TestLineCollision(	const FVector3D& start,
									const FVector3D& end,
									CollisionData_t& coll,
									int rayMask, eqPhysCollisionFilter* filterParams)
{
#ifdef PROFILE
	//PROFILE_FUNC();
#endif
	IVector2D startCell, endCell;

	//Threading::CScopedMutex m(m_mutex);

	m_grid.GetPointAt(start, startCell.x, startCell.y);
	m_grid.GetPointAt(end, endCell.x, endCell.y);

	coll.position = end;
	coll.fract = 32768.0f;

	BoundingBox rayBox;
	rayBox.AddVertex(start);
	rayBox.AddVertex(end);

	InternalTestLineCollisionCells(	startCell.y, startCell.x,
									endCell.y, endCell.x,
									start, end,
									rayBox,
									coll,
									rayMask,
									filterParams,
									static_cast<fnSingleObjectLineCollisionCheck>(&CEqPhysics::TestLineSingleObject), NULL);

	if (coll.fract > 1.0f)
		coll.fract = 1.0f;

	return (coll.fract < 1.0f);
}

//----------------------------------------------------------------------------------------------------
//
//	TestConvexSweepCollision
//		- Casts convex shape in the physics world
//
//----------------------------------------------------------------------------------------------------
bool CEqPhysics::TestConvexSweepCollision(	btCollisionShape* shape,
											const Quaternion& rotation,
											const FVector3D& start,
											const FVector3D& end,
											CollisionData_t& coll,
											int rayMask, eqPhysCollisionFilter* filterParams)
{
#ifdef PROFILE
	//PROFILE_FUNC();
#endif
	//Threading::CScopedMutex m(m_mutex);

	coll.position = end;
	coll.fract = 32768.0f;

	sweptTestParams_t params;
	params.rotation = rotation;
	params.shape = shape;

	btTransform startTrans;
	ConvertMatrix4ToBullet(startTrans, rotation);

	btVector3 shapeMins, shapeMaxs;
	shape->getAabb(startTrans, shapeMins, shapeMaxs);

	BoundingBox rayBox;
	BoundingBox shapeBox;
	ConvertBulletToDKVectors(shapeBox.minPoint, shapeMins);
	ConvertBulletToDKVectors(shapeBox.maxPoint, shapeMaxs);

	Vector3D sBoxSize = shapeBox.GetSize();

	rayBox.AddVertex(start);
	rayBox.AddVertex(end);

	rayBox.AddVertex(rayBox.minPoint - sBoxSize);
	rayBox.AddVertex(rayBox.maxPoint + sBoxSize);

	IVector2D startCell, endCell;

	Vector3D lineDir = fastNormalize(Vector3D(end-start));

	//m_grid.GetPointAt(start - sBoxSize*lineDir, startCell.x, startCell.y);
	//m_grid.GetPointAt(end + sBoxSize*lineDir, endCell.x, endCell.y);

	m_grid.GetPointAt(start, startCell.x, startCell.y);
	m_grid.GetPointAt(end , endCell.x, endCell.y);

	InternalTestLineCollisionCells(	startCell.y, startCell.x,
									endCell.y, endCell.x,
									start, end,
									rayBox,
									coll,
									rayMask,
									filterParams,
									static_cast<fnSingleObjectLineCollisionCheck>(&CEqPhysics::TestConvexSweepSingleObject), &params);

	if (coll.fract > 1.0f)
		coll.fract = 1.0f;

	return (coll.fract < 1.0f);
}

//-------------------------------------------------------------------------------------------------

class CEqRayTestCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	CEqRayTestCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld) : ClosestRayResultCallback(rayFromWorld, rayToWorld)
	{
		m_surfMaterialId = 0;
		m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		// do default result
		btScalar res = ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

		// if something is a NaN we have to deny it
		if (rayResult.m_hitNormalLocal != rayResult.m_hitNormalLocal || rayResult.m_hitFraction != rayResult.m_hitFraction)
			return 1.0f;

		// check our object is triangle mesh
		CEqCollisionObject* obj = (CEqCollisionObject*)m_collisionObject->getUserPointer();

		if(obj)
		{
			if(obj->GetMesh() && rayResult.m_localShapeInfo)
			{
				CEqBulletIndexedMesh* mesh = obj->GetMesh();
				m_surfMaterialId = mesh->getSubPartMaterialId( rayResult.m_localShapeInfo->m_shapePart );
			}

			if(m_surfMaterialId == -1)
				m_surfMaterialId = obj->m_surfParam;
		}

		return res;
	}

	int m_surfMaterialId;
};

bool CEqPhysics::CheckAllowContactTest(eqPhysCollisionFilter* filterParams, CEqCollisionObject* object)
{
	if (!filterParams)
		return true;

	bool checkStatic = (filterParams->flags & EQPHYS_FILTER_FLAG_STATICOBJECTS) && !object->IsDynamic();
	bool checkDynamic = (filterParams->flags & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS) && object->IsDynamic();

	if (checkStatic || checkDynamic)
	{
		bool checkUserData = (filterParams->flags & EQPHYS_FILTER_FLAG_CHECKUSERDATA) > 0;

		if(filterParams->type == EQPHYS_FILTER_TYPE_INCLUDE_ONLY)
		{
			// include this object
			if( !filterParams->HasObject(checkUserData ? object->GetUserData() : object) )
				return false;
		}
		else //if(filterParams->type == EQPHYS_FILTER_TYPE_EXCLUDE)
		{
			// exclude this object
			if( filterParams->HasObject(checkUserData ? object->GetUserData() : object) )
				return false;
		}
	}

	return true;
}



bool CEqPhysics::TestLineSingleObject(
	CEqCollisionObject* object,
	const FVector3D& start,
	const FVector3D& end,
	const BoundingBox& rayBox,
	CollisionData_t& coll,
	int rayMask,
	eqPhysCollisionFilter* filterParams,
	void* args)
{
	if(!object)
		return false;

	bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));

	if (!forceRaycast && (object->m_flags & COLLOBJ_NO_RAYCAST))
		return false;

	if(!(rayMask & object->GetContents()))
		return false;

	if (!object->m_collObject)
		return false;

	if (!CheckAllowContactTest(filterParams, object))
		return false;

	if(!object->m_aabb_transformed.Intersects(rayBox))
		return false;

	btMatrix3x3 btident3;
	btident3.setIdentity();

	Matrix4x4 obj_mat; // don't use fixed point, it's damn slow
	object->ConstructRenderMatrix(obj_mat);

	btTransform transIdent;
	ConvertMatrix4ToBullet(transIdent, transpose(obj_mat));

	btVector3 strt;
	btVector3 endt;
	ConvertPositionToBullet(strt, start);
	ConvertPositionToBullet(endt, end);

	btTransform startTrans(btident3, strt);
	btTransform endTrans(btident3, endt);

#if BT_BULLET_VERSION >= 283 // new bullet
	btCollisionObjectWrapper objWrap(NULL, object->m_shape, object->m_collObject, transIdent, 0, 0);
#else
    btCollisionObjectWrapper objWrap(NULL, object->m_shape, object->m_collObject, transIdent);
#endif

	CEqRayTestCallback rayCallback(strt, endt);
	m_collisionWorld->rayTestSingleInternal( startTrans, endTrans, &objWrap, rayCallback);

	m_numRayQueries++;

	// put our result
	if(rayCallback.hasHit())
	{
		Vector3D pos;

		coll.fract = rayCallback.m_closestHitFraction;
		ConvertBulletToDKVectors(pos, rayCallback.m_hitPointWorld);
		coll.position = pos;
		ConvertBulletToDKVectors(coll.normal, rayCallback.m_hitNormalWorld);
		coll.materialIndex = rayCallback.m_surfMaterialId;
		coll.hitobject = object;

		return true;
	}

	return false;
}


//-------------------------------------------------------------------------------------------------------------------------------


class CEqConvexTestCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	CEqConvexTestCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld) : ClosestConvexResultCallback(rayFromWorld, rayToWorld)
	{
		m_closestHitFraction = PHYSICS_WORLD_MAX_UNITS;
		m_surfMaterialId = 0;
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& rayResult,bool normalInWorldSpace)
	{
		// do default result
		btScalar res = ClosestConvexResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

		// if something is a NaN we have to deny it
		if (rayResult.m_hitNormalLocal != rayResult.m_hitNormalLocal || rayResult.m_hitFraction != rayResult.m_hitFraction)
			return 1.0f;

		// check our object is triangle mesh
		CEqCollisionObject* obj = (CEqCollisionObject*)m_hitCollisionObject->getUserPointer();

		if(obj)
		{
			if(obj->GetMesh() && rayResult.m_localShapeInfo)
			{
				CEqBulletIndexedMesh* mesh = obj->GetMesh();
				m_surfMaterialId = mesh->getSubPartMaterialId( rayResult.m_localShapeInfo->m_shapePart );
			}

			if(m_surfMaterialId == -1)
				m_surfMaterialId = obj->m_surfParam;
		}

		return res;
	}

	int m_surfMaterialId;
};

//-------------------------------------------------------------------------------------------------

bool CEqPhysics::TestConvexSweepSingleObject(	CEqCollisionObject* object,
												const FVector3D& start,
												const FVector3D& end,
												const BoundingBox& raybox,
												CollisionData_t& coll,
												int rayMask,
												eqPhysCollisionFilter* filterParams,
												void* args)
{
	bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));

	if (!forceRaycast && (object->m_flags & COLLOBJ_NO_RAYCAST))
		return false;

	if(!(rayMask & object->GetContents()))
		return false;

	if (!object->m_collObject)
		return false;

	if (!CheckAllowContactTest(filterParams, object))
		return false;

	if(!object->m_aabb_transformed.Intersects(raybox))
		return false;

	btMatrix3x3 btident3;
	btident3.setIdentity();

	sweptTestParams_t* params = (sweptTestParams_t*)args;

	Matrix4x4 obj_mat; // don't use fixed point, it's damn slow
	object->ConstructRenderMatrix(obj_mat);

	obj_mat = transpose(obj_mat);

	// NEW
	btTransform transIdent;
	ConvertMatrix4ToBullet(transIdent, obj_mat);

	btVector3 strt;
	btVector3 endt;

	ConvertPositionToBullet(strt, start);
	ConvertPositionToBullet(endt, end);

	Matrix4x4 shapeTransform(params->rotation);

	// define start and end trasformations
	btTransform startTrans;
	ConvertMatrix4ToBullet(startTrans, shapeTransform);
	startTrans.setOrigin(strt);

	btTransform endTrans(startTrans.getBasis(), endt);

	CEqConvexTestCallback convexCallback(strt,endt);

	int shapeType = params->shape->getShapeType();

	if(	shapeType > CONCAVE_SHAPES_START_HERE)
	{
		ASSERTMSG(false, "Only convex shapes are supported as concave shapes!");
		return false;
	}

	// only trace first shape
	btConvexShape* testShape = (btConvexShape*)params->shape;

	// THIS MAY CRASH HERE IF YOU CAST WRONG SHAPE
	m_collisionWorld->objectQuerySingle(testShape, startTrans, endTrans,
				object->m_collObject,
				object->m_shape,
				transIdent,
				convexCallback,
				0.01f);

	// put our result
	if(convexCallback.hasHit())
	{
		Vector3D pos;
		coll.fract = convexCallback.m_closestHitFraction;

		ConvertBulletToDKVectors(pos, convexCallback.m_hitPointWorld);
		coll.position = pos;

		ConvertBulletToDKVectors(coll.normal, convexCallback.m_hitNormalWorld);

		coll.materialIndex = convexCallback.m_surfMaterialId;
		coll.hitobject = object;

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------


void CEqPhysics::DebugDrawBodies(int mode)
{
	if (mode >= 1 && mode != 4 && mode != 5)
	{
		for (int i = 0; i < m_dynObjects.numElem(); i++)
		{
			CEqRigidBody* body = m_dynObjects[i];

			ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 0.8f);

			if (body->IsFrozen())
				bodyCol = ColorRGBA(0.2, 1, 0.1f, 0.8f);

			debugoverlay->OrientedBox3D(
				body->m_aabb.minPoint, body->m_aabb.maxPoint, body->GetPosition(), body->GetOrientation(), bodyCol);
		}

		if (mode >= 2)
			m_grid.DebugRender();

		if (mode >= 3)
		{
			for (int i = 0; i < m_staticObjects.numElem(); i++)
			{
				CEqCollisionObject* obj = m_staticObjects[i];

				debugoverlay->OrientedBox3D(
					obj->m_aabb.minPoint, obj->m_aabb.maxPoint, obj->GetPosition(), obj->GetOrientation(), ColorRGBA(1, 1, 0.2, 0.8f));
			}
		}
	}
	else if (mode == 5)	// only grid
	{
		m_grid.DebugRender();
	}
	else
	{
		for (int i = 0; i < m_dynObjects.numElem(); i++)
		{
			CEqRigidBody* body = m_dynObjects[i];

			ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 1.0f);

			if (body->IsFrozen())
				bodyCol = ColorRGBA(0.2, 1, 0.1f, 1.0f);

			debugoverlay->Box3D(body->m_aabb_transformed.minPoint, body->m_aabb_transformed.maxPoint, bodyCol, 0.0f);
		}
	}
}