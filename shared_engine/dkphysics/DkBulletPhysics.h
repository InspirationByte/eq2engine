//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics powered by Bullet
///////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IDkPhysics.h"

class CPhysicsObject;
class CCharacterController;
class DkPhysicsJoint;
class DkPhysicsRope;


#define __BT_SKIP_UINT64_H	// SDL2 support

// Bullet headers
#include <btBulletDynamicsCommon.h>
#include <BulletSoftBody/btSoftBodyRigidBodyCollisionConfiguration.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>

class DkPhysics : public IPhysics
{
public:
	// Make it friend because we accessing the private members
	friend class								CPhysicsObject;
	friend class								CCharacterController;
	friend class								DkPhysicsJoint;
	friend class								DkPhysicsRope;

	// Constructor & destructor
												DkPhysics();
												~DkPhysics();

	bool										IsInitialized() const {return true;}

	// Initialize physics
	bool										Init(int nSceneSize);

	// Makes physics scene
	bool										CreateScene();

	// Destroy scene
	void										DestroyScene();

	// Returns true if hardware acceleration is available
	bool										IsSupportsHardwareAcceleration();

	// Generic traceLine for physics
	void										InternalTraceLine(	const Vector3D &tracestart, const Vector3D &traceend,
																	int groupmask,
																	internaltrace_t* trace, 
																	IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0
																	);

	void 										InternalTraceBox(const Vector3D &tracestart, const Vector3D &traceend,
																	const Vector3D& boxSize,int groupmask,
																	internaltrace_t* trace, 
																	IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0,
																	Matrix4x4* transform = nullptr
																	);

	void 										InternalTraceSphere(const Vector3D &tracestart, const Vector3D &traceend,
																	float sphereRadius,int groupmask,
																	internaltrace_t* trace,
																	IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0
																	);

	void										InternalTraceShape(const Vector3D &tracestart, const Vector3D &traceend,
																	int shapeId, int groupmask, 
																	internaltrace_t* trace,
																	IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0,
																	Matrix4x4* transform = nullptr
																	);


	// finds material by it's name
	phySurfaceMaterial_t*						FindMaterial(const char* pszName);

	// returns pointer to material list
	Array<phySurfaceMaterial_t*>*				GetMaterialList() {return &m_physicsMaterialDesc;}

	// Update funciton
	void										Simulate(float dt, int substeps = 0);
	
	void										DrawDebug();

	// updates events
	void										UpdateEvents();

	// special command that enumerates every physics object and calls EACHOBJECTFUNCTION procedure for it (such as explosion, etc.)
	void										DoForEachObject(EACHOBJECTFUNCTION procedure, void *data = nullptr);

	//****** Creators ******
	// Any physics object creator
	IPhysicsObject*								CreateStaticObject(physmodelcreateinfo_t *info, int nCollisionGroupFlags = 0xFFFFFFFF);

	IPhysicsObject*								CreateObject(const studioPhysData_t* data, int nObject = 0); // Creates physics object

	IPhysicsObject*								CreateObjectCustom(int numShapes, int* shapeIdxs, const char* surfaceProps, float mass); // Creates physics object

	// creates physics joint
	IPhysicsJoint*								CreateJoint(IPhysicsObject* pObjectA, IPhysicsObject* pObjectB, const Matrix4x4 &transformA, const Matrix4x4 &transformB, bool bDisableCollisionBetweenBodies);

	// creates physics rope
	IPhysicsRope*								CreateRope(const Vector3D &pointA, const Vector3D &pointB, int numSegments);

	// creates primitive shape
	int											AddPrimitiveShape(pritimiveinfo_t &info);


	//****** Object Destructor ******
	void										DestroyPhysicsObject(IPhysicsObject *pObject);

	void										DestroyPhysicsJoint(IPhysicsJoint *pJoint);

	void										DestroyPhysicsRope(IPhysicsRope *pRope);

	void										DestroyPhysicsObjects();

	void										DestroyShapeCache();


protected:
	btRigidBody*								LocalCreateRigidBody(float mass, const Vector3D &massCenter, const btTransform& startTransform,btCollisionShape* shape);

private:
	btBroadphaseInterface*						m_broadphase;

	btCollisionDispatcher*						m_dispatcher;

	btConstraintSolver*							m_solver;

	btSoftRigidDynamicsWorld*					m_dynamicsWorld;

	btSoftBodyWorldInfo							m_softBodyWorldInfo;

	// for tracing
	btBoxShape									hBox;
	btSphereShape								hSphere;

	btSoftBodyRigidBodyCollisionConfiguration*	m_collisionConfiguration;

	Array <IPhysicsObject*>						m_pPhysicsObjectList{ PP_SL };
	Array <IPhysicsJoint*>						m_pJointList{ PP_SL };
	Array <IPhysicsRope*>						m_pRopeList{ PP_SL };

	Array <btCollisionShape*>					m_collisionShapes{ PP_SL };
	Array <phySurfaceMaterial_t*>				m_physicsMaterialDesc{ PP_SL };
	Array <btTriangleIndexVertexArray*>			m_triangleMeshes{ PP_SL };

	int											m_nSceneSize;

	float										m_fPhysicsNextTime;
	float										m_fPhysicsCurTimestep;

	Threading::CEqMutex							m_Mutex;
	Threading::CEqSignal						m_WorkDoneSignal;
};
