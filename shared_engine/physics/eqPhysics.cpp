//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>
#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#include <BulletCollision/NarrowPhaseCollision/btRaycastCallback.h>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IEqParallelJobs.h"
#include "utils/KeyValues.h"

#include "render/IDebugOverlay.h"

#include "eqPhysics.h"
#include "eqCollision_Callback.h"
#include "eqCollision_ObjectGrid.h"
#include "eqPhysics_Body.h"
#include "eqPhysics_Contstraint.h"
#include "eqPhysics_Controller.h"
#include "eqBulletIndexedMesh.h"
#include "BulletConvert.h"

#define ENABLE_CONTACT_GROUPING

using namespace EqBulletUtils;
using namespace Threading;
static CEqMutex s_eqPhysMutex;

static constexpr const int PHYSGRID_WORLD_SIZE			= 24;	// compromised betwen memory usage and performance
static constexpr const float PHYSICS_WORLD_MAX_UNITS	= 65535.0f;
static constexpr const float PHYSGRID_BOX_TOLERANCE		= 0.1f;

DECLARE_CVAR_F(ph_margin);

DECLARE_CVAR(ph_showcontacts, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(ph_erp, "0.15", "Collision correction", CV_CHEAT);
DECLARE_CVAR(ph_carVsCarErp, "0.15", "Car versus car erp", CV_CHEAT);

CEqCollisionObject* eqContactPair::GetOppositeTo(CEqCollisionObject* obj) const
{
	return (obj == bodyA) ? bodyB : bodyA;
}

CEqCollisionObject* eqCollisionPairData::GetOppositeTo(CEqCollisionObject* obj) const
{
	return (obj == bodyA) ? bodyB : bodyA;
}

//------------------------------------------------------------------------------------------------------------

static void* eqBtAlloc(size_t size)
{
	return PPDAlloc(size, PPSourceLine::Make("BulletPhysics", 0));
}

static void eqBtFree(void* ptr)
{
	PPFree(ptr);
}

static inline int btInternalGetHash(int partId, int triangleIndex)
{
	return (partId << (31 - MAX_NUM_PARTS_IN_BITS)) | triangleIndex;
}

/// Adjusts collision for using single side, ignoring internal triangle edges
/// If this info map is missing, or the triangle is not store in this map, nothing will be done
static void AdjustSingleSidedContact(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0)
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
		: btManifoldResult(obj0Wrap, obj1Wrap)
		, m_center(center)
		, m_singleSided(singleSided)
	{
		m_closestPointDistanceThreshold = 0.0f;
	}

	virtual void addContactPoint(const btVector3& normalOnBInWorld, const btVector3& pointInWorld, btScalar depth)
	{
		const bool isSwapped = m_manifoldPtr->getBody0() != m_body0Wrap->getCollisionObject();

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

		// if something is a NaN we have to deny it
		if (cp.m_positionWorldOnA != cp.m_positionWorldOnA || 
			cp.m_normalWorldOnB != cp.m_normalWorldOnB)
			return;

		Vector3D position;
		ConvertBulletToDKVectors(position, cp.m_positionWorldOnA);
		position -= m_center;
		
		Vector3D normal;
		ConvertBulletToDKVectors(normal, cp.m_normalWorldOnB);

		const btCollisionShape* shape1 = colObj1Wrap->getCollisionShape();

		int materialIndex = -1;
		if (shape1->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE)
		{
			CEqCollisionObject* obj = reinterpret_cast<CEqCollisionObject*>(colObj1Wrap->getCollisionObject()->getUserPointer());
			if (obj && obj->GetMesh())
			{
				CEqBulletIndexedMesh* mesh = obj->GetMesh();
				materialIndex = mesh->GetSubpartMaterialIdx(cp.m_partId1);
			}
		}

		const float distance = cp.getDistance();

#ifdef ENABLE_CONTACT_GROUPING		
		for(eqCollisionInfo& coll : m_collisions)
		{
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
#endif // ENABLE_CONTACT_GROUPING

		if (m_collisions.numElem() >= m_collisions.numAllocated())
			return;
		
		eqCollisionInfo& data = m_collisions.append();
		ConvertBulletToDKVectors(data.normal, cp.m_normalWorldOnB);
		ConvertBulletToDKVectors(position, cp.m_positionWorldOnA);
		data.position = position - m_center;
		data.materialIndex = materialIndex;
		data.fract = distance;
		data.pad = 1;
	}

	FixedArray<eqCollisionInfo, 64>	m_collisions;
	Vector3D						m_center;
	bool							m_singleSided;
};

//------------------------------------------------------------------------------------------------------------

CEqPhysics::CEqPhysics()
{
}

CEqPhysics::~CEqPhysics()
{

}

void CEqPhysics::InitWorld()
{
	btAlignedAllocSetCustom(eqBtAlloc, eqBtFree);

	// collision configuration contains default setup for memory, collision setup
	m_collConfig = PPNew btDefaultCollisionConfiguration();

	// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_collDispatcher = PPNew btCollisionDispatcher( m_collConfig );

	// still required for raycasts
	m_collisionWorld = PPNew btCollisionWorld(m_collDispatcher, nullptr, m_collConfig);

	m_dispatchInfo = PPNew btDispatcherInfo();
	m_dispatchInfo->m_enableSatConvex = true;
	m_dispatchInfo->m_useContinuous = true;
	m_dispatchInfo->m_stepCount = 1;
	//m_dispatchInfo->m_enableSPU = false;
	//m_dispatchInfo->m_useEpa = false;
	//m_dispatchInfo->m_useConvexConservativeDistanceUtil = false;
}

void CEqPhysics::InitGrid()
{
	m_grid = PPNew CEqCollisionBroadphaseGrid(this, PHYSGRID_WORLD_SIZE, Vector3D(-EQPHYS_MAX_WORLDSIZE), Vector3D(EQPHYS_MAX_WORLDSIZE));

	// add all objects to the grid
	for(int i = 0; i < m_dynObjects.numElem(); i++)
		SetupBodyOnCell(m_dynObjects[i]);

	for(int i = 0; i < m_staticObjects.numElem(); i++)
		m_grid->AddStaticObjectToGrid(m_staticObjects[i]);

	for(int i = 0; i < m_ghostObjects.numElem(); i++)
		SetupBodyOnCell(m_ghostObjects[i]);
}

void CEqPhysics::DestroyWorld()
{
	for(int i = 0; i < m_dynObjects.numElem(); i++)
		delete m_dynObjects[i];

	m_dynObjects.clear(true);
	m_moveable.clear(true);

	for(int i = 0; i < m_staticObjects.numElem(); i++)
		delete m_staticObjects[i];
	m_staticObjects.clear(true);

	for(int i = 0; i < m_ghostObjects.numElem(); i++)
		delete m_ghostObjects[i];

	m_ghostObjects.clear(true);

	// update the controllers
	for (int i = 0; i < m_controllers.numElem(); i++)
		m_controllers[i]->SetEnabled(false);

	m_controllers.clear(true);
	m_constraints.clear(true);

	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
		delete m_physSurfaceParams[i];

	m_physSurfaceParams.clear(true);

	SAFE_DELETE(m_collisionWorld);
	SAFE_DELETE(m_collDispatcher);
	SAFE_DELETE(m_collConfig);
	SAFE_DELETE(m_dispatchInfo);
}

void CEqPhysics::DestroyGrid()
{
	SAFE_DELETE(m_grid);
}

void CEqPhysics::AddSurfaceParamFromKV(const char* name, const KVSection* kvSection)
{
	const int foundIdx = arrayFindIndexF(m_physSurfaceParams, [name](const eqPhysSurfParam* other) { return !other->name.CompareCaseIns(name); });
	if (foundIdx != -1)
	{
		ASSERT_FAIL("AddSurfaceParam - %s already added\n", name);
		return;
	}

	eqPhysSurfParam* surfParam = PPNew eqPhysSurfParam;
	surfParam->id = m_physSurfaceParams.append(surfParam);
	surfParam->name = name;
	surfParam->contentsMask = KV_GetValueInt(kvSection->FindSection("contentsMask"), 0, -1);
	surfParam->friction = KV_GetValueFloat(kvSection->FindSection("friction"), 0, PHYSICS_DEFAULT_FRICTION);
	surfParam->restitution = KV_GetValueFloat(kvSection->FindSection("restitution"), 0, PHYSICS_DEFAULT_RESTITUTION);
	surfParam->tirefriction = KV_GetValueFloat(kvSection->FindSection("tirefriction"), 0, PHYSICS_DEFAULT_TIRE_FRICTION);
	surfParam->tirefriction_traction = KV_GetValueFloat(kvSection->FindSection("tirefriction_traction"), 0, PHYSICS_DEFAULT_TIRE_TRACTION);
	surfParam->word = KV_GetValueString(kvSection->FindSection("surfaceword"), 0, "C")[0];
}

const int CEqPhysics::FindSurfaceParamID(const char* name) const
{
	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
	{
		if (!m_physSurfaceParams[i]->name.CompareCaseIns(name))
			return i;
	}
	return -1;
}

const eqPhysSurfParam* CEqPhysics::FindSurfaceParam(const char* name) const
{
	const int surfParamId = FindSurfaceParamID(name);
	if (surfParamId == -1)
		return nullptr;

	return m_physSurfaceParams[surfParamId];
}

const eqPhysSurfParam* CEqPhysics::GetSurfaceParamByID(int id) const
{
	if (id == -1)
		return nullptr;

	return m_physSurfaceParams[id];
}

#ifdef DEBUG
#define CHECK_ALREADY_IN_LIST(list, obj) ASSERT_MSG(arrayFindIndex(list, obj) == -1, "Object already added")
#else
#define CHECK_ALREADY_IN_LIST(list, obj)
#endif

void CEqPhysics::AddToMoveableList( CEqRigidBody* body )
{
	if(!body)
		return;	
	
	if (body->m_flags & BODY_MOVEABLE)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	body->m_flags |= BODY_MOVEABLE;

	CHECK_ALREADY_IN_LIST(m_moveable, body);
	m_moveable.append( body );

	if(body->m_callbacks)
		body->m_callbacks->OnStartMove();
}

void CEqPhysics::RemoveFromMoveableList(CEqRigidBody* body)
{
	if (!(body->m_flags & BODY_MOVEABLE))
		return;

	body->m_flags &= ~BODY_MOVEABLE;
	m_moveable.fastRemove(body);

	if (body->m_callbacks)
		body->m_callbacks->OnStopMove();
}

void CEqPhysics::AddToWorld( CEqRigidBody* body, bool moveable )
{
	if(!body)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	CHECK_ALREADY_IN_LIST(m_dynObjects, body);

	body->m_flags |= COLLOBJ_TRANSFORM_DIRTY | COLLOBJ_BOUNDBOX_DIRTY;

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

	eqPhysGridCell* cell = body->GetCell();

	if (cell)
	{
		CScopedMutex m(s_eqPhysMutex);
		cell->dynamicObjList.unlinkNode(body);
	}

	const bool result = m_dynObjects.fastRemove(body);
	if (result)
		RemoveFromMoveableList(body);

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

	//CScopedMutex m(s_eqPhysMutex);

	// add extra flags to objects
	object->m_flags = COLLOBJ_ISGHOST | COLLOBJ_DISABLE_RESPONSE | COLLOBJ_NO_RAYCAST;

	if (!object->m_callbacks)
		object->m_flags |= COLLOBJ_COLLISIONLIST;

	m_ghostObjects.append(object);

	if(m_grid)
	{
		if(object->GetMesh() != nullptr)
		{
			m_grid->AddStaticObjectToGrid( object );
		}
		else
			SetupBodyOnCell( object );
	}
}

void CEqPhysics::DestroyGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	if(m_grid)
	{
		if(object->GetMesh() != nullptr)
		{
			m_grid->RemoveStaticObjectFromGrid(object);
		}
		else
		{
			eqPhysGridCell* cell = object->GetCell();

			if (cell)
			{
				CScopedMutex m(s_eqPhysMutex);
				cell->dynamicObjList.unlinkNode(object);
			}
		}
	}

	if(!m_ghostObjects.fastRemove(object))
		return;

	delete object;
}

void CEqPhysics::AddStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	m_staticObjects.append(object);

	if(m_grid)
		m_grid->AddStaticObjectToGrid( object );
}

void CEqPhysics::RemoveStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	if (!m_staticObjects.fastRemove(object))
		return;

	if (m_grid)
		m_grid->RemoveStaticObjectFromGrid(object);
}

void CEqPhysics::DestroyStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	if (!m_staticObjects.fastRemove(object))
		return;

	if (m_grid)
		m_grid->RemoveStaticObjectFromGrid(object);

	delete object;
}

bool CEqPhysics::IsValidStaticObject( CEqCollisionObject* obj ) const
{
    if(obj->IsDynamic())
        return false;

    return arrayFindIndex(m_staticObjects, obj ) != -1;
}

bool CEqPhysics::IsValidBody( CEqCollisionObject* body ) const
{
    if(!body->IsDynamic())
        return false;

	return arrayFindIndex(m_dynObjects, static_cast<CEqRigidBody*>(body)) != -1;
}

void CEqPhysics::AddConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	//CScopedMutex m(s_eqPhysMutex);
	m_constraints.append( constraint );
}

void CEqPhysics::RemoveConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	//CScopedMutex m(s_eqPhysMutex);
	m_constraints.fastRemove( constraint );
}

void CEqPhysics::AddController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//CScopedMutex m(s_eqPhysMutex);
	m_controllers.append( controller );

	controller->AddedToWorld( this );
}

void CEqPhysics::RemoveController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//CScopedMutex m(s_eqPhysMutex);
	if(!m_controllers.fastRemove( controller ))
		return;

	controller->RemovedFromWorld( this );
}

void CEqPhysics::DestroyController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	//CScopedMutex m(s_eqPhysMutex);

	if(!m_controllers.fastRemove(controller))
		return;

	controller->RemovedFromWorld( this );
	delete controller;
}

//-----------------------------------------------------------------------------------------------

void CEqPhysics::DetectBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt)
{
	// apply filters
	if(!bodyA->CheckCanCollideWith(bodyB))
		return;

	// test radius between bodies
	const float lenA = lengthSqr(bodyA->m_aabb.GetSize());
	const float lenB = lengthSqr(bodyB->m_aabb.GetSize());

#pragma todo("SolveBodyCollisions - add speed vector for more 'swept' broadphase detection")

	const FVector3D centerOffset = (bodyA->GetPosition()-bodyB->GetPosition());

	const float distBetweenObjects = lengthSqr(Vector3D(centerOffset));

	// yep, center is a length also...
	if(distBetweenObjects > lenA+lenB)
		return;

	// check the contact pairs of bodyB (because it has been already processed by the order)
	// if we had any contact pair with bodyA we should discard this collision
	for (const eqContactPair& pair : bodyB->m_contactPairs)
	{
		if (pair.bodyA == bodyB && pair.bodyB == bodyA)
			return;
	}

	// trasform collision objects and test

	// prepare for testing...
	btCollisionObject* objA = bodyA->m_collObject;
	btCollisionObject* objB = bodyB->m_collObject;
	if (!objA || !objB)
		return;

	// body a
	Matrix4x4 eqTransA = Matrix4x4( bodyA->GetOrientation() );
	eqTransA.translate(bodyA->GetShapeCenter());
	eqTransA = transpose(eqTransA);
	eqTransA.rows[3] += Vector4D(bodyA->GetPosition()+centerOffset, 1.0f);

	// body b
	Matrix4x4 eqTransB = Matrix4x4( bodyB->GetOrientation() );
	eqTransB.translate(bodyB->GetShapeCenter());
	eqTransB = transpose(eqTransB);
	eqTransB.rows[3] += Vector4D(bodyB->GetPosition()+centerOffset, 1.0f);

	btTransform transA;
	btTransform transB;

	ConvertMatrix4ToBullet(transA, eqTransA);
	ConvertMatrix4ToBullet(transB, eqTransB);

	//objA->setWorldTransform(transA);
	//objB->setWorldTransform(transB);

	btCollisionObjectWrapper obA(nullptr, bodyA->m_shape, objA, transA, -1, -1);
	btCollisionObjectWrapper obB(nullptr, bodyB->m_shape, objB, transB, -1, -1);

	CEqManifoldResult cbResult(&obA, &obB, true, centerOffset);

	{
		btCollisionAlgorithm* algorithm = nullptr;

		// FIXME:
		// Due to btCompoundShape producing unreliable results, there is a really slow checks appear...
		for (const btCollisionShape* shapeB : bodyB->GetBulletCollisionShapes())
		{
			for (const btCollisionShape* shapeA : bodyA->GetBulletCollisionShapes())
			{
				btCollisionObjectWrapper obA(nullptr, shapeA, objA, transA, -1, -1);
				btCollisionObjectWrapper obB(nullptr, shapeB, objB, transB, -1, -1);

				if(!algorithm)
					algorithm = m_collDispatcher->findAlgorithm(&obA, &obB, nullptr, BT_CONTACT_POINT_ALGORITHMS);

				algorithm->processCollision(&obA, &obB, *m_dispatchInfo, &cbResult);
			}
		}

		if (algorithm)
		{
			algorithm->~btCollisionAlgorithm();
			m_collDispatcher->freeCollisionAlgorithm(algorithm);
		}
	}

	// so collision test were performed, get our results to contact pairs
	const int numCollResults = cbResult.m_collisions.numElem();
	const float iterDelta = 1.0f / numCollResults;

	for(eqCollisionInfo& coll : cbResult.m_collisions)
	{
		if (bodyA->m_contactPairs.numElem() == bodyA->m_contactPairs.numAllocated())
			break;

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		if(hitDepth < 0 && !(bodyA->m_flags & COLLOBJ_ISGHOST))
			continue;

		eqContactPair& newPair = bodyA->m_contactPairs.append();
		newPair.normal = hitNormal;
		newPair.flags = 0;
		newPair.depth = hitDepth;
		newPair.position = hitPos;
		newPair.bodyA = bodyA;
		newPair.bodyB = bodyB;
		newPair.dt = iterDelta;

		newPair.restitutionA = bodyA->GetRestitution();
		newPair.frictionA = bodyA->GetFriction();
		
		newPair.restitutionB = bodyB->GetRestitution();
		newPair.frictionB = bodyB->GetFriction();

#ifdef ENABLE_DEBUG_DRAWING
		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), EqString::Format("penetration depth: %f", hitDepth), 1.0f);
		}
#endif // ENABLE_DEBUG_DRAWING
	}
}

void CEqPhysics::DetectStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt)
{
	if(staticObj == nullptr || bodyB == nullptr)
		return;

	if(!staticObj->CheckCanCollideWith(bodyB))
		return;

	if( !staticObj->m_aabb_transformed.Intersects(bodyB->m_aabb_transformed))
		return;

	Vector3D center = (staticObj->GetPosition()-bodyB->GetPosition());

	// prepare for testing...
	btCollisionObject* objA = staticObj->m_collObject;
	btCollisionObject* objB = bodyB->m_collObject;
	if (!objA || !objB)
		return;

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
	Matrix4x4 eqTransB_vel;

	{
		// body B
		eqTransB_orig = Matrix4x4( bodyB->GetOrientation() );
		eqTransB_orig.translate(bodyB->GetShapeCenter());
		eqTransB_orig = transpose(eqTransB_orig);
		eqTransB_orig.rows[3] += Vector4D(bodyB->GetPosition()+center, 1.0f);
	}

	{
		FVector3D addVelToPos = normalize(bodyB->GetLinearVelocity());
		eqTransB_vel = Matrix4x4(bodyB->GetOrientation());
		eqTransB_vel.translate(bodyB->GetShapeCenter());
		eqTransB_vel = transpose(eqTransB_orig);
		eqTransB_vel.rows[3] += Vector4D(bodyB->GetPosition() + center + addVelToPos, 1.0f);
	}

	btTransform transA; 
	btTransform transB;

	ConvertMatrix4ToBullet(transA, eqTransA);
	ConvertMatrix4ToBullet(transB, eqTransB_orig);

	btTransform transB_vel;
	ConvertMatrix4ToBullet(transB_vel, eqTransB_vel);

	objA->setWorldTransform(transA);
	objB->setWorldTransform(transB);

	btVector3 velocity;
	ConvertDKToBulletVectors(velocity, bodyB->GetLinearVelocity());
	objB->setInterpolationWorldTransform(transB_vel);

	btCollisionObjectWrapper obA(nullptr, staticObj->m_shape, objA, transA, -1, -1);
	btCollisionObjectWrapper obB(nullptr, bodyB->m_shape, objB, transB, -1, -1);
	
	CEqManifoldResult cbResult(&obA, &obB, /*(bodyB->m_flags & BODY_ISCAR)*/true, center);

	{
		btCollisionAlgorithm* algorithm = nullptr;

		const int bodyContents = bodyB->GetContents();

		const CEqBulletIndexedMesh* staticIndexedMesh = staticObj->GetMesh();
		ArrayCRef<btCollisionShape*> staticShapes = staticObj->GetBulletCollisionShapes();

		// FIXME:
		// Due to btCompoundShape producing unreliable results, there is a really slow checks appear...
		for (const btCollisionShape* shapeB : bodyB->GetBulletCollisionShapes())
		{
			for (int i = 0; i < staticShapes.numElem(); ++i)
			{
				if (staticIndexedMesh)
				{
					const int surfMaterialIdx = staticIndexedMesh->GetSubpartMaterialIdx(i);
					const eqPhysSurfParam* surfParam = GetSurfaceParamByID(surfMaterialIdx);

					// skip the shape if collide mask not meeting expectation
					if (surfParam && (surfParam->contentsMask & bodyContents) != bodyContents)
						continue;
				}

				btCollisionObjectWrapper obA(nullptr, staticShapes[i], objA, transA, -1, -1);
				btCollisionObjectWrapper obB(nullptr, shapeB, objB, transB, -1, -1);

				if (!algorithm)
					algorithm = m_collDispatcher->findAlgorithm(&obA, &obB, nullptr, BT_CONTACT_POINT_ALGORITHMS);

				algorithm->processCollision(&obA, &obB, *m_dispatchInfo, &cbResult);
			}
		}

		if (algorithm)
		{
			algorithm->~btCollisionAlgorithm();
			m_collDispatcher->freeCollisionAlgorithm(algorithm);
		}
	}

	const int numCollResults = cbResult.m_collisions.numElem();
	const float iterDelta = 1.0f / numCollResults;

	for(eqCollisionInfo& coll : cbResult.m_collisions)
	{
		if (bodyB->m_contactPairs.numElem() == bodyB->m_contactPairs.numAllocated())
			break;

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		if(hitDepth < 0 && !(staticObj->m_flags & COLLOBJ_ISGHOST))
			continue;

		if(hitDepth > 1.0f)
			hitDepth = 1.0f;

		eqContactPair& newPair = bodyB->m_contactPairs.append();

		newPair.normal = hitNormal;
		newPair.flags = COLLPAIRFLAG_OBJECTA_STATIC;
		newPair.depth = hitDepth;
		newPair.position = hitPos;
		newPair.bodyA = staticObj;
		newPair.bodyB = bodyB;
		newPair.dt = iterDelta;

		const eqPhysSurfParam* sparam = GetSurfaceParamByID(coll.materialIndex);

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
#ifdef ENABLE_DEBUG_DRAWING
		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), EqString::Format("penetration depth: %f", hitDepth), 1.0f);
		}
#endif // ENABLE_DEBUG_DRAWING
	}
}

void CEqPhysics::SetupBodyOnCell( CEqCollisionObject* body )
{
	// check body is in the world
	if(!m_grid)
		return;

	eqPhysGridCell* oldCell = body->GetCell();

	// get new cell
	eqPhysGridCell* newCell = m_grid->GetPreallocatedCellAtPos( body->GetPosition() );

	// move object in grid
	if (newCell != oldCell)
	{
		CScopedMutex m(s_eqPhysMutex);
		if (oldCell)
			oldCell->dynamicObjList.unlinkNode(body);

		if (newCell)
			newCell->dynamicObjList.insertNodeLast(body);

		body->SetCell(newCell);
	}
}

void CEqPhysics::IntegrateSingle(CEqRigidBody* body)
{
	eqPhysGridCell* oldCell = body->GetCell();

	// move object
	body->Integrate( m_fDt );

	const bool bodyFrozen = body->IsFrozen();
	const bool forceSetCell = !oldCell && bodyFrozen;

	if(!bodyFrozen && body->IsCanIntegrate(true) || forceSetCell)
	{
		// get new cell
		eqPhysGridCell* newCell = m_grid->GetCellAtPos( body->GetPosition() );

		// move object in grid if it's a really new cell
		if (newCell != oldCell)
		{
			CScopedMutex m(s_eqPhysMutex);
			if (oldCell)
				oldCell->dynamicObjList.unlinkNode(body);

			if (newCell)
				newCell->dynamicObjList.insertNodeLast(body);

			body->SetCell(newCell);
		}
	}
}

void CEqPhysics::DetectCollisionsSingle(CEqRigidBody* body)
{
	// don't refresh frozen object, other will wake up us (or user)
	if (body->IsFrozen())
		return;

	// skip collision detection on iteration
	if (!body->IsCanIntegrate())
		return;

	const bool disabledCollisionChecks = (body->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK);

	body->UpdateBoundingBoxTransform();
	const BoundingBox aabb = body->m_aabb_transformed;
	
	// get the grid box range for searching collision objects
	IAARectangle gridRange;
	m_grid->FindBoxRange(aabb, gridRange, PHYSGRID_BOX_TOLERANCE);

	const IVector2D rangeSize = gridRange.GetSize();
	ASSERT_MSG(rangeSize.x < 100 && rangeSize.y < 100, "Dynamic object might be too large in physics world");

	// in this range do all collision checks
	// might be slow
	for(int y = gridRange.leftTop.y; y <= gridRange.rightBottom.y; y++)
	{
		for(int x = gridRange.leftTop.x; x <= gridRange.rightBottom.x; x++)
		{
			const eqPhysGridCell* ncell = m_grid->GetCellAt( x, y );
			if(!ncell)
				continue;
						
			// iterate over static objects in cell
			for (CEqCollisionObject* obj : ncell->gridObjects)
				DetectStaticVsBodyCollision(obj, body, body->GetLastFrameTime());

			// if object is only affected by other dynamic objects, don't waste my cycles!
			if (disabledCollisionChecks)
				continue;

			// iterate over dynamic objects in cell
			CEqCollisionObject* dynObj = ncell->dynamicObjList.getFirst();
			for (; dynObj; dynObj = dynObj->next)
			{
				if (dynObj == body)
					continue;

				if (dynObj->IsDynamic())
					DetectBodyCollisions(body, static_cast<CEqRigidBody*>(dynObj), body->GetLastFrameTime());
				else // purpose for triggers
					DetectStaticVsBodyCollision(dynObj, body, body->GetLastFrameTime());
			}
		}
	}
}

void CEqPhysics::ProcessContactPair(eqContactPair& pair)
{
	CEqRigidBody* bodyB = static_cast<CEqRigidBody*>(pair.bodyB);
	const int bodyAFlags = pair.bodyA->m_flags;
	const int bodyBFlags = bodyB->m_flags;

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

			// apply response
			pair.normal *= -1.0f;
			pair.depth *= -1.0f;

			const float combinedErp = ph_erp.GetFloat() + max(0.0f, pair.bodyA->m_erp + pair.bodyB->m_erp);
			const float positionalError = pair.depth * pair.dt;

			bodyB->m_position += pair.normal * positionalError * combinedErp;
			bodyB->m_prevPosition += pair.normal * positionalError * combinedErp;
			
			appliedImpulse = CEqRigidBody::ApplyImpulseResponseTo(pair, positionalError * combinedErp * 2.0f);
			//appliedImpulse =  CEqRigidBody::ApplyImpulseResponseTo(bodyB, pair.position, pair.normal, 0.0, pair.restitutionA, pair.frictionA);
		}

		bodyADisableResponse = (bodyAFlags & COLLOBJ_DISABLE_RESPONSE);
	}
	else
	{
		CEqRigidBody* bodyA = static_cast<CEqRigidBody*>(pair.bodyA);

		const bool isCarCollidingWithCar = (bodyAFlags & BODY_ISCAR) && (bodyBFlags & BODY_ISCAR);
		const float varyErp = (isCarCollidingWithCar ? ph_carVsCarErp.GetFloat() : ph_erp.GetFloat());

		const int bodyAFlags = pair.bodyA->m_flags;
		const int bodyBFlags = bodyB->m_flags;

		const float combinedErp = varyErp + max(0.0f, pair.bodyA->m_erp + pair.bodyB->m_erp);
		const float positionalError = pair.depth * pair.dt;

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

	eqCollisionPairData tempPairData;

	//-----------------------------------------------
	// OBJECT A
	{
		FixedArray<eqCollisionPairData, PHYSICS_COLLISION_LIST_MAX>& pairs = pair.bodyA->m_collisionList;

		eqCollisionPairData collData;
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
			pairs.append(std::move(collData));
	}

	//-----------------------------------------------
	// OBJECT B
	{
		FixedArray<eqCollisionPairData, PHYSICS_COLLISION_LIST_MAX>& pairs = pair.bodyB->m_collisionList;

		eqCollisionPairData collData;
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
			pairs.append(std::move(collData));
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
	if(!m_grid)
		return;

	// save delta
	m_fDt = deltaTime;

	{
		PROF_EVENT("Constraints PreApply");
		// prepare all the constraints
		for (IEqPhysicsConstraint* constr : m_constraints)
		{
			if (!constr->IsEnabled())
				continue;
			constr->PreApply(m_fDt);
		}
	}

	{
		PROF_EVENT("Controllers Update");
		// update the controllers
		for (IEqPhysicsController* contr : m_controllers)
		{
			if (!contr->IsEnabled())
				continue;
			contr->Update(m_fDt);
		}
	}
	
	Array<CEqRigidBody*> movingMoveables{ PP_SL };
	movingMoveables.resize(m_moveable.numElem());

	{
		PROF_EVENT("Moving Bodies Integrate");

		// move all bodies
		for (CEqRigidBody* body : m_moveable)
		{
			// execute pre-simulation callbacks
			IEqPhysCallback* callbacks = body->m_callbacks;

			if (callbacks) 	// execute pre-simulation callbacks
				callbacks->PreSimulate(m_fDt);

			// clear contact pairs and results
			body->ClearContacts();

			// apply velocities
			IntegrateSingle(body);

			if (!body->IsFrozen())
				movingMoveables.append(body);
		}
	}

	m_fDt = deltaTime;

	if(preIntegrFunc)
		preIntegrFunc(m_fDt, iteration);

	{
		PROF_EVENT("Moving Bodies CollDet");
		// calculate collisions
		for (CEqRigidBody* body : movingMoveables)
			DetectCollisionsSingle(body);
	}

	{
		PROF_EVENT("Moving Bodies Update");
		// solve positions
		for (CEqRigidBody* body : movingMoveables)
			body->Update(m_fDt);
	}
	
	{
		PROF_EVENT("Moving Bodies Process Contact Pairs");

		// process generated contact pairs
		for (CEqRigidBody* body : movingMoveables)
		{
			for (eqContactPair& pair : body->m_contactPairs)
				ProcessContactPair(pair);

			IEqPhysCallback* callbacks = body->m_callbacks;
			if (callbacks) // execute post simulation callbacks
				callbacks->PostSimulate(m_fDt);
		}
	}

	{
		PROF_EVENT("Constraits apply");

		// update all constraints
		for (IEqPhysicsConstraint* constr : m_constraints)
		{
			if (!constr->IsEnabled())
				continue;
			constr->Apply(m_fDt);
		}
	}

	m_numRayQueries = 0;
}

//----------------------------------------------------------------------------------------------------
//
// VOXEL TRACING FOR RAYS
//
//----------------------------------------------------------------------------------------------------
template <typename F>
void CEqPhysics::InternalTestLineCollisionCells(const Vector2D& startCell, const Vector2D& endCell,
	const FVector3D& start,
	const FVector3D& end,
	const BoundingBox& rayBox,
	eqCollisionInfo& coll,
	int rayMask,
	const eqPhysCollisionFilter* filterParams,
	F func,
	void* args)
{
	static constexpr const int s_maxClosestTestTries = 2;

	Set<CEqCollisionObject*> skipObjects(PP_SL);
	{
		const IVector2D cell(floor(startCell.x), floor(startCell.y));
		if (cell == IVector2D(floor(endCell.x), floor(endCell.y)))
		{
			TestLineCollisionOnCell(cell.y, cell.x, start, end, rayBox, coll, skipObjects, rayMask, filterParams, func, args);
			return;
		}
	}

	const float difX = endCell.x - startCell.x;
	const float difY = endCell.y - startCell.y;
	const float dist = fabs(difX) + fabs(difY);
	const float oneByDist = 1.0f / dist;

	const float dx = difX * oneByDist;
	const float dy = difY * oneByDist;

	int closestTries = 0;
	for (int i = 0; i <= ceil(dist) && closestTries < s_maxClosestTestTries; ++i)
	{
		const int x = static_cast<int>(floor(startCell.x + dx * float(i)));
		const int y = static_cast<int>(floor(startCell.y + dy * float(i)));

		// if can't traverse further - stop.
		if (!TestLineCollisionOnCell(y, x, start, end, rayBox, coll, skipObjects, rayMask, filterParams, func, args))
		{
			++closestTries;
		}
	}
}

//-----------------------------------------------------------------------------------------------------------------------------

template <typename F>
bool CEqPhysics::TestLineCollisionOnCell(int y, int x,
	const FVector3D& start, const FVector3D& end,
	const BoundingBox& rayBox,
	eqCollisionInfo& coll,
	Set<CEqCollisionObject*>& skipObjects,
	int rayMask, const eqPhysCollisionFilter* filterParams,
	F func,
	void* args)
{
	if (!m_grid)
		return false;

	eqPhysGridCell* cell = m_grid->GetCellAt(x,y);

	if (!cell)
		return true;

	float closest = coll.fract;

	Vector2D cellMin, cellMax;
	m_grid->GetCellBoundsXZ(x, y, cellMin, cellMax);

	// try to stop propagating ray across grid
	{
		const Vector2D rayStart = start.xz();
		const Vector2D rayDir = Vector2D(end.xz() - start.xz());

		const AARectangle cellRect(cellMin, cellMax);

		float tnear, tfar;
		if (cellRect.IntersectsRay(rayStart, rayDir, tnear, tfar) && tnear > closest)
			return false;
	}

	int objectTypeTesting = EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
	if (filterParams)
		objectTypeTesting = filterParams->flags & (EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS);

	// TODO: special flag that ignores bound check
	const bool staticInBoundTest = true; // (rayBox.minPoint.y <= cell->cellBoundUsed); TEMPORARY ALLOWED because Chicago bridges. 

#ifdef ENABLE_DEBUG_DRAWING
	if(staticInBoundTest && m_debugRaycast)
	{
		const float cellBound = cell->cellBoundUsed;
		debugoverlay->Box3D(Vector3D(cellMin.x, -cellBound, cellMin.y), Vector3D(cellMax.x, cellBound, cellMax.y), ColorRGBA(1, 0, 0, 0.25f));
	}
#endif

	bool hit = false;
	bool hitClosest = false;
	
	// static objects are not checked if line is not in Y bound
	if(staticInBoundTest && (objectTypeTesting & EQPHYS_FILTER_FLAG_STATICOBJECTS))
	{
		CScopedMutex m(s_eqPhysMutex);

		for (CEqCollisionObject* object : cell->gridObjects)
		{
			if (skipObjects.contains(object))
				continue;

			eqCollisionInfo tempColl;
			if (!(this->*func)(object, start, end, rayBox, tempColl, closest, rayMask, filterParams, args))
				continue;

			hit = true;

			if (tempColl.fract < closest)
			{
				skipObjects.insert(object);

				closest = tempColl.fract;
				coll = tempColl;
				hitClosest = true;
			}
		}
	}

	if(objectTypeTesting & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS)
	{
		CScopedMutex m(s_eqPhysMutex);

		CEqCollisionObject* object = cell->dynamicObjList.getFirst();
		for (; object; object = object->next)
		{
			if (skipObjects.contains(object))
				continue;

			eqCollisionInfo tempColl;
			if (!(this->*func)(object, start, end, rayBox, tempColl, closest, rayMask, filterParams, args))
				continue;

			hit = true;

			if (tempColl.fract < closest)
			{
				skipObjects.insert(object);

				closest = tempColl.fract;
				coll = tempColl;
				hitClosest = true;
			}
		}
	}

	if (hit)
		return !hitClosest; // continue

	return true;
}

//-----------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
//
//	TestLineCollision
//		- Casts line in the physics world
//
//----------------------------------------------------------------------------------------------------
bool CEqPhysics::TestLineCollision(	const FVector3D& start, const FVector3D& end,
									eqCollisionInfo& coll,
									int rayMask, const eqPhysCollisionFilter* filterParams)
{
	if (!m_grid) {
		return false;
	}

	Vector2D startCell, endCell;

	//CScopedMutex m(s_eqPhysMutex);
	m_grid->GetPointAt(start, startCell);
	m_grid->GetPointAt(end, endCell);

	coll.position = end;
	coll.fract = 10.0f;

	BoundingBox rayBox;
	rayBox.AddVertex(start);
	rayBox.AddVertex(end);

	InternalTestLineCollisionCells(	startCell, endCell,
									start, end,
									rayBox,
									coll,
									rayMask,
									filterParams,
									&CEqPhysics::TestLineSingleObject, nullptr);

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
bool CEqPhysics::TestConvexSweepCollision(const btCollisionShape* shape,
											const Quaternion& rotation,
											const FVector3D& start, const FVector3D& end,
											eqCollisionInfo& coll,
											int rayMask, 
											const eqPhysCollisionFilter* filterParams)
{
	if (!m_grid) {
		return false;
	}
	//CScopedMutex m(s_eqPhysMutex);

	coll.position = end;
	coll.fract = 32768.0f;

	sweptTestParams_t params;
	params.rotation = rotation;
	params.shape = shape;

	btTransform startTrans;
	ConvertMatrix4ToBullet(startTrans, rotation);

	btVector3 shapeMins, shapeMaxs;
	shape->getAabb(startTrans, shapeMins, shapeMaxs);

	BoundingBox shapeBox;
	ConvertBulletToDKVectors(shapeBox.minPoint, shapeMins);
	ConvertBulletToDKVectors(shapeBox.maxPoint, shapeMaxs);

	BoundingBox rayBox;
	rayBox.AddVertex(start);
	rayBox.AddVertex(end);

	const Vector3D sBoxSize = shapeBox.GetSize();
	rayBox.AddVertex(rayBox.minPoint - sBoxSize);
	rayBox.AddVertex(rayBox.maxPoint + sBoxSize);

	Vector2D startCell, endCell;
	m_grid->GetPointAt(start, startCell);
	m_grid->GetPointAt(end , endCell);

	InternalTestLineCollisionCells(	startCell, endCell,
									start, end,
									rayBox,
									coll,
									rayMask,
									filterParams,
									&CEqPhysics::TestConvexSweepSingleObject, &params);

	if (coll.fract > 1.0f)
		coll.fract = 1.0f;

	return (coll.fract < 1.0f);
}

//-------------------------------------------------------------------------------------------------

class CEqRayTestCallback : public btCollisionWorld::ClosestRayResultCallback
{
public:
	CEqRayTestCallback(const btVector3&	rayFromWorld,const btVector3&	rayToWorld)
		: ClosestRayResultCallback(rayFromWorld, rayToWorld)
	{
		m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		if (rayResult.m_hitFraction > m_closestHitFraction)
			return 1.0f;

		// do default result
		const btScalar hitFraction = ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

		// if something is a NaN we have to deny it
		if (rayResult.m_hitNormalLocal != rayResult.m_hitNormalLocal || rayResult.m_hitFraction != rayResult.m_hitFraction)
			return 1.0f;

		// check our object is triangle mesh
		CEqCollisionObject* obj = reinterpret_cast<CEqCollisionObject*>(m_collisionObject->getUserPointer());
		if(obj)
		{
			if(obj->GetMesh() && rayResult.m_localShapeInfo)
			{
				CEqBulletIndexedMesh* mesh = obj->GetMesh();
				m_surfMaterialId = mesh->GetSubpartMaterialIdx( rayResult.m_localShapeInfo->m_shapePart );
			}

			if(m_surfMaterialId == -1)
				m_surfMaterialId = obj->m_surfParam;
		}

		return hitFraction;
	}

	int m_surfMaterialId{ -1 };
};

bool CEqPhysics::CheckAllowContactTest(const eqPhysCollisionFilter* filterParams, const CEqCollisionObject* object)
{
	if (!filterParams)
		return true;

	// skip objects with some contents
	if (object->GetContents() & filterParams->ignoreContentsMask)
		return false;

	const bool objIsDynamic = object->IsDynamic();

	const bool checkStatic = (filterParams->flags & EQPHYS_FILTER_FLAG_STATICOBJECTS) && !objIsDynamic;
	const bool checkDynamic = (filterParams->flags & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS) && objIsDynamic;

	if (checkStatic || checkDynamic)
	{
		const bool checkUserData = (filterParams->flags & EQPHYS_FILTER_FLAG_BY_USERDATA) > 0;

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
	eqCollisionInfo& coll,
	float closestHit,
	int rayMask,
	const eqPhysCollisionFilter* filterParams,
	void* args)
{
	if(!object)
		return false;

	const bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));

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

#if 0
	{
		const Vector3D rayVec = Vector3D(end - start);
		const float oneByRayDist = 1.0f / length(rayVec);
		const Vector3D rayDir = rayVec * oneByRayDist;
	
		float hitNear, hitFar;
		if (!object->m_aabb_transformed.IntersectsRay(start, rayDir, hitNear, hitFar))
			return false;
	
		hitNear *= oneByRayDist;
		hitFar *= oneByRayDist;
	
		if (hitNear > 1.0f || hitNear > closestHit) {
			return false;
		}
	}
#endif

	const Quaternion& objQuat = object->m_orientation;
	const Vector3D& position = object->m_position;

	const btTransform objTransform(btQuaternion(-objQuat.x, -objQuat.y, -objQuat.z, objQuat.w));

	const Vector3D lineStartLocal = start - position;
	const Vector3D lineEndLocal = end - position;

	btVector3 strt;
	btVector3 endt;
	ConvertPositionToBullet(strt, lineStartLocal);
	ConvertPositionToBullet(endt, lineEndLocal);

	const btTransform startTrans(btMatrix3x3::getIdentity(), strt);
	const btTransform endTrans(btMatrix3x3::getIdentity(), endt);

	CEqRayTestCallback hitResultCallback(strt, endt);

	const CEqBulletIndexedMesh* indexedMesh = object->GetMesh();
	ArrayCRef<btCollisionShape*> objectShapes = object->GetBulletCollisionShapes();
	for (int i = 0; i < objectShapes.numElem(); ++i)
	{
		if (indexedMesh)
		{
			const int surfMaterialIdx = indexedMesh->GetSubpartMaterialIdx(i);
			const eqPhysSurfParam* surfParam = GetSurfaceParamByID(surfMaterialIdx);

			// skip the shape if collide mask not meeting expectation
			if (surfParam && (surfParam->contentsMask & rayMask) != rayMask)
				continue;
		}

		btCollisionObjectWrapper objWrap(nullptr, objectShapes[i], object->m_collObject, objTransform, -1, -1);
		m_collisionWorld->rayTestSingleInternal(startTrans, endTrans, &objWrap, hitResultCallback);
	}

	m_numRayQueries++;

	// put our result
	if (!hitResultCallback.hasHit())
		return false;

	Vector3D hitPoint;
	ConvertBulletToDKVectors(hitPoint, hitResultCallback.m_hitPointWorld);
	ConvertBulletToDKVectors(coll.normal, hitResultCallback.m_hitNormalWorld);

	coll.position = hitPoint + position;
	coll.fract = hitResultCallback.m_closestHitFraction;
	coll.materialIndex = hitResultCallback.m_surfMaterialId;
	coll.hitobject = object;

	return true;
}


//-------------------------------------------------------------------------------------------------------------------------------


class CEqConvexTestCallback : public btCollisionWorld::ClosestConvexResultCallback
{
public:
	CEqConvexTestCallback(const btVector3& rayFromWorld,const btVector3& rayToWorld) 
		: ClosestConvexResultCallback(rayFromWorld, rayToWorld)
	{
		m_closestHitFraction = PHYSICS_WORLD_MAX_UNITS;
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& rayResult, bool normalInWorldSpace)
	{
		if (rayResult.m_hitFraction > m_closestHitFraction)
			return 1.0f;

		// do default result
		const btScalar hitFraction = ClosestConvexResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

		// if something is a NaN we have to deny it
		if (rayResult.m_hitNormalLocal != rayResult.m_hitNormalLocal ||
			rayResult.m_hitFraction != rayResult.m_hitFraction)
		{
			return 1.0f;
		}

		// check our object is triangle mesh
		const CEqCollisionObject* obj = (CEqCollisionObject*)m_hitCollisionObject->getUserPointer();

		if(obj)
		{
			if(obj->GetMesh() && rayResult.m_localShapeInfo)
			{
				CEqBulletIndexedMesh* mesh = obj->GetMesh();
				m_surfMaterialId = mesh->GetSubpartMaterialIdx( rayResult.m_localShapeInfo->m_shapePart );
			}

			if(m_surfMaterialId == -1)
				m_surfMaterialId = obj->m_surfParam;
		}

		return hitFraction;
	}

	int m_surfMaterialId{ -1 };
};

//-------------------------------------------------------------------------------------------------

bool CEqPhysics::TestConvexSweepSingleObject(CEqCollisionObject* object,
												const FVector3D& start,
												const FVector3D& end,
												const BoundingBox& raybox,
												eqCollisionInfo& coll,
												float closestHit,
												int rayMask,
												const eqPhysCollisionFilter* filterParams,
												void* args)
{
	const bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));

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

	const sweptTestParams_t& params = *(sweptTestParams_t*)args;

	const Quaternion objQuat = object->m_orientation;
	const Vector3D position = object->m_position;

	const btTransform objTransform(btQuaternion(-objQuat.x, -objQuat.y, -objQuat.z, objQuat.w));

	const Vector3D lineStartLocal = start - position;
	const Vector3D lineEndLocal = end - position;

	btVector3 strt;
	btVector3 endt;
	ConvertPositionToBullet(strt, lineStartLocal);
	ConvertPositionToBullet(endt, lineEndLocal);

	const btQuaternion shapeRotation(-params.rotation.x, -params.rotation.y, -params.rotation.z, params.rotation.w);
	const btTransform startTrans(shapeRotation, strt);
	const btTransform endTrans(shapeRotation, endt);

	if(params.shape->getShapeType() > CONCAVE_SHAPES_START_HERE)
	{
		ASSERT_FAIL("Only convex shapes are supported as concave shapes!");
		return false;
	}

	CEqConvexTestCallback hitResultCallback(strt, endt);

	const CEqBulletIndexedMesh* indexedMesh = object->GetMesh();
	ArrayCRef<btCollisionShape*> objectShapes = object->GetBulletCollisionShapes();
	for (int i = 0; i < objectShapes.numElem(); ++i)
	{
		if (indexedMesh)
		{
			const int surfMaterialIdx = indexedMesh->GetSubpartMaterialIdx(i);
			const eqPhysSurfParam* surfParam = GetSurfaceParamByID(surfMaterialIdx);

			// skip the shape if collide mask not meeting expectation
			if (surfParam && (surfParam->contentsMask & rayMask) != rayMask)
				continue;
		}

		btCollisionObjectWrapper objWrap(nullptr, objectShapes[i], object->m_collObject, objTransform, -1, -1);
		m_collisionWorld->objectQuerySingleInternal((btConvexShape*)params.shape, startTrans, endTrans, &objWrap, hitResultCallback, 0.01f);
	}

	// put our result
	if (!hitResultCallback.hasHit())
		return false;

	Vector3D hitPoint;
	ConvertBulletToDKVectors(hitPoint, hitResultCallback.m_hitPointWorld);
	ConvertBulletToDKVectors(coll.normal, hitResultCallback.m_hitNormalWorld);

	coll.position = hitPoint + position;
	coll.fract = hitResultCallback.m_closestHitFraction;
	coll.materialIndex = hitResultCallback.m_surfMaterialId;
	coll.hitobject = object;

	return true;
}

//--------------------------------------------------------------------------------------------------------------

void CEqPhysics::DebugDrawBodies(int mode)
{
#ifdef ENABLE_DEBUG_DRAWING
	if (mode >= 1 && mode != 4 && mode != 5)
	{
		for (CEqRigidBody* body: m_dynObjects)
		{
			ColorRGBA bodyCol(0.2, 1, 1, 0.8f);

			if (body->IsFrozen())
				bodyCol = ColorRGBA(0.2, 1, 0.1f, 0.8f);

			debugoverlay->OrientedBox3D(
				body->m_aabb.minPoint, body->m_aabb.maxPoint, body->GetPosition(), body->GetOrientation(), bodyCol);
		}

		if (mode >= 2)
			m_grid->DebugRender();

		if (mode >= 3)
		{
			for (CEqCollisionObject* obj: m_staticObjects)
			{
				debugoverlay->OrientedBox3D(
					obj->m_aabb.minPoint, obj->m_aabb.maxPoint, obj->GetPosition(), obj->GetOrientation(), ColorRGBA(1, 1, 0.2, 0.8f));
			}
		}
	}
	else if (mode == 5)	// only grid
	{
		m_grid->DebugRender();
	}
	else
	{
		for (CEqRigidBody* body: m_dynObjects)
		{
			ColorRGBA bodyCol(0.2, 1, 1, 1.0f);

			if (body->IsFrozen())
				bodyCol = ColorRGBA(0.2, 1, 0.1f, 1.0f);

			debugoverlay->Box3D(body->m_aabb_transformed.minPoint, body->m_aabb_transformed.maxPoint, bodyCol, 0.0f);
		}
	}
#endif // ENABLE_DEBUG_DRAWING
}
