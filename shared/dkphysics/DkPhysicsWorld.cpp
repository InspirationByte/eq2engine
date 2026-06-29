//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics powered by Jolt
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/IFileSystem.h"
#include "utils/KeyValues.h"

// Jolt headers
#include "DkJoltPCH.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Constraints/SixDOFConstraint.h>
#include <Jolt/Geometry/ConvexHullBuilder.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include "DkPhysicsInit.h"
#include "DkPhysicsWorld.h"
#include "DkPhysicsObject.h"
#include "DkPhysicsJoint.h"

#include "materialsystem1/IMaterial.h"
#include "materialsystem1/IMaterialVar.h"
#include "render/IDebugOverlay.h"
#include "physics/IStudioShapeCache.h"
#include "physics/PhysicsCollisionGroup.h"

DECLARE_CVAR(ph_gravity, "9.81", "World gravity", CV_CHEAT);
DECLARE_CVAR(ph_velocitySteps, "4", nullptr, CV_CHEAT);
DECLARE_CVAR(ph_positionSteps, "2", nullptr, CV_CHEAT);




/*
DECLARE_CVAR(ph_framerateApprox, "120", "Physics framerate approximately", CV_ARCHIVE);
DECLARE_CVAR(ph_contactMinDist, "-0.005f", "Minimum distance/intersection for contact", CV_CHEAT);
DECLARE_CVAR(ph_erp1, "0.005f", nullptr, 0);
DECLARE_CVAR(ph_debugSerialize, "0", nullptr, 0);
DECLARE_CVAR(ph_debugDraw, "0", "performs wireframe rendering of convex objects", CV_CHEAT);
DECLARE_CVAR(ph_worldExtraMargin, "0.005", "World geometry thickness (change needs unload/reload of level)", CV_ARCHIVE);

DECLARE_CVAR(ph_ccdMotionThresholdScale, "0.15f", nullptr, 0);
DECLARE_CVAR(ph_CcdSweptSphereRadiusScale, "0.5f", nullptr, 0);

static bool EQCheckNeedsCollision(btCollisionObjectWrapper* body0,btCollisionObjectWrapper* body1)
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

static void EQNearCallback(btBroadphasePair& collisionPair, btCollisionDispatcher& dispatcher, const btDispatcherInfo& dispatchInfo)
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

static bool EQContactAddedCallback(btManifoldPoint& cp,const btCollisionObjectWrapper* colObj0,int partId0,int index0,const btCollisionObjectWrapper* colObj1,int partId1,int index1)
{
	//AdjustSingleSidedContact(cp,colObj0, colObj1, partId0, index0);
	//AdjustSingleSidedContact(cp,colObj1, colObj0, partId1, index1);

	DkPhysicsObject*		pObjA = (DkPhysicsObject*)colObj0->getCollisionObject()->getUserPointer();
	DkPhysicsObject*		pObjB = (DkPhysicsObject*)colObj1->getCollisionObject()->getUserPointer();

	if(!pObjA)
		return false;

	if(pObjA->m_nFlags & PO_NO_EVENTS)
		return false;

	if(pObjA->m_nFlags & PO_BLOCKEVENTS)
		return false;

	if(pObjA && pObjB)
	{
		if (cp.getDistance() < ph_contactMinDist.GetFloat())
		{
			// add contact to the physics object, to iterate in game dll
			pObjA->AddContactEventFromManifoldPoint(&cp, pObjB);
		}
	}

	return false;
}
*/

namespace Layers
{
	static constexpr JPH::ObjectLayer NON_MOVING = 0;
	static constexpr JPH::ObjectLayer MOVING = 1;
	static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

namespace BroadPhaseLayers
{
	static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
	static constexpr JPH::BroadPhaseLayer MOVING(1);
	static constexpr JPH::uint NUM_LAYERS(2);
};

class jphDkObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
{
public:
	static jphDkObjectLayerPairFilter Instance;

	bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

jphDkObjectLayerPairFilter jphDkObjectLayerPairFilter::Instance;

class jphDkObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
	static jphDkObjectVsBroadPhaseLayerFilter Instance;

	bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case Layers::MOVING:
			return true;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

jphDkObjectVsBroadPhaseLayerFilter jphDkObjectVsBroadPhaseLayerFilter::Instance;

class jphDkBPLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
	static jphDkBPLayerInterface Instance;

	jphDkBPLayerInterface()
	{
		// Create a mapping table from object to broad phase layer
		m_objectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		m_objectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	JPH::uint GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return m_objectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override
	{
		switch ((JPH::BroadPhaseLayer::Type)inLayer)
		{
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
			return "NON_MOVING";
		case (JPH::BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
			return "MOVING";
		default:
			JPH_ASSERT(false); 
			return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	JPH::BroadPhaseLayer	m_objectToBroadPhase[Layers::NUM_LAYERS];
};

jphDkBPLayerInterface jphDkBPLayerInterface::Instance;

class jphDkContactListener : public JPH::ContactListener
{
public:
	static jphDkContactListener Instance;

	JPH::ValidateResult	OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override
	{
		return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
	{
	}

	void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override
	{
	}

	void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override
	{
	}
};

jphDkContactListener jphDkContactListener::Instance;

class jphDkBodyActivationListener : public JPH::BodyActivationListener
{
public:
	static jphDkBodyActivationListener Instance;

	void OnBodyActivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
	{
	}

	void OnBodyDeactivated(const JPH::BodyID &inBodyID, JPH::uint64 inBodyUserData) override
	{
	}
};

jphDkBodyActivationListener jphDkBodyActivationListener::Instance;

// Initialize physics
bool DkPhysics::Init(int nSceneSize)
{
	KVSection surfParamsKvs;
	if(!KV_LoadFromFile("scripts/SurfaceParams.def", -1, surfParamsKvs))
	{
		MsgError("Error! Physics surface definition file 'scripts/SurfaceParams.def' not found\n");
		return false;
	}

	physSurfaceInfo_t& physMaterial = m_materials.append();
	physMaterial.name = "default";

	for(const KVSection& sec : surfParamsKvs.Keys())
	{
		if (!CString::Compare(sec.GetName(), "#include"))
			continue;

		physSurfaceInfo_t& physMaterial = m_materials.append();

		KVSection* pBaseNamePair = sec.FindSection("base");
		if(pBaseNamePair)
		{
			const physSurfaceInfo_t* param = FindMaterial( KV_GetValueString(pBaseNamePair));
			if(param)
				physMaterial = *param;
			else
				DevMsg(DEVMSG_CORE, "script error: physics surface properties '%s' doesn't exist\n", KV_GetValueString(pBaseNamePair) );
		}

		physMaterial.name = sec.GetName();
		sec.Get("friction").GetValues(physMaterial.friction);
		sec.Get("damping").GetValues(physMaterial.dampening);
		sec.Get("density").GetValues(physMaterial.density);
		physMaterial.surfaceword = *KV_GetValueString(sec.FindSection("word"), 0, "C");

		sec.Get("footsteps").GetValues(physMaterial.footStepSound);
		sec.Get("bulletImpact").GetValues(physMaterial.bulletImpactSound);
		sec.Get("scrape").GetValues(physMaterial.scrapeSound);
		sec.Get("impactLight").GetValues(physMaterial.lightImpactSound);
		sec.Get("impactHeavy").GetValues(physMaterial.heavyImpactSound);		
	}

	return true;
}

// Makes physics scene
bool DkPhysics::CreateScene()
{
	ASSERT_MSG(m_jphPhysSys == nullptr, "Physics scene is already created");

	constexpr uint jphMaxBodies = 65536;		// TODO: argument
	constexpr uint jphNumBodyMutexes = 0;
	constexpr uint jphMaxBodyPairs = 65536;
	constexpr uint jphMaxContactConstraints = 10240;

	m_jphPhysSys = new JPH::PhysicsSystem();
	m_jphPhysSys->Init(jphMaxBodies, jphNumBodyMutexes, jphMaxBodyPairs, jphMaxContactConstraints, 
		jphDkBPLayerInterface::Instance, 
		jphDkObjectVsBroadPhaseLayerFilter::Instance, 
		jphDkObjectLayerPairFilter::Instance);

	JPH::PhysicsSettings jphPhysSettings;
	jphPhysSettings.mNumVelocitySteps = ph_velocitySteps.GetInt();
	jphPhysSettings.mNumPositionSteps = ph_positionSteps.GetInt();

	m_jphPhysSys->SetGravity(JPH::Vec3(0.0f, -ph_gravity.GetFloat(), 0.0f));
	m_jphPhysSys->SetPhysicsSettings(jphPhysSettings);

	return true;
}

// Destroy scene
void DkPhysics::DestroyScene()
{
	DestroyPhysicsObjects();

	for(JPH::Shape* jphShape : m_collisionShapes)
		jphShape->Release();
	m_collisionShapes.clear();

	SAFE_DELETE(m_jphPhysSys);
}

void DkPhysics::DoForEachObject(PhysEachObjFn procedure, void *data)
{
	for(DkPhysicsObject* obj : m_objects)
		procedure(obj, data);
}

const physSurfaceInfo_t* DkPhysics::FindMaterial(const char* pszName) const
{
	for(const physSurfaceInfo_t& physMat : m_materials)
	{
		if(!physMat.name.CompareCaseIns(pszName))
			return &physMat;
	}
	return nullptr;
}

void DkPhysics::Simulate(float dt, int substeps)
{
	if(!m_jphPhysSys)
	{
		ASSERT_FAIL("Physics scene is not created");
		return;
	}

	m_jphPhysSys->SetGravity(JPH::Vec3(0.0f, -ph_gravity.GetFloat(), 0.0f));
	m_jphPhysSys->Update(dt, substeps, GetJoltTempAlloc(), GetJoltJobSystem());
}

void DkPhysics::UpdateEvents()
{
	for(DkPhysicsObject* obj : m_objects)
		obj->ClearContactEvents();
}

void DkPhysics::DrawDebug()
{
}

void DkPhysics::CastLine(const Vector3D &tracestart, const Vector3D &traceend, int groupmask, physCast_t *trace, IPhysicsObject** pIgnoreList, int numIgnored)
{
	ASSERT(trace);
	ASSERT(!numIgnored || numIgnored && pIgnoreList);

	Vector3D rayVec = traceend - tracestart;
	const float rayLength = length(rayVec);
	const JPH::RRayCast jphRay{ Convert::ToVec3(tracestart), Convert::ToVec3(rayVec) };

	*trace = {};
	trace->origin = tracestart;
	trace->traceEnd = traceend;

	// TODO: groupmask to layer filters
	// TODO: pIgnoreList
	//JPH::SpecifiedBroadPhaseLayerFilter jphBPLayerFilter(BroadPhaseLayers::MOVING);
	//JPH::SpecifiedObjectLayerFilter jphObjLayerFilter(Layers::MOVING);

	JPH::RayCastResult jphRayHit;
	const bool hasHit = m_jphPhysSys->GetNarrowPhaseQuery().CastRay(jphRay, jphRayHit/*, jphBPLayerFilter, jphObjLayerFilter*/);
	if(!hasHit)
		return;

	JPH::BodyLockRead jphLock(m_jphPhysSys->GetBodyLockInterface(), jphRayHit.mBodyID);
	if (!jphLock.Succeeded())
		return;

	const JPH::RVec3 jphHitPosition = jphRay.GetPointOnRay(jphRayHit.mFraction);
	const JPH::Body& jphHitBody = jphLock.GetBody();
	const JPH::Vec3 jphHitNormal = jphHitBody.GetWorldSpaceSurfaceNormal(jphRayHit.mSubShapeID2, jphHitPosition);

	DkPhysicsObject* hitObj = reinterpret_cast<DkPhysicsObject*>(jphHitBody.GetUserData());
	trace->normal = Convert::FromVec3(jphHitNormal);
	trace->traceEnd = Convert::FromVec3(jphHitPosition) + trace->normal * F_EPS;
	trace->hitObj = hitObj;
	trace->hitMaterial = hitObj->m_pRMaterial;
	trace->fraction = jphRayHit.mFraction;
}

void DkPhysics::InternalCastShape(const Vector3D &tracestart, const Vector3D &traceend, const JPH::Shape* shape, int groupmask, physCast_t *trace, IPhysicsObject** pIgnoreList, int numIgnored, Matrix4x4* transform)
{
	ASSERT(trace);
	ASSERT(!numIgnored || numIgnored && pIgnoreList);

	Vector3D rayVec = traceend - tracestart;
	const float rayLength = length(rayVec);
	const JPH::RRayCast jphRay{ Convert::ToVec3(tracestart), Convert::ToVec3(rayVec) };

	*trace = {};
	trace->origin = tracestart;
	trace->traceEnd = traceend;

	JPH::Mat44 jphShapeRotation = transform ? Convert::ToMat44Transposed(*transform) : JPH::Mat44::sIdentity();
	JPH::RShapeCast jphShapeCast = JPH::RShapeCast::sFromWorldTransform(shape, JPH::Vec3::sOne(), JPH::RMat44::sTranslation(jphRay.mOrigin) * jphShapeRotation, jphRay.mDirection);

	// TODO: groupmask to layer filters
	// TODO: pIgnoreList
	//JPH::SpecifiedBroadPhaseLayerFilter jphBPLayerFilter(BroadPhaseLayers::MOVING);
	//JPH::SpecifiedObjectLayerFilter jphObjLayerFilter(Layers::MOVING);

	JPH::ShapeCastSettings jphCastSettings;

	JPH::ClosestHitCollisionCollector<JPH::CastShapeCollector> jphCollector;
	m_jphPhysSys->GetNarrowPhaseQuery().CastShape(jphShapeCast, jphCastSettings, jphRay.mOrigin, jphCollector/*, jphBPLayerFilter, jphObjLayerFilter*/);
	if(!jphCollector.HadHit())
		return;

	JPH::BodyLockRead jphLock(m_jphPhysSys->GetBodyLockInterface(), jphCollector.mHit.mBodyID2);
	if (!jphLock.Succeeded())
		return;

	const JPH::RVec3 jphHitPosition = jphRay.GetPointOnRay(jphCollector.mHit.mFraction);
	const JPH::Body& jphHitBody = jphLock.GetBody();
	const JPH::Vec3 jphHitNormal = jphHitBody.GetWorldSpaceSurfaceNormal(jphCollector.mHit.mSubShapeID2, jphHitPosition);

	DkPhysicsObject* hitObj = reinterpret_cast<DkPhysicsObject*>(jphHitBody.GetUserData());
	trace->normal = Convert::FromVec3(jphHitNormal);
	trace->traceEnd = Convert::FromVec3(jphHitPosition) + trace->normal * F_EPS;
	trace->hitObj = hitObj;
	trace->hitMaterial = hitObj->m_pRMaterial;
	trace->fraction = jphCollector.mHit.mFraction;
}

void DkPhysics::CastBox(const Vector3D &tracestart, const Vector3D &traceend, const Vector3D& boxSize, int groupmask, physCast_t *trace, IPhysicsObject** pIgnoreList, int numIgnored, Matrix4x4* transform)
{
	const JPH::BoxShape jphBoxShape(Convert::ToVec3(boxSize));
	InternalCastShape(tracestart, traceend, &jphBoxShape, groupmask, trace, pIgnoreList, numIgnored, transform);
}

void DkPhysics::CastSphere(const Vector3D &tracestart, const Vector3D &traceend, float sphereRadius,int groupmask,physCast_t* trace, IPhysicsObject** pIgnoreList, int numIgnored)
{
	const JPH::SphereShape jphSphereShape(sphereRadius);
	InternalCastShape(tracestart, traceend, &jphSphereShape, groupmask, trace, pIgnoreList, numIgnored);
}

// Generic traceLine for physics
void DkPhysics::CastShape(const Vector3D &tracestart, const Vector3D &traceend, int shapeId, int groupmask, physCast_t* trace, IPhysicsObject** pIgnoreList, int numIgnored, Matrix4x4* transform)
{
	InternalCastShape(tracestart, traceend, m_collisionShapes[shapeId], groupmask, trace, pIgnoreList, numIgnored);
}

void DkPhysics::DestroyPhysicsObject(IPhysicsObject *pObject)
{
	DkPhysicsObject* physObj = static_cast<DkPhysicsObject*>(pObject);
	{
		Threading::CScopedMutex m(m_Mutex);
		if (!m_objects.fastRemove(physObj))
			return;
	}

	JPH::BodyID jphBodyId = physObj->m_jphObjId;
	delete physObj;

	JPH::BodyInterface& jphBodyIface = m_jphPhysSys->GetBodyInterface();
	jphBodyIface.RemoveBody(jphBodyId);
	jphBodyIface.DestroyBody(jphBodyId);
}

void DkPhysics::DestroyPhysicsJoint(IPhysicsJoint *pJoint)
{
	DkPhysicsJoint* physJoint = static_cast<DkPhysicsJoint*>(pJoint);
	{
		Threading::CScopedMutex m(m_Mutex);
		if (!m_joints.fastRemove(physJoint))
			return;
	}
	m_jphPhysSys->RemoveConstraint(physJoint->m_jphContraint);
	delete physJoint;
}

void DkPhysics::DestroyPhysicsObjects()
{
	Threading::CScopedMutex m(m_Mutex);
	
	while(!m_joints.isEmpty())
		DestroyPhysicsJoint(m_joints.front());

	while(!m_objects.isEmpty())
		DestroyPhysicsObject(m_objects.front());
}

static JPH::Ref<JPH::Shape> jphCreatePrimitiveShape(const physPrimitiveInfo_t& priminfo)
{
	if(priminfo.primType == PHYSPRIM_BOX)
		return new JPH::BoxShape(JPH::Vec3(priminfo.boxInfo.boxSizeX, priminfo.boxInfo.boxSizeY, priminfo.boxInfo.boxSizeZ));
	else if(priminfo.primType == PHYSPRIM_SPHERE)
		return new JPH::SphereShape(priminfo.sphereRadius);
	else if(priminfo.primType == PHYSPRIM_CAPSULE)
		return new JPH::CapsuleShape(priminfo.capsuleInfo.height, priminfo.capsuleInfo.radius);
	else
		MsgError("Invalid primitive type (%d)\n", priminfo.primType);

	return nullptr;
}

static JPH::ShapeRefC jphCreateMeshShape(const physObjectInfo_t& info)
{
	const physShapeInfo_t& shapeInfo = *info.data;
	if(info.genConvex)
	{
		JPH::Array<JPH::Vec3> jphVertList;
		jphVertList.reserve(shapeInfo.numVertices);
		ubyte* vertPtr = reinterpret_cast<ubyte*>(shapeInfo.vertices) + shapeInfo.vertexPosOffset;
		for(int i = 0; i < shapeInfo.numVertices; ++i)
		{
			const JPH::Vec3 vertFlt = Convert::ToVec3(*reinterpret_cast<const Vector3D*>(vertPtr + shapeInfo.vertexSize));
			jphVertList.push_back(vertFlt);
		}

		JPH::ConvexHullShapeSettings jphHullSettings(jphVertList, info.convexMargin > 0 ? info.convexMargin : JPH::cDefaultConvexRadius);

		auto jphResult = jphHullSettings.Create();
		return jphResult.Get();
	}

	JPH::VertexList jphVertList;
	JPH::IndexedTriangleList jphTriList;
	jphVertList.reserve(shapeInfo.numVertices);

	ubyte* vertPtr = reinterpret_cast<ubyte*>(shapeInfo.vertices) + shapeInfo.vertexPosOffset;
	for(int i = 0; i < shapeInfo.numVertices; ++i)
	{
		const JPH::Float3& vertFlt = *reinterpret_cast<JPH::Float3*>(vertPtr + shapeInfo.vertexSize * i);
		jphVertList.push_back(vertFlt);
	}

	jphTriList.reserve(shapeInfo.numIndices / 3);
	for(int i = 0; i < shapeInfo.numIndices; i += 3)
	{
		JPH::IndexedTriangle jphTri(shapeInfo.indices[i], shapeInfo.indices[i+1], shapeInfo.indices[i+2]);
		jphTriList.push_back(jphTri);
	}

	JPH::MeshShapeSettings jphMeshSettings(jphVertList, jphTriList);
	auto jphResult = jphMeshSettings.Create();
	return jphResult.Get();
}

int DkPhysics::AddPrimitiveShape(const physPrimitiveInfo_t &info)
{
	JPH::Ref<JPH::Shape> shape = jphCreatePrimitiveShape(info);
	if(!shape)
		return -1;
	shape->AddRef();
	return m_collisionShapes.append(shape);
}

IPhysicsObject* DkPhysics::CreateStaticObject(const physObjectInfo_t& info, int nCollisionGroup)
{
	int nCollideMask = COLLISION_GROUP_ALL;

	if(info.data && info.data->pMaterial)
	{
		const physShapeInfo_t& shapeInfo = *info.data;
		MatIntProxy mv_nocollide = shapeInfo.pMaterial->FindMaterialVar("noCollide");
		if(mv_nocollide.Get() > 0)
			return nullptr;

		MatIntProxy mv_clip = shapeInfo.pMaterial->FindMaterialVar("playerCip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_PLAYERCLIP;
			nCollideMask = COLLISION_GROUP_PLAYER;
		}

		mv_clip = shapeInfo.pMaterial->FindMaterialVar("npcClip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_NPCCLIP;
			nCollideMask = COLLISION_GROUP_ACTORS;
		}

		mv_clip = shapeInfo.pMaterial->FindMaterialVar("physClip");
		if(mv_clip.Get() > 0)
		{
			nCollisionGroup = COLLISION_GROUP_PHYSCLIP;
			nCollideMask = COLLISION_GROUP_OBJECTS | COLLISION_GROUP_DEBRIS;
		}

		if(shapeInfo.pMaterial->GetFlags() & MATERIAL_FLAG_WATER)
		{
			nCollisionGroup = COLLISION_GROUP_WATER;
			nCollideMask = COLLISION_GROUP_PROJECTILES;
		}
		else if(shapeInfo.pMaterial->GetFlags() & MATERIAL_FLAG_SKY)
		{
			nCollisionGroup = COLLISION_GROUP_SKYBOX;
			nCollideMask = 0;
		}
	}

	JPH::ShapeRefC jphShape = jphCreateMeshShape(info);
	if(!jphShape)
	{
		MsgError("Can't create physics object without shape!\n");
		return nullptr;
	}

	const bool isDynamic = info.mass > 0;
	const JPH::EMotionType jphMotionType = isDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
	const JPH::ObjectLayer jphObjLayer =  isDynamic ? Layers::MOVING : Layers::NON_MOVING;
	JPH::BodyCreationSettings jphBodySetting(jphShape, JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::Quat::sIdentity(), jphMotionType, jphObjLayer);

	JPH::BodyInterface& jphBodyIface = m_jphPhysSys->GetBodyInterface();
	JPH::BodyID jphBodyId = jphBodyIface.CreateAndAddBody(jphBodySetting, JPH::EActivation::DontActivate);

	const char* materialName = "default";
	IMaterial* rendMaterial = nullptr;
	if(info.data)
	{
		if(info.data->surfaceprops != nullptr)
		{
			materialName = info.data->surfaceprops;
		}
		else
		{
			MatStringProxy surfacePropsVar = info.data->pMaterial->FindMaterialVar("surfaceProps");
			if(surfacePropsVar.IsValid())
				materialName = surfacePropsVar.Get();
		}
		rendMaterial = info.data->pMaterial;
	}

	const physSurfaceInfo_t* material = FindMaterial(materialName);
	if(!material)
	{
		MsgError("Invalid physics material '%s'!\n", materialName);
		materialName = "default";
		material = FindMaterial(materialName);
		ASSERT_MSG(material, "No 'default' physics material");
	}

	DkPhysicsObject* physObj = PPNew DkPhysicsObject(*m_jphPhysSys, jphBodyId);
	physObj->m_physMaterial = material;
	physObj->m_pRMaterial = rendMaterial;
	physObj->AddFlags(PO_NO_EVENTS);
	physObj->SetContents( nCollisionGroup );
	physObj->SetCollisionMask( nCollideMask );
	physObj->SetFriction(material->friction);

	jphBodyIface.SetUserData(jphBodyId, reinterpret_cast<JPH::uint64>(physObj));

	{
		Threading::CScopedMutex m(m_Mutex);
		m_objects.append(physObj);
	}

	return physObj;
}

IPhysicsObject* DkPhysics::CreateStudioObject(const StudioPhysData& data, int nObject)
{
	const StudioPhyObjData& objData = data.objects[nObject];

	JPH::ShapeRefC jphShape = reinterpret_cast<JPH::Shape*>(objData.shapeCacheRefs[0]);
	if(objData.desc.numShapes > 1)
	{
		JPH::StaticCompoundShapeSettings jphShapeSettings;
		for(int i = 0; i < objData.desc.numShapes; i++)
		{
			JPH::Shape* jphSubShape = reinterpret_cast<JPH::Shape*>(objData.shapeCacheRefs[i]);
			jphShapeSettings.AddShape(JPH::Vec3::sZero(), JPH::Quat::sIdentity(), jphSubShape);
		}
		auto result = jphShapeSettings.Create();
		jphShape = result.Get();
	}

	const bool isDynamic = objData.desc.mass > 0;
	const JPH::EMotionType jphMotionType = isDynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static;
	const JPH::ObjectLayer jphObjLayer =  isDynamic ? Layers::MOVING : Layers::NON_MOVING;
	JPH::BodyCreationSettings jphBodySetting(jphShape, JPH::Vec3(0.0f, 0.0f, 0.0f), JPH::Quat::sIdentity(), jphMotionType, jphObjLayer);
	jphBodySetting.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
	jphBodySetting.mMassPropertiesOverride.ScaleToMass(objData.desc.mass);

	JPH::BodyInterface& jphBodyIface = m_jphPhysSys->GetBodyInterface();
	JPH::BodyID jphBodyId = jphBodyIface.CreateAndAddBody(jphBodySetting, JPH::EActivation::DontActivate);

	const char* materialName =  objData.desc.surfaceprops;
	const physSurfaceInfo_t* material = FindMaterial( materialName );
	if(!material)
	{
		MsgError("Invalid physics material '%s'!\n", materialName);
		materialName = "default";
		material = FindMaterial(materialName);
		ASSERT_MSG(material, "No 'default' physics material");
	}

	DkPhysicsObject* physObj = PPNew DkPhysicsObject(*m_jphPhysSys, jphBodyId);
	physObj->m_physMaterial = material;
	physObj->SetFriction(material->friction);
	jphBodyIface.SetUserData(jphBodyId, reinterpret_cast<JPH::uint64>(physObj));

	{
		Threading::CScopedMutex m(m_Mutex);
		m_objects.append(physObj);
	}

	return physObj;
}

IPhysicsJoint* DkPhysics::CreateJoint(	IPhysicsObject* pObjectA, IPhysicsObject* pObjectB, 
										const Matrix4x4 &transformA, const Matrix4x4 &transformB,
										bool bDisableCollisionBetweenBodies)
{
	DkPhysicsObject* pObjA = static_cast<DkPhysicsObject*>(pObjectA);
	DkPhysicsObject* pObjB = static_cast<DkPhysicsObject*>(pObjectB);
	if(!pObjA || !pObjB)
		return nullptr;

	JPH::BodyLockWrite jphBodyLockA(m_jphPhysSys->GetBodyLockInterfaceNoLock(), pObjA->m_jphObjId);
	JPH::BodyLockWrite jphBodyLockB(m_jphPhysSys->GetBodyLockInterfaceNoLock(), pObjB->m_jphObjId);
	if (!jphBodyLockA.Succeeded() || !jphBodyLockB.Succeeded())
		return nullptr;
	
	JPH::SixDOFConstraintSettings jph6DOFSettings;
	jph6DOFSettings.mPosition1 = Convert::ToVec3(transformA.getTranslationComponent());
	jph6DOFSettings.mAxisX1 = Convert::ToVec3(transformA.rows[0].xyz());
	jph6DOFSettings.mAxisY1 = Convert::ToVec3(transformA.rows[1].xyz());

	jph6DOFSettings.mPosition2 = Convert::ToVec3(transformB.getTranslationComponent());
	jph6DOFSettings.mAxisX2 = Convert::ToVec3(transformB.rows[0].xyz());
	jph6DOFSettings.mAxisY2 = Convert::ToVec3(transformB.rows[1].xyz());

	JPH::TwoBodyConstraint* jphConstraint = jph6DOFSettings.Create(jphBodyLockA.GetBody(), jphBodyLockB.GetBody());
	m_jphPhysSys->AddConstraint(jphConstraint);

	DkPhysicsJoint* physJoint = PPNew DkPhysicsJoint(jphConstraint, pObjA, pObjB);

	// TODO: collision group, GroupFilterTable for bDisableCollisionBetweenBodies

	{
		Threading::CScopedMutex m(m_Mutex);
		m_joints.append(physJoint);
	}

	return physJoint;
}
