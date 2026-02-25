//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics public header
//
// TODO:	cleanup in code, move some structures to physics_shared.h
//			simplify usage
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IPhysicsObject.h"
#include "dkphysics/IPhysicsJoint.h"
#include "dkphysics/IPhysicsRope.h"

class IMaterial;
struct StudioPhysData;

using PhysEachObjFn = void (*)( IPhysicsObject* pObject, void *data);

// this is an tracer
// used only by engine and external tracer in game dll (CastLine/CastBox/CastSphere)
struct physCast_t
{
	Vector3D 		origin = vec3_undef;
	Vector3D 		traceEnd = vec3_undef;
	Vector3D 		normal = vec3_forward;
	Vector2D 		uv = vec2_zero;
	float 			fraction = 1.0f;

	IPhysicsObject*	hitObj = nullptr;
	IMaterial*		hitMaterial = nullptr;
};

struct physSurfaceInfo_t // engine has array of this materials
{
	EqString 	name;

	EqString	footStepSound  = "physics.footstep";
	EqString	lightImpactSound = "physics.lightimpact";
	EqString	heavyImpactSound = "physics.heavyimpact";
	EqString	bulletImpactSound = "physics.bulletimpact";
	EqString	scrapeSound = "physics.scrape";

	float 		dampening = 1.0f;
	float 		friction= 1.0f;
	float 		density = 100.0f;
	ubyte 		surfaceword = 'C';
	int			nFlags = 0; // SURFACE_FLAG_* flags
};

enum EPhysicsPrimitive
{
	PHYSPRIM_BOX	 = 0,
	PHYSPRIM_SPHERE	 = 1,
	PHYSPRIM_CAPSULE = 2,
};

struct physPrimitiveInfo_t // TODO: remove me
{
	struct Capsule
	{
		float radius;
		float height;
	};

	struct Box
	{
		float boxSizeX;
		float boxSizeY;
		float boxSizeZ;
	};

	union
	{
		Box		boxInfo;
		Capsule	capsuleInfo;
		float 	sphereRadius;
	};

	EPhysicsPrimitive primType;
};

// collision model data
struct physShapeInfo_t // TODO: remove me
{
	void*			vertices;
	int				numVertices;

	int 			vertexPosOffset; // position offset in vertex structure
	int 			vertexSize; // vertex stride size

	uint*			indices;
	uint			numIndices;

	IMaterial*		pMaterial;
	char*			surfaceprops;
};

struct physObjectInfo_t // TODO: remove me
{
	physShapeInfo_t*	data{ nullptr };

	Vector3D		massCenter = vec3_zero;
	float			mass = 1.0f;
	float			damping = 1.0f;
	float			rotdamping = 0.0f;
	float			convexMargin = -1.0f;

	bool			genConvex = false;
	bool			flipXAxis = true;
	bool			isStatic = false;
};

class IPhysics : public IEqCoreModule
{
public:
	CORE_INTERFACE("LegacyPhysics_003")

	virtual bool 							Init(int nSceneSize) = 0;

	virtual bool 							CreateScene() = 0;
	virtual void 							DestroyScene() = 0;

	//****** Simulation ******

	virtual void 							Simulate(float dt, int substeps = 0) = 0;

	virtual void							DrawDebug() = 0;

	virtual void							UpdateEvents() = 0;

	virtual void 							CastLine(const Vector3D &tracestart, const Vector3D &traceend,int groupmask,physCast_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0) = 0;
	virtual void 							CastBox(const Vector3D &tracestart, const Vector3D &traceend, const Vector3D& boxSize,int groupmask,physCast_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0, Matrix4x4* transform = nullptr) = 0;
	virtual void 							CastSphere(const Vector3D &tracestart, const Vector3D &traceend, float sphereRadius,int groupmask,physCast_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0) = 0;
	virtual void							CastShape(const Vector3D &tracestart, const Vector3D &traceend, int shapeId, int groupmask,physCast_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0, Matrix4x4* transform = nullptr) = 0;


	virtual const physSurfaceInfo_t*		FindMaterial(const char* pszName) const = 0;
	virtual ArrayCRef<physSurfaceInfo_t>	GetMaterialList() const = 0;

	virtual void							DoForEachObject(PhysEachObjFn procedure, void *data = nullptr) = 0;

	//****** Creators ******

	virtual IPhysicsObject*					CreateStaticObject(const physObjectInfo_t& info, int nCollisionGroupFlags = 0xFFFFFFFF) = 0;
	virtual IPhysicsObject*					CreateStudioObject(const StudioPhysData& data, int nObject = 0) = 0;

	virtual IPhysicsJoint*					CreateJoint(IPhysicsObject* pObjectA, IPhysicsObject* pObjectB, const Matrix4x4 &transformA, const Matrix4x4 &transformB, bool bDisableCollisionBetweenBodies) = 0;

	//****** Geometry cache ******
	virtual int								AddPrimitiveShape(const physPrimitiveInfo_t &info) = 0;

	//****** Destructors ******

	virtual void							DestroyPhysicsObject(IPhysicsObject *pObject) = 0;
	virtual void							DestroyPhysicsJoint(IPhysicsJoint *pJoint) = 0;
	virtual void							DestroyPhysicsObjects() = 0;
};

extern IPhysics* physics;
