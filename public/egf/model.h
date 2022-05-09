//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry Formats
//////////////////////////////////////////////////////////////////////////////////

#ifndef MODEL_H
#define MODEL_H

#include "core/dktypes.h"
#include "core/ppmem.h"
#include "ds/Array.h"
#include "utils/strtools.h"

#include "math/DkMath.h"

// for ER_PrimitiveType
#include "materialsystem1/renderers/ShaderAPI_defs.h"

#include "motionpackage.h"

#include "physmodel.h"



// some definitions

enum EEGFPrimType
{
	EGFPRIM_INVALID			= -1,
	EGFPRIM_TRIANGLES		= PRIM_TRIANGLES,
	EGFPRIM_TRIANGLE_FAN	= PRIM_TRIANGLE_FAN,
	EGFPRIM_TRI_STRIP		= PRIM_TRIANGLE_STRIP,
};

// index sizes for vertex buffers
#define INDEX_SIZE_SHORT	2
#define INDEX_SIZE_INT		4

// LIMITS for all model formats

enum EEGFLimits
{
	MAX_IKCHAIN_BONES	=	32,		// tweak this if your "tail" longer than 32 bones
	MAX_MODELLODS		=	8,		// maximum lods per model
	MAX_MOTIONPACKAGES	=	8,		// maximum allowed motion packages to be used in model
	MAX_STUDIOMATERIALS	=	32,		// maximum allowed materials in model
};

// Base model header
struct basemodelheader_s
{
	int					ident;
	uint8				version;

	int					flags;	// Model flags

	int					size;	// Size of model
};

ALIGNED_TYPE(basemodelheader_s, 4) basemodelheader_t;

// Base model material search path descriptor
struct materialpathdesc_s
{
	char				searchPath[128];
};

ALIGNED_TYPE(materialpathdesc_s, 4) materialpathdesc_t;

// material descriptor
struct motionpackagedesc_s
{
	char				packageName[128]; //only this?
};

ALIGNED_TYPE(motionpackagedesc_s, 4) motionpackagedesc_t;

//----------------------------------------------------------------------------------------------

// equilibrium model format
#include "model_eq.h"

// egf model hardware vertex
typedef struct EGFHwVertex_s
{
	EGFHwVertex_s() {}

	EGFHwVertex_s(studiovertexdesc_t& initFrom)
	{
		pos = Vector4D(initFrom.point, 1.0f);
		texcoord = initFrom.texCoord;

		tangent = initFrom.tangent;
		binormal = initFrom.binormal;
		normal = initFrom.normal;

		for(int i = 0; i < 4; i++)
			boneIndices[i] = -1;

		memset(boneWeights,0,sizeof(boneWeights));

		for(int i = 0; i < initFrom.boneweights.numweights; i++)
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
} EGFHwVertex_t;

// Declare the EGF vertex format
static VertexFormatDesc_t g_EGFHwVertexFormat[] = {
	{ 0, 4, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_HALF, "position" },		// position
	{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "texcoord" },		// texcoord 0

	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "tangent" },		// Tangent (TC1)
	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "binormal" },		// Binormal (TC2)
	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "normal" },		// Normal (TC3)

	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "boneid" },		// Bone indices (hw skinning), (TC4)
	{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF, "bonew" }			// Bone weights (hw skinning), (TC5)
};

// shape cache data
struct studioPhysShapeCache_t
{
	physgeominfo_t	shape_info;
	void*			cachedata;
};

struct studioPhysObject_t
{
	PPMEM_MANAGED_OBJECT();

	char			name[32];
	physobject_t	object;
	void*			shapeCache[MAX_GEOM_PER_OBJECT];		// indexes of geomdata
};

// physics model data from POD
struct studioPhysData_t
{
	PPMEM_MANAGED_OBJECT();

	studioPhysData_t()
	{
		modeltype = 0;

		objects = nullptr;
		numObjects = 0;

		joints = nullptr;
		numJoints = 0;

		shapes = nullptr;
		numShapes = 0;

		vertices = nullptr;
		numVertices = 0;

		indices = nullptr;
		numIndices = 0;
	}

	int modeltype;

	studioPhysObject_t* objects;	// array, because it may be dynamic or ragdoll.
	int numObjects;

	physjoint_t* joints;
	int numJoints;				// because it may be merged

	studioPhysShapeCache_t* shapes;
	int numShapes;

	Vector3D*	vertices;
	int			numVertices;

	int*		indices;
	int			numIndices;
};

// hardware data for the MOD_STUDIO
struct studioHwData_t
{
	PPMEM_MANAGED_OBJECT();

	studioHwData_t()
	{
		studio = nullptr;
		modelrefs = nullptr;
		joints = nullptr;
		numMotionPackages = 0;
		numMaterialGroups = 0;
	}

	// loaded/cached studio model
	studiohdr_t*		studio;

	int					numUsedMaterials;
	int					numMaterialGroups;

	// POD data
	studioPhysData_t		physModel;

	// lod models
	struct modelRef_t
	{
		// offset in hw index buffer to this lod, for each geometry group
		struct groupDesc_t
		{
			int	firstindex;
			int indexcount;
		} *groupDescs;
	} *modelrefs;

	// joints, for
	struct joint_t
	{
		char				name[44]; // bone name

		int					bone_id; // index of this bone
		int					parentbone; // parent index

		Matrix4x4			absTrans; // base absolute transform
		Matrix4x4			localTrans; // local transform

		Vector3D			position; // bone initial position
		Vector3D			rotation; // bone initial rotation

		Array<int>			childs; // child bones

		int					chain_id;
		int					link_id;
	}*	joints;

	// model motion package loaded and expanded data
	struct motionData_t
	{
		// animations
		int					numAnimations;

		struct animation_t
		{
			char name[44];

			// bones, in count of studiohwdata_t::numJoints
			struct boneframe_t
			{
				int				numFrames;
				animframe_t*	keyFrames;
			}*	bones;
		}*	animations;

		// sequences
		int					numsequences;
		sequencedesc_t*		sequences;

		// events
		int					numEvents;
		sequenceevent_t*	events;

		// pose controllers
		int					numPoseControllers;
		posecontroller_t*	poseControllers;

		animframe_t*		frames;
	}*	motiondata[MAX_MOTIONPACKAGES]; // shared cacheable motion data, this may be used for different models

	int					numMotionPackages;
};

typedef studioHwData_t::modelRef_t								studioModelRef_t;
typedef studioHwData_t::modelRef_t::groupDesc_t					studioModelRefGroupDesc_t;
typedef studioHwData_t::joint_t									studioJoint_t;
typedef studioHwData_t::motionData_t							studioMotionData_t;
typedef studioHwData_t::motionData_t::animation_t				studioAnimation_t;
typedef studioHwData_t::motionData_t::animation_t::boneframe_t	studioBoneFrame_t;

//------------------------------------------------------------------------------

inline int PhysModel_FindObjectId(studioPhysData_t* model, const char* name)
{
	for(int i = 0; i < model->numObjects; i++)
	{
		if(!stricmp(model->objects[i].name, name))
			return i;
	}

	return -1;
}

#endif //MODEL_H
