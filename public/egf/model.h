//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry Formats
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "studiomodel.h"
#include "physmodel.h"
#include "motionpackage.h"

typedef struct VertexFormatDesc_s VertexFormatDesc_t;

// those are just runtime limits, they don't affect file data.
static constexpr const int MAX_MOTIONPACKAGES = 8;		// maximum allowed motion packages to be used in model
static constexpr const int MAX_STUDIOMATERIALS = 32;	// maximum allowed materials in model

enum EEGFPrimType
{
	EGFPRIM_INVALID			= -1,
	EGFPRIM_TRIANGLES		= 0,
	EGFPRIM_TRIANGLE_FAN	= 1,
	EGFPRIM_TRI_STRIP		= 2,
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

// shape cache data
struct studioPhysShapeCache_t
{
	physgeominfo_t	shapeInfo;
	void*			cachedata;
};

struct studioPhysObject_t
{
	char			name[32];
	physobject_t	object;
	void*			shapeCache[MAX_PHYS_GEOM_PER_OBJECT];		// indexes of geomdata
};

// physics model data from POD
struct studioPhysData_t
{
	int						usageType{ 0 };

	studioPhysObject_t*		objects{ nullptr };
	int						numObjects{ 0 };

	physjoint_t*			joints{ nullptr };
	int						numJoints{ 0 };

	studioPhysShapeCache_t* shapes{ nullptr };
	int						numShapes{ 0 };

	Vector3D*				vertices{ nullptr };
	int						numVertices{ 0 };

	int*					indices{ nullptr };
	int						numIndices{ 0 };
};

inline int PhysModel_FindObjectId(const studioPhysData_t* model, const char* name)
{
	for (int i = 0; i < model->numObjects; i++)
	{
		if (!stricmp(model->objects[i].name, name))
			return i;
	}

	return -1;
}

struct studioMotionData_t
{
	// animations
	int					numAnimations{ 0 };

	struct animation_t
	{
		char	name[44]{ 0 };
		//int		numFrames{ 0 };

		// bones, in count of studiohwdata_t::numJoints
		struct boneKeyFrames_t
		{
			int				numFrames{ 0 };
			animframe_t*	keyFrames{ nullptr };
		}*		bones{ nullptr };
	}*animations{ nullptr };

	// sequences
	int					numsequences{ 0 };
	sequencedesc_t*		sequences{ nullptr };

	// events
	int					numEvents{ 0 };
	sequenceevent_t*	events{ nullptr };

	// pose controllers
	int					numPoseControllers{ 0 };
	posecontroller_t*	poseControllers{ nullptr };

	animframe_t*		frames{ nullptr };
};

struct studioJoint_t
{
	Matrix4x4			absTrans;
	Matrix4x4			localTrans;
	FixedArray<int, 16>	childs;

	const bonedesc_t* bone{ nullptr };

	int					boneId{ -1 };
	int					parent{ -1 };

	int					ikChainId{ -1 };
	int					ikLinkId{ -1 };
};

typedef studioMotionData_t::animation_t						studioAnimation_t;
typedef studioMotionData_t::animation_t::boneKeyFrames_t	studioBoneAnimation_t;

//------------------------------------------------------------------------------

// egf model hardware vertex
struct EGFHwVertex_t
{
	static int GetVertexFormatDesc(const VertexFormatDesc_t** desc);

	EGFHwVertex_t() = default;
	EGFHwVertex_t(const studiovertexdesc_t& initFrom)
	{
		ASSERT(initFrom.boneweights.numweights <= MAX_MODEL_VERTEX_WEIGHTS);

		pos = Vector4D(initFrom.point, 1.0f);
		texcoord = initFrom.texCoord;

		tangent = initFrom.tangent;
		binormal = initFrom.binormal;
		normal = initFrom.normal;

		memset(boneWeights, 0, sizeof(boneWeights));
		for (int i = 0; i < MAX_MODEL_VERTEX_WEIGHTS; i++)
			boneIndices[i] = -1;

		for (int i = 0; i < min(initFrom.boneweights.numweights, MAX_MODEL_VERTEX_WEIGHTS); i++)
		{
			boneIndices[i] = initFrom.boneweights.bones[i];
			boneWeights[i] = initFrom.boneweights.weight[i];
		}
	}

	TVec4D<half>	pos;
	TVec2D<half>	texcoord;

	TVec3D<half>	tangent;
	half			unused1;	// half float types are unsupported with v3d, turn them into v4d

	TVec3D<half>	binormal;
	half			unused2;

	TVec3D<half>	normal;
	half			unused3;

	half			boneIndices[4];
	half			boneWeights[4];
};