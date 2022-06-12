//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry Formats
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/renderers/ShaderAPI_defs.h"	// ER_PrimitiveType

#include "studiomodel.h"
#include "physmodel.h"
#include "motionpackage.h"

enum EEGFPrimType
{
	EGFPRIM_INVALID			= -1,
	EGFPRIM_TRIANGLES		= PRIM_TRIANGLES,
	EGFPRIM_TRIANGLE_FAN	= PRIM_TRIANGLE_FAN,
	EGFPRIM_TRI_STRIP		= PRIM_TRIANGLE_STRIP,
};

// LIMITS for all model formats
enum EEGFLimits
{
	MAX_MOTIONPACKAGES	=	8,		// maximum allowed motion packages to be used in model
	MAX_STUDIOMATERIALS	=	32,		// maximum allowed materials in model
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

//----------------------------------------------------------------------------------------------

// egf model hardware vertex
struct EGFHwVertex_t
{
	EGFHwVertex_t() = default;
	EGFHwVertex_t(const studiovertexdesc_t& initFrom)
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
};

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
	char			name[32];
	physobject_t	object;
	void*			shapeCache[MAX_GEOM_PER_OBJECT];		// indexes of geomdata
};

// physics model data from POD
struct studioPhysData_t
{
	int						modeltype{ 0 };

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

// hardware data for the MOD_STUDIO
struct studioHwData_t
{
	studiohdr_t*		studio{ nullptr };

	int					numUsedMaterials{ 0 };
	int					numMaterialGroups{ 0 };

	studioPhysData_t	physModel;

	// lod models
	struct modelRef_t
	{
		// offset in hw index buffer to this lod, for each geometry group
		struct groupDesc_t
		{
			int	firstindex;
			int indexcount;
		} *groupDescs;
	} *modelrefs{ nullptr };

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

		Array<int>			childs{ PP_SL }; // child bones

		int					chain_id;
		int					link_id;
	}*	joints{ nullptr };

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
	}*	motiondata[MAX_MOTIONPACKAGES];

	int					numMotionPackages{ 0 };
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
