//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Graphics File
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define EQUILIBRIUM_MODEL_VERSION		13
#define EQUILIBRIUM_MODEL_SIGNATURE		MCHAR4('E','Q','G','F')

#define MAX_MODEL_LODS				8
#define MAX_MODEL_PATH_LENGTH		256
#define MAX_MODEL_PART_PATH_LENGTH	128
#define MAX_MODEL_PART_NAME_LENGTH	44

// use EGF_VERSION_CHANGE tags for defining changes

// Base model material search path descriptor
struct materialpathdesc_s
{
	char				searchPath[MAX_MODEL_PART_PATH_LENGTH];
};
ALIGNED_TYPE(materialpathdesc_s, 4) materialpathdesc_t;

// material descriptor
struct motionpackagedesc_s
{
	char				packageName[MAX_MODEL_PART_PATH_LENGTH]; //only this?
};
ALIGNED_TYPE(motionpackagedesc_s, 4) motionpackagedesc_t;

struct studioiklink_s
{
	int bone;

	// link limits
	Vector3D	mins;
	Vector3D	maxs;

	float		damping;
};
ALIGNED_TYPE(studioiklink_s, 4) studioiklink_t;

// studiomodel ik chain
struct studioikchain_s
{
	char name[MAX_MODEL_PART_NAME_LENGTH];

	int numLinks;
	int linksOffset;

	inline studioiklink_t *pLink( int i ) const 
	{
		return (studioiklink_t *)(((ubyte *)this) + linksOffset) + i; 
	};
};
ALIGNED_TYPE(studioikchain_s, 4) studioikchain_t;

// bone attachment
struct studioattachment_s
{
	char name[MAX_MODEL_PART_NAME_LENGTH];
	int8 bone_id;

	Vector3D position;
	Vector3D angles;

	ubyte unused; // unused
};
ALIGNED_TYPE(studioattachment_s, 4) studioattachment_t;

// lod parameters
struct studiolodparams_s
{
	float		distance;
	int			flags;
};
ALIGNED_TYPE(studiolodparams_s, 4) studiolodparams_t;

// lod models
struct studiolodmodel_s
{
	int8				modelsIndexes[MAX_MODEL_LODS]; // lod model indexes, points to studiomodeldesc_t
};
ALIGNED_TYPE(studiolodmodel_s, 4) studiolodmodel_t;

// body groups
struct studiobodygroup_s
{
	char				name[MAX_MODEL_PART_NAME_LENGTH];		// bodygroup name
	int					lodModelIndex; // lod model index, points to studiolodmodel_t

	ubyte				unused[2];		// unused
};
ALIGNED_TYPE(studiobodygroup_s, 4) studiobodygroup_t;

// material descriptor
struct studiomaterialdesc_s
{
	char				materialname[32]; //only this?
};
ALIGNED_TYPE(studiomaterialdesc_s, 4) studiomaterialdesc_t;

// bone weight data
struct boneweight_s
{
	float				weight[4];
	int8				bones[4]; 
	int8				numweights;

	ubyte				unused; // unused
};
ALIGNED_TYPE(boneweight_s, 4) boneweight_t;

// Vertex descriptor
struct studiovertexdesc_s
{
	Vector3D			point;
	Vector2D			texCoord;

	// TBN in EGF 12 is computed by compiler
	Vector3D			tangent;
	Vector3D			binormal;
	Vector3D			normal;

	boneweight_t		boneweights;
};
ALIGNED_TYPE(studiovertexdesc_s, 4) studiovertexdesc_t;

// model group
struct modelgroupdesc_s
{
	// Don't keep the group name, use material name only
	int8				materialIndex;

	// Vertices stream
	int32				numVertices;
	int					vertexOffset;			// pointer to points data

	inline studiovertexdesc_t *pVertex( int i ) const 
	{
		return (studiovertexdesc_t *)(((ubyte *)this) + vertexOffset) + i; 
	};

	uint32				numIndices;
	int					indicesOffset;		// pointer to point indices data

	inline uint32 *pVertexIdx( int i ) const 
	{
		return (uint32 *)(((ubyte *)this) + indicesOffset) + i; 
	};

	// primitive type for EGF. If model is optimized, you can use it.
	int8				primitiveType;
};
ALIGNED_TYPE(modelgroupdesc_s, 4) modelgroupdesc_t;

// model descriptor
struct studiomodeldesc_s
{
	uint8				numGroups;
	int					groupsOffset;		// Groups data index
	uint8				lodIndex;

	inline modelgroupdesc_t *pGroup( int i ) const 
	{
		return (modelgroupdesc_t *)(((ubyte *)this) + groupsOffset) + i; 
	};
};
ALIGNED_TYPE(studiomodeldesc_s, 4) studiomodeldesc_t;

// bone descriptor
struct bonedesc_s
{
	char					name[MAX_MODEL_PART_NAME_LENGTH];	//Bone name

    int8					parent;		//Parent bone index

    Vector3D				rotation;	//Starting rotation
    Vector3D				position;	//Starting position
};
ALIGNED_TYPE(bonedesc_s, 4) bonedesc_t;

// Model
struct modelheader_s
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

	uint8				numModels;				// model references count
	int					modelsOffset;			// model reference data offset

	inline studiomodeldesc_t*	pModelDesc( int i ) const 
	{
		return (studiomodeldesc_t *)(((ubyte *)this) + modelsOffset) + i; 
	};

//---------------------------------------------------------------
// body groups
//---------------------------------------------------------------

	uint8				numBodyGroups;			// body groups count
	int					bodyGroupsOffset;		// body groups offset

	inline studiobodygroup_t*	pBodyGroups( int i ) const 
	{
		return (studiobodygroup_t *)(((ubyte *)this) + bodyGroupsOffset) + i; 
	};

//---------------------------------------------------------------
// lod models
//---------------------------------------------------------------

	uint8				numLods;				// LOD models count
	int					lodsOffset;				// LOD models offset

	inline studiolodmodel_t*	pLodModel( int i ) const 
	{
		return (studiolodmodel_t *)(((ubyte *)this) + lodsOffset) + i; 
	};

//---------------------------------------------------------------
// lod parameters
//---------------------------------------------------------------

	uint8				numLodParams;			// LOD params count
	int					lodParamsOffset;		// LOD params offset

	inline studiolodparams_t*	pLodParams( int i ) const 
	{
		return (studiolodparams_t *)(((ubyte *)this) + lodParamsOffset) + i; 
	};

//---------------------------------------------------------------
// materials
//---------------------------------------------------------------

	uint8				numMaterials;			// Material count
	int					materialsOffset;		// Materials offset

	inline studiomaterialdesc_t*	pMaterial( int i ) const 
	{
		return (studiomaterialdesc_t *)(((ubyte *)this) + materialsOffset) + i; 
	};

//---------------------------------------------------------------
// motion packages
//---------------------------------------------------------------

	uint8				numMotionPackages;			// motion package count
	int					packagesOffset;				// motion package offset

	inline motionpackagedesc_t*	pPackage( int i ) const 
	{
		return (motionpackagedesc_t *)(((ubyte *)this) + packagesOffset) + i; 
	};

//---------------------------------------------------------------
// material search pathes
//---------------------------------------------------------------

	uint8				numMaterialSearchPaths;		// Material path count
	int					materialSearchPathsOffset;	// Material pathes offset

	inline materialpathdesc_t*	pMaterialSearchPath( int i ) const 
	{
		return (materialpathdesc_t *)(((ubyte *)this) + materialSearchPathsOffset) + i; 
	};

//---------------------------------------------------------------
// model bones
//---------------------------------------------------------------

	uint8				numBones;				// bones count
	int					bonesOffset;			// bone data offset

	inline bonedesc_t*	pBone( int i ) const 
	{
		return (bonedesc_t *)(((ubyte *)this) + bonesOffset) + i; 
	};

//---------------------------------------------------------------
// bone attachments
//---------------------------------------------------------------

	uint8				numAttachments;				// attachment count
	int					attachmentsOffset;			// attachment data offset

	inline studioattachment_t*	pAttachment( int i ) const 
	{
		return (studioattachment_t *)(((ubyte *)this) + attachmentsOffset) + i; 
	};

//---------------------------------------------------------------
// IK chains
//---------------------------------------------------------------

	uint8				numIKChains;				// ik chain count
	int					ikChainsOffset;				// ik chain data offset

	inline studioikchain_t*	pIkChain( int i ) const 
	{
		return (studioikchain_t *)(((ubyte *)this) + ikChainsOffset) + i; 
	};

};
ALIGNED_TYPE(modelheader_s, 4) studiohdr_t;

//---------------------------------------------------------------
// 
// USEFUL FUNCTIONS FOR EGF
//
//---------------------------------------------------------------

//---------------------------------------------------------------
// Searches for studio bone, returns ID
//---------------------------------------------------------------
inline int Studio_FindBoneId(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numBones; i++)
	{
		if(!stricmp(pStudioHdr->pBone(i)->name, pszName))
			return i;
	}

	return -1;
}

//---------------------------------------------------------------
// Searches for studio bone, returns bone
//---------------------------------------------------------------
inline bonedesc_t* Studio_FindBone(studiohdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindBoneId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pBone(id);
}

//---------------------------------------------------------------
// Searches for body group
//---------------------------------------------------------------
inline int Studio_FindBodyGroupId(studiohdr_t* pStudioHdr, const char* pszName)
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
inline studiobodygroup_t* Studio_FindBodyGroup(studiohdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindBodyGroupId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pBodyGroups(id);
}

//---------------------------------------------------------------
// Searches for IK chain
//---------------------------------------------------------------
inline studioikchain_t* Studio_FindIkChain(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numIKChains; i++)
	{
		if(!stricmp(pStudioHdr->pIkChain(i)->name, pszName))
			return pStudioHdr->pIkChain(i);
	}

	return nullptr;
}

//---------------------------------------------------------------
// Searches for attachment, returns id
//---------------------------------------------------------------
inline int Studio_FindAttachmentId(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numAttachments; i++)
	{
		if(!stricmp(pStudioHdr->pAttachment(i)->name, pszName))
			return i;
	}

	return -1;
}

//---------------------------------------------------------------
// Searches for attachment
//---------------------------------------------------------------
inline studioattachment_t* Studio_FindAttachment(studiohdr_t* pStudioHdr, const char* pszName)
{
	int id = Studio_FindAttachmentId(pStudioHdr, pszName);

	if(id == -1)
		return nullptr;

	return pStudioHdr->pAttachment(id);
}
