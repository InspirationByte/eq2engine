#include "core/core_common.h"
#include "eqPhysics_Broadphase.h"
#include "eqCollision_Object.h"
#include "physics/BulletConvert.h"
#include "eqPhysics_Defs.h"

static constexpr float BROADPHASE_DBVT_MARGIN = 1.0f;

eqPhysBroadphaseUnit* CEqPhysicsBroadphase::CreateUnit(const BoundingBox& bbox, CEqCollisionObject* collObj)
{
	using namespace EqBulletUtils;

	eqPhysBroadphaseUnit* newUnit = new (m_unitAlloc.allocate()) eqPhysBroadphaseUnit;
	const int setIdx = collObj->IsDynamic() ? DYNAMIC_SET : FIXED_SET;

	btDbvtVolume dbvtBox;
	ConvertDKToBulletVectors(dbvtBox.tMins(), bbox.minPoint);
	ConvertDKToBulletVectors(dbvtBox.tMaxs(), bbox.maxPoint);

	newUnit->bbox = bbox;
	newUnit->object = collObj;
	newUnit->setIdx = setIdx;
	newUnit->leaf = m_sets[setIdx].insert(dbvtBox, newUnit);
	return newUnit;
}

void CEqPhysicsBroadphase::DestroyUnit(eqPhysBroadphaseUnit* unit)
{
	m_sets[unit->setIdx].remove(unit->leaf);
	m_unitAlloc.deallocate(unit);
}

void CEqPhysicsBroadphase::SetAabb(eqPhysBroadphaseUnit* unit, const BoundingBox& bbox)
{
	using namespace EqBulletUtils;

	btDbvtVolume newDbvtBox;
	ConvertDKToBulletVectors(newDbvtBox.tMins(), bbox.minPoint);
	ConvertDKToBulletVectors(newDbvtBox.tMaxs(), bbox.maxPoint);

	if (unit->setIdx == DYNAMIC_SET)
	{
		if (Intersect(unit->leaf->volume, newDbvtBox))
		{
			// Moving
			const Vector3D delta = bbox.minPoint - unit->bbox.minPoint;

			btVector3 velocity;
			ConvertDKToBulletVectors(velocity, unit->bbox.GetSize() * m_prediction);
			if (delta[0] < 0) velocity[0] = -velocity[0];
			if (delta[1] < 0) velocity[1] = -velocity[1];
			if (delta[2] < 0) velocity[2] = -velocity[2];

			m_sets[DYNAMIC_SET].update(unit->leaf, newDbvtBox, velocity, BROADPHASE_DBVT_MARGIN);
		}
		else
		{
			// Teleporting
			m_sets[DYNAMIC_SET].update(unit->leaf, newDbvtBox);
		}
	}
	else if (unit->setIdx == FIXED_SET)
	{
		m_sets[FIXED_SET].remove(unit->leaf);
		unit->leaf = m_sets[FIXED_SET].insert(newDbvtBox, unit);
	}
}

struct CEqPhysicsBroadphase::RayTester : btDbvt::ICollide
{
	const ProcessObjectFunc& processFunc;

	RayTester(const ProcessObjectFunc& processFunc)
		: processFunc(processFunc)
	{
	}
	void Process(const btDbvtNode* leaf)
	{
		eqPhysBroadphaseUnit* unit = (eqPhysBroadphaseUnit*)leaf->data;
		processFunc(unit->object);
	}
};

struct CEqPhysicsBroadphase::AABBTester : btDbvt::ICollide
{
	const ProcessObjectFunc& processFunc;

	AABBTester(const ProcessObjectFunc& processFunc)
		: processFunc(processFunc)
	{
	}
	void Process(const btDbvtNode* leaf)
	{
		eqPhysBroadphaseUnit* unit = (eqPhysBroadphaseUnit*)leaf->data;
		processFunc(unit->object);
	}
};

void CEqPhysicsBroadphase::RayTest(const Vector3D& rayFrom, const Vector3D& rayTo, const EqFunction<void(CEqCollisionObject* collObj)>& processFunc, int physFilterFlags, const BoundingBox& shapeBox)
{
	if (!physFilterFlags)
		return;

	using namespace EqBulletUtils;

	btDbvtVolume dbvtShapeBox;
	ConvertDKToBulletVectors(dbvtShapeBox.tMins(), shapeBox.minPoint);
	ConvertDKToBulletVectors(dbvtShapeBox.tMaxs(), shapeBox.maxPoint);

	// what about division by zero? --> just set rayDirection[i] to INF/BT_LARGE_FLOAT
	const Vector3D rayVec = rayTo - rayFrom;
	const Vector3D rayDir = normalize(rayVec);
	btVector3 rayDirectionInverse;
	rayDirectionInverse[0] = rayDir[0] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[0];
	rayDirectionInverse[1] = rayDir[1] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[1];
	rayDirectionInverse[2] = rayDir[2] == btScalar(0.0) ? btScalar(BT_LARGE_FLOAT) : btScalar(1.0) / rayDir[2];

	uint signs[3];
	signs[0] = rayDirectionInverse[0] < 0.0;
	signs[1] = rayDirectionInverse[1] < 0.0;
	signs[2] = rayDirectionInverse[2] < 0.0;

	const float maxLambda = dot(rayDir, rayVec);

	btVector3 from, to;
	ConvertDKToBulletVectors(from, rayFrom);
	ConvertDKToBulletVectors(to, rayTo);

	static thread_local btAlignedObjectArray<const btDbvtNode*> stack;
	RayTester rayTester(processFunc);

	if (physFilterFlags & EQPHYS_FILTER_FLAG_STATICOBJECTS)
	{
		m_sets[FIXED_SET].rayTestInternal(m_sets[FIXED_SET].m_root,
			from,
			to,
			rayDirectionInverse,
			signs,
			maxLambda,
			dbvtShapeBox.Mins(),
			dbvtShapeBox.Maxs(),
			stack,
			rayTester);
	}

	if (physFilterFlags & EQPHYS_FILTER_FLAG_DYNAMICOBJECTS)
	{
		m_sets[DYNAMIC_SET].rayTestInternal(m_sets[DYNAMIC_SET].m_root,
			from,
			to,
			rayDirectionInverse,
			signs,
			maxLambda,
			dbvtShapeBox.Mins(),
			dbvtShapeBox.Maxs(),
			stack,
			rayTester);
	}
}

void CEqPhysicsBroadphase::BoxTest(const BoundingBox& bbox, const EqFunction<void(CEqCollisionObject* collObj)>& processFunc)
{
	using namespace EqBulletUtils;

	btDbvtVolume dbvtBox;
	ConvertDKToBulletVectors(dbvtBox.tMins(), bbox.minPoint);
	ConvertDKToBulletVectors(dbvtBox.tMaxs(), bbox.maxPoint);

	AABBTester aabbTester(processFunc);

	// process all children, that overlap with  the given AABB bounds
	m_sets[FIXED_SET].collideTV(m_sets[1].m_root, dbvtBox, aabbTester);
	m_sets[DYNAMIC_SET].collideTV(m_sets[0].m_root, dbvtBox, aabbTester);
}