//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry Formats
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "studiomodel.h"
#include "physmodel.h"
#include "motionpackage.h"

constexpr EqStringRef s_egfMotionPackageExt = "mop";
constexpr EqStringRef s_egfPhysicsObjectExt = "pod";

// those are just runtime limits, they don't affect file data.
static constexpr const int MAX_MOTIONPACKAGES = 8;		// maximum allowed motion packages to be used in model
static constexpr const int MAX_STUDIOMATERIALS = 32;	// maximum allowed materials in model

enum EStudioPrimType
{
	STUDIO_PRIM_TRIANGLES	= 0,
	STUDIO_PRIM_TRI_STRIP	= 2,
};

// Base model header
struct basemodelheader_s
{
	int		ident;
	uint8	version;
	int		flags;
	int		size;
};
ALIGNED_TYPE(basemodelheader_s, 4) basemodelheader_t;

// LumpFile header
struct lumpfilehdr_s
{
	int		ident;
	int		version;
	int		numLumps;
};
ALIGNED_TYPE(lumpfilehdr_s, 4) lumpfilehdr_t;

struct lumpfilelump_s
{
	int		type;
	int		size;
};
ALIGNED_TYPE(lumpfilelump_s, 4) lumpfilelump_t;

//----------------------------------------------------------------------------------------------

// physics model data from POD
struct StudioPhysData
{
	using ShapeRefList = void*[MAX_PHYS_GEOM_PER_OBJECT];
	struct Shape
	{
		physgeominfo_t	shapeInfo;
		void*			cacheRef;
	};

	struct Object
	{
		physobject_t	object;
		EqString		name;
		ShapeRefList	shapeCacheRefs;
	};

	ArrayRef<Object>		objects{ nullptr };
	ArrayRef<physjoint_t>	joints{ nullptr };
	ArrayRef<Shape>			shapes{ nullptr };
	ArrayRef<Vector3D>		vertices{ nullptr };
	ArrayRef<int>			indices{ nullptr };

	EPhysModelUsage			usageType{ PHYSMODEL_USAGE_NONE };
};

using StudioPhyObjData = StudioPhysData::Object;
using StudioPhyShapeData = StudioPhysData::Shape;

inline int PhysModel_FindObjectId(const StudioPhysData& physData, const char* name)
{
	const int idx = arrayFindIndexF(physData.objects, [name](const StudioPhyObjData& obj) {
		return !obj.name.CompareCaseIns(name);
	});
	return idx;
}

struct StudioMotionData
{
	EqString					name;
	ArrayRef<animationdesc_t>	animations{ nullptr };
	ArrayRef<sequencedesc_t>	sequences{ nullptr };
	ArrayRef<sequenceevent_t>	events{ nullptr };
	ArrayRef<posecontroller_t>	poseControllers{ nullptr };
	ArrayRef<animframe_t>		frames{ nullptr };
};

struct StudioJoint
{
	Matrix4x4			absTrans;
	Matrix4x4			invAbsTrans;
	Matrix4x4			localTrans;
	FixedArray<int, 16>	childs;

	const studioBoneDesc_t* bone{ nullptr };

	int					boneId{ -1 };
	int					parent{ -1 };

	int					ikChainId{ -1 };
	int					ikLinkId{ -1 };
};

