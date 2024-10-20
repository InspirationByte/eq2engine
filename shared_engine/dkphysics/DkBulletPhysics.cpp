//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics powered by Bullet
//////////////////////////////////////////////////////////////////////////////////

#include <BulletCollision/CollisionDispatch/btInternalEdgeUtility.h>
#include <BulletCollision/CollisionShapes/btShapeHull.h>
#include <BulletCollision/Gimpact/btGImpactShape.h>
#include <BulletCollision/Gimpact/btGImpactCollisionAlgorithm.h>
#include <BulletSoftBody/btSoftBodyHelpers.cpp>

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"

#include "DkBulletPhysics.h"
#include "materialsystem1/IMaterial.h"
#include "materialsystem1/IMaterialVar.h"

#include "render/IDebugOverlay.h"

#include "physics/IStudioShapeCache.h"
#include "physics/PhysicsCollisionGroup.h"

#include "DkBulletObject.h"
#include "DkPhysicsJoint.h"
#include "DkPhysicsRope.h"
#include "dkphysics/physcoord.h"

#include "physics/BulletConvert.h"


using namespace Threading;
using namespace EqBulletUtils;

DECLARE_CVAR(ph_gravity,"800","World gravity",CV_CHEAT);
DECLARE_CVAR(ph_iterations,"10","Physics iterations",CV_CHEAT);
DECLARE_CVAR(ph_framerate_approx, "200", "Physics framerate approximately",CV_ARCHIVE);
DECLARE_CVAR(ph_contact_min_dist, "-0.005f", "Minimum distance/intersection for contact", CV_CHEAT);
DECLARE_CVAR(ph_erp1, "0.25f", nullptr, 0);
DECLARE_CVAR(ph_debug_serializeworld, "0", nullptr, 0);
DECLARE_CVAR(ph_drawdebug, "0", "performs wireframe rendering of convex objects", CV_CHEAT);
DECLARE_CVAR(ph_worldextramargin, "0.25", "World geometry thickness (change needs unload/reload of level)", CV_ARCHIVE);
DECLARE_CVAR(ph_shapemarginscale, "1.0f", "Shape geometry thickness", CV_ARCHIVE);
DECLARE_CVAR(ph_ccdMotionThresholdScale, "0.15f", nullptr, 0);
DECLARE_CVAR(ph_CcdSweptSphereRadiusScale, "0.5f", nullptr, 0);

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

	/*
	btTriangleInfoMap* triangleInfoMapPtr = (btTriangleInfoMap*) trimesh->getTriangleInfoMap();
	if (!triangleInfoMapPtr)
		return;

	int hash = btInternalGetHash(partId0,index0);

	btTriangleInfo* info = triangleInfoMapPtr->find(hash);
	if (!info)
		return;
	*/
	const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(colObj0Wrap->getCollisionShape());

	btVector3 tri_normal;
	tri_shape->calcNormal(tri_normal);

	btVector3 newNormal = colObj0Wrap->getCollisionObject()->getWorldTransform().getBasis()*tri_normal;

	if(cp.m_normalWorldOnB.dot(newNormal) <= 0)
		cp.m_normalWorldOnB = newNormal;
}

// single-sided collision implementation
extern ContactAddedCallback		gContactAddedCallback;

bool EQCheckNeedsCollision(btCollisionObjectWrapper* body0,btCollisionObjectWrapper* body1)
{
	IPhysicsObject* pObject1 = (IPhysicsObject*)body0->getCollisionObject()->getUserPointer();
	IPhysicsObject* pObject2 = (IPhysicsObject*)body1->getCollisionObject()->getUserPointer();

	if(pObject1 && pObject2)
	{
		if((pObject1->GetContents() & pObject2->GetCollisionMask()) || (pObject1->GetCollisionMask() & pObject2->GetContents()))
			return true;
		else
			return false;
	}

	return true;
}

void EQNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& dispatchInfo)
{
	btCollisionObject* colObj0 = (btCollisionObject*)collisionPair.m_pProxy0->m_clientObject;
	btCollisionObject* colObj1 = (btCollisionObject*)collisionPair.m_pProxy1->m_clientObject;

	btCollisionObjectWrapper w0(nullptr, colObj0->getCollisionShape(), colObj0, colObj0->getWorldTransform(), 0,0);
	btCollisionObjectWrapper w1(nullptr, colObj1->getCollisionShape(), colObj1, colObj1->getWorldTransform(), 0,0);

	if( EQCheckNeedsCollision( &w0, &w1 ) )
		btCollisionDispatcher::defaultNearCallback(collisionPair, dispatcher, dispatchInfo);
}

struct IWClosestConvexSweepResultCB : public btCollisionWorld::ClosestConvexResultCallback
{
	IWClosestConvexSweepResultCB(const btVector3& rayFromWorld,const btVector3&	rayToWorld, int nGroupFlags, IPhysicsObject** pIgnoreList, int numIgnored) 
		: btCollisionWorld::ClosestConvexResultCallback(rayFromWorld,rayToWorld)
	{
		nCurrentCheckFlags = nGroupFlags;
		m_pIgnore = pIgnoreList;
		m_nNumIgnored = numIgnored;
	}

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
		collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		btCollisionObject* pCollObject = (btCollisionObject*)proxy0->m_clientObject;
		if(pCollObject)
		{
			IPhysicsObject* pObject1 = (IPhysicsObject*)pCollObject->getUserPointer();
			if(pObject1)
			{
				if(m_pIgnore && (m_nNumIgnored > 0))
				{
					for(int i = 0; i < m_nNumIgnored;i++)
					{
						if(m_pIgnore[i] == pObject1)
							return false;
					}
				}

				if(pObject1->GetContents() & nCurrentCheckFlags && pObject1->GetCollisionMask() & nCurrentCheckFlags)
					return collides;
				else
					return false;

			}
		}

		return collides;
	}

	int nCurrentCheckFlags;

	IPhysicsObject** m_pIgnore;
	int m_nNumIgnored;
};

struct IWClosestRayResultCB : public btCollisionWorld::ClosestRayResultCallback
{
	IWClosestRayResultCB(const btVector3& rayFromWorld,const btVector3&	rayToWorld, uint nGroupFlags, IPhysicsObject** pIgnoreList, int numIgnored)
		: btCollisionWorld::ClosestRayResultCallback(rayFromWorld,rayToWorld)
	{
		nCurrentCheckFlags = nGroupFlags;
		m_pIgnore = pIgnoreList;
		m_nNumIgnored = numIgnored;
	}

	virtual bool needsCollision(btBroadphaseProxy* proxy0) const
	{
		bool collides = (proxy0->m_collisionFilterGroup & m_collisionFilterMask) != 0;
		collides = collides && (m_collisionFilterGroup & proxy0->m_collisionFilterMask);

		btCollisionObject* pCollObject = (btCollisionObject*)proxy0->m_clientObject;
		if(pCollObject)
		{
			IPhysicsObject* pObject1 = (IPhysicsObject*)pCollObject->getUserPointer();
			if(pObject1)
			{
				if(m_pIgnore && (m_nNumIgnored > 0))
				{
					for(int i = 0; i < m_nNumIgnored;i++)
					{
						if(m_pIgnore[i] == pObject1)
							return false;
					}
				}

				if(pObject1->GetContents() & nCurrentCheckFlags && pObject1->GetCollisionMask() & nCurrentCheckFlags)
					return collides;
				else
					return false;

			}
		}

		return collides;
	}

	uint nCurrentCheckFlags;

	IPhysicsObject** m_pIgnore;
	int m_nNumIgnored;
};

DkPhysics::DkPhysics() : hBox(btVector3(1,1,1)), hSphere(1), m_WorkDoneSignal(true)
{
	// 32, yet
	m_nSceneSize = 32;

	m_collisionConfiguration = nullptr;
	m_dispatcher = nullptr;
	m_broadphase = nullptr;
	m_solver = nullptr;
	m_dynamicsWorld = nullptr;

	m_fPhysicsNextTime = 0.0f;
}

DkPhysics::~DkPhysics()
{

}

// Initialize physics
bool DkPhysics::Init(int nSceneSize)
{
	// Always init the physics when new map is loading
	m_nSceneSize = nSceneSize;

	KeyValues surfParamsKvs;
	if(!surfParamsKvs.LoadFromFile("scripts/SurfaceParams.def"))
	{
		MsgError("Error! Physics surface definition file 'scripts/SurfaceParams.def' not found\n");
		CrashMsg("Error! Physics surface definition file 'scripts/SurfaceParams.def' not found, Exiting.\n");
		return false;
	}

	for(const KVSection* pSec : surfParamsKvs.Keys())
	{
		if (!pSec->name.Compare("#include"))
		{
			continue;
		}

		phySurfaceMaterial_t* pMaterial = PPNew phySurfaceMaterial_t;
		DefaultMaterialParams(pMaterial);

		pMaterial->name = pSec->name;

		KVSection* pBaseNamePair = pSec->FindSection("base");
		if(pBaseNamePair)
		{
			phySurfaceMaterial_t* param = FindMaterial( KV_GetValueString(pBaseNamePair));
			if(param)
			{
				CopyMaterialParams(param, pMaterial);
			}
			else
				DevMsg(DEVMSG_CORE, "script error: physics surface properties '%s' doesn't exist\n", KV_GetValueString(pBaseNamePair) );
		}

		pMaterial->friction = KV_GetValueFloat(pSec->FindSection("friction"), 0, 1.0f);
		pMaterial->dampening = KV_GetValueFloat(pSec->FindSection("damping"), 0, 1.0f);
		pMaterial->density = KV_GetValueFloat(pSec->FindSection("density"), 0, 100.0f);
		pMaterial->surfaceword = KV_GetValueString(pSec->FindSection("surfaceword"), 0, "C")[0];

		KVSection* pPair = pSec->FindSection("footsteps");
		if(pPair)
			pMaterial->footStepSound = KV_GetValueString(pPair);

		pPair = pSec->FindSection("bulletimpact");
		if(pPair)
			pMaterial->bulletImpactSound = KV_GetValueString(pPair);

		pPair = pSec->FindSection("scrape");
		if(pPair)
			pMaterial->scrapeSound = KV_GetValueString(pPair);

		pPair = pSec->FindSection("impactlight");
		if(pPair)
			pMaterial->lightImpactSound = KV_GetValueString(pPair);

		pPair = pSec->FindSection("impactheavy");
		if(pPair)
			pMaterial->heavyImpactSound = KV_GetValueString(pPair);

		m_physicsMaterialDesc.append(pMaterial);
	}

	return true;
}

class DkPhysicsDebugDrawer : public btIDebugDraw
{
	void drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
	{
#ifndef EQLC
		Vector3D _from, _to, col;
		ConvertPositionToEq(_from, from);
		ConvertPositionToEq(_to, to);
		ConvertBulletToDKVectors(col, color);

		debugoverlay->Line3D(_from, _to, ColorRGBA(col, 1), ColorRGBA(col, 1), 0.25f);
#endif
	}

	void drawTriangle(const btVector3& v0,const btVector3& v1,const btVector3& v2,const btVector3& /*n0*/,const btVector3& /*n1*/,const btVector3& /*n2*/,const btVector3& color, btScalar alpha)
	{
#ifndef EQLC
		Vector3D _v0, _v1, _v2, _col;
		ConvertBulletToDKVectors(_col, color);
		ColorRGBA col(_col, alpha);

		ConvertPositionToEq(_v0, v0);
		ConvertPositionToEq(_v1, v1);
		ConvertPositionToEq(_v2, v2);

		debugoverlay->Polygon3D(_v0,_v1,_v2, col);
		//drawTriangle(v0,v1,v2,color,alpha);
#endif
	}

	void drawTriangle(const btVector3& v0,const btVector3& v1,const btVector3& v2,const btVector3& color, btScalar alpha)
	{
#ifndef EQLC
		Vector3D _v0, _v1, _v2, _col;
		ConvertBulletToDKVectors(_col, color);
		ColorRGBA col(_col, alpha);

		ConvertPositionToEq(_v0, v0);
		ConvertPositionToEq(_v1, v1);
		ConvertPositionToEq(_v2, v2);

		debugoverlay->Polygon3D(_v0, _v1, _v2, col);
#endif
	}

	void reportErrorWarning(const char * warn)
	{
		DevMsg(DEVMSG_CORE, "EqPhysics warining: %s\n", warn);
	}

	void draw3dText(const btVector3 &position,const char *text)
	{
#ifndef EQLC
		Vector3D pos;
		ConvertPositionToEq(pos, position);
		debugoverlay->Text3D(pos,-1, color_white, text);
#endif
	}

	void setDebugMode(int mode)
	{

	}

	int getDebugMode() const
	{
		return btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawConstraintLimits | btIDebugDraw::DBG_DrawWireframe;
	}

	void drawContactPoint(const btVector3 &from,const btVector3 &to,btScalar cnt,int c,const btVector3 &v)
	{

	}
};

static DkPhysicsDebugDrawer s_PhysicsDebugDrawer;

btDynamicsWorld* g_pPhysicsWorld = nullptr;

bool EQContactAddedCallback(btManifoldPoint& cp,const btCollisionObjectWrapper* colObj0,int partId0,int index0,const btCollisionObjectWrapper* colObj1,int partId1,int index1)
{
	//AdjustSingleSidedContact(cp,colObj0, colObj1, partId0, index0);
	//AdjustSingleSidedContact(cp,colObj1, colObj0, partId1, index1);

	CPhysicsObject*		pObjA = (CPhysicsObject*)colObj0->getCollisionObject()->getUserPointer();
	CPhysicsObject*		pObjB = (CPhysicsObject*)colObj1->getCollisionObject()->getUserPointer();

	if(!pObjA)
		return false;

	if(pObjA->m_nFlags & PO_NO_EVENTS)
		return false;

	if(pObjA->m_nFlags & PO_BLOCKEVENTS)
		return false;

	if(pObjA && pObjB)
	{
		if (cp.getDistance() < ph_contact_min_dist.GetFloat())
		{
			// add contact to the physics object, to iterate in game dll
			pObjA->AddContactEventFromManifoldPoint(&cp, pObjB);
		}
	}

	return false;
}


// Makes physics scene
bool DkPhysics::CreateScene()
{
	m_fPhysicsNextTime = 0.0f;

	// collision configuration contains default setup for memory, collision setup
	m_collisionConfiguration = PPNew btSoftBodyRigidBodyCollisionConfiguration;

	// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
	m_dispatcher = PPNew btCollisionDispatcher(m_collisionConfiguration);

	m_dispatcher->setNearCallback(EQNearCallback);

	btVector3 worldMin(-F_INFINITY,-F_INFINITY,-F_INFINITY);
	btVector3 worldMax(F_INFINITY, F_INFINITY, F_INFINITY);

	bt32BitAxisSweep3* sweepBP = new bt32BitAxisSweep3(worldMin,worldMax);
	m_broadphase = sweepBP;

	///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
	btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
	m_solver = sol;

	m_dynamicsWorld = new btSoftRigidDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfiguration);

	m_softBodyWorldInfo.m_sparsesdf.Reset();

	m_softBodyWorldInfo.m_dispatcher = m_dispatcher;
	m_softBodyWorldInfo.m_broadphase = m_broadphase;

	m_softBodyWorldInfo.air_density		=	(btScalar)1.2;
	m_softBodyWorldInfo.water_density	=	0;
	m_softBodyWorldInfo.water_offset	=	0;
	m_softBodyWorldInfo.water_normal	=	btVector3(0,1,0);
	m_softBodyWorldInfo.m_gravity.setValue(0,-ph_gravity.GetFloat(),0);

	m_dynamicsWorld->setGravity(btVector3(0,-ph_gravity.GetFloat(),0));

	//m_dynamicsWorld->setDebugDrawer(&s_PhysicsDebugDrawer);

	m_softBodyWorldInfo.m_sparsesdf.Initialize();

	m_dynamicsWorld->getSolverInfo().m_splitImpulse = true;
	//m_dynamicsWorld->getSolverInfo().m_splitImpulsePenetrationThreshold = -1.0f;
	//m_dynamicsWorld->getSolverInfo().m_numIterations = 4;
	m_dynamicsWorld->getSolverInfo().m_solverMode |= SOLVER_ENABLE_FRICTION_DIRECTION_CACHING;
	 


	//m_dynamicsWorld->getDispatchInfo().m_enableSPU = true;
	//m_dynamicsWorld->getDispatchInfo().m_allowedCcdPenetration = 0.1f;
	m_dynamicsWorld->getDispatchInfo().m_useConvexConservativeDistanceUtil = true;
	//m_dynamicsWorld->getDispatchInfo().m_enableSatConvex = true;
	//m_dynamicsWorld->getDispatchInfo().m_useContinuous = true;

	// set physics world
	g_pPhysicsWorld = m_dynamicsWorld;

	// register GImpact shape collision algorithm, this is usually for BSP objects
	btGImpactCollisionAlgorithm::registerAlgorithm((btCollisionDispatcher*)m_dynamicsWorld->getDispatcher());

	gContactAddedCallback = EQContactAddedCallback;

	gDisableDeactivation = false;

	//m_WorkDoneSignal.Raise();

	return true;
}

// Destroy scene
void DkPhysics::DestroyScene()
{
	// do cleanup in the reverse order of creation/initialization
	DestroyPhysicsObjects();

	// remove the rigidbodies from the dynamics world and delete them
	int i;
	for (i = m_dynamicsWorld->getNumCollisionObjects()-1; i >= 0 ;i--)
	{
		btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState())
			delete body->getMotionState();

		m_dynamicsWorld->removeCollisionObject( obj );
		delete obj;
	}

	DestroyShapeCache();

	m_collisionShapes.clear();

	for (int j=0;j<m_triangleMeshes.numElem();j++)
	{
		delete m_triangleMeshes[j];
	}

	m_triangleMeshes.clear();

	if(m_dynamicsWorld)
		delete m_dynamicsWorld;

	if(m_solver)
		delete m_solver;
	
	if(m_broadphase)
		delete m_broadphase;

	if(m_dispatcher)
		delete m_dispatcher;

	if(m_collisionConfiguration)
		delete m_collisionConfiguration;

	m_collisionConfiguration = nullptr;
	m_dispatcher = nullptr;
	m_broadphase = nullptr;
	m_solver = nullptr;
	m_dynamicsWorld = nullptr;
}

// Returns true if hardware acceleration is available
bool DkPhysics::IsSupportsHardwareAcceleration()
{
	// for now there is no hardware acceleration
	return false;
}

// Generic traceLine for physics
void DkPhysics::InternalTraceLine(const Vector3D &tracestart, const Vector3D &traceend, int groupmask, internaltrace_t *trace, IPhysicsObject** pIgnoreList, int numIgnored)
{
	btVector3 strt; 
	btVector3 end;
	ConvertPositionToBullet(strt, tracestart);
	ConvertPositionToBullet(end, traceend);

	trace->origin = tracestart;
	trace->traceEnd = traceend;
	trace->normal = vec3_zero;
	trace->fraction = 1.0f;
	trace->uv = Vector2D(0);
	trace->hitObj = nullptr;
	trace->hitMaterial = nullptr;

	IWClosestRayResultCB rayCallback( strt, end, groupmask, pIgnoreList, numIgnored);
	//rayCallback.m_collisionObject

	//m_WorkDoneSignal.Wait();

	//m_Mutex.Lock();
	m_dynamicsWorld->rayTest(strt,end,rayCallback);
	//m_Mutex.Unlock();

	if(rayCallback.hasHit())
	{
		trace->hitObj = (IPhysicsObject*)rayCallback.m_collisionObject->getUserPointer();
		trace->fraction = rayCallback.m_closestHitFraction;
		ConvertPositionToEq(trace->traceEnd, rayCallback.m_hitPointWorld);
		ConvertBulletToDKVectors(trace->normal, rayCallback.m_hitNormalWorld);
		trace->hitMaterial = ((CPhysicsObject*)trace->hitObj)->m_pRMaterial;

		trace->traceEnd += trace->normal * F_EPS;
	}
}

// Generic traceLine for physics
void DkPhysics::InternalTraceBox(const Vector3D &tracestart, const Vector3D &traceend, const Vector3D& boxSize,int groupmask,internaltrace_t *trace, IPhysicsObject** pIgnoreList, int numIgnored, Matrix4x4 *externalBoxTransform)
{
	btVector3 strt;
	btVector3 end;
	ConvertPositionToBullet(strt, tracestart);
	ConvertPositionToBullet(end, traceend);

	trace->origin = tracestart;
	trace->traceEnd = traceend;
	trace->normal = vec3_zero;
	trace->fraction = 1.0f;
	trace->uv = Vector2D(0);
	trace->hitObj = nullptr;

	IWClosestConvexSweepResultCB convexCallback( strt, end, groupmask, pIgnoreList, numIgnored);

	btVector3 scaling;
	ConvertPositionToBullet(scaling, boxSize);

	hBox.setLocalScaling(scaling);

	btTransform startTr;
	startTr.setIdentity();

	if(externalBoxTransform)
		ConvertMatrix4ToBullet(startTr, *externalBoxTransform);

	startTr.setOrigin(strt);

	btTransform endTr = btTransform::getIdentity();
	if(externalBoxTransform)
		endTr.setFromOpenGLMatrix((float*)externalBoxTransform->rows);

	endTr.setOrigin(end);

	//m_Mutex.Lock();

	//m_WorkDoneSignal.Wait();

	m_dynamicsWorld->convexSweepTest(&hBox,startTr,endTr, convexCallback);

	//m_Mutex.Unlock();

	if(convexCallback.hasHit())
	{
		trace->hitObj = (IPhysicsObject*)convexCallback.m_hitCollisionObject->getUserPointer();
		trace->fraction = convexCallback.m_closestHitFraction;
		trace->traceEnd = tracestart + ((traceend - tracestart) * trace->fraction);
		ConvertBulletToDKVectors(trace->normal, convexCallback.m_hitNormalWorld);
	}
}

// Generic traceLine for physics
void DkPhysics::InternalTraceSphere(const Vector3D &tracestart, const Vector3D &traceend, float sphereRadius,int groupmask,internaltrace_t* trace, IPhysicsObject** pIgnoreList, int numIgnored)
{
	btVector3 strt;
	btVector3 end;
	ConvertPositionToBullet(strt, tracestart);
	ConvertPositionToBullet(end, traceend);

	trace->origin = tracestart;
	trace->traceEnd = traceend;
	trace->normal = vec3_zero;
	trace->fraction = 1.0f;
	trace->uv = Vector2D(0);
	trace->hitObj = nullptr;

	IWClosestConvexSweepResultCB convexCallback( strt, end, groupmask, pIgnoreList, numIgnored);

	hSphere.setLocalScaling(btVector3(EQ2BULLET(sphereRadius),EQ2BULLET(sphereRadius),EQ2BULLET(sphereRadius)));

	btTransform startTr;
	startTr.setOrigin(strt);

	btTransform endTr;
	endTr.setOrigin(end);

	//m_WorkDoneSignal.Wait();

	//m_Mutex.Lock();
	m_dynamicsWorld->convexSweepTest(&hSphere,startTr,endTr, convexCallback);
	//m_Mutex.Unlock();


	if(convexCallback.hasHit())
	{
		trace->hitObj = (IPhysicsObject*)convexCallback.m_hitCollisionObject->getUserPointer();
		trace->fraction = convexCallback.m_closestHitFraction;
		ConvertPositionToEq(trace->traceEnd, convexCallback.m_hitPointWorld);
		ConvertBulletToDKVectors(trace->normal, convexCallback.m_hitNormalWorld);
	}
}

// Generic traceLine for physics
void DkPhysics::InternalTraceShape(const Vector3D &tracestart, const Vector3D &traceend, int shapeId, int groupmask, internaltrace_t* trace, IPhysicsObject** pIgnoreList, int numIgnored, Matrix4x4* transform)
{
	ASSERT(shapeId != -1 );

	btVector3 strt;
	btVector3 end;
	ConvertPositionToBullet(strt, tracestart);
	ConvertPositionToBullet(end, traceend);

	trace->origin = tracestart;
	trace->traceEnd = traceend;
	trace->normal = vec3_zero;
	trace->fraction = 1.0f;
	trace->uv = Vector2D(0);
	trace->hitObj = nullptr;

	IWClosestConvexSweepResultCB convexCallback( strt, end, groupmask, pIgnoreList, numIgnored);

	btTransform startTr;
	startTr.setIdentity();

	if(transform)
		ConvertMatrix4ToBullet(startTr, *transform);

	startTr.setOrigin(strt);

	btTransform endTr = btTransform::getIdentity();
	if(transform)
		endTr.setFromOpenGLMatrix((float*)transform->rows);

	endTr.setOrigin(end);

	//m_WorkDoneSignal.Wait();

	//m_Mutex.Lock();
	m_dynamicsWorld->convexSweepTest( (btConvexShape*)m_collisionShapes[shapeId], startTr, endTr, convexCallback);
	//m_Mutex.Unlock();


	if(convexCallback.hasHit())
	{
		trace->hitObj = (IPhysicsObject*)convexCallback.m_hitCollisionObject->getUserPointer();
		trace->fraction = convexCallback.m_closestHitFraction;
		ConvertPositionToEq(trace->traceEnd, convexCallback.m_hitPointWorld);
		ConvertBulletToDKVectors(trace->normal, convexCallback.m_hitNormalWorld);
	}
}

// special command that enumerates every physics object and calls EACHOBJECTFUNCTION procedure for it (such as explosion, etc.)
void DkPhysics::DoForEachObject(EACHOBJECTFUNCTION procedure, void *data)
{
	for(int i = 0; i < m_pPhysicsObjectList.numElem(); i++)
	{
		CPhysicsObject* pPhysObj = (CPhysicsObject*)m_pPhysicsObjectList[i];

		if(pPhysObj->m_pPhyObjectPointer->isStaticObject())
			continue;

		// call this with specified data
		procedure(pPhysObj, data);
	}
}

// finds material by it's name
phySurfaceMaterial_t* DkPhysics::FindMaterial(const char* pszName)
{
	for(int i = 0; i < m_physicsMaterialDesc.numElem(); i++)
	{
		if(!m_physicsMaterialDesc[i]->name.CompareCaseIns(pszName))
			return m_physicsMaterialDesc[i];
	}

	DevMsg(DEVMSG_CORE, "DkPhysics::FindMaterial: %s not found\n", pszName);

	return nullptr;
}

// updates events
void DkPhysics::UpdateEvents()
{
	// remove all previous events
	for(int i = 0; i < m_pPhysicsObjectList.numElem(); i++)
	{
		CPhysicsObject* pPhysObj = (CPhysicsObject*)m_pPhysicsObjectList[i];

		pPhysObj->ClearContactEvents();
	}
}

#define PHY_STEPS ph_iterations.GetInt()
#define DKFIXED_TIMESTEP (1.0f/ph_framerate_approx.GetFloat())

// Update funciton
void DkPhysics::Simulate(float dt, int substeps)
{
#ifndef EQLC
	if(m_dynamicsWorld)
	{
		//m_Mutex.Lock();

		if(ph_debug_serializeworld.GetBool())
		{
			ph_debug_serializeworld.SetBool(false);

			int maxSerializeBufferSize = 1024*1024*25;
			
			btDefaultSerializer* serializer = new btDefaultSerializer( maxSerializeBufferSize );

			m_dynamicsWorld->serialize(serializer);

			IFilePtr file = g_fileSystem->Open("testFile.bullet", "wb", SP_ROOT);
			file->Write(serializer->getBufferPointer(),serializer->getCurrentBufferSize(),1);

			delete serializer;
		}

		m_dynamicsWorld->getSolverInfo().m_erp  = ph_erp1.GetFloat();

		m_dynamicsWorld->setGravity(btVector3(0,-ph_gravity.GetFloat(),0));
		m_softBodyWorldInfo.m_gravity.setValue(0,-ph_gravity.GetFloat(),0);
		
		float dt_div = dt/substeps;

		int nIterations = ph_iterations.GetInt();

		float fTimeMultipler = 1.0;

		//m_Mutex.Unlock();

		//float fPhysFramerate = (1.0f / ph_framerate_approx.GetFloat())*sys_timescale.GetFloat()*gpGlobals->timescale;
		//if(fPhysFramerate > dt_div)
		//	fPhysFramerate = dt_div;

		m_fPhysicsNextTime += dt;

		float fFixedTimeStep = DKFIXED_TIMESTEP*fTimeMultipler;

		if(m_fPhysicsNextTime > fFixedTimeStep)
		{
			dt_div = m_fPhysicsNextTime/substeps;

			for(int i = 0; i < substeps; i++)
			{
				//m_WorkDoneSignal.Clear();

				// Update world first
				m_dynamicsWorld->stepSimulation( dt_div, nIterations, fFixedTimeStep );

				// after each substep raise signal
				//m_WorkDoneSignal.Raise();
			}

			m_fPhysicsNextTime = 0.0f;
		}


		m_softBodyWorldInfo.m_sparsesdf.GarbageCollect();
	}
#endif
}

void DkPhysics::DrawDebug()
{
	m_Mutex.Lock();
		
	if(ph_drawdebug.GetBool())
	{
		for(int i = 0; i < m_pPhysicsObjectList.numElem(); i++)
		{
			CPhysicsObject* pPhysObj = (CPhysicsObject*)m_pPhysicsObjectList[i];

			if(pPhysObj->m_pPhyObjectPointer->isStaticObject())
				continue;

			m_dynamicsWorld->debugDrawObject(pPhysObj->m_pPhyObjectPointer->getWorldTransform(), pPhysObj->m_pPhyObjectPointer->getCollisionShape(), btVector3(0,1,1));
		}

		for(int i = 0; i < m_pJointList.numElem(); i++)
		{
			m_dynamicsWorld->debugDrawConstraint(((DkPhysicsJoint*)m_pJointList[i])->m_pJointPointer);
		}
	}
		
	m_Mutex.Unlock();
}

btRigidBody* DkPhysics::LocalCreateRigidBody(float mass, const Vector3D &massCenter, const btTransform& startTransform, btCollisionShape* shape)
{
	btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

	//rigidbody is dynamic if and only if mass is non zero, otherwise static
	bool isDynamic = (mass != 0.0f);

	btVector3 localInertia(0,0,0);
	if (isDynamic)
		shape->calculateLocalInertia(mass, localInertia);

	btTransform mass_center_transform;
	btVector3 btMassCenter;
	ConvertPositionToBullet(btMassCenter, massCenter);
	mass_center_transform.setIdentity();
	mass_center_transform.setOrigin(btMassCenter);
	mass_center_transform.inverse();

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);

	btRigidBody* body = new btRigidBody(cInfo);

	if(!isDynamic)
	{
		body->setCenterOfMassTransform(mass_center_transform);
		body->setCollisionFlags(btCollisionObject::CF_STATIC_OBJECT);
		body->setAngularFactor(0);
	}

	body->setWorldTransform(startTransform);

	//m_dynamicsWorld->addRigidBody(body);
	//m_dynamicsWorld->addCollisionObject(

	return body;
}

btCollisionShape* InternalGenerateMesh(physmodelcreateinfo_t *info, Array <btTriangleIndexVertexArray*> *triangleMeshes)
{
	if(!info->genConvex)
	{
		btTriangleIndexVertexArray* pTriMesh = new btTriangleIndexVertexArray(
			(int)info->data->numIndices/3, (int*)info->data->indices, sizeof(int)*3,
			info->data->numVertices, (float*)info->data->vertices, sizeof(Vector3D));

		triangleMeshes->append( pTriMesh );

		return new btBvhTriangleMeshShape(pTriMesh, false);
	}
	else
	{
		const dkCollideData_t* cdata = info->data;

		if(info->convexMargin != -1)
		{
			btTriangleIndexVertexArray* pTriMesh = new btTriangleIndexVertexArray(
				(int)cdata->numIndices/3, (int*)cdata->indices, sizeof(int)*3,
				cdata->numVertices, (float*)cdata->vertices, sizeof(Vector3D));

			btConvexTriangleMeshShape tmpConvexShape(pTriMesh);

			//create a hull approximation
			btShapeHull hull(&tmpConvexShape);
			

			btScalar margin = (info->convexMargin == -2.0f) ? tmpConvexShape.getMargin() : info->convexMargin;
			hull.buildHull( EQ2BULLET(margin) );

			tmpConvexShape.setUserPointer(&hull);

			// TODO: PhysGen code
			btConvexHullShape* convexShape = new btConvexHullShape();

			for (int i = 0; i < hull.numIndices(); i++)
			{
				uint32 index = hull.getIndexPointer()[i];
				convexShape->addPoint(hull.getVertexPointer()[index]);
			}

			delete pTriMesh;

			return convexShape;
		}
		else
		{
			btConvexHullShape* convexShape = new btConvexHullShape;

			for(int i = 0; i < cdata->numVertices; i++)
			{
				btVector3 pos;
				ubyte* vertexPtr = (ubyte*)cdata->vertices + cdata->vertexPosOffset;
				ConvertPositionToBullet(pos, *(Vector3D*)(vertexPtr + cdata->vertexSize * i));

				convexShape->addPoint(pos, false);
			}

			convexShape->recalcLocalAabb();

			return convexShape;
		}
	}

	return nullptr;
}

btCollisionShape* InternalPrimitiveCreate(pritimiveinfo_t *priminfo)
{
	if(priminfo->primType == PHYSPRIM_BOX)
	{
		btVector3 vec;
		ConvertPositionToBullet(vec, *(Vector3D*)&priminfo->boxInfo);

		btCollisionShape* shape = new btBoxShape(vec);
		return shape;
	}
	else if(priminfo->primType == PHYSPRIM_SPHERE)
	{
		btCollisionShape* shape = new btSphereShape(EQ2BULLET(priminfo->sphereRadius));
		return shape;
	}
	else if(priminfo->primType == PHYSPRIM_CAPSULE)
	{
		btCollisionShape* shape = new btCapsuleShape(EQ2BULLET(priminfo->capsuleInfo.radius),EQ2BULLET(priminfo->capsuleInfo.height));
		return shape;
	}
	else
	{
		MsgError("Invalid primitive type (%d)\n", priminfo->primType);
	}

	return nullptr;
}
/*
// creates compound object
IPhysicsObject* DkPhysics::CreateCompoundObject(physmodelcreateinfo_t *info, pritimiveinfo_t **priminfos, int numObjects, int nCollisionGroupFlags)
{
	if(priminfos == nullptr)
	{
		MsgError("Can't create compound physics object without any shape!\n");
		return nullptr;
	}

	btCompoundShape* compound_shape = DNew(btCompoundShape);

	m_collisionShapes.append(compound_shape);

	for(int i = 0; i < numObjects; i++)
	{
		btCollisionShape* shape = InternalPrimitiveCreate(priminfos[i]);

		btTransform trans;
		trans.setIdentity();

		compound_shape->addChildShape(trans, shape);

		m_collisionShapes.append(shape);
	}

	btTransform startTransform;
	startTransform.setIdentity();

	CPhysicsObject* pPhysicsObject = DNew(CPhysicsObject);
	pPhysicsObject->m_pPhyObjectPointer = (btRigidBody*)LocalCreateRigidBody(info->mass, info->mass_center, startTransform, compound_shape);

	char* materialName = "default";

	phySurfaceMaterial_t* material = FindMaterial(materialName);

	if(!material)
	{
		MsgError("Invalid physics material '%s'!\n", materialName);
		materialName = "default";

		material = FindMaterial(materialName);
	}

	pPhysicsObject->m_pPhysMaterial = material;

	// disable simulation for first frames
	pPhysicsObject->m_pPhyObjectPointer->setActivationState(ISLAND_SLEEPING);

	m_dynamicsWorld->addRigidBody(pPhysicsObject->m_pPhyObjectPointer);

	pPhysicsObject->m_pPhyObjectPointer->setUserPointer(pPhysicsObject);

	pPhysicsObject->SetContents(nCollisionGroupFlags);

	DevMsg(2, "Creating compound primitived object\n" );

	m_pPhysicsObjectList.append(pPhysicsObject);

	return pPhysicsObject;
}
*/

// Primitives
IPhysicsObject* DkPhysics::CreateStaticObject(physmodelcreateinfo_t *info, int nCollisionGroup)
{
	int nCollideMask = COLLISION_GROUP_ALL;

	if(info->data && info->data->pMaterial)
	{
		MatIntProxy mv_nocollide = info->data->pMaterial->FindMaterialVar("nocollide");

		if(mv_nocollide.Get() > 0)
			return nullptr;

		MatIntProxy mv_clip = info->data->pMaterial->FindMaterialVar("playerclip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_PLAYERCLIP;
			nCollideMask = COLLISION_GROUP_PLAYER;
		}

		mv_clip = info->data->pMaterial->FindMaterialVar("npcclip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_NPCCLIP;
			nCollideMask = COLLISION_GROUP_ACTORS;
		}

		mv_clip = info->data->pMaterial->FindMaterialVar("physclip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_PHYSCLIP;
			nCollideMask = COLLISION_GROUP_OBJECTS | COLLISION_GROUP_DEBRIS;
		}

		if(info->data->pMaterial->GetFlags() & MATERIAL_FLAG_WATER)
		{
			nCollisionGroup = COLLISION_GROUP_WATER;
			nCollideMask = COLLISION_GROUP_PROJECTILES;
		}
		else if(info->data->pMaterial->GetFlags() & MATERIAL_FLAG_SKY)
		{
			nCollisionGroup = COLLISION_GROUP_SKYBOX;
			nCollideMask = 0;
		}
	}

	btCollisionShape* shape = nullptr;

	Array<btTriangleIndexVertexArray*> triangle_mesges(PP_SL);

	shape = InternalGenerateMesh(info, &m_triangleMeshes);

	/*
		m_trimap = new btTriangleInfoMap();
		//now you can adjust some thresholds in triangleInfoMap  if needed.

		btGenerateInternalEdgeInfo((btBvhTriangleMeshShape*)m_shape, m_trimap);
	*/

	if(!shape)
	{
		MsgError("Can't create physics object without shape!\n");
		return nullptr;
	}

	//Msg("World geom margin: %g\n", shape->getMargin());

	shape->setMargin(EQ2BULLET(ph_worldextramargin.GetFloat()));

	btTransform startTransform;
	startTransform.setIdentity();

	CPhysicsObject* pPhysicsObject = PPNew CPhysicsObject;

	CScopedMutex m(m_Mutex);

	m_collisionShapes.append(shape);

	pPhysicsObject->m_pPhyObjectPointer = (btRigidBody*)LocalCreateRigidBody(info->mass, info->massCenter,startTransform, shape);

	const char* materialName = "default";

	if(info->data)
	{
		if(info->data->surfaceprops != nullptr)
		{
			materialName = info->data->surfaceprops;
		}
		else
		{
			MatStringProxy surfacePropsVar = info->data->pMaterial->FindMaterialVar("surfaceprops");

			if(surfacePropsVar.IsValid())
				materialName = surfacePropsVar.Get();
		}

		pPhysicsObject->m_pRMaterial = info->data->pMaterial;
	}

	phySurfaceMaterial_t* material = FindMaterial(materialName);
	if(!material)
	{
		MsgError("Invalid physics material '%s'!\n", materialName);
		materialName = "default";
		material = FindMaterial(materialName);
	}

	pPhysicsObject->m_pPhysMaterial = material;

	// disable simulation for first frames
	pPhysicsObject->m_pPhyObjectPointer->setActivationState( ISLAND_SLEEPING );

	m_dynamicsWorld->addRigidBody(pPhysicsObject->m_pPhyObjectPointer);
	pPhysicsObject->m_pPhyObjectPointer->setUserPointer(pPhysicsObject);
	pPhysicsObject->SetContents( nCollisionGroup );
	pPhysicsObject->SetCollisionMask( nCollideMask );
	if(material)
		pPhysicsObject->SetFriction(material->friction);

	// don't add events!
	pPhysicsObject->AddFlags(PO_NO_EVENTS);
	
	pPhysicsObject->m_pPhyObjectPointer->setCollisionFlags(pPhysicsObject->m_pPhyObjectPointer->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	DevMsg(DEVMSG_CORE, "Creating static physics object\n" );

	m_pPhysicsObjectList.append(pPhysicsObject);

	return pPhysicsObject;
}

// creates physics joint
IPhysicsJoint* DkPhysics::CreateJoint(IPhysicsObject* pObjectA,IPhysicsObject* pObjectB, const Matrix4x4 &transformA, const Matrix4x4 &transformB, bool bDisableCollisionBetweenBodies)
{
	if(!pObjectA || !pObjectB)
		return nullptr;

	CScopedMutex m(m_Mutex);

	DkPhysicsJoint* pNewJoint = PPNew DkPhysicsJoint;

	m_pJointList.append(pNewJoint);

	pNewJoint->m_pObjectA = pObjectA;
	pNewJoint->m_pObjectB = pObjectB;

	CPhysicsObject *pObjA = (CPhysicsObject*)pObjectA;
	CPhysicsObject *pObjB = (CPhysicsObject*)pObjectB;

	btTransform transA;
	btTransform transB;
	ConvertMatrix4ToBullet(transA, transformA);
	ConvertMatrix4ToBullet(transB, transformB);

	//pNewJoint->m_pJointPointer

	pNewJoint->m_pJointPointer = new btGeneric6DofConstraint(*pObjA->m_pPhyObjectPointer, *pObjB->m_pPhyObjectPointer, transA, transB, true);

	pNewJoint->m_pJointPointer->setDbgDrawSize(5);

	pNewJoint->m_pJointPointer->getTranslationalLimitMotor()->m_restitution = 0.0f;
	pNewJoint->m_pJointPointer->getTranslationalLimitMotor()->m_limitSoftness = 2.5f;

	for(int i = 0; i < 3; i++)
	{
		pNewJoint->m_pJointPointer->getRotationalLimitMotor(i)->m_bounce = 0.0f;
		pNewJoint->m_pJointPointer->getRotationalLimitMotor(i)->m_limitSoftness = 12.5f;
		pNewJoint->m_pJointPointer->getRotationalLimitMotor(i)->m_damping = 0.6f;
		pNewJoint->m_pJointPointer->getRotationalLimitMotor(i)->m_stopERP = 0.0f;
		pNewJoint->m_pJointPointer->getRotationalLimitMotor(i)->m_stopCFM = 0.0f;
	}

	m_dynamicsWorld->addConstraint(pNewJoint->m_pJointPointer, bDisableCollisionBetweenBodies);

	return pNewJoint;
}

// creates physics rope
IPhysicsRope* DkPhysics::CreateRope(const Vector3D &pointA, const Vector3D &pointB, int numSegments)
{
	CScopedMutex m(m_Mutex);

	btVector3 pA, pB;
	ConvertPositionToBullet(pA, pointA);
	ConvertPositionToBullet(pB, pointB);

	DkPhysicsRope* pRope = PPNew DkPhysicsRope;

	pRope->m_pRopeBody = btSoftBodyHelpers::CreateRope(m_softBodyWorldInfo, pA, pB, numSegments, 0);

	m_dynamicsWorld->addSoftBody(pRope->m_pRopeBody);

	m_pRopeList.append(pRope);

	return pRope;
}

int DkPhysics::AddPrimitiveShape(pritimiveinfo_t &info)
{
	btCollisionShape* primitive = InternalPrimitiveCreate(&info);

	return m_collisionShapes.append(primitive);
}

// Creates physics object
IPhysicsObject* DkPhysics::CreateObject( const StudioPhysData* data, int nObject )
{
	CScopedMutex m(m_Mutex);

	const StudioPhyObjData& objData = data->objects[nObject];

	DevMsg(DEVMSG_CORE, "Creating physics object\n");
	DevMsg(DEVMSG_CORE, "mass = %f (%f)\n", objData.desc.mass, objData.desc.mass * METERS_PER_UNIT_INV);
	DevMsg(DEVMSG_CORE, "surfaceprops = %s\n", objData.desc.surfaceprops);
	DevMsg(DEVMSG_CORE, "shapes = %d\n", objData.desc.numShapes);

	btCollisionShape* pShape = nullptr;

	// first determine shapes
	if(objData.desc.numShapes > 1)
	{
		btCompoundShape* pCompoundShape = new btCompoundShape;
		pShape = pCompoundShape;

		for(int i = 0; i < objData.desc.numShapes; i++)
		{
			btCollisionShape* shape = (btCollisionShape*)objData.shapeCacheRefs[i];
			pCompoundShape->addChildShape(btTransform::getIdentity(), shape);
		}
	}
	else
	{
		// not a compound object, skipping
		pShape = (btCollisionShape*)objData.shapeCacheRefs[0];
	}

	pShape->setMargin(EQ2BULLET(0.05f));

	btVector3 offset, massCenter;
	ConvertPositionToBullet(offset, objData.desc.offset);
	ConvertPositionToBullet(massCenter, objData.desc.massCenter);

	// make object offset
	btTransform object_start_transform;
	object_start_transform.setIdentity();
	object_start_transform.setOrigin(offset);

	// make mass center
	btTransform masscenter_transform;
	masscenter_transform.setIdentity();
	masscenter_transform.setOrigin(massCenter);

	btVector3 vLocalInertia(0,0,0);
	pShape->calculateLocalInertia(objData.desc.mass * METERS_PER_UNIT_INV, vLocalInertia);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* pMotionState = new btDefaultMotionState(object_start_transform, masscenter_transform);
	btRigidBody::btRigidBodyConstructionInfo constructInfo(objData.desc.mass * METERS_PER_UNIT_INV, pMotionState, pShape, vLocalInertia);

	// if(object->softbody)
	// else
	btRigidBody* pBody = new CEqRigidBody(constructInfo);

	// create physics object
	CPhysicsObject* pPhysicsObject = PPNew CPhysicsObject;
	
	// set body of it
	pPhysicsObject->m_pPhyObjectPointer = pBody;

	// initialize physics materials
	char pszMatName[64];
	pszMatName[0] = 0;
	strcpy(pszMatName, objData.desc.surfaceprops);

	phySurfaceMaterial_t* pPhysMaterial = FindMaterial( pszMatName );
	if(!pPhysMaterial)
	{
		pszMatName[0] = 0;
		strcpy(pszMatName, "default");

		pPhysMaterial = FindMaterial(pszMatName);
	}

	pPhysicsObject->m_pPhysMaterial = pPhysMaterial;

	// disable simulation for first frames
	pPhysicsObject->m_pPhyObjectPointer->setActivationState(ISLAND_SLEEPING);

	m_dynamicsWorld->addRigidBody(pPhysicsObject->m_pPhyObjectPointer);
	pPhysicsObject->m_pPhyObjectPointer->setUserPointer(pPhysicsObject);

	// setup material
	if (pPhysMaterial)
	{
		pPhysicsObject->SetFriction(pPhysMaterial->friction);
		pPhysicsObject->SetDamping(pPhysMaterial->dampening, pPhysMaterial->dampening);
		//pPhysicsObject->SetRestitution(pPhysMaterial->restitution);
	}

	float fSize = 1.0f;
	btVector3 vTemp(0,0,0);
	pShape->getBoundingSphere(vTemp, fSize);

	pPhysicsObject->m_pPhyObjectPointer->setDeactivationTime(0.1f);
	pPhysicsObject->m_pPhyObjectPointer->setSleepingThresholds(10.0f, 10.0f);
	//pPhysicsObject->m_pPhyObjectPointer->setCcdMotionThreshold( fSize * ph_ccdMotionThresholdScale.GetFloat() );
	//pPhysicsObject->m_pPhyObjectPointer->setCcdSweptSphereRadius( fSize * ph_CcdSweptSphereRadiusScale.GetFloat() );

	pPhysicsObject->m_pPhyObjectPointer->setCollisionFlags(pPhysicsObject->m_pPhyObjectPointer->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	m_pPhysicsObjectList.append(pPhysicsObject);
	
	return pPhysicsObject;
}

IPhysicsObject* DkPhysics::CreateObjectCustom(int numShapes, int* shapeIdxs, const char* surfaceProps, float mass)
{
	ASSERT_MSG(numShapes > 0, "IDkPhysics::CreateObjectCustom - no shapes");

	CScopedMutex m(m_Mutex);

	DevMsg(DEVMSG_CORE, "Creating custom physics object\n");
	DevMsg(DEVMSG_CORE, "mass = %f (%f)\n", mass, mass * METERS_PER_UNIT_INV);
	DevMsg(DEVMSG_CORE, "surfaceprops = %s\n", surfaceProps);

	btCollisionShape* pShape = nullptr;

	// first determine shapes
	if(numShapes > 1)
	{
		btCompoundShape* pCompoundShape = new btCompoundShape;
		pShape = pCompoundShape;

		for(int i = 0; i < numShapes; i++)
		{
			btCollisionShape* shape = (btCollisionShape*)m_collisionShapes[shapeIdxs[i]];
			pCompoundShape->addChildShape(btTransform::getIdentity(), shape);
		}
	}
	else
	{
		// not a compound object, skipping
		pShape = (btCollisionShape*)m_collisionShapes[shapeIdxs[0]];
	}

	pShape->setMargin(EQ2BULLET(0.05f));

	// make object offset
	btTransform object_start_transform;
	object_start_transform.setIdentity();
	object_start_transform.setOrigin(btVector3(0.0f, 0.0f, 0.0f));

	// make mass center
	btTransform masscenter_transform;
	masscenter_transform.setIdentity();
	masscenter_transform.setOrigin(btVector3(0.0f,0.0f,0.0f));

	btVector3 vLocalInertia(0,0,0);
	pShape->calculateLocalInertia(mass * METERS_PER_UNIT_INV, vLocalInertia);

	//using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
	btDefaultMotionState* pMotionState = new btDefaultMotionState(object_start_transform, masscenter_transform);
	btRigidBody::btRigidBodyConstructionInfo constructInfo(mass * METERS_PER_UNIT_INV, pMotionState, pShape, vLocalInertia);

	// if(object->softbody)
	// else
	btRigidBody* pBody = new CEqRigidBody(constructInfo);

	// create physics object
	CPhysicsObject* pPhysicsObject = PPNew CPhysicsObject;
	
	// set body of it
	pPhysicsObject->m_pPhyObjectPointer = pBody;

	// initialize physics materials
	char pszMatName[64];
	pszMatName[0] = 0;
	strcpy(pszMatName, surfaceProps);

	phySurfaceMaterial_t* pPhysMaterial = FindMaterial( pszMatName );
	if(!pPhysMaterial)
	{
		pszMatName[0] = 0;
		strcpy(pszMatName, "default");

		pPhysMaterial = FindMaterial(pszMatName);
	}

	pPhysicsObject->m_pPhysMaterial = pPhysMaterial;

	// disable simulation for first frames
	pPhysicsObject->m_pPhyObjectPointer->setActivationState(ISLAND_SLEEPING);

	m_dynamicsWorld->addRigidBody(pPhysicsObject->m_pPhyObjectPointer);
	pPhysicsObject->m_pPhyObjectPointer->setUserPointer(pPhysicsObject);

	// setup material
	pPhysicsObject->SetFriction(pPhysMaterial->friction);
	pPhysicsObject->SetDamping(pPhysMaterial->dampening, pPhysMaterial->dampening);
	//pPhysicsObject->SetRestitution(pPhysMaterial->restitution);

	float fSize = 1.0f;
	btVector3 vTemp(0,0,0);
	pShape->getBoundingSphere(vTemp, fSize);

	pPhysicsObject->m_pPhyObjectPointer->setDeactivationTime(0.1f);
	pPhysicsObject->m_pPhyObjectPointer->setSleepingThresholds(10.0f, 10.0f);
	pPhysicsObject->m_pPhyObjectPointer->setCcdMotionThreshold( fSize * ph_ccdMotionThresholdScale.GetFloat() );
	pPhysicsObject->m_pPhyObjectPointer->setCcdSweptSphereRadius( fSize * ph_CcdSweptSphereRadiusScale.GetFloat() );

	pPhysicsObject->m_pPhyObjectPointer->setCollisionFlags(pPhysicsObject->m_pPhyObjectPointer->getCollisionFlags()  | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	m_pPhysicsObjectList.append(pPhysicsObject);
	
	return pPhysicsObject;
}

//****** Object Destructors ******
void DkPhysics::DestroyPhysicsObject(IPhysicsObject *pObject)
{
	CScopedMutex m(m_Mutex);

	bool anyFound = false;
	for(int i = 0; i < m_pPhysicsObjectList.numElem(); i++)
	{
		if(pObject == m_pPhysicsObjectList[i])
		{
			anyFound = true;
			break;
		}
	}

	if(!anyFound)
		return;

	CPhysicsObject* pPhysObj = (CPhysicsObject*)pObject;
	if(pPhysObj)
	{
		// remove manually compound shapes because it doesn't uses a cache
		if(	pPhysObj->m_pPhyObjectPointer != nullptr &&
			pPhysObj->m_pPhyObjectPointer->getCollisionShape() != nullptr &&
			pPhysObj->m_pPhyObjectPointer->getCollisionShape()->isCompound())
		{
			btCompoundShape* pCompShape = (btCompoundShape*)pPhysObj->m_pPhyObjectPointer->getCollisionShape();
			delete pCompShape;
		}

		m_dynamicsWorld->removeRigidBody(pPhysObj->m_pPhyObjectPointer);

		m_pPhysicsObjectList.remove(pPhysObj);

		//delete pPhysObj->m_pPhyObjectPointer;
		delete pPhysObj;
	}
}

void DkPhysics::DestroyPhysicsJoint(IPhysicsJoint *pJoint)
{
	CScopedMutex m(m_Mutex);

	DkPhysicsJoint *pDkJoint = (DkPhysicsJoint*)pJoint;

	if(!pDkJoint)
		return;

	bool anyFound = false;
	for(int i = 0; i < m_pJointList.numElem(); i++)
	{
		if(pDkJoint == m_pJointList[i])
		{
			anyFound = true;
			break;
		}
	}

	if(!anyFound)
		return;

	m_pJointList.remove(pDkJoint);

	delete pDkJoint;
}

void DkPhysics::DestroyPhysicsRope(IPhysicsRope *pRope)
{
	CScopedMutex m(m_Mutex);

	DkPhysicsRope *pDkRope = (DkPhysicsRope*)pRope;

	if(!pDkRope)
		return;

	bool anyFound = false;
	for(int i = 0; i < m_pRopeList.numElem(); i++)
	{
		if(pDkRope == m_pRopeList[i])
		{
			anyFound = true;
			break;
		}
	}

	if(!anyFound)
		return;

	m_dynamicsWorld->removeSoftBody(pDkRope->m_pRopeBody);

	delete pDkRope->m_pRopeBody;

	m_pRopeList.remove(pDkRope);

	delete pDkRope;
}

void DkPhysics::DestroyPhysicsObjects()
{
	CScopedMutex m(m_Mutex);

	for(int i = 0; i < m_pRopeList.numElem(); i++)
	{
		DestroyPhysicsRope(m_pRopeList[i]);
		i--;
	}

	for(int i = 0; i < m_pJointList.numElem(); i++)
	{
		DestroyPhysicsJoint(m_pJointList[i]);
		i--;
	}

	for(int i = 0; i < m_pPhysicsObjectList.numElem(); i++)
	{
		DevMsg(DEVMSG_CORE, "Destroying physics object\n");
		DestroyPhysicsObject(m_pPhysicsObjectList[i]);
		i--;
	}
}

void DkPhysics::DestroyShapeCache()
{
	// delete collision shapes
	for (int i = 0; i < m_collisionShapes.numElem(); i++)
	{
		btCollisionShape* shape = m_collisionShapes[i];
		delete shape;
	}
}