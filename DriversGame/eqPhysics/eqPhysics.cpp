//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium fixed point 3D physics engine
//
//	FEATURES:
//				Fixed point object positions
//				Best works with fixed timestep
//				Simple dynamics build from ground up
//				Raycasting along with sweep collision test
//				Using Bullet Collision Library for fast collision detection
//
//////////////////////////////////////////////////////////////////////////////////

#include "IDebugOverlay.h"
#include "ConVar.h"
#include "DebugInterface.h"
#include "utils/KeyValues.h"

#include "eqPhysics.h"
#include "eqPhysics_Body.h"
#include "eqPhysics_Contstraint.h"
#include "eqPhysics_Controller.h"

#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"

#include "eqBulletIndexedMesh.h"

#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"

#include "Shiny.h"

#include "../shared_engine/physics/BulletConvert.h"

using namespace EqBulletUtils;
using namespace Threading;

#define PHYSGRID_WORLD_SIZE 24	// compromised betwen memory usage and performance

extern ConVar ph_margin;

ConVar ph_showcontacts("ph_showcontacts", "0", NULL, CV_CHEAT);
ConVar ph_erp("ph_erp", "0.015", "Error correction", CV_CHEAT);

// cvar value mostly depends on velocity
ConVar ph_grid_tolerance("ph_grid_tolerance", "0.05", NULL, CV_CHEAT);

const int collisionList_Max = 16;

CEqCollisionObject* CollisionPairData_t::GetOppositeTo(CEqCollisionObject* obj)
{
	return (obj == bodyA) ? bodyB : bodyA;
}

//------------------------------------------------------------------------------------------------------------


static int btInternalGetHash(int partId, int triangleIndex)
{
	int hash = (partId<<(31-MAX_NUM_PARTS_IN_BITS)) | triangleIndex;
	return hash;
}

/// Adjusts collision for using single side, ignoring internal triangle edges
/// If this info map is missing, or the triangle is not store in this map, nothing will be done
void AdjustSingleSidedContact(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap)
{
	//btAssert(colObj0->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE);
	if (colObj0Wrap->getCollisionShape()->getShapeType() != TRIANGLE_SHAPE_PROXYTYPE)
		return;

	btBvhTriangleMeshShape* trimesh = 0;

	if( colObj0Wrap->getCollisionObject()->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE )
	   trimesh = ((btScaledBvhTriangleMeshShape*)colObj0Wrap->getCollisionObject()->getCollisionShape())->getChildShape();
	else
	   trimesh = (btBvhTriangleMeshShape*)colObj0Wrap->getCollisionObject()->getCollisionShape();

	const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(colObj0Wrap->getCollisionShape());

	btVector3 tri_normal;
	tri_shape->calcNormal(tri_normal);

	btVector3 newNormal = colObj0Wrap->getCollisionObject()->getWorldTransform().getBasis()*tri_normal;
	cp.m_normalWorldOnB = colObj0Wrap->getCollisionObject()->getWorldTransform().getBasis()*tri_normal;
}

//----------------------------------------------------------------------------------------------

class EqPhysContactResultCallback : public btCollisionWorld::ContactResultCallback
{
public:
	EqPhysContactResultCallback(bool singleSided, const Vector3D& center) : m_center(center), m_singleSided(singleSided)
	{
	}

	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		if(m_singleSided)
			AdjustSingleSidedContact(cp, colObj1Wrap);
		else
			btAdjustInternalEdgeContacts(cp, colObj1Wrap,colObj0Wrap, partId1, index1);

		int numColls = m_collisions.numElem();
		m_collisions.setNum(numColls+1);

		CollisionData_t& data = m_collisions[numColls];

		data.fract = cp.getDistance();
		data.normal = ConvertBulletToDKVectors(cp.m_normalWorldOnB);
		data.position = ConvertBulletToDKVectors(cp.m_positionWorldOnA) - m_center;
		data.materialIndex = -1;

		if ( colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE )
		{
			CEqCollisionObject* obj = (CEqCollisionObject*)colObj1Wrap->getCollisionObject()->getUserPointer();

			if(obj && obj->GetMesh())
			{
				CEqBulletIndexedMesh* mesh = (CEqBulletIndexedMesh*)obj->GetMesh();
				data.materialIndex = mesh->getSubPartMaterialId(partId1);
			}
		}

		return 1.0f;
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

void DefaultSurfaceParams(eqPhysSurfParam_t* param)
{
	param->id = -1;

	param->name = "invalid";
	param->word = 'C';

	param->friction = 12.9;
	param->restitution = 0.1;
	param->tirefriction = 0.2;
	param->tirefriction_traction = 1.0;
}

void InitSurfaceParams( DkList<eqPhysSurfParam_t*>& list )
{
	// parse physics file
	KeyValues kvs;
	if (kvs.LoadFromFile("scripts/physics_surface_params.txt"))
	{
		for (int i = 0; i < kvs.GetRootSection()->keys.numElem(); i++)
		{
			kvkeybase_t* pSec = kvs.GetRootSection()->keys[i];

			if (stricmp(pSec->name, "#include"))
			{
				eqPhysSurfParam_t* pMaterial = new eqPhysSurfParam_t;
				DefaultSurfaceParams(pMaterial);

				pMaterial->name = pSec->name;

				int mat_idx = list.append(pMaterial);

				kvkeybase_t* pPair = pSec->FindKeyBase("friction");
				if (pPair)
					pMaterial->friction = KV_GetValueFloat(pPair, 0, 1.0f);

				pPair = pSec->FindKeyBase("restitution");
				if (pPair)
					pMaterial->restitution = KV_GetValueFloat(pPair, 0, 1.0f);

				pPair = pSec->FindKeyBase("tirefriction");
				if (pPair)
					pMaterial->tirefriction = KV_GetValueFloat(pPair, 0, 1.0f);

				pPair = pSec->FindKeyBase("tirefriction_traction");
				if (pPair)
					pMaterial->tirefriction_traction = KV_GetValueFloat(pPair, 0, 1.0f);

				pPair = pSec->FindKeyBase("surfaceword");
				if (pPair)
					pMaterial->word = KV_GetValueString(pPair)[0];

				pMaterial->id = mat_idx;

			}
		}
	}
}

void CEqPhysics::InitWorld()
{
	// collision configuration contains default setup for memory, collision setup
	m_collConfig = new btDefaultCollisionConfiguration();

	// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_collDispatcher = new btCollisionDispatcher( m_collConfig );
	m_collDispatcher->setDispatcherFlags( btCollisionDispatcher::CD_DISABLE_CONTACTPOOL_DYNAMIC_ALLOCATION );

	// broadphase not required
	m_collisionWorld = new btCollisionWorld(m_collDispatcher, NULL, m_collConfig);

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

	if(m_collisionWorld)
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
	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
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

	Threading::CScopedMutex m(m_mutex);

	CHECK_ALREADY_IN_LIST(m_moveable, body);

	m_moveable.append( body );
}

void CEqPhysics::AddToWorld( CEqRigidBody* body, bool moveable )
{
	if(!body)
		return;

	Threading::CScopedMutex m(m_mutex);

	CHECK_ALREADY_IN_LIST(m_dynObjects, body);

	m_dynObjects.append(body);

	if(moveable)
		AddToMoveableList( body );
	else
		SetupBodyOnCell( body );
}

void CEqPhysics::RemoveFromWorld( CEqRigidBody* body )
{
	if(!body)
		return;

	Threading::CScopedMutex m(m_mutex);

	m_dynObjects.fastRemove(body);
	m_moveable.fastRemove(body);
}

void CEqPhysics::DestroyBody( CEqRigidBody* body )
{
	if(!body)
		return;

	Threading::CScopedMutex m(m_mutex);

	collgridcell_t* cell = body->GetCell();

	if(cell)
		cell->m_dynamicObjs.fastRemove(body);

	m_dynObjects.fastRemove(body);
	m_moveable.fastRemove(body);
	delete body;
}

void CEqPhysics::AddGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

	// add extra flags to objects
	object->m_flags = COLLOBJ_ISGHOST | COLLOBJ_COLLISIONLIST | COLLOBJ_DISABLE_RESPONSE | COLLOBJ_NO_RAYCAST;

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

	Threading::CScopedMutex m(m_mutex);

	if(m_grid.IsInit())
	{
		if(object->GetMesh() != NULL)
		{
			m_grid.RemoveStaticObjectFromGrid(object);
		}
		else
		{
			collgridcell_t* cell = object->GetCell();

			if(cell)
				cell->m_dynamicObjs.fastRemove(object);
		}
	}

	m_ghostObjects.fastRemove(object);
	delete object;
}

void CEqPhysics::AddStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

	m_staticObjects.append(object);

	if(m_grid.IsInit())
		m_grid.AddStaticObjectToGrid( object );
}

void CEqPhysics::RemoveStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	if(m_grid.IsInit())
		m_grid.RemoveStaticObjectFromGrid(object);

	if(!m_staticObjects.remove(object))
		MsgError("CEqPhysics::RemoveStaticObject - INVALID\n");
}

void CEqPhysics::DestroyStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

	if(m_grid.IsInit())
		m_grid.RemoveStaticObjectFromGrid(object);

	if(m_staticObjects.remove(object))
		delete object;
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

	Threading::CScopedMutex m(m_mutex);
	m_constraints.append( constraint );
}

void CEqPhysics::RemoveConstraint( IEqPhysicsConstraint* constraint )
{
	if(!constraint)
		return;

	Threading::CScopedMutex m(m_mutex);
	m_constraints.fastRemove( constraint );
}

void CEqPhysics::AddController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	Threading::CScopedMutex m(m_mutex);
	m_controllers.append( controller );

	controller->AddedToWorld( this );
}

void CEqPhysics::RemoveController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	Threading::CScopedMutex m(m_mutex);
	m_controllers.fastRemove( controller );

	controller->RemovedFromWorld( this );
}

void CEqPhysics::DestroyController( IEqPhysicsController* controller )
{
	if(!controller)
		return;

	Threading::CScopedMutex m(m_mutex);

	if(m_controllers.remove(controller))
	{
		controller->RemovedFromWorld( this );
		delete controller;
	}
		
	else
		MsgError("CEqPhysics::DestroyController - INVALID\n");
}

//-----------------------------------------------------------------------------------------------

void CEqPhysics::SolveBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt)
{
	PROFILE_FUNC();

	if(!bodyA->CheckCanCollideWith(bodyB))
		return;

	// don't waste my time
	if(	(bodyA->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK) &&
		!(bodyB->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK))
		return;

	// test radius between bodies
	float lenA = lengthSqr(bodyA->m_aabb.GetSize());
	float lenB = lengthSqr(bodyB->m_aabb.GetSize());

#pragma todo("SolveBodyCollisions - add speed vector for more 'swept' broadphase detection")

	FVector3D center = (bodyA->GetPosition()-bodyB->GetPosition());

	float distBetweenObjects = lengthSqr(center);

	// yep, center is a length also...
	if(distBetweenObjects > lenA+lenB)
		return;

	DkList<ContactPair_t>& pairs = bodyA->m_contactPairs;

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

	PROFILE_BEGIN(shapeOperations);

	// prepare for testing...
	btCollisionObject* objA = bodyA->m_collObject;
	btCollisionObject* objB = bodyB->m_collObject;

	btBoxShape boxShapeA(ConvertDKToBulletVectors(bodyA->m_aabb.GetSize()*0.5f));
	boxShapeA.setMargin(ph_margin.GetFloat());

	btBoxShape boxShapeB(ConvertDKToBulletVectors(bodyB->m_aabb.GetSize()*0.5f));
	boxShapeB.setMargin(ph_margin.GetFloat());

	btCollisionObject boxObjectA;
	boxObjectA.setCollisionShape(&boxShapeA);

	btCollisionObject boxObjectB;
	boxObjectB.setCollisionShape(&boxShapeB);

	PROFILE_END();

	if(bodyA->m_flags & BODY_BOXVSDYNAMIC)
		objA = &boxObjectA;

	if(bodyB->m_flags & BODY_BOXVSDYNAMIC)
		objB = &boxObjectB;

	PROFILE_BEGIN(matrixOperations);

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

	btTransform transA = ConvertMatrix4ToBullet(eqTransA);
	btTransform transB = ConvertMatrix4ToBullet(eqTransB);

	objA->setWorldTransform(transA);
	objB->setWorldTransform(transB);

	PROFILE_END();

	EqPhysContactResultCallback cbResult(true,center);

	PROFILE_BEGIN(contactPairTest);
	m_collisionWorld->contactPairTest(objA, objB, cbResult);
	PROFILE_END();

	int numCollResults = cbResult.m_collisions.numElem();

	float iter_delta = 1.0f / numCollResults;

	for(int j = 0; j < numCollResults; j++)
	{
		CollisionData_t& coll = cbResult.m_collisions[j];

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		if(hitDepth < 0 && !(bodyA->m_flags & COLLOBJ_ISGHOST))
			continue;

		int idx = pairs.append(ContactPair_t());

		ContactPair_t& newPair = pairs[idx];
		newPair.normal = hitNormal;
		newPair.flags = 0;
		newPair.depth = hitDepth;
		newPair.position = hitPos;
		newPair.bodyA = bodyA;
		newPair.bodyB = bodyB;
		newPair.dt = iter_delta;

		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), 0.0f, "penetration depth: %f", hitDepth);
		}
	}
}

void CEqPhysics::SolveStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt, DkList<ContactPair_t>& contactPairs)
{
	PROFILE_FUNC();

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

	PROFILE_BEGIN(matrixOperations);

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

	btTransform transA = ConvertMatrix4ToBullet(eqTransA);
	btTransform transB = ConvertMatrix4ToBullet(eqTransB_orig);
	//btTransform transB_vel = ConvertMatrix4ToBullet(eqTransB_vel);

	objA->setWorldTransform(transA);
	objB->setWorldTransform(transB);

	PROFILE_END();

	EqPhysContactResultCallback	cbResult(/*(bodyB->m_flags & BODY_ISCAR)*/true, center);

	PROFILE_BEGIN(contactPairTest);
	m_collisionWorld->contactPairTest(objA, objB, cbResult);
	PROFILE_END();

	int numCollResults = cbResult.m_collisions.numElem();

	float iter_delta = 1.0f / numCollResults;

	for(int j = 0; j < numCollResults; j++)
	{
		CollisionData_t& coll = cbResult.m_collisions[j];

		Vector3D	hitNormal = coll.normal;
		float		hitDepth = -coll.fract; // so hit depth is the time
		FVector3D	hitPos = coll.position;

		// convex shapes are front faced?
		if( !staticObj->m_shape->isConcave() )
		{
			hitNormal *= -1.0f;
			hitDepth *= -1.0f;
		}

		if(hitDepth < 0 && !(staticObj->m_flags & COLLOBJ_ISGHOST))
			continue;

		if(hitDepth > 1.0f)
			hitDepth = 1.0f;

		int idx = contactPairs.append(ContactPair_t());
		ContactPair_t& newPair = contactPairs[idx];
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

		if(ph_showcontacts.GetBool())
		{
			debugoverlay->Box3D(hitPos-0.01f,hitPos+0.01f, ColorRGBA(1,1,0,0.15f), 1.0f);
			debugoverlay->Line3D(hitPos, hitPos+hitNormal, ColorRGBA(0,0,1,1), ColorRGBA(0,0,1,1), 1.0f);
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), 0.0f, "penetration depth: %f", hitDepth);
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
		if (oldCell)
			oldCell->m_dynamicObjs.fastRemove(body);

		if (newCell)
			newCell->m_dynamicObjs.append(body);

		body->SetCell(newCell);
	}
}

#define NEIGHBORCELLS_OFFS_XDX(x, f)	{x-f, x, x+f, x, x-f, x+f, x+f, x-f}
#define NEIGHBORCELLS_OFFS_YDY(y, f)	{y, y-f, y, y+f, y-f, y-f, y+f, y+f}

void CEqPhysics::IntegrateSingle(CEqRigidBody* body)
{
	PROFILE_FUNC();

	collgridcell_t* oldCell = body->GetCell();

	// move object
	body->Integrate( m_fDt );

	if(body->IsCanIntegrate(true))
	{
		// get new cell
		collgridcell_t* newCell = m_grid.GetCellAtPos( body->GetPosition() );

		// move object in grid
		if (newCell != oldCell)
		{
			if (oldCell)
				oldCell->m_dynamicObjs.fastRemove(body);

			if (newCell)
				newCell->m_dynamicObjs.append(body);

			body->SetCell(newCell);
		}
	}

}

ConVar ph_test1("ph_test1", "0");

void CEqPhysics::DetectCollisionsSingle(CEqRigidBody* body)
{
	PROFILE_FUNC();

	// don't refresh frozen object, other will wake up us (or user)
	if (body->IsFrozen())
		return;

	// skip collision detection on iteration
	if (!body->IsCanIntegrate())
		return;

	//if(ph_test1.GetBool())
	{
		BoundingBox& aabb = body->m_aabb_transformed;

		IVector2D crMin, crMax;
		m_grid.FindBoxRange(aabb.minPoint, aabb.maxPoint, crMin, crMax, ph_grid_tolerance.GetFloat() );

		// in this range do all collision checks
		// might be slow
		for(int y = crMin.y; y < crMax.y+1; y++)
		{
			for(int x = crMin.x; x < crMax.x+1; x++)
			{
				collgridcell_t* ncell = m_grid.GetCellAt( x, y );

				if(ncell)
				{
					// check for static objects
					for (int j = 0; j < ncell->m_gridObjects.numElem(); j++)
						SolveStaticVsBodyCollision(ncell->m_gridObjects[j], body, body->GetLastFrameTime(), body->m_contactPairs);

					// check for dynamic objects in cell
					for (int j = 0; j < ncell->m_dynamicObjs.numElem(); j++)
					{
						CEqCollisionObject* collObj = ncell->m_dynamicObjs[j];

						if (collObj == body)
							continue;

						if(collObj->IsDynamic())
							SolveBodyCollisions(body, (CEqRigidBody*)ncell->m_dynamicObjs[j], body->GetLastFrameTime());
						else
							SolveStaticVsBodyCollision(ncell->m_dynamicObjs[j], body, body->GetLastFrameTime(), body->m_contactPairs);
					}
				}
			}
		}
	}
}

ConVar ph_carVsCarErp("ph_carVsCarErp", "0.15", "Car versus car erp", CV_CHEAT);

void CEqPhysics::ProcessContactPair(const ContactPair_t& pair)
{
	PROFILE_FUNC();

	CEqRigidBody* bodyB = (CEqRigidBody*)pair.bodyB;

	float appliedImpulse = 0.0f;
	float impactVelocity = 0.0f;

	bool bodyADisableResponse = false;

	float combinedErp = ph_erp.GetFloat() + pair.bodyA->m_erp + pair.bodyB->m_erp;

	if (pair.flags & COLLPAIRFLAG_OBJECTA_STATIC)
	{
		CEqCollisionObject* bodyA = pair.bodyA;

		// correct position
		if (!(bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) && pair.depth > 0)
		{
			float positionalError = combinedErp * pair.depth / pair.dt;

			bodyB->SetPosition(bodyB->GetPosition() + pair.normal*positionalError);

			impactVelocity = fabs(dot(pair.normal, bodyB->GetVelocityAtWorldPoint(pair.position)));

			if (impactVelocity < 0)
				impactVelocity = 0.0f;

			// apply response
			appliedImpulse = CEqRigidBody::ApplyImpulseResponseTo(bodyB, pair.position, pair.normal, positionalError, pair.restitutionA, pair.frictionA);
		}

		bodyADisableResponse = (bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) > 0;
	}
	else
	{
		CEqRigidBody* bodyA = (CEqRigidBody*)pair.bodyA;

		float positionalError = combinedErp * pair.depth / pair.dt;

		bool isCarCollidingWithCar = (bodyA->m_flags & BODY_ISCAR) && (bodyB->m_flags & BODY_ISCAR);

		if (isCarCollidingWithCar)
		{
			positionalError = ph_carVsCarErp.GetFloat() * pair.depth / pair.dt;
		}

		// correct position
		if (!(bodyA->m_flags & BODY_INFINITEMASS) && !(bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE) && pair.depth > 0)
			bodyA->SetPosition(bodyA->GetPosition() + pair.normal*positionalError);

		if (!(bodyB->m_flags & BODY_INFINITEMASS) && !(bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) && pair.depth > 0)
			bodyB->SetPosition(bodyB->GetPosition() - pair.normal*positionalError);

		impactVelocity = fabs( dot(pair.normal, bodyA->GetVelocityAtWorldPoint(pair.position) - bodyB->GetVelocityAtWorldPoint(pair.position)) );

		// apply response
		appliedImpulse = 2.0f * CEqRigidBody::ApplyImpulseResponseTo2(bodyA, bodyB, pair.position, pair.normal, positionalError);

		bodyADisableResponse = (bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) > 0;
	}

	DkList<CollisionPairData_t>& pairsA = pair.bodyA->m_collisionList;
	DkList<CollisionPairData_t>& pairsB = pair.bodyB->m_collisionList;

	// All objects have collision list
	if ((pair.bodyA->m_flags & COLLOBJ_COLLISIONLIST) &&
		pairsA.numElem() < collisionList_Max)
	{
		int oldNum = pairsA.numElem();
		pairsA.setNum(oldNum+1);

		CollisionPairData_t& collData = pairsA[oldNum];
		collData.bodyA = pair.bodyA;
		collData.bodyB = pair.bodyB;
		collData.fract = pair.depth;
		collData.normal = pair.normal;
		collData.position = pair.position;
		collData.appliedImpulse = appliedImpulse; // because subtracted
		collData.impactVelocity = impactVelocity;
		collData.flags = 0;

		if (pair.bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE)
			collData.flags |= COLLPAIRFLAG_OBJECTA_NO_RESPONSE;

		if (bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE)
			collData.flags |= COLLPAIRFLAG_OBJECTB_NO_RESPONSE;
	}

	if ((bodyB->m_flags & COLLOBJ_COLLISIONLIST) &&
		pairsB.numElem() < collisionList_Max)
	{
		int oldNum = pairsB.numElem();
		pairsB.setNum(oldNum+1);

		CollisionPairData_t& collData = pairsB[oldNum];
		collData.bodyA = pair.bodyB;
		collData.bodyB = pair.bodyA;
		collData.fract = pair.depth;
		collData.normal = pair.normal;
		collData.position = pair.position;
		collData.appliedImpulse = appliedImpulse; // because subtracted
		collData.impactVelocity = impactVelocity;
		collData.flags = 0;

		if ((bodyB->m_flags & BODY_ISCAR) && !(pair.flags & COLLPAIRFLAG_OBJECTA_STATIC))
			collData.flags = COLLPAIRFLAG_NO_SOUND;

		if (bodyADisableResponse)
			collData.flags |= COLLPAIRFLAG_OBJECTB_NO_RESPONSE;

		if (bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE)
			collData.flags |= COLLPAIRFLAG_OBJECTA_NO_RESPONSE;
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

	PROFILE_FUNC();
	
	// prepare all the constraints
	for (int i = 0; i < m_constraints.numElem(); i++)
	{
		if(m_constraints[i]->IsEnabled())
		{
			m_constraints[i]->PreApply( m_fDt );
		}		
	}

	// move all bodies
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		if(body->m_callbacks)
			body->m_callbacks->PreSimulate( m_fDt );

		// clear contact pairs and results
		body->m_contactPairs.clear(false);
		body->m_collisionList.clear( false );

		IntegrateSingle(body);
	}

	// update the controllers
	for (int i = 0; i < m_controllers.numElem(); i++)
	{
		if(m_controllers[i]->IsEnabled())
		{
			m_controllers[i]->Update( m_fDt );
		}
	}

	// update all constraints
	for (int i = 0; i < m_constraints.numElem(); i++)
	{
		if(m_constraints[i]->IsEnabled())
		{
			m_constraints[i]->Apply( m_fDt );
		}
	}

	m_fDt = deltaTime;

	if(preIntegrFunc)
		preIntegrFunc(m_fDt, iteration);

	// calculate collisions
	for (int i = 0; i < m_moveable.numElem(); i++)
		DetectCollisionsSingle( m_moveable[i] );

	// process generated contact pairs
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		for (int j = 0; j < body->m_contactPairs.numElem(); j++)
			ProcessContactPair(body->m_contactPairs[j]);
	}

	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		if(body->m_callbacks)
			body->m_callbacks->PostSimulate(m_fDt);
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

	Threading::CScopedMutex m(m_mutex);

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

	// static objects are not checked if line is not in Y bound
	if(staticInBoundTest && (objectTypeTesting & 0x1))
	{
		for (int i = 0; i < cell->m_gridObjects.numElem(); i++)
		{
			CollisionData_t tempColl;

			if ((this->*func)(cell->m_gridObjects[i], start, end, rayBox, tempColl, rayMask, filterParams, args) &&
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
		// test dynamic objects within cell
		for (int i = 0; i < cell->m_dynamicObjs.numElem(); i++)
		{
			CollisionData_t tempColl;

			if ((this->*func)(cell->m_dynamicObjs[i], start, end, rayBox, tempColl, rayMask, filterParams, args) &&
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
	//PROFILE_FUNC();
	IVector2D startCell, endCell;

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
	//PROFILE_FUNC();

	coll.position = end;
	coll.fract = 32768.0f;

	sweptTestParams_t params;
	params.rotation = rotation;
	params.shape = shape;

	btTransform startTrans = ConvertMatrix4ToBullet(rotation);

	btVector3 shapeMins, shapeMaxs;
	shape->getAabb(startTrans, shapeMins, shapeMaxs);

	BoundingBox rayBox;
	BoundingBox shapeBox(ConvertBulletToDKVectors(shapeMins), ConvertBulletToDKVectors(shapeMaxs));

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
	}

	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace)
	{
		// do default result
		btScalar res = ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

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

	// NEW
	btTransform transIdent = ConvertMatrix4ToBullet(transpose(obj_mat));

	btVector3 strt = ConvertPositionToBullet(start);
	btVector3 endt = ConvertPositionToBullet(end);

	/*
	// OLD

	btTransform transIdent;
	transIdent.setIdentity();
	Matrix4x4 inv_obj_mat = !(obj_mat);

	// transform ray over object
	Vector3D ray_start = (inv_obj_mat*Vector4D(start, 1.0f)).xyz();
	Vector3D ray_end = (inv_obj_mat*Vector4D(end, 1.0f)).xyz();

	btVector3 strt = ConvertPositionToBullet(ray_start);
	btVector3 endt = ConvertPositionToBullet(ray_end);
	*/

	btTransform startTrans(btident3, strt);
	btTransform endTrans(btident3, endt);



#if BT_BULLET_VERSION >= 283 // new bullet
	btCollisionObjectWrapper objWrap(NULL, object->m_shape, object->m_collObject, transIdent, 0, 0);
#else
    btCollisionObjectWrapper objWrap(NULL, object->m_shape, object->m_collObject, transIdent);
#endif

	CEqRayTestCallback rayCallback(ConvertDKToBulletVectors(start), ConvertDKToBulletVectors(end));
	m_collisionWorld->rayTestSingleInternal( startTrans, endTrans, &objWrap, rayCallback);

	m_numRayQueries++;

	// put our result
	if(rayCallback.hasHit())
	{
		coll.fract = rayCallback.m_closestHitFraction;
		coll.position = ConvertBulletToDKVectors(rayCallback.m_hitPointWorld);
		coll.normal = ConvertBulletToDKVectors(rayCallback.m_hitNormalWorld);
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
		m_closestHitFraction = MAX_COORD_UNITS;
		m_surfMaterialId = 0;
	}

	btScalar addSingleResult(btCollisionWorld::LocalConvexResult& rayResult,bool normalInWorldSpace)
	{
		// do default result
		btScalar res = ClosestConvexResultCallback::addSingleResult(rayResult, normalInWorldSpace);

		m_surfMaterialId = -1;

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
	btTransform transIdent = ConvertMatrix4ToBullet(obj_mat);

	btVector3 strt = ConvertPositionToBullet(start);
	btVector3 endt = ConvertPositionToBullet(end);

	Matrix4x4 shapeTransform(params->rotation);

	// define start and end trasformations
	btTransform startTrans = ConvertMatrix4ToBullet(shapeTransform);
	startTrans.setOrigin(strt);

	btTransform endTrans(startTrans.getBasis(), endt);


	CEqConvexTestCallback convexCallback(strt,endt);

#pragma todo("ASSERT ON WRONG SHAPE TYPE!")

	int shapeType = params->shape->getShapeType();

	if(	shapeType > CONCAVE_SHAPES_START_HERE
		&& shapeType < CONCAVE_SHAPES_END_HERE)
	{
		ASSERTMSG(false, "Only compound shapes are supported as concave shapes!");
		return false;
	}

	btConvexShape* testShape = NULL;

	if( shapeType == COMPOUND_SHAPE_PROXYTYPE)
	{
		btCompoundShape* compShape = (btCompoundShape*)params->shape;

		testShape = (btConvexShape*)compShape->getChildShape(0);
	}
	else
		testShape = (btConvexShape*)params->shape;

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
		coll.fract = convexCallback.m_closestHitFraction;

		coll.position = ConvertBulletToDKVectors(convexCallback.m_hitPointWorld);
		coll.normal = ConvertBulletToDKVectors(convexCallback.m_hitNormalWorld);

		coll.materialIndex = convexCallback.m_surfMaterialId;
		coll.hitobject = object;

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------

void UTIL_DebugDrawOBB(const FVector3D& pos, const Vector3D& mins, const Vector3D& maxs, const Matrix4x4& matrix, const ColorRGBA& color);

void CEqPhysics::DebugDrawBodies(int mode)
{
	for(int i = 0; i < m_dynObjects.numElem(); i++)
	{
		Matrix4x4 mat(m_dynObjects[i]->GetOrientation());

		ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 0.1f);

		if(m_dynObjects[i]->IsFrozen())
			bodyCol = ColorRGBA(0.2, 1, 0.1f, 0.1f);

		UTIL_DebugDrawOBB(m_dynObjects[i]->GetPosition(),m_dynObjects[i]->m_aabb.minPoint, m_dynObjects[i]->m_aabb.maxPoint, mat, bodyCol);
	}

	if(mode >= 2)
		m_grid.DebugRender();

	if(mode >= 3)
	{
		for(int i = 0; i < m_staticObjects.numElem(); i++)
		{
			Matrix4x4 mat(m_staticObjects[i]->GetOrientation());
			UTIL_DebugDrawOBB(m_staticObjects[i]->GetPosition(),m_staticObjects[i]->m_aabb.minPoint, m_staticObjects[i]->m_aabb.maxPoint, mat, ColorRGBA(1, 1, 0.2, 0.1f));
		}
	}
}
