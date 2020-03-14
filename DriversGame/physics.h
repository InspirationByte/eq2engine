//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics layer
//////////////////////////////////////////////////////////////////////////////////

#ifndef PHYSICS_H
#define PHYSICS_H

#include "dktypes.h"
#include "utils/DkList.h"
#include "math/DkMath.h"
#include "core_base_header.h"

#include "eqPhysics/eqPhysics.h"
#include "eqPhysics/eqPhysics_Body.h"
#include "eqPhysics/eqCollision_Object.h"

#include "drivers_coord.h"

const float PHYSICS_FRAME_INTERVAL = (1.0f / 60.0f);

class CHeightTileField;
class CGameObject;

// the physics high frequency object
class CPhysicsHFObject : public IEqPhysCallback
{
public:
	CPhysicsHFObject( CEqCollisionObject* pObj, CGameObject* owner = NULL );
	~CPhysicsHFObject();

	void			PreSimulate( float fDt );
	void			PostSimulate( float fDt );
	void			OnCollide(CollisionPairData_t& pair);

	void			UpdateOrigin();

	CEqRigidBody*	GetBody() const { return (CEqRigidBody*)m_object; }

	CGameObject*	m_owner;
};

// Physics engine/world
class CPhysicsEngine
{
public:
								CPhysicsEngine();

	void						SceneInit();
	void						SceneShutdown();

#ifdef EDITOR
	void						SceneInitBroadphase();
	void						SceneDestroyBroadphase();
#endif // EDITOR

	void						Simulate( float fDt, int numIterations, FNSIMULATECALLBACK preIntegrateFn = NULL );
	void						ForceUpdateObjects();


	// object add/remove functions
	void						AddObject( CPhysicsHFObject* pPhysObject );
	void						AddHeightField(CHeightTileField* pField);

	void						RemoveObject( CPhysicsHFObject* pPhysObject );
	void						RemoveHeightField( CHeightTileField* pPhysObject );


	bool						TestLine(const FVector3D& start, const FVector3D& end, CollisionData_t& coll, int rayMask = COLLISION_MASK_ALL, eqPhysCollisionFilter* filter = NULL);
	bool						TestConvexSweep(btCollisionShape* shape, const Quaternion& rotation, const FVector3D& start, const FVector3D& end, CollisionData_t& coll, int rayMask = COLLISION_MASK_ALL, eqPhysCollisionFilter* filter = NULL);

	eqPhysSurfParam_t*			FindSurfaceParam(const char* name);
	eqPhysSurfParam_t*			GetSurfaceParamByID(int id);

	CEqPhysics					m_physics;

protected:

	DkList<CPhysicsHFObject*>	m_hfBodies;
	DkList<CHeightTileField*>	m_heightFields;		// heightfield is sort of static objects

	float						m_dtAccumulator;
};

extern CPhysicsEngine* g_pPhysics;

//------------------------------------------------------------


#endif // PHYSICS_H
