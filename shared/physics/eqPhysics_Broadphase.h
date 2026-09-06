#pragma once
#include <BulletCollision/BroadphaseCollision/btDbvt.h>

struct btBroadphaseRayCallback;
struct btBroadphaseAabbCallback;
class CEqCollisionObject;

struct eqPhysBroadphaseUnit
{
	eqPhysBroadphaseUnit*	links[2]{ nullptr };
	btDbvtNode*				leaf{ nullptr };
	CEqCollisionObject*		object{ nullptr };
	BoundingBox				bbox;

	int						setIdx{ 0 };
};

class CEqPhysicsBroadphase
{
public:
	using Unit = eqPhysBroadphaseUnit;
	using ProcessObjectFunc = EqFunction<void(CEqCollisionObject* collObj)>;
	struct RayTester;
	struct AABBTester;

	Unit*	CreateUnit(const BoundingBox& bbox, CEqCollisionObject* collObj);
	void	DestroyUnit(Unit* unit);

	void	SetAabb(Unit* unit, const BoundingBox& bbox);

	void	RayTest(const Vector3D& rayFrom, const Vector3D& rayTo, const ProcessObjectFunc& processFunc, int physFilterFlags, const BoundingBox& shapeBox = BoundingBox(0, 0));
	void	BoxTest(const BoundingBox& bbox, const ProcessObjectFunc& processFunc, int physFilterFlags);

	static btDbvtNode*	AllocNode();
	static void			FreeNode(btDbvtNode* node);

private:
	enum
	{
		DYNAMIC_SET	= 0,	// Dynamic set index
		FIXED_SET	= 1,	// Fixed set index
		STAGE_COUNT = 2		// Number of stages
	};

	static MemoryPool<btDbvtNode>		s_nodeAlloc;
	MemoryPool<Unit>	m_unitAlloc{ PP_SL };
	btDbvt				m_sets[2];
	float				m_prediction{ 0.1f };
};