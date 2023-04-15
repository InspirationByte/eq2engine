//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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

// 2.2 lbs / kg
#define POUNDS_PER_KG	(2.2f)
#define KG_PER_POUND	(1.0f/POUNDS_PER_KG)

// convert from pounds to kg
#define lbs2kg(x)		((x)*KG_PER_POUND)
#define kg2lbs(x)		((x)*POUNDS_PER_KG)

#define SURFACE_FLAG_CLIMBABLE			(1 << 1)
#define SURFACE_FLAG_NO_DECAL			(1 << 2)
#define SURFACE_FLAG_SILENT				(1 << 3)

#define PHYSPRIM_BOX				0
#define PHYSPRIM_SPHERE				1
#define PHYSPRIM_CAPSULE			2

class IMaterial;
struct studioPhysData_t;

typedef void (*EACHOBJECTFUNCTION)( IPhysicsObject* pObject, void *data);

// this is an tracer
// used only by engine and external tracer in game dll (TraceLine/TraceBox/TraceSphere)
struct internaltrace_t
{
	Vector3D origin;
	Vector3D traceEnd;
	Vector3D normal;

	Vector2D uv;

	IPhysicsObject*	hitObj;
	IMaterial*		hitMaterial;
	
	float fraction;
};

struct phySurfaceMaterial_t // engine has array of this materials
{
	float dampening;
	float friction;

	float density;

	ubyte surfaceword;

	EqString name;

	EqString footStepSound;
	EqString lightImpactSound;
	EqString heavyImpactSound;
	EqString bulletImpactSound;
	EqString scrapeSound;

	int nFlags; // SURFACE_FLAG_* flags
};

inline void CopyMaterialParams(phySurfaceMaterial_t* pFrom, phySurfaceMaterial_t* pTo)
{
	pTo->dampening = pFrom->dampening;
	pTo->friction = pFrom->friction;
	pTo->density = pFrom->density;

	pTo->surfaceword = pFrom->surfaceword;

	pTo->footStepSound = xstrdup(pFrom->footStepSound);
	pTo->lightImpactSound = xstrdup(pFrom->lightImpactSound);
	pTo->heavyImpactSound = xstrdup(pFrom->heavyImpactSound);
	pTo->bulletImpactSound = xstrdup(pFrom->bulletImpactSound);
	pTo->scrapeSound = xstrdup(pFrom->scrapeSound);
	pTo->nFlags = pFrom->nFlags;
}

inline void DefaultMaterialParams(phySurfaceMaterial_t* pMaterial)
{
	pMaterial->dampening = 1.0f;
	pMaterial->friction = 1.0f;
	pMaterial->density = 100.0f;

	pMaterial->surfaceword = 'C';

	pMaterial->footStepSound = xstrdup("physics.footstep");
	pMaterial->lightImpactSound = xstrdup("physics.lightimpact");
	pMaterial->heavyImpactSound = xstrdup("physics.heavyimpact");
	pMaterial->bulletImpactSound = xstrdup("physics.bulletimpact");
	pMaterial->scrapeSound = xstrdup("physics.bulletimpact");
	pMaterial->nFlags = 0;
}

struct capsuleInfo_t // TODO: remove me
{
	float radius;
	float height;
};

struct boxInfo_t // TODO: remove me
{
	float boxSizeX;
	float boxSizeY;
	float boxSizeZ;
};

struct pritimiveinfo_t // TODO: remove me
{
	int8 primType;

	union
	{
		float sphereRadius;
		capsuleInfo_t capsuleInfo;
		boxInfo_t boxInfo;
	};
};

// collision model data
struct dkCollideData_t // TODO: remove me
{
	void* vertices;
	int	numVertices;

	int vertexPosOffset; // position offset in vertex structure
	int vertexSize; // vertex stride size

	uint* indices;
	uint numIndices;

	IMaterial* pMaterial;
	char* surfaceprops;
};

struct physmodelcreateinfo_t // TODO: remove me
{
	dkCollideData_t* data;

	Vector3D mass_center;
	float mass;
	float damping;
	float rotdamping;

	bool genConvex;
	float convexMargin;
	bool flipXAxis;
	// float convexDecompRatio;

	bool isStatic; // not affected by mass, damping and impulses
};

static void SetDefaultPhysModelInfoParams(physmodelcreateinfo_t* info) // TODO: remove me
{
	info->mass_center = vec3_zero;
	info->mass = 1.0f;
	info->isStatic = false;
	info->damping = 1.0f;
	info->rotdamping = 0.0f;
	info->convexMargin = -1.0f;
	info->genConvex = false;
	info->flipXAxis = true;
}

class IPhysics : public IEqCoreModule
{
public:
	CORE_INTERFACE("LegacyPhysics_001")

	virtual bool 								Init(int nSceneSize) = 0;						// Initialize physics
	virtual bool 								IsSupportsHardwareAcceleration() = 0;			// Returns true if hardware acceleration is available

	virtual bool 								CreateScene() = 0;								// Makes physics scene
	virtual void 								DestroyScene() = 0;								// Destroys all scene including shape cache

	//****** Simulation ******

	virtual void 								Simulate(float dt, int substeps = 0) = 0;		// Updates physics scene

	virtual void								DrawDebug() = 0;

	virtual void								UpdateEvents() = 0;								// updates events

	virtual void 								InternalTraceLine(const Vector3D &tracestart, const Vector3D &traceend,int groupmask,internaltrace_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0) = 0;	// internal trace line
	virtual void 								InternalTraceBox(const Vector3D &tracestart, const Vector3D &traceend, const Vector3D& boxSize,int groupmask,internaltrace_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0, Matrix4x4* transform = nullptr) = 0;	// internal trace box
	virtual void 								InternalTraceSphere(const Vector3D &tracestart, const Vector3D &traceend, float sphereRadius,int groupmask,internaltrace_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0) = 0; // internal trace sphere
	virtual void								InternalTraceShape(const Vector3D &tracestart, const Vector3D &traceend, int shapeId, int groupmask,internaltrace_t *trace, IPhysicsObject** pIgnoreList = nullptr, int numIgnored = 0, Matrix4x4* transform = nullptr) = 0;


	virtual phySurfaceMaterial_t*				FindMaterial(const char* pszName) = 0;			// finds surface material parameters by it's name

	virtual Array<phySurfaceMaterial_t*>*		GetMaterialList() = 0;				// returns pointer to material list

	virtual void								DoForEachObject(EACHOBJECTFUNCTION procedure, void *data = nullptr) = 0;	// special command that enumerates every physics object and calls EACHOBJECTFUNCTION procedure for it (such as explosion, etc.)

	//****** Creators ******

	virtual IPhysicsObject*						CreateStaticObject(physmodelcreateinfo_t *info, int nCollisionGroupFlags = 0xFFFFFFFF) = 0;

	virtual IPhysicsObject*						CreateObject(const studioPhysData_t* data, int nObject = 0) = 0; // Creates physics object
	virtual IPhysicsObject*						CreateObjectCustom(int numShapes, int* shapeIdxs, const char* surfaceProps, float mass) = 0; // Creates physics object

	virtual IPhysicsJoint*						CreateJoint(IPhysicsObject* pObjectA, IPhysicsObject* pObjectB, const Matrix4x4 &transformA, const Matrix4x4 &transformB, bool bDisableCollisionBetweenBodies) = 0; // creates physics joint
	virtual IPhysicsRope*						CreateRope(const Vector3D &pointA, const Vector3D &pointB, int numSegments) = 0; // creates physics rope

	//****** Geometry cache ******
	virtual int									AddPrimitiveShape(pritimiveinfo_t &info) = 0;

	//****** Destructors ******

	virtual void								DestroyPhysicsObject(IPhysicsObject *pObject) = 0; // removes physics object from world
	virtual void								DestroyPhysicsJoint(IPhysicsJoint *pJoint) = 0; // removes physics joint from world
	virtual void								DestroyPhysicsRope(IPhysicsRope *pRope) = 0; // removes physics rope from world

	virtual void								DestroyShapeCache() = 0; // destroys shape cache
	virtual void								DestroyPhysicsObjects() = 0; // releases all physics objects
};

extern IPhysics* physics;
