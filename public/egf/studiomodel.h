//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define EQUILIBRIUM_MODEL_VERSION		13
#define EQUILIBRIUM_MODEL_SIGNATURE		MCHAR4('E','Q','G','F')

#define ENABLE_OLD_VERTEX_FORMAT		0

static constexpr const int MAX_MODEL_LODS				= 8;
static constexpr const int MAX_MODEL_PATH_LENGTH		= 256;
static constexpr const int MAX_MODEL_PART_PATH_LENGTH	= 128;
static constexpr const int MAX_MODEL_PART_NAME_LENGTH	= 44;
static constexpr const int MAX_MODEL_VERTEX_WEIGHTS		= 4;

static constexpr const uint8 EGF_INVALID_IDX = 0xff;

enum EStudioFlags
{
	STUDIO_FLAG_NEW_VERTEX_FMT	= (1 << 0),
};

enum EStudioLODFlags
{
	STUDIO_LOD_FLAG_MANUAL		= (1 << 0),
};

enum EStudioVertexStreamType : int
{
	STUDIO_VERTSTREAM_POS_UV		= 0,
	STUDIO_VERTSTREAM_TBN			= 1,
	STUDIO_VERTSTREAM_BONEWEIGHT	= 2,
	STUDIO_VERTSTREAM_COLOR			= 3,

	// TODO: more UVs
};

enum EStudioVertexStreamFlag : int
{
	STUDIO_VERTFLAG_POS_UV			= (1 << STUDIO_VERTSTREAM_POS_UV),
	STUDIO_VERTFLAG_TBN				= (1 << STUDIO_VERTSTREAM_TBN),
	STUDIO_VERTFLAG_BONEWEIGHT		= (1 << STUDIO_VERTSTREAM_BONEWEIGHT),
	STUDIO_VERTFLAG_COLOR			= (1 << STUDIO_VERTSTREAM_COLOR),
};

// use EGF_VERSION_CHANGE tags for defining changes

// Base model material search path descriptor
struct materialPathDesc_s
{
	char				searchPath[MAX_MODEL_PART_PATH_LENGTH];
};
ALIGNED_TYPE(materialPathDesc_s, 4) materialPathDesc_t;

// material descriptor
struct motionPackageDesc_s
{
	char				packageName[MAX_MODEL_PART_PATH_LENGTH]; //only this?
};
ALIGNED_TYPE(motionPackageDesc_s, 4) motionPackageDesc_t;

struct studioIkLink_s
{
	int bone;

	// link limits
	Vector3D	mins;
	Vector3D	maxs;

	float		damping;
};
ALIGNED_TYPE(studioIkLink_s, 4) studioIkLink_t;

// studiomodel ik chain
struct studioIkChain_s
{
	char name[MAX_MODEL_PART_NAME_LENGTH];

	int numLinks;
	int linksOffset;

	inline studioIkLink_t *pLink( int i ) const 
	{
		return (studioIkLink_t *)(((ubyte *)this) + linksOffset) + i; 
	};
};
ALIGNED_TYPE(studioIkChain_s, 4) studioIkChain_t;

// bone attachment
struct studioTransform_s
{
	char			name[MAX_MODEL_PART_NAME_LENGTH];
	Matrix4x4		transform;
	uint8			attachBoneIdx;
};
ALIGNED_TYPE(studioTransform_s, 4) studioTransform_t;

// lod parameters
struct studioLodParams_s
{
	float			distance;
	ubyte			flags;
	ubyte			unused[3];
};
ALIGNED_TYPE(studioLodParams_s, 4) studioLodParams_t;

// lod models
struct studioLodModel_s
{
	uint8			modelsIndexes[MAX_MODEL_LODS]; // lod model indexes, points to studioMeshGroupDesc_t
};
ALIGNED_TYPE(studioLodModel_s, 4) studioLodModel_t;

// body groups
struct studioBodyGroup_s
{
	char			name[MAX_MODEL_PART_NAME_LENGTH];		// bodygroup name
	uint8			lodModelIndex;							// lod model index, points to studioLodModel_t

	ubyte			unused[7];								// unused
};
ALIGNED_TYPE(studioBodyGroup_s, 4) studioBodyGroup_t;

// material descriptor
struct studioMaterialDesc_s
{
	char			materialname[32]; //only this?
};
ALIGNED_TYPE(studioMaterialDesc_s, 4) studioMaterialDesc_t;

//-------------------------------------------

// base vertex stream
struct studioVertexPosUv_s
{
	Vector3D		point;
	Vector2D		texCoord;
};
ALIGNED_TYPE(studioVertexPosUv_s, 4) studioVertexPosUv_t;

// Weight extra vertex stream
struct studioBoneWeight_s
{
	float			weight[MAX_MODEL_VERTEX_WEIGHTS];
	int8			bones[MAX_MODEL_VERTEX_WEIGHTS];
	int8			numweights;

	ubyte			unused; // unused
};
ALIGNED_TYPE(studioBoneWeight_s, 4) studioBoneWeight_t;

// TBN extra vertex stream
struct studioVertexTBN_s
{
	Vector3D		tangent;
	Vector3D		binormal;
	Vector3D		normal;
};
ALIGNED_TYPE(studioVertexTBN_s, 4) studioVertexTBN_t;

// Color extra vertex stream
struct studioVertexColor_s
{
	uint			color;	// dword packed color
};
ALIGNED_TYPE(studioVertexColor_s, 4) studioVertexColor_t;

// Vertex descriptor (EGF version <= 13)
struct studioVertexDesc_s
{
	Vector3D		point;
	Vector2D		texCoord;

	// TBN in EGF 12 is computed by compiler
	Vector3D		tangent;
	Vector3D		binormal;
	Vector3D		normal;

	studioBoneWeight_t	boneweights;
};
ALIGNED_TYPE(studioVertexDesc_s, 4) studioVertexDesc_t;

//-------------------------------------------

// mesh
struct studioMeshDesc_s
{
	int8				materialIndex;
	int8				unused[3];

	// Vertices stream
	int32				numVertices;
	int					vertexOffset;			// pointer to points data

	//inline studioVertexDesc_t *pVertex( int i ) const 
	//{
	//	return (studioVertexDesc_t *)(((ubyte *)this) + vertexOffset) + i; 
	//};

	inline studioVertexPosUv_t* pPosUvs(int i) const
	{
		const int stride 
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? sizeof(studioBoneWeight_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_COLOR) ? sizeof(studioVertexColor_t) : 0);

		return (studioVertexPosUv_t*)(((ubyte*)this) + vertexOffset + stride * i);
	};

	inline studioVertexTBN_t* pTBNs(int i) const
	{
		const int ofse
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0);

		const int stride
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? sizeof(studioBoneWeight_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_COLOR) ? sizeof(studioVertexColor_t) : 0);

		return (studioVertexTBN_t*)(((ubyte*)this) + vertexOffset + ofse + stride * i);
	};

	inline studioBoneWeight_t* pBoneWeight(int i) const
	{
		const int ofse
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0);

		const int stride
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? sizeof(studioBoneWeight_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_COLOR) ? sizeof(studioVertexColor_t) : 0);

		return (studioBoneWeight_t*)(((ubyte*)this) + vertexOffset + ofse + stride * i);
	};

	inline studioVertexColor_t* pColor(int i) const
	{
		const int ofse
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? sizeof(studioBoneWeight_t) : 0);

		const int stride
			= ((vertexType & STUDIO_VERTFLAG_POS_UV) ? sizeof(studioVertexPosUv_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_TBN) ? sizeof(studioVertexTBN_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_BONEWEIGHT) ? sizeof(studioBoneWeight_t) : 0)
			+ ((vertexType & STUDIO_VERTFLAG_COLOR) ? sizeof(studioVertexColor_t) : 0);

		return (studioVertexColor_t*)(((ubyte*)this) + vertexOffset + ofse + stride * i);
	};

	uint32				numIndices;
	int					indicesOffset;		// pointer to point indices data

	inline uint32 *pVertexIdx( int i ) const 
	{
		return (uint32 *)(((ubyte *)this) + indicesOffset) + i; 
	};

	int8				primitiveType;		// EEGFPrimType
	int8				vertexType;			// EStudioVertexStreamFlag

	int8				unused2[2];
};
ALIGNED_TYPE(studioMeshDesc_s, 4) studioMeshDesc_t;

// mesh group
struct studioMeshGroupDesc_s
{
	uint8				numMeshes;
	int					meshesOffset;		// Groups data index
	uint8				transformIdx;

	inline studioMeshDesc_t *pMesh( int i ) const 
	{
		return (studioMeshDesc_t *)(((ubyte *)this) + meshesOffset) + i; 
	};
};
ALIGNED_TYPE(studioMeshGroupDesc_s, 4) studioMeshGroupDesc_t;

// bone descriptor
struct studioBoneDesc_s
{
	char					name[MAX_MODEL_PART_NAME_LENGTH];	//Bone name

    int8					parent;		//Parent bone index

    Vector3D				rotation;	//Starting rotation
    Vector3D				position;	//Starting position
};
ALIGNED_TYPE(studioBoneDesc_s, 4) studioBoneDesc_t;

// Model
struct studioModelHeader_s
{
	// basemodelheader_t part
	int					ident;
	uint8				version;
	int					flags;
	int					length;

	char				modelName[MAX_MODEL_PATH_LENGTH];			// extra model name

//---------------------------------------------------------------
// Model descriptors
//---------------------------------------------------------------

	uint8				numMeshGroups;				// model references count
	int					meshGroupsOffset;			// model reference data offset

	inline studioMeshGroupDesc_t*	pMeshGroupDesc( int i ) const 
	{
		return (studioMeshGroupDesc_t *)(((ubyte *)this) + meshGroupsOffset) + i; 
	};

//---------------------------------------------------------------
// body groups
//---------------------------------------------------------------

	uint8				numBodyGroups;			// body groups count
	int					bodyGroupsOffset;		// body groups offset

	inline studioBodyGroup_t*	pBodyGroups( int i ) const 
	{
		return (studioBodyGroup_t *)(((ubyte *)this) + bodyGroupsOffset) + i; 
	};

//---------------------------------------------------------------
// lod models
//---------------------------------------------------------------

	uint8				numLods;				// LOD models count
	int					lodsOffset;				// LOD models offset

	inline studioLodModel_t*	pLodModel( int i ) const 
	{
		return (studioLodModel_t *)(((ubyte *)this) + lodsOffset) + i; 
	};

//---------------------------------------------------------------
// lod parameters
//---------------------------------------------------------------

	uint8				numLodParams;			// LOD params count
	int					lodParamsOffset;		// LOD params offset

	inline studioLodParams_t*	pLodParams( int i ) const 
	{
		return (studioLodParams_t *)(((ubyte *)this) + lodParamsOffset) + i; 
	};

//---------------------------------------------------------------
// materials
//---------------------------------------------------------------

	uint8				numMaterials;			// Material count
	int					materialsOffset;		// Materials offset

	inline studioMaterialDesc_t*	pMaterial( int i ) const 
	{
		return (studioMaterialDesc_t *)(((ubyte *)this) + materialsOffset) + i; 
	};

//---------------------------------------------------------------
// motion packages
//---------------------------------------------------------------

	uint8				numMotionPackages;			// motion package count
	int					packagesOffset;				// motion package offset

	inline motionPackageDesc_t*	pPackage( int i ) const 
	{
		return (motionPackageDesc_t *)(((ubyte *)this) + packagesOffset) + i; 
	};

//---------------------------------------------------------------
// material search pathes
//---------------------------------------------------------------

	uint8				numMaterialSearchPaths;		// Material path count
	int					materialSearchPathsOffset;	// Material pathes offset

	inline materialPathDesc_t*	pMaterialSearchPath( int i ) const 
	{
		return (materialPathDesc_t *)(((ubyte *)this) + materialSearchPathsOffset) + i; 
	};

//---------------------------------------------------------------
// model bones
//---------------------------------------------------------------

	uint8				numBones;				// bones count
	int					bonesOffset;			// bone data offset

	inline studioBoneDesc_t*	pBone( int i ) const 
	{
		return (studioBoneDesc_t *)(((ubyte *)this) + bonesOffset) + i; 
	};

//---------------------------------------------------------------
// transform info
//---------------------------------------------------------------

	uint8				numTransforms;				// attachment count
	int					transformsOffset;			// attachment data offset

	inline studioTransform_t*	pTransform( int i ) const 
	{
		return (studioTransform_t *)(((ubyte *)this) + transformsOffset) + i; 
	};

//---------------------------------------------------------------
// IK chains
//---------------------------------------------------------------

	uint8				numIKChains;				// ik chain count
	int					ikChainsOffset;				// ik chain data offset

	inline studioIkChain_t*	pIkChain( int i ) const 
	{
		return (studioIkChain_t *)(((ubyte *)this) + ikChainsOffset) + i; 
	};

};
ALIGNED_TYPE(studioModelHeader_s, 4) studioHdr_t;

//---------------------------------------------------------------
// 
// USEFUL FUNCTIONS FOR EGF
//
//---------------------------------------------------------------

//---------------------------------------------------------------
// Searches for studio bone, returns ID
//---------------------------------------------------------------
inline int Studio_FindBoneId(const studioHdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numBones; i++)
	{
		if(!stricmp(pStudioHdr->pBone(i)->name, pszName))
			return i;
	}

	return -1;
}

//-------------------studioModelHeader_seturns bone
//---------------------------------------------------------------
inline studioBoneDesc_t* Studio_FindBone(const studioHdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindBoneId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pBone(id);
}

//---------------------------------------------------------------
// Searches for body group
//---------------------------------------------------------------
inline int Studio_FindBodyGroupId(const studioHdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numBodyGroups; i++)
	{
		if(!stricmp(pStudioHdr->pBodyGroups(i)->name, pszName))
			return i;
	}

	return -1;
}

//---------------------------------------------------------------
// Searches for body group
//---------------------------------------------------------------
inline studioBodyGroup_t* Studio_FindBodyGroup(const studioHdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindBodyGroupId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pBodyGroups(id);
}

//---------------------------------------------------------------
// Searches for IK chain
//---------------------------------------------------------------
inline studioIkChain_t* Studio_FindIkChain(const studioHdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numIKChains; i++)
	{
		if(!stricmp(pStudioHdr->pIkChain(i)->name, pszName))
			return pStudioHdr->pIkChain(i);
	}

	return nullptr;
}

//---------------------------------------------------------------
// Searches for transform, returns id
//---------------------------------------------------------------
inline int Studio_FindTransformId(const studioHdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numTransforms; i++)
	{
		if(!stricmp(pStudioHdr->pTransform(i)->name, pszName))
			return i;
	}

	return -1;
}

//---------------------------------------------------------------
// Searches for attachment
//---------------------------------------------------------------
inline studioTransform_t* Studio_FindAttachment(const studioHdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindTransformId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pTransform(id);
}
