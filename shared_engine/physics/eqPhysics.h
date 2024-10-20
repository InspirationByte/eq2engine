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
//				Using Bullet Collision Library for fast collision detection
//
//////////////////////////////////////////////////////////////////////////////////

/*
DONE:
		- Rigid body dynamics
		- Collision detecion
		- Collision shapes
		- Static object optimization
		- Dynamic objects optimization
		- Line test for dynamic objects
		- Swept test
		- Constraints (car doors, hoods, other)
TODO:
		- Multithreaded integration, collision detection and collision response
		- Multithreaded line test (test bunch of lines)
*/

#pragma once
#include "eqPhysics_Defs.h"

// max world size is +/-32768, limited by FReal
static constexpr const float EQPHYS_MAX_WORLDSIZE = 32767.0f;

static constexpr const float PHYSICS_DEFAULT_FRICTION = 0.5f;
static constexpr const float PHYSICS_DEFAULT_RESTITUTION = 0.25f;
static constexpr const float PHYSICS_DEFAULT_TIRE_FRICTION = 0.2f;
static constexpr const float PHYSICS_DEFAULT_TIRE_TRACTION = 1.0f;

struct btDispatcherInfo;
class btCollisionWorld;
class btCollisionConfiguration;
class btCollisionDispatcher;
class btCollisionShape;

struct eqCollisionInfo;
struct eqContactPair;
struct KVSection;
class CEqCollisionObject;
class CEqRigidBody;
class CEqCollisionBroadphaseGrid;
class IEqPhysicsConstraint;
class IEqPhysicsController;


typedef void (*FNSIMULATECALLBACK)(float fDt, int iterNum);

//--------------------------------------------------------------------------------------------------------------

class CEqPhysics
{
	struct sweptTestParams_t
	{
		Quaternion rotation;
		const btCollisionShape*	shape;
	};

public:
	CEqPhysics();
	~CEqPhysics();

	void							InitWorld();										///< initializes world
	void							InitGrid();											///< initializes broadphase grid

	void							DestroyWorld();										///< destroys world
	void							DestroyGrid();										///< destroys broadphase grid

	void							AddSurfaceParamFromKV(const char* name, const KVSection* kvSection);
	const int						FindSurfaceParamID(const char* name) const;
	const eqPhysSurfParam*		FindSurfaceParam(const char* name) const;
	const eqPhysSurfParam*		GetSurfaceParamByID(int id) const;

	void							AddToMoveableList( CEqRigidBody* body );			///< adds object to moveable list
	void							RemoveFromMoveableList( CEqRigidBody* body );

	void							AddToWorld( CEqRigidBody* body, bool moveable = true );	///< adds to processing
	bool							RemoveFromWorld( CEqRigidBody* body );				///< removes body from world, not deleting
	void							DestroyBody( CEqRigidBody* body );					///< destroys body

	void							SetupBodyOnCell( CEqCollisionObject* body );		///< only rigid body and ghost objects

	void							AddGhostObject( CEqCollisionObject* object );		///< adds ghost object
	void							DestroyGhostObject( CEqCollisionObject* object );	///< removes ghost object

	void							AddStaticObject( CEqCollisionObject* object );		///< adds collision object as static body
	void							RemoveStaticObject( CEqCollisionObject* object );	///< removes static object
	void							DestroyStaticObject( CEqCollisionObject* object );	///< destroys static object

    bool                            IsValidStaticObject( CEqCollisionObject* obj ) const;
	bool                            IsValidBody( CEqCollisionObject* body ) const;

	void							AddConstraint( IEqPhysicsConstraint* constraint );		///< adds constraint to the world
	void							RemoveConstraint( IEqPhysicsConstraint* constraint );	///< removes constraint from the world

	void							AddController( IEqPhysicsController* controller );		///< adds controller to the world
	void							RemoveController( IEqPhysicsController* controller );	///< removes controller from the world
	void							DestroyController( IEqPhysicsController* controlelr );	///< destroys controller

	//< Performs a line test in the world
	bool							TestLineCollision(	const FVector3D& start,
														const FVector3D& end,
														eqCollisionInfo& coll,
														int rayMask = COLLISION_MASK_ALL, 
														const eqPhysCollisionFilter* filterParams = nullptr);

	///< Pushes convex in the world for closest collision
	bool							TestConvexSweepCollision(const btCollisionShape* shape,
																const Quaternion& rotation,
																const FVector3D& start,
																const FVector3D& end,
																eqCollisionInfo& coll,
																int rayMask = COLLISION_MASK_ALL, 
																const eqPhysCollisionFilter* filterParams = nullptr);
	///< Performs a line test for a single object.
	///< start, end are world coordinates
	bool							TestLineSingleObject(CEqCollisionObject* object,
															const FVector3D& start,
															const FVector3D& end,
															const BoundingBox& raybox,
															eqCollisionInfo& coll,
															float closestHit,
															int rayMask,
															const eqPhysCollisionFilter* filterParams,
															void* args = nullptr);

	// Pushes a convex sweep for closest collision for a single object.
	///< start, end are world coordinates
	bool							TestConvexSweepSingleObject( CEqCollisionObject* object,
																const FVector3D& start,
																const FVector3D& end,
																const BoundingBox& raybox,
																eqCollisionInfo& coll,
																float closestHit,
																int rayMask,
																const eqPhysCollisionFilter* filterParams,
																void* args = nullptr);

	///< draws physics bounding boxes
	void							DebugDrawBodies(int mode);

	///< Simulates physics
	void							SimulateStep( float deltaTime, int iteration, FNSIMULATECALLBACK preIntegrFunc);	///< simulates physics

	//------------------------------------------------------

	//------------------------------------------------------

	///< Integrates single body without collision detection
	void							IntegrateSingle(CEqRigidBody* body);

	///< detects body collisions
	void							DetectCollisionsSingle(CEqRigidBody* body);

	///< processes contact pairs
	void							ProcessContactPair(eqContactPair& pair);

	// checks collision (made especially for rays, but could be used in other situations)
	bool							CheckAllowContactTest(const eqPhysCollisionFilter* filterParams, const CEqCollisionObject* object);

	void							SetDebugRaycast(bool enable) {m_debugRaycast = enable;}

	void							DetectBodyCollisions(CEqRigidBody* bodyA, CEqRigidBody* bodyB, float fDt);
	void							DetectStaticVsBodyCollision(CEqCollisionObject* staticObj, CEqRigidBody* bodyB, float fDt);

	//static void						CellCollisionDetectionJob(void* data, int iter);

protected:

	typedef bool (fnSingleObjectLineCollisionCheck)(CEqCollisionObject* object,
		const FVector3D& start,
		const FVector3D& end,
		const BoundingBox& raybox,
		eqCollisionInfo& coll,
		float closestHit,
		int rayMask,
		const eqPhysCollisionFilter* filterParams,
		void* args);

	///< tests line versus some objects
	template <typename F>
	bool							TestLineCollisionOnCell(int y, int x,
															const FVector3D& start, const FVector3D& end,
															const BoundingBox& rayBox,
															eqCollisionInfo& coll,
															Set<CEqCollisionObject*>& skipObjects,
															int rayMask,
															const eqPhysCollisionFilter* filterParams,
															F func,
															void* args = nullptr);

	///< Performs collision tests in broadphase grid
	template <typename F>
	void							InternalTestLineCollisionCells(	const Vector2D& startCell, const Vector2D& endCell,
																	const FVector3D& start, const FVector3D& end,
																	const BoundingBox& rayBox,
																	eqCollisionInfo& coll,
																	int rayMask,
																	const eqPhysCollisionFilter* filterParams,
																	F func,
																	void* args = nullptr);

	CEqCollisionBroadphaseGrid*		m_grid{ nullptr };

protected:

	Array<eqPhysSurfParam*>			m_physSurfaceParams{ PP_SL };

	Array<CEqRigidBody*>			m_moveable{ PP_SL };

	Array<CEqRigidBody*>			m_dynObjects{ PP_SL };
	Array<CEqCollisionObject*>		m_staticObjects{ PP_SL };

	Array<CEqCollisionObject*>		m_ghostObjects{ PP_SL };

	Array<IEqPhysicsConstraint*>	m_constraints{ PP_SL };
	Array<IEqPhysicsController*>	m_controllers{ PP_SL };

	btDispatcherInfo*				m_dispatchInfo{ nullptr };
	btCollisionWorld*				m_collisionWorld{ nullptr };
	btCollisionConfiguration*		m_collConfig{ nullptr };
	btCollisionDispatcher*			m_collDispatcher{ nullptr };

	int								m_numRayQueries{ 0 };
	float							m_fDt{ 0.0f };
	bool							m_debugRaycast{ false };
};
