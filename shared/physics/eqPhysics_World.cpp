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
#include "utils/KeyValues.h"

#include "render/IDebugOverlay.h"

#include "eqPhysics_World.h"
#include "eqCollision_Callback.h"
#include "eqPhysics_Body.h"
#include "eqPhysics_Contstraint.h"
#include "eqPhysics_Controller.h"
#include "eqPhysics_Broadphase.h"
#include "eqBulletIndexedMesh.h"
#include "BulletConvert.h"

#define ENABLE_CONTACT_GROUPING

static Threading::CEqReadWriteLock s_eqPhysDynamicRWLock;

static constexpr const int PHYSGRID_WORLD_SIZE			= 24;	// compromised betwen memory usage and performance
static constexpr const float PHYSICS_WORLD_MAX_UNITS	= 65535.0f;
static constexpr const float PHYSICS_GRID_COLLISION_TOLERANCE	= 0.5f;

DECLARE_CVAR_F(ph_margin);

DECLARE_CVAR(ph_showContacts, "0", nullptr, CV_CHEAT);
DECLARE_CVAR(ph_erp, "0.15", "Collision correction", CV_CHEAT);
DECLARE_CVAR(ph_carVsCarErp, "0.15", "Car versus car erp", CV_CHEAT);
DECLARE_CVAR(ph_useJobs, "1", nullptr, CV_CHEAT);

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
		using namespace EqBulletUtils;

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

		if (m_collisions.isFull())
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

template<typename F>
struct CEqBroadphaseCallback : btBroadphaseAabbCallback
{
	F m_func;
	CEqBroadphaseCallback(F func)
		: m_func(func)
	{
	}

	bool process(const btBroadphaseProxy* proxy) override
	{
		if (!proxy)
			return false;
		m_func(reinterpret_cast<CEqCollisionObject*>(proxy->m_clientObject));
		return true;
	}
};

template<typename F>
struct CEqBroadphaseRayCallback : btBroadphaseRayCallback
{
	F m_func;
	CEqBroadphaseRayCallback(const Vector3D& rayVec, F func)
		: m_func(func)
	{
		const Vector3D rayDir = normalize(rayVec);

		///what about division by zero? --> just set rayDirection[i] to INF/BT_LARGE_FLOAT
		m_rayDirectionInverse[0] = rayDir[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[0];
		m_rayDirectionInverse[1] = rayDir[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[1];
		m_rayDirectionInverse[2] = rayDir[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[2];
		m_signs[0] = m_rayDirectionInverse[0] < 0.0;
		m_signs[1] = m_rayDirectionInverse[1] < 0.0;
		m_signs[2] = m_rayDirectionInverse[2] < 0.0;

		m_lambda_max = dot(rayDir, rayVec);
	}

	bool process(const btBroadphaseProxy* proxy) override
	{
		if (!proxy)
			return false;
		m_func(reinterpret_cast<CEqCollisionObject*>(proxy->m_clientObject));
		return true;
	}
};

//------------------------------------------------------------------------------------------------------------

class CEqPhysicsWorld::PreSimulateJob : public BatchedJob<CEqRigidBody*>
{
public:
	PreSimulateJob(BatchItemList& batchItems)
		: BatchedJob("PreSimulateJob")
		, m_batchItems(batchItems)
	{
	}

	void 			SetDeltaTime(float dt) { m_dt = dt; }

private:
	BatchItems		GetJobItems() override { return m_batchItems; }
	void			Process(CEqRigidBody* jobItem) override;

	BatchItemList& 	m_batchItems;
	float			m_dt{ 0.0f };
};

void CEqPhysicsWorld::PreSimulateJob::Process(CEqRigidBody* body)
{
	IEqPhysCallback* callbacks = body->m_callbacks;
	if (callbacks)
		callbacks->PreSimulate(m_dt);
}

class CEqPhysicsWorld::CollisionDetectionJob : public BatchedJob<CEqRigidBody*>
{
public:
	CollisionDetectionJob(CEqPhysicsWorld& thisWorld)
		: BatchedJob("CollisionDetectionJob")
		, m_thisWorld(thisWorld)
	{
	}
private:
	BatchItems		GetJobItems() override { return m_thisWorld.m_simMovingMoveables; }
	void			Process(CEqRigidBody* body) override { m_thisWorld.DetectCollisionsSingle(body); }

	CEqPhysicsWorld&	m_thisWorld;
};

//------------------------------------------------------------------------------------------------------------

class EqBtTLSDispatcher : public btCollisionDispatcher
{
public:
	EqBtTLSDispatcher(btCollisionConfiguration* collisionConfiguration) :
		btCollisionDispatcher(collisionConfiguration)
	{
	}

	btPersistentManifold* 	getNewManifold(const btCollisionObject* b0, const btCollisionObject* b1) override;
	void 					releaseManifold(btPersistentManifold* manifold) override;
};

//------------------------------------------------------------------------------------------------------------

CEqPhysicsWorld::CEqPhysicsWorld(CEqJobManager& jobMng)
	: m_jobMng(jobMng)
{
}

CEqPhysicsWorld::~CEqPhysicsWorld()
{
}

btPersistentManifold* EqBtTLSDispatcher::getNewManifold(const btCollisionObject* body0, const btCollisionObject* body1)
{
	//optional relative contact breaking threshold, turned on by default (use setDispatcherFlags to switch off feature for improved performance)
	const btScalar contactBreakingThreshold = (m_dispatcherFlags & btCollisionDispatcher::CD_USE_RELATIVE_CONTACT_BREAKING_THRESHOLD)
		? btMin(body0->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold), body1->getCollisionShape()->getContactBreakingThreshold(gContactBreakingThreshold))
		: gContactBreakingThreshold;

	const btScalar contactProcessingThreshold = btMin(body0->getContactProcessingThreshold(), body1->getContactProcessingThreshold());

	static thread_local btPersistentManifold s_manifold;
	s_manifold = btPersistentManifold(body0, body1, 0, contactBreakingThreshold, contactProcessingThreshold);

	return &s_manifold;
}

void EqBtTLSDispatcher::releaseManifold(btPersistentManifold* manifold)
{
	clearManifold(manifold);
}

void CEqPhysicsWorld::InitWorld()
{
	btAlignedAllocSetCustom(eqBtAlloc, eqBtFree);

	// collision configuration contains default setup for memory, collision setup
	m_collConfig = PPNew btDefaultCollisionConfiguration();
	m_collDispatcher = PPNew EqBtTLSDispatcher( m_collConfig );

	// still required for raycasts
	m_collisionWorld = PPNew btCollisionWorld(m_collDispatcher, nullptr, m_collConfig);

	m_dispatchInfo = PPNew btDispatcherInfo();

	if(ph_useJobs.GetBool())
	{
		m_preSimJob = PPNew PreSimulateJob(m_moveable);
		m_collDetJob = PPNew CollisionDetectionJob(*this);		
	}
}

void CEqPhysicsWorld::InitGrid(const BoundingBox& worldBBox)
{
	m_broadphase = PPNew CEqPhysicsBroadphase();

	for(CEqRigidBody* body : m_dynObjects)
		SetupCollisionObjectBroadphase(body);

	for(CEqCollisionObject* collObj : m_ghostObjects)
		SetupCollisionObjectBroadphase(collObj);

	for (CEqCollisionObject* collObj : m_staticObjects)
		SetupCollisionObjectBroadphase(collObj);
}

void CEqPhysicsWorld::DestroyGrid()
{
	for (CEqRigidBody* body : m_dynObjects)
		body->m_broadphaseUnit = nullptr;

	for (CEqCollisionObject* collObj : m_ghostObjects)
		collObj->m_broadphaseUnit = nullptr;

	for (CEqCollisionObject* collObj : m_staticObjects)
		collObj->m_broadphaseUnit = nullptr;

	SAFE_DELETE(m_broadphase);
}

void CEqPhysicsWorld::DestroyWorld()
{
	for (int i = 0; i < m_controllers.numElem(); i++)
		m_controllers[i]->SetEnabled(false);

	m_dynObjects.clear(true);
	m_moveable.clear(true);
	m_staticObjects.clear(true);
	m_ghostObjects.clear(true);
	m_controllers.clear(true);
	m_constraints.clear(true);
	m_physSurfaceParams.clear(true);

	SAFE_DELETE(m_collisionWorld);
	SAFE_DELETE(m_collDispatcher);
	SAFE_DELETE(m_collConfig);
	SAFE_DELETE(m_dispatchInfo);

	SAFE_DELETE(m_preSimJob);
	SAFE_DELETE(m_collDetJob);
}

void CEqPhysicsWorld::AddSurfaceParamFromKV(const char* name, const KVSection& kvSection)
{
	const int foundIdx = arrayFindIndexF(m_physSurfaceParams, [name](const eqPhysSurfParam& other) { return !other.name.CompareCaseIns(name); });
	if (foundIdx != -1)
	{
		ASSERT_FAIL("AddSurfaceParam - %s already added\n", name);
		return;
	}

	const int id = m_physSurfaceParams.numElem();
	eqPhysSurfParam& surfParam = m_physSurfaceParams.append();
	surfParam.id = id;
	surfParam.name = name;
	kvSection.Get("collideMask").GetValues(surfParam.collideMask);
	kvSection.Get("contents").GetValues(surfParam.contents);
	kvSection.Get("friction").GetValues(surfParam.friction);
	kvSection.Get("restitution").GetValues(surfParam.restitution);
	kvSection.Get("tirefriction").GetValues(surfParam.tirefriction);
	kvSection.Get("tirefriction_traction").GetValues(surfParam.tirefriction_traction);
	surfParam.word = *KV_GetValueString(kvSection.FindSection("surfaceword"), 0, "C");
}

const int CEqPhysicsWorld::FindSurfaceParamID(const char* name) const
{
	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
	{
		if (!m_physSurfaceParams[i].name.CompareCaseIns(name))
			return i;
	}
	return -1;
}

const eqPhysSurfParam* CEqPhysicsWorld::FindSurfaceParam(const char* name) const
{
	const int surfParamId = FindSurfaceParamID(name);
	if (surfParamId == -1)
		return nullptr;

	return &m_physSurfaceParams[surfParamId];
}

const eqPhysSurfParam* CEqPhysicsWorld::GetSurfaceParamByID(int id) const
{
	if (id == -1)
		return nullptr;

	return &m_physSurfaceParams[id];
}

#ifdef DEBUG
#define CHECK_ALREADY_IN_LIST(list, obj) ASSERT_MSG(arrayFindIndex(list, obj) == -1, "Object already added")
#else
#define CHECK_ALREADY_IN_LIST(list, obj)
#endif

void CEqPhysicsWorld::AddToMoveableList( CEqRigidBody* body )
{
	if(!body)
		return;	
	
	if (body->m_flags & BODY_MOVEABLE)
		return;

	body->m_flags |= BODY_MOVEABLE;

	CHECK_ALREADY_IN_LIST(m_moveable, body);
	m_moveable.append( body );

	if(body->m_callbacks)
		body->m_callbacks->OnStartMove();
}

void CEqPhysicsWorld::RemoveFromMoveableList(CEqRigidBody* body)
{
	if (!(body->m_flags & BODY_MOVEABLE))
		return;

	body->m_flags &= ~BODY_MOVEABLE;
	m_moveable.fastRemove(body);

	if (body->m_callbacks)
		body->m_callbacks->OnStopMove();
}

void CEqPhysicsWorld::AddBody( CEqRigidBody* body, bool moveable )
{
	if(!body)
		return;

	CHECK_ALREADY_IN_LIST(m_dynObjects, body);

	body->m_flags |= COLLOBJ_BOUNDBOX_DIRTY;

	m_dynObjects.append(body);

	if(moveable)
		AddToMoveableList( body );
	else
		SetupCollisionObjectBroadphase( body );
}

bool CEqPhysicsWorld::RemoveBody( CEqRigidBody* body )
{
	if(!body)
		return false;

	if (body->m_broadphaseUnit)
	{
		Threading::CScopedWriteLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->DestroyUnit(body->m_broadphaseUnit);
	}
	body->m_broadphaseUnit = nullptr;

	const bool result = m_dynObjects.fastRemove(body);
	if (result)
		RemoveFromMoveableList(body);

	return result;
}

void CEqPhysicsWorld::AddGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	// add extra flags to objects
	object->m_flags = COLLOBJ_ISGHOST | COLLOBJ_DISABLE_RESPONSE | COLLOBJ_NO_RAYCAST | COLLOBJ_BROADPHASE_DIRTY;

	if (!object->m_callbacks)
		object->m_flags |= COLLOBJ_COLLISIONLIST;

	m_ghostObjects.append(object);
	SetupCollisionObjectBroadphase(object);
}

bool CEqPhysicsWorld::RemoveGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return false;

	if (object->m_broadphaseUnit)
	{
		Threading::CScopedWriteLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->DestroyUnit(object->m_broadphaseUnit);
	}
	object->m_broadphaseUnit = nullptr;

	if(!m_ghostObjects.fastRemove(object))
		return false;

	return true;
}

void CEqPhysicsWorld::AddStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	m_staticObjects.append(object);
	SetupCollisionObjectBroadphase( object );
}

bool CEqPhysicsWorld::RemoveStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return false;

	if (!m_staticObjects.fastRemove(object))
		return false;

	if (object->m_broadphaseUnit)
	{
		Threading::CScopedWriteLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->DestroyUnit(object->m_broadphaseUnit);
	}
	object->m_broadphaseUnit = nullptr;

	return true;
}

bool CEqPhysicsWorld::IsValidStaticObject( CEqCollisionObject* obj ) const
{
    if(obj->IsDynamic())
        return false;

    return arrayFindIndex(m_staticObjects, obj ) != -1;
}

bool CEqPhysicsWorld::IsValidBody( CEqCollisionObject* body ) const
{
    if(!body->IsDynamic())
        return false;

	return arrayFindIndex(m_dynObjects, static_cast<CEqRigidBody*>(body)) != -1;
}

void CEqPhysicsWorld::AddConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	m_constraints.append( constraint );
}

bool CEqPhysicsWorld::RemoveConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return false;

	return m_constraints.fastRemove( constraint );
}

void CEqPhysicsWorld::AddController( IEqPhysController* controller )
{
	if(!controller)
		return;

	m_controllers.append( controller );

	controller->AddedToWorld( this );
}

bool CEqPhysicsWorld::RemoveController( IEqPhysController* controller )
{
	if(!controller)
		return false;

	if(!m_controllers.fastRemove( controller ))
		return false;

	controller->RemovedFromWorld( this );
	return true;
}

//-----------------------------------------------------------------------------------------------

static bool collObjCheckCollisionMask( CEqCollisionObject* objA, CEqCollisionObject* objB )
{
	if((objA->GetContents() & objB->GetCollideMask()) || (objA->GetCollideMask() & objB->GetContents()))
		return true;

	return false;
}

void CEqPhysicsWorld::DetectBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt)
{
	using namespace EqBulletUtils;

	// apply filters
	if(!collObjCheckCollisionMask(bodyA, bodyB))
		return;

	// test radius between bodies
	const float lenA = lengthSqr(bodyA->GetLocalAABB().GetSize());
	const float lenB = lengthSqr(bodyB->GetLocalAABB().GetSize());

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
		if(ph_showContacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), EqString::Format("penetration depth: %f", hitDepth), 1.0f);
		}
#endif // ENABLE_DEBUG_DRAWING
	}
}

void CEqPhysicsWorld::DetectStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt)
{
	using namespace EqBulletUtils;

	if(staticObj == nullptr || bodyB == nullptr)
		return;

	if(!collObjCheckCollisionMask(staticObj, bodyB))
		return;

	if( !staticObj->GetWorldAABB().Intersects(bodyB->GetWorldAABB()))
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
		const int bodyCollMask = bodyB->GetCollideMask();

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
					if (surfParam && !(bodyContents & surfParam->collideMask) && !(bodyCollMask & surfParam->contents))
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
		if(ph_showContacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), EqString::Format("penetration depth: %f", hitDepth), 1.0f);
		}
#endif // ENABLE_DEBUG_DRAWING
	}
}

void CEqPhysicsWorld::SetupCollisionObjectBroadphase( CEqCollisionObject* collObj )
{
	if (!(collObj->m_flags & COLLOBJ_BROADPHASE_DIRTY))
		return;

	using namespace Threading;
	using namespace EqBulletUtils;

	// check body is in the world
	if (collObj->m_broadphaseUnit)
	{
		Threading::CScopedWriteLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->SetAabb(collObj->m_broadphaseUnit, collObj->GetWorldAABB());
	}
	else
	{
		if (!m_broadphase)
			return;

		Threading::CScopedWriteLocker m(s_eqPhysDynamicRWLock);
		collObj->m_broadphaseUnit = m_broadphase->CreateUnit(collObj->GetWorldAABB(), collObj);
	}

	collObj->m_flags &= ~COLLOBJ_BROADPHASE_DIRTY;
}

void CEqPhysicsWorld::IntegrateSingle(CEqRigidBody* body)
{
	using namespace Threading;

	// move object
	body->Integrate( m_fDt );
	SetupCollisionObjectBroadphase(body);
}

void CEqPhysicsWorld::DetectCollisionsSingle(CEqRigidBody* body)
{
	if (body->IsFrozen() || !body->IsCanIntegrate())
		return;

	body->UpdateBoundingBoxTransform();

	const bool disabledCollisionChecks = (body->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK);
	int objectTypeTesting = EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
	if(disabledCollisionChecks)
		objectTypeTesting = EQPHYS_FILTER_FLAG_STATICOBJECTS;

	// TODO: optimize broadphase with pairs
	// right now it is O^2
	auto broadphaseCb = [this, body](CEqCollisionObject* collObj) {
		if (collObj == body)
			return;

		if (collObj->IsDynamic())
		{
			DetectBodyCollisions(body, static_cast<CEqRigidBody*>(collObj), body->GetLastFrameTime());
		}
		else // purpose for triggers
		{
			DetectStaticVsBodyCollision(collObj, body, body->GetLastFrameTime());
		}
	};

	{
		Threading::CScopedReadLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->BoxTest(body->GetWorldAABB(), broadphaseCb, objectTypeTesting);
	}
}

void CEqPhysicsWorld::ProcessContactPair(eqContactPair& pair)
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

void CEqPhysicsWorld::SimulateStep(float deltaTime, int iteration, FNSIMULATECALLBACK preIntegrFunc)
{
	if(!m_broadphase)
		return;

	m_fDt = deltaTime;

	{
		PROF_EVENT("Constraints PreApply");
		for (IEqPhysicsConstraint* constr : m_constraints)
		{
			if (!constr->IsEnabled())
				continue;
			constr->PreApply(m_fDt);
		}
	}

	{
		PROF_EVENT("Controllers Update");
		for (IEqPhysController* contr : m_controllers)
		{
			if (!contr->IsEnabled())
				continue;
			contr->Update(m_fDt);
		}
	}
	
	m_simMovingMoveables.clear();
	m_simMovingMoveables.reserve(m_moveable.numElem());

	if(m_preSimJob)
	{
		m_preSimJob->InitSignal();
		m_preSimJob->InitJob();
		m_preSimJob->SetDeltaTime(m_fDt);
		m_preSimJob->StartJobs(m_jobMng);
		m_preSimJob->GetSignal()->Wait();
	}
	else
	{
		PROF_EVENT("Moving Bodies PreSim");
		for (CEqRigidBody* body : m_moveable)
		{
			IEqPhysCallback* callbacks = body->m_callbacks;
			if (callbacks)
				callbacks->PreSimulate(m_fDt);
		}
	}

	{
		PROF_EVENT("Moving Bodies Integrate");
		for (CEqRigidBody* body : m_moveable)
		{
			body->ClearContacts();
			IntegrateSingle(body);

			if (!body->IsFrozen())
				m_simMovingMoveables.append(body);
		}
	}

	m_fDt = deltaTime;

	if(preIntegrFunc)
		preIntegrFunc(m_fDt, iteration);

	if(m_collDetJob)
	{
		m_collDetJob->InitSignal();
		m_collDetJob->InitJob();
		m_collDetJob->StartJobs(m_jobMng);
		m_collDetJob->GetSignal()->Wait();
	}
	else
	{
		PROF_EVENT("Moving Bodies CollDet");
		for (CEqRigidBody* body : m_simMovingMoveables)
			DetectCollisionsSingle(body);
	}

	{
		PROF_EVENT("Moving Bodies Update");
		for (CEqRigidBody* body : m_simMovingMoveables)
			body->Update(m_fDt);
	}
	
	{
		PROF_EVENT("Moving Bodies Process Contact Pairs");
		for (CEqRigidBody* body : m_simMovingMoveables)
		{
			for (eqContactPair& pair : body->m_contactPairs)
				ProcessContactPair(pair);

			IEqPhysCallback* callbacks = body->m_callbacks;
			if (callbacks)
				callbacks->PostSimulate(m_fDt);
		}
	}

	{
		PROF_EVENT("Constraits apply");
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
//	TestLineCollision
//		- Casts line in the physics world
//
//----------------------------------------------------------------------------------------------------
bool CEqPhysicsWorld::TestLineCollision(
	const FVector3D& start, const FVector3D& end,
	eqCollisionInfo& coll,
	int rayMask, const eqPhysCollisionFilter* filterParams)
{
	using namespace EqBulletUtils;

	if (!m_broadphase)
		return false;

	int objectTypeTesting = EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
	if (filterParams)
		objectTypeTesting = filterParams->flags & (EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS);

	static thread_local Set<CEqCollisionObject*> skipObjects(PP_SL);
	skipObjects.clear();

	coll.fract = COM_FLT_MAX;
	float closest = coll.fract;

	BoundingBox rayBox;
	rayBox.AddVertex(start);
	rayBox.AddVertex(end);

	bool processed = false;
	auto broadphaseCb = [&](CEqCollisionObject* collObj) {
		processed = true;
		const bool isDynamic = collObj->IsDynamic();
		if (!isDynamic && !(objectTypeTesting & EQPHYS_FILTER_FLAG_STATICOBJECTS))
			return;

		if (isDynamic && !(objectTypeTesting & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS))
			return;

		const bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));
		if (!forceRaycast && (collObj->m_flags & COLLOBJ_NO_RAYCAST))
			return;

		if (!(rayMask & collObj->GetContents()))
			return;

		if (!CheckAllowContactTest(filterParams, collObj))
			return;

		if (!collObj->m_collObject)
			return;

		if (skipObjects.contains(collObj))
			return;
		skipObjects.insert(collObj);

		if (!rayBox.Intersects(collObj->GetWorldAABB()))
			return;

		eqCollisionInfo tempColl;
		if (TestLineSingleObject(collObj, start, end, rayBox, tempColl, closest, rayMask, filterParams, nullptr))
		{
			if (tempColl.fract < closest)
			{
				closest = tempColl.fract;
				coll = tempColl;
			}
		}
	};

	{
		Threading::CScopedReadLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->RayTest(start, end, broadphaseCb, objectTypeTesting);
	}

	coll.fract = min(coll.fract, 1.0f);
	return coll.fract < 1.0f;
}

//----------------------------------------------------------------------------------------------------
//
//	TestConvexSweepCollision
//		- Casts convex shape in the physics world
//
//----------------------------------------------------------------------------------------------------
bool CEqPhysicsWorld::TestConvexSweepCollision(
	const btCollisionShape* shape,
	const Quaternion& rotation,
	const FVector3D& start, const FVector3D& end,
	eqCollisionInfo& coll,
	int rayMask, 
	const eqPhysCollisionFilter* filterParams)
{
	using namespace EqBulletUtils;

	if (!m_broadphase)
		return false;

	int objectTypeTesting = EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS;
	if (filterParams)
		objectTypeTesting = filterParams->flags & (EQPHYS_FILTER_FLAG_STATICOBJECTS | EQPHYS_FILTER_FLAG_DYNAMICOBJECTS);

	static thread_local Set<CEqCollisionObject*> skipObjects(PP_SL);
	skipObjects.clear();

	SweptTestParams params;
	params.rotation = rotation;
	params.shape = shape;

	coll.fract = COM_FLT_MAX;
	float closest = coll.fract;

	btTransform startTrans;
	ConvertMatrix4ToBullet(startTrans, rotation);

	btVector3 shapeMins, shapeMaxs;
	shape->getAabb(startTrans, shapeMins, shapeMaxs);

	BoundingBox shapeBox;
	ConvertBulletToDKVectors(shapeBox.minPoint, shapeMins);
	ConvertBulletToDKVectors(shapeBox.maxPoint, shapeMaxs);

	const Vector3D sBoxSize = shapeBox.GetSize();
	BoundingBox rayBox;
	rayBox.AddVertex(start - sBoxSize);
	rayBox.AddVertex(end + sBoxSize);

	bool processed = false;
	auto broadphaseCb = [&](CEqCollisionObject* collObj) {
		processed = true;
		const bool isDynamic = collObj->IsDynamic();
		if (!isDynamic && !(objectTypeTesting & EQPHYS_FILTER_FLAG_STATICOBJECTS))
			return;

		if (isDynamic && !(objectTypeTesting & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS))
			return;

		const bool forceRaycast = (filterParams && (filterParams->flags & EQPHYS_FILTER_FLAG_FORCE_RAYCAST));
		if (!forceRaycast && (collObj->m_flags & COLLOBJ_NO_RAYCAST))
			return;

		if (!(rayMask & collObj->GetContents()))
			return;

		if (!collObj->m_collObject)
			return;

		if (!CheckAllowContactTest(filterParams, collObj))
			return;

		if (skipObjects.contains(collObj))
			return;
		skipObjects.insert(collObj);

		if (!rayBox.Intersects(collObj->GetWorldAABB()))
			return;

		eqCollisionInfo tempColl;
		if (TestConvexSweepSingleObject(collObj, start, end, rayBox, tempColl, closest, rayMask, filterParams, &params))
		{
			if (tempColl.fract < closest)
			{
				closest = tempColl.fract;
				coll = tempColl;
			}
		}
	};

	{
		btVector3 rayStart, rayEnd;
		ConvertDKToBulletVectors(rayStart, start);
		ConvertDKToBulletVectors(rayEnd, end);

		Threading::CScopedReadLocker m(s_eqPhysDynamicRWLock);
		m_broadphase->RayTest(start, end, broadphaseCb, objectTypeTesting, shapeBox);
	}

	coll.fract = min(coll.fract, 1.0f);
	return coll.fract < 1.0f;
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
				m_surfMaterialId = obj->GetSurfParamId();
		}

		return hitFraction;
	}

	int m_surfMaterialId{ -1 };
};

bool CEqPhysicsWorld::CheckAllowContactTest(const eqPhysCollisionFilter* filterParams, const CEqCollisionObject* object)
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


bool CEqPhysicsWorld::TestLineSingleObject(
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
	using namespace EqBulletUtils;

	if(!object)
		return false;

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
			if (surfParam && (surfParam->collideMask & rayMask) != rayMask)
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
				m_surfMaterialId = obj->GetSurfParamId();
		}

		return hitFraction;
	}

	int m_surfMaterialId{ -1 };
};

//-------------------------------------------------------------------------------------------------

bool CEqPhysicsWorld::TestConvexSweepSingleObject(CEqCollisionObject* object,
												const FVector3D& start,
												const FVector3D& end,
												const BoundingBox& raybox,
												eqCollisionInfo& coll,
												float closestHit,
												int rayMask,
												const eqPhysCollisionFilter* filterParams,
												void* args)
{
	using namespace EqBulletUtils;

	const SweptTestParams& params = *reinterpret_cast<SweptTestParams*>(args);

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
			if (surfParam && (surfParam->collideMask & rayMask) != rayMask)
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

void CEqPhysicsWorld::DebugDrawBodies(int mode)
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
				body->GetLocalAABB().minPoint, body->GetLocalAABB().maxPoint, body->GetPosition(), body->GetOrientation(), bodyCol);
		}

		//if (mode >= 2)
		//	m_grid->DebugRender();

		if (mode >= 3)
		{
			for (CEqCollisionObject* obj: m_staticObjects)
			{
				debugoverlay->OrientedBox3D(
					obj->GetLocalAABB().minPoint, obj->GetLocalAABB().maxPoint, obj->GetPosition(), obj->GetOrientation(), ColorRGBA(1, 1, 0.2, 0.8f));
			}
		}
	}
	else if (mode == 5)	// only grid
	{
		//m_grid->DebugRender();
	}
	else
	{
		for (CEqRigidBody* body: m_dynObjects)
		{
			ColorRGBA bodyCol(0.2, 1, 1, 1.0f);

			if (body->IsFrozen())
				bodyCol = ColorRGBA(0.2, 1, 0.1f, 1.0f);

			debugoverlay->Box3D(body->GetWorldAABB().minPoint, body->GetWorldAABB().maxPoint, bodyCol, 0.0f);
		}
	}
#endif // ENABLE_DEBUG_DRAWING
}
