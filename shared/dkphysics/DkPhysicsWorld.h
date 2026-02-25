//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics powered by Jolt
///////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IDkPhysics.h"

class DkPhysicsObject;
class DkPhysicsJoint;
class DkPhysicsRope;

namespace JPH {
class PhysicsSystem;
class Body;
class BodyCreationSettings;
using ShapeRefC = RefConst<Shape>;
};

class DkPhysics : public IPhysics
{
	friend class DkPhysicsObject;
	friend class DkPhysicsJoint;
	friend class DkPhysicsRope;
public:
	~DkPhysics() = default;
	DkPhysics() = default;

	bool							IsInitialized() const { return true; }

	bool							Init(int nSceneSize);

	bool							CreateScene();
	void							DestroyScene();

	void							CastLine(const Vector3D &tracestart, const Vector3D &traceend,
											int groupmask,
											physCast_t* trace, 
											IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0
											);

	void 							CastBox(const Vector3D &tracestart, const Vector3D &traceend,
											const Vector3D& boxSize, int groupmask,
											physCast_t* trace, 
											IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0,
											Matrix4x4* transform = nullptr
											);

	void 							CastSphere(const Vector3D &tracestart, const Vector3D &traceend,
											float sphereRadius, int groupmask,
											physCast_t* trace,
											IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0
											);

	void 							CastShape(const Vector3D &tracestart, const Vector3D &traceend,
											int shapeId, int groupmask,
											physCast_t* trace,
											IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0,
											Matrix4x4* transform = nullptr
											);

	const physSurfaceInfo_t*		FindMaterial(const char* pszName) const;
	ArrayCRef<physSurfaceInfo_t>	GetMaterialList() const { return m_materials; }

	void							Simulate(float dt, int substeps = 0);
	void							DrawDebug();

	void							UpdateEvents();
	void							DoForEachObject(PhysEachObjFn procedure, void *data = nullptr);

	IPhysicsObject*					CreateStaticObject(const physObjectInfo_t& info, int nCollisionGroupFlags = 0xFFFFFFFF);
	IPhysicsObject*					CreateStudioObject(const StudioPhysData& data, int nObject = 0);
	IPhysicsJoint*					CreateJoint(IPhysicsObject* pObjectA, IPhysicsObject* pObjectB, const Matrix4x4 &transformA, const Matrix4x4 &transformB, bool bDisableCollisionBetweenBodies);

	int								AddPrimitiveShape(const physPrimitiveInfo_t &info);

	void							DestroyPhysicsObject(IPhysicsObject *pObject);
	void							DestroyPhysicsJoint(IPhysicsJoint *pJoint);
	void							DestroyPhysicsObjects();

protected:
	void							InternalCastShape(const Vector3D &tracestart, const Vector3D &traceend,
														const JPH::Shape* shape, int groupmask, 
														physCast_t* trace,
														IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0,
														Matrix4x4* transform = nullptr
														);

private:
	JPH::PhysicsSystem*				m_jphPhysSys{ nullptr };

	Array<DkPhysicsObject*>			m_objects{ PP_SL };
	Array<DkPhysicsJoint*>			m_joints{ PP_SL };

	Array<JPH::ShapeRefC>			m_collisionShapes{ PP_SL };
	Array<physSurfaceInfo_t>		m_materials{ PP_SL };

	Threading::CEqMutex				m_Mutex;
};
