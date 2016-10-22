//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Geometry Formats
//////////////////////////////////////////////////////////////////////////////////

#ifndef MODEL_H
#define MODEL_H

#include "math/DkMath.h"
#include "utils/DkList.h"
#include "dktypes.h"

// for PrimitiveType_e
#include "materialsystem/renderers/ShaderAPI_defs.h"

#include "motionpackage.h"

#include "physmodel.h"

// some definitions

enum EGFPrimType_e
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

#define MAX_IKCHAIN_BONES	32		// tweak this if your "tail" longer than 32 bones

#define MAX_MODELLODS		8		// maximum lods per model

#define MAX_MOTIONPACKAGES	8		// maximum allowed motion packages to be used in model

#define MAX_STUDIOGROUPS	32		// maximum allowed materials in model

// Base model header
struct basemodelheader_s
{
	int					m_nIdentifier;
	uint8				m_nVersion;

	int					m_nFlags;	// Model flags

	int					m_nLength;	// Size of model
};

ALIGNED_TYPE(basemodelheader_s, 4) basemodelheader_t;

// Base model material search path descriptor
typedef struct materialpathdesc_s
{
	char				m_szSearchPathString[128];
}materialpathdesc_t;

// material descriptor
struct motionpackagedesc_s
{
	char				packagename[128]; //only this?
};

ALIGNED_TYPE(motionpackagedesc_s, 4) motionpackagedesc_t;

// this structure holds in transformation
struct joint_t
{
	char				name[44]; // bone name

	int					bone_id; // index of this bone
	int					parentbone; // parent index

	Matrix4x4			absTrans; // base absolute transform
	Matrix4x4			localTrans; // local transform

	Vector3D			position; // bone initial position
	Vector3D			rotation; // bone initial rotation

	DkList<int>			childs; // child bones

	int					chain_id;
	int					link_id;
};

// animation frames for each bone
struct boneframe_t
{
	int				numframes;
	animframe_t*	keyframes;
};

// model animation
struct modelanimation_t
{
	char			name[44];

	// bones, in count of studiohwdata_t::numJoints
	boneframe_t*	bones;
};

// equilibrium model format
#include "model_eq.h"

// egf model hardware vertex
struct EGFHwVertex_t
{
	EGFHwVertex_t() {}

	EGFHwVertex_t(studiovertexdesc_t* initFrom)
	{
		pos = initFrom->point;
		texcoord = initFrom->texCoord;

		tangent = initFrom->tangent;
		binormal = initFrom->binormal;
		normal = initFrom->normal;

		for(int i = 0; i < 4; i++)
			boneIndices[i] = -1;

		memset(boneWeights,0,sizeof(boneWeights));

		for(int i = 0; i < initFrom->boneweights.numweights; i++)
		{
			boneIndices[i] = initFrom->boneweights.bones[i];
			boneWeights[i] = initFrom->boneweights.weight[i];
		}
	}

	Vector3D		pos;
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

// shape cache data
struct physmodelshapecache_t
{
	physgeominfo_t	shape_info;
	void*			cachedata;
};

struct physobjectdata_t
{
	physobject_t	object;
	void*			shapeCache[MAX_GEOM_PER_OBJECT];		// indexes of geomdata
};

// physics model data from POD
struct physmodeldata_t
{
	int modeltype;

	physobjectdata_t* objects;	// array, because it may be dynamic or ragdoll.
	int numobjects;

	physjoint_t* joints;
	int numjoints;				// because it may be merged

	physmodelshapecache_t* shapes;
	int numshapes;

	Vector3D*	vertices;
	int			numVertices;

	int*		indices;
	int			numIndices;
};

struct hwgroup_desc_t
{
	int	firstindex;
	int indexcount;
};

// lod data
struct hwmodelref_t
{
	hwgroup_desc_t*		groupdescs; // offset in hw index buffer to this lod, for each geometry group
};

// model motion package loaded and expanded data
struct studiomotiondata_t
{
	// animations
	int					numanimations;
	modelanimation_t*	animations;

	// sequences
	int					numsequences;
	sequencedesc_t*		sequences;

	// events
	int					numevents;
	sequenceevent_t*	events;

	// pose controllers
	int					numposecontrollers;
	posecontroller_t*	posecontrollers;

	animframe_t*		frames;
};

// hardware data for the MOD_STUDIO
struct studiohwdata_t
{
	// loaded/cached studio model
	studiohdr_t*		pStudioHdr;

	// POD data
	physmodeldata_t		m_physmodel;

	// lod models
	hwmodelref_t*		modelrefs;

	// joints, for
	joint_t*			joints;

	// shared cacheable motion data, this may be used for different models
	studiomotiondata_t*	motiondata[MAX_MOTIONPACKAGES];
	int					numMotionPackages;
};

#endif //MODEL_H
