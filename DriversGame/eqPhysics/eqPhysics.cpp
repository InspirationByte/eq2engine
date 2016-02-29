//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium 3D fixed point physics
//////////////////////////////////////////////////////////////////////////////////

#include "IDebugOverlay.h"
#include "ConVar.h"
#include "DebugInterface.h"
#include "utils/KeyValues.h"

#include "eqPhysics.h"
#include "eqPhysics_Body.h"

#include "BulletCollision/CollisionShapes/btTriangleShape.h"
#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"

#include "eqBulletIndexedMesh.h"

//#include "eqParallelJobs.h"
#include "eqGlobalMutex.h"

#include "Shiny.h"

#include "../shared_engine/physics/BulletConvert.h"

using namespace EqBulletUtils;
using namespace Threading;

#define PHYSGRID_WORLD_SIZE 24	// compromised betwen memory usage and performance

ConVar ph_debugrender("ph_debugrender", "0", "no desc", CV_CHEAT);
ConVar ph_showcontacts("ph_showcontacts", "0", "no desc", CV_CHEAT);
ConVar ph_erp("ph_erp", "0.01", "Error correction", CV_CHEAT);
ConVar ph_centerCollisionPos("ph_centerCollisionPos", "1", "no desc", CV_CHEAT);

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
void AdjustSingleSidedContact(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap,const btCollisionObjectWrapper* colObj1Wrap, int partId0, int index0)
{
	//btAssert(colObj0->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE);
	if (colObj0Wrap->getCollisionShape()->getShapeType() != TRIANGLE_SHAPE_PROXYTYPE)
		return;

	btBvhTriangleMeshShape* trimesh = 0;

	if( colObj0Wrap->getCollisionObject()->getCollisionShape()->getShapeType() == SCALED_TRIANGLE_MESH_SHAPE_PROXYTYPE )
	   trimesh = ((btScaledBvhTriangleMeshShape*)colObj0Wrap->getCollisionObject()->getCollisionShape())->getChildShape();
	else
	   trimesh = (btBvhTriangleMeshShape*)colObj0Wrap->getCollisionObject()->getCollisionShape();

	btTriangleInfoMap* triangleInfoMapPtr = (btTriangleInfoMap*) trimesh->getTriangleInfoMap();
	if (!triangleInfoMapPtr)
		return;

	int hash = btInternalGetHash(partId0,index0);

	btTriangleInfo* info = triangleInfoMapPtr->find(hash);
	if (!info)
		return;

	const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(colObj0Wrap->getCollisionShape());

	btVector3 tri_normal;
	tri_shape->calcNormal(tri_normal);

	cp.m_normalWorldOnB = colObj0Wrap->getCollisionObject()->getWorldTransform().getBasis()*tri_normal;
}

//----------------------------------------------------------------------------------------------

class EqPhysContactResultCallback : public btCollisionWorld::ContactResultCallback
{
public:
	btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0, const btCollisionObjectWrapper* colObj1Wrap, int partId1, int index1)
	{
		AdjustSingleSidedContact(cp,colObj1Wrap,colObj0Wrap, partId1,index1);

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
				data.materialIndex = obj->GetMesh()->getSubPartMaterialId(partId1);
			}
		}

		return 1.0f;
	}

	DkList<CollisionData_t> m_collisions;
	Vector3D				m_center;
};

//------------------------------------------------------------------------------------------------------------

CEqPhysics::CEqPhysics() : m_mutex(GetGlobalMutex(MUTEXPURPOSE_PHYSICS))
{

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

#ifndef EDITOR
	m_grid.Init(this, PHYSGRID_WORLD_SIZE, Vector3D(-EQPHYS_MAX_WORLDSIZE), Vector3D(EQPHYS_MAX_WORLDSIZE));
#endif // EDITOR

	InitSurfaceParams( m_physSurfaceParams );
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

#ifndef EDITOR
	m_grid.Destroy();
#endif // EDITOR


	for (int i = 0; i < m_physSurfaceParams.numElem(); i++)
		delete m_physSurfaceParams[i];

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

#ifndef EDITOR
	if(object->GetMesh() != NULL)
	{
		m_grid.AddStaticObjectToGrid( object );
	}
	else
		SetupBodyOnCell( object );
#endif // EDITOR
}

void CEqPhysics::DestroyGhostObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

#ifndef EDITOR
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
#endif // EDITOR

	m_ghostObjects.fastRemove(object);
	delete object;
}

void CEqPhysics::AddStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

	m_staticObjects.append(object);

#ifndef EDITOR
	m_grid.AddStaticObjectToGrid( object );
#endif // EDITOR
}

void CEqPhysics::RemoveStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

#ifndef EDITOR
	m_grid.RemoveStaticObjectFromGrid(object);
#endif // EDITOR

	if(!m_staticObjects.remove(object))
		MsgError("CEqPhysics::RemoveStaticObject - INVALID\n");
}

void CEqPhysics::DestroyStaticObject( CEqCollisionObject* object )
{
	if(!object)
		return;

	Threading::CScopedMutex m(m_mutex);

#ifndef EDITOR
	m_grid.RemoveStaticObjectFromGrid(object);
#endif // EDITOR

	if(m_staticObjects.remove(object))
		delete object;
	else
		MsgError("CEqPhysics::DestroyStaticObject - INVALID\n");
}

/*
bool CEqPhysics::IsValidStaticObject( CEqCollisionObject* obj )
{

}

bool CEqPhysics::IsValidBody( CEqRigidBody* obj )
{

}
*/
extern ConVar ph_margin;

void CEqPhysics::SolveBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt, DkList<ContactPair_t>& pairs)
{
	PROFILE_FUNC();

	if(!bodyA->CheckCanCollideWith(bodyB))
		return;

	// don't waste my time
	if(	(bodyA->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK) &&
		!(bodyB->m_flags & COLLOBJ_DISABLE_COLLISION_CHECK))
		return;

	{
		// don't process collisions again
		for (int i = 0; i < pairs.numElem(); i++)
		{
			if ((pairs[i].bodyA == bodyA && pairs[i].bodyB == bodyB) ||
				(pairs[i].bodyA == bodyB && pairs[i].bodyB == bodyA))
				return;
		}
	}

	// test radius between bodies
	float lenA = lengthSqr(bodyA->m_aabb.GetSize());
	float lenB = lengthSqr(bodyB->m_aabb.GetSize());

#pragma todo("SolveBodyCollisions - add speed vector for more 'swept' broadphase detection")

	FVector3D center = (bodyA->GetPosition()-bodyB->GetPosition());

	float distBetweenObjects = lengthSqr(center);

	// yep, center is a length also...
	if(distBetweenObjects > lenA+lenB)
		return;

	//bool isCarCollidingWithCar = (bodyA->m_flags & BODY_ISCAR) && (bodyB->m_flags & BODY_ISCAR);

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

	/*
	if(isCarCollidingWithCar)
	{
		objA = &boxObjectA;
		objB = &boxObjectB;
	}*/

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

	EqPhysContactResultCallback cbResult;
	cbResult.m_center = center;

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
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), "penetration depth: %f", hitDepth);
		}

		//ProcessContactPair(newPair, fDt);
	}
}

//ConVar ph_checkMethod("ph_checkMethod", "0", "0 - sphere vs sphere, 1 - sphere vs box, 3 - box vs box");

void CEqPhysics::SolveStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt, DkList<ContactPair_t>& contactPairs)
{
	PROFILE_FUNC();

	if(staticObj == NULL || bodyB == NULL)
		return;

	if(!staticObj->CheckCanCollideWith(bodyB))
		return;

	// test radius between bodies
	//float lenB = lengthSqr(bodyB->m_aabb.GetSize());

	/*
	switch(ph_checkMethod.GetInt())
	{
		case 0: // sphere vs sphere
		{
			if( !(staticObj->m_aabb_transformed.SquaredDistPointAABB(bodyB->GetPosition()) <= lenB))
				return;
			break;
		}
		case 1: // shere vs box
		{
			if(!staticObj->m_aabb_transformed.IntersectsSphere(bodyB->GetPosition(), lenB))
				return;
			break;
		}
		case 2: // box vs box
		{
			if( !staticObj->m_aabb_transformed.Intersects(bodyB->m_aabb_transformed))
				return;
			break;
		}
	}*/

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

	EqPhysContactResultCallback cbResult;
	cbResult.m_center = center;

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
			debugoverlay->Text3D(hitPos, 50.0f, ColorRGBA(1,1,0,1), "penetration depth: %f", hitDepth);
		}

		//ProcessContactPair(newPair, fDt);
	}
}

void CEqPhysics::PrepareSimulateStep()
{

/*
	// clear collision list of ghost objects and assign new cell if changed
	for(int i = 0; i < m_ghostObjects.numElem(); i++)
	{
		CEqCollisionObject* obj = m_ghostObjects[i];

		if(obj->GetMesh() != NULL)	// probably is a station
			continue;

		// assign cells
		collgridcell_t* oldCell = obj->GetCell();

		// get new cell
		collgridcell_t* newCell = m_grid.GetCellAtPos(obj->GetPosition());

		// move object in grid
		if (newCell != oldCell)
		{
			if (oldCell)
				oldCell->m_dynamicObjs.fastRemove(obj);

			if (newCell)
				newCell->m_dynamicObjs.append(obj);

			obj->SetCell(newCell);
		}
	}
	*/
}

void CEqPhysics::SetupBodyOnCell( CEqCollisionObject* body )
{
	// check body is in the world

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

void CEqPhysics::IntegrateSingle(CEqRigidBody* body, float deltaTime)
{
	PROFILE_FUNC();

	collgridcell_t* oldCell = body->GetCell();

	// move object
	body->Integrate(deltaTime);

	if(body->IsCanIterate(true))
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

void CEqPhysics::DetectCollisionsSingle(CEqRigidBody* body, float deltaTime, DkList<ContactPair_t>& pairs)
{
	PROFILE_FUNC();

	// don't refresh frozen object, other will wake up us (or user)
	if (body->IsFrozen())
		return;

	// skip collision detection on iteration
	if (!body->IsCanIterate())
		return;

	//if(ph_test1.GetBool())
	{
		BoundingBox& aabb = body->m_aabb_transformed;

		IVector2D crMin, crMax;
		m_grid.FindBoxRange(aabb.minPoint, aabb.maxPoint, crMin, crMax, 0.0f );

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
						SolveStaticVsBodyCollision(ncell->m_gridObjects[j], body, body->GetLastFrameTime(), pairs);

					// check for dynamic objects in cell
					for (int j = 0; j < ncell->m_dynamicObjs.numElem(); j++)
					{
						CEqCollisionObject* collObj = ncell->m_dynamicObjs[j];

						if (collObj == body)
							continue;

						if(collObj->IsDynamic())
							SolveBodyCollisions(body, (CEqRigidBody*)ncell->m_dynamicObjs[j], body->GetLastFrameTime(), pairs);
						else
							SolveStaticVsBodyCollision(ncell->m_dynamicObjs[j], body, body->GetLastFrameTime(), pairs);
					}
				}
			}
		}
	}

	/*else
	{
		collgridcell_t* cell = body->GetCell();

		if (!cell)
			return;

		// check for static objects
		for (int j = 0; j < cell->m_gridObjects.numElem(); j++)
			SolveStaticVsBodyCollision(cell->m_gridObjects[j], body, body->GetLastFrameTime(), pairs);


		// check for dynamic objects in cell
		for (int j = 0; j < cell->m_dynamicObjs.numElem(); j++)
		{
			CEqCollisionObject* collObj = cell->m_dynamicObjs[j];

			if (collObj == body)
				continue;

			if(collObj->IsDynamic())
			{
				SolveBodyCollisions(body, (CEqRigidBody*)cell->m_dynamicObjs[j], body->GetLastFrameTime(), pairs);
			}
			else
			{
				SolveStaticVsBodyCollision(cell->m_dynamicObjs[j], body, body->GetLastFrameTime(), pairs);
			}
		}

		int dx[8] = NEIGHBORCELLS_OFFS_XDX(cell->x, 1);
		int dy[8] = NEIGHBORCELLS_OFFS_YDY(cell->y, 1);

		// repeat for neighbour cells
		for (int j = 0; j < 8; j++)
		{
			collgridcell_t* ncell = m_grid.GetCellAt(dx[j], dy[j]);

			if (!ncell)
				continue;

			// check for dynamic objects
			for (int k = 0; k < ncell->m_dynamicObjs.numElem(); k++)
			{
				//if (ncell->m_dynamicObjs[k] == body)
				//	continue;

				if(ncell->m_dynamicObjs[k]->IsDynamic())
				{
					SolveBodyCollisions(body, (CEqRigidBody*)ncell->m_dynamicObjs[k], body->GetLastFrameTime(), pairs);
				}
				else
				{
					SolveStaticVsBodyCollision(ncell->m_dynamicObjs[k], body, body->GetLastFrameTime(), pairs);
				}
			}
		}
	}
	*/
}

ConVar ph_carVsCarErp("ph_carVsCarErp", "0.15", "Car versus car erp", CV_CHEAT);

void CEqPhysics::ProcessContactPair(const ContactPair_t& pair, float deltaTime)
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

			impactVelocity = -dot(pair.normal, bodyB->GetVelocityAtWorldPoint(pair.position));

			if (impactVelocity < 0)
				impactVelocity = 0.0f;

			// apply response
			appliedImpulse = ApplyImpulseResponseTo(bodyB, pair.position, pair.normal, positionalError, pair.restitutionA, pair.frictionA);
		}
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

		impactVelocity = -dot(pair.normal, bodyA->GetVelocityAtWorldPoint(pair.position) - bodyB->GetVelocityAtWorldPoint(pair.position));

		if (impactVelocity < 0)
			impactVelocity = 0.0f;

		// apply response
		appliedImpulse = 2.0f*ApplyImpulseResponseTo2(bodyA, bodyB, pair.position, pair.normal, positionalError);

		bodyADisableResponse = (bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) > 0;
	}

	DkList<CollisionPairData_t>& pairsA = pair.bodyA->m_collisionList;
	DkList<CollisionPairData_t>& pairsB = pair.bodyB->m_collisionList;

	// All objects have collision list
	if ((pair.bodyA->m_flags & COLLOBJ_COLLISIONLIST) &&
		pairsA.numElem() < collisionList_Max
		/*!(bodyB->m_flags & BODY_DISABLE_RESPONSE)*/)
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

		if ((bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE))
			collData.flags |= COLLPAIRFLAG_OBJECTB_NO_RESPONSE;

		//pair.bodyA->m_collisionList.append(collData);

	}

	if ((bodyB->m_flags & COLLOBJ_COLLISIONLIST) &&
		pairsB.numElem() < collisionList_Max
		/*!bodyADisableResponse*/)
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

		//bodyB->m_collisionList.append(collData);

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

	PROFILE_FUNC();

	static DkList<ContactPair_t> contactPairs;

	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		if(body->m_callbacks)
			body->m_callbacks->PreSimulate( deltaTime );

		body->m_collisionList.clear( false );

		IntegrateSingle(body, deltaTime);
	}

	if(preIntegrFunc)
		preIntegrFunc(deltaTime, iteration);

	// calculate collisions
	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];
		DetectCollisionsSingle(body, deltaTime, contactPairs);
	}

	for (int i = 0; i < contactPairs.numElem(); i++)
		ProcessContactPair(contactPairs[i], deltaTime);

	contactPairs.clear(false);

	for (int i = 0; i < m_moveable.numElem(); i++)
	{
		CEqRigidBody* body = m_moveable[i];

		if(body->m_callbacks)
			body->m_callbacks->PostSimulate( deltaTime );
	}

	m_numRayQueries = 0;
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
	int i;               // loop counter
	int ystep, xstep;    // the step on y and x axis
	int error;           // the error accumulated during the increment
	int errorprev;       // *vision the previous value of the error variable
	int y = y1, x = x1;  // the line points
	int ddy, ddx;        // compulsory variables: the double values of dy and dx
	int dx = x2 - x1;
	int dy = y2 - y1;

	if( TestLineCollisionOnCell(y1, x1, start, end, rayBox, coll, rayMask, filterParams, func, args) )
		return;

	// NB the last point can't be here, because of its previous point (which has to be verified)
	if (dy < 0)
	{
		ystep = -1;
		dy = -dy;
	}
	else
		ystep = 1;

	if (dx < 0)
	{
		xstep = -1;
		dx = -dx;
	}
	else
		xstep = 1;

	ddy = 2 * dy;  // work with double values for full precision
	ddx = 2 * dx;

	if (ddx >= ddy)
	{
		// first octant (0 <= slope <= 1)
		// compulsory initialization (even for errorprev, needed when dx==dy)
		errorprev = error = dx;  // start in the middle of the square
		for (i=0 ; i < dx ; i++)
		{
			// do not use the first point (already done)
			x += xstep;
			error += ddy;
			if (error > ddx)
			{
				// increment y if AFTER the middle ( > )
				y += ystep;
				error -= ddx;
				// three cases (octant == right->right-top for directions below):
				if (error + errorprev < ddx)  // bottom square also
				{
					if(TestLineCollisionOnCell(y - ystep, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
				}
				else if (error + errorprev > ddx)  // left square also
				{
					TestLineCollisionOnCell(y, x - xstep, start, end, rayBox, coll, rayMask, filterParams, func, args);
					return;
				}
				else
				{  // corner: bottom and left squares also
					if(TestLineCollisionOnCell(y - ystep, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;

					if(TestLineCollisionOnCell(y, x - xstep, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
				}
			}
			if( TestLineCollisionOnCell(y, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
				return;
			errorprev = error;
		}
	}
	else
	{
		// the same as above
		errorprev = error = dy;
		for (i=0 ; i < dy ; i++)
		{
			y += ystep;
			error += ddx;
			if (error > ddy)
			{
				x += xstep;
				error -= ddy;
				if (error + errorprev < ddy)
				{
					if(TestLineCollisionOnCell(y, x - xstep, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
				}
				else if (error + errorprev > ddy)
				{
					if(TestLineCollisionOnCell(y - ystep, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
				}
				else
				{
					if(TestLineCollisionOnCell(y, x - xstep, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
					if(TestLineCollisionOnCell(y - ystep, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
						return;
				}
			}

			if(TestLineCollisionOnCell(y, x, start, end, rayBox, coll, rayMask, filterParams, func, args))
				return;

			errorprev = error;
		}
	}
	// assert ((y == y2) && (x == x2));  // the last point (y2,x2) has to be the same with the last point of the algorithm
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

	m_grid.GetPointAt(start - sBoxSize*lineDir, startCell.x, startCell.y);
	m_grid.GetPointAt(end + sBoxSize*lineDir, endCell.x, endCell.y);

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

	// NEW
	btTransform transIdent = ConvertMatrix4ToBullet(transpose(obj_mat));

	btVector3 strt = ConvertPositionToBullet(start);
	btVector3 endt = ConvertPositionToBullet(end);

	Matrix4x4 shapeTransform(params->rotation);

	/*
	btTransform transIdent;
	transIdent.setIdentity();

	Matrix4x4 inv_obj_mat = !(obj_mat);

	Matrix4x4 shapeTransform(params->rotation);
	shapeTransform = inv_obj_mat * shapeTransform;

	// transform ray over object
	Vector3D ray_start = (inv_obj_mat*Vector4D(start, 1.0f)).xyz();
	Vector3D ray_end = (inv_obj_mat*Vector4D(end, 1.0f)).xyz();

	btVector3 strt = ConvertPositionToBullet(ray_start);
	btVector3 endt = ConvertPositionToBullet(ray_end);
	*/
	//btTransform startTrans(btident3, strt);
	//btTransform endTrans(btident3, endt);


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
		// TODO: fix some tolerance
		coll.position = (obj_mat * Vector4D(ConvertBulletToDKVectors(convexCallback.m_hitPointWorld), 1.0f)).xyz();
		coll.normal = (obj_mat.getRotationComponent() * ConvertBulletToDKVectors(convexCallback.m_hitNormalWorld));
		coll.materialIndex = convexCallback.m_surfMaterialId;
		coll.hitobject = object;

		return true;
	}

	return false;
}

//--------------------------------------------------------------------------------------------------------------

void UTIL_DebugDrawOBB(const FVector3D& pos, const Vector3D& mins, const Vector3D& maxs, const Matrix4x4& matrix, const ColorRGBA& color);

void CEqPhysics::DebugDrawBodies()
{
	if(!ph_debugrender.GetBool())
		return;

	for(int i = 0; i < m_dynObjects.numElem(); i++)
	{
		Matrix4x4 mat(m_dynObjects[i]->GetOrientation());

		ColorRGBA bodyCol = ColorRGBA(0.2, 1, 1, 0.1f);

		if(m_dynObjects[i]->IsFrozen())
			bodyCol = ColorRGBA(0.2, 1, 0.1f, 0.1f);

		UTIL_DebugDrawOBB(m_dynObjects[i]->GetPosition(),m_dynObjects[i]->m_aabb.minPoint, m_dynObjects[i]->m_aabb.maxPoint, mat, bodyCol);
	}

	if(ph_debugrender.GetInt() >= 2)
		m_grid.DebugRender();

	if(ph_debugrender.GetInt() >= 3)
	{
		for(int i = 0; i < m_staticObjects.numElem(); i++)
		{
			Matrix4x4 mat(m_staticObjects[i]->GetOrientation());
			UTIL_DebugDrawOBB(m_staticObjects[i]->GetPosition(),m_staticObjects[i]->m_aabb.minPoint, m_staticObjects[i]->m_aabb.maxPoint, mat, ColorRGBA(1, 1, 0.2, 0.1f));
		}
	}
}
