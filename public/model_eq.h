//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Geometry File
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQMODEL_H
#define EQMODEL_H

// EQGF is a base format for all DarkTech Engines (egf, eaf, dgf)
#define EQUILIBRIUM_MODEL_VERSION		13
#define EQUILIBRIUM_MODEL_SIGNATURE		(('F'<<24)+('G'<<16)+('Q'<<8)+'E')

// use EGF_VERSION_CHANGE tags for defining changes

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
	char name[44];

	int numlinks;
	int linksoffset;

	inline studioiklink_t *pLink( int i ) const 
	{
		return (studioiklink_t *)(((ubyte *)this) + linksoffset) + i; 
	};
};

ALIGNED_TYPE(studioikchain_s, 4) studioikchain_t;

// bone attachment
struct studioattachment_s
{
	char name[44];
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
	int8				lodmodels[MAX_MODELLODS]; // lod model indexes, points to studiomodeldesc_t
};

ALIGNED_TYPE(studiolodmodel_s, 4) studiolodmodel_t;

// body groups
struct studiobodygroup_s
{
	char				name[44];		// bodygroup name
	int					lodmodel_index; // lod model index, points to studiolodmodel_t

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

// egf optimized loading data
struct egf_ohd_data_t
{
	int			ident;
	char		model_path[256];
	int			numIndices;
	int			numVertices;
	Vector3D	mins;
	Vector3D	maxs;
	int			numGroups;
	int			numBones;
};

// model group
struct modelgroupdesc_s
{
	// Don't keep the group name, use material name only
	int8				materialIndex;

	// Vertices stream
	int32				numvertices;
	int					vertexoffset;			// pointer to points data

	inline studiovertexdesc_t *pVertex( int i ) const 
	{
		return (studiovertexdesc_t *)(((ubyte *)this) + vertexoffset) + i; 
	};

	uint32				numindices;
	int					indicesoffset;		// pointer to point indices data

	inline uint32 *pVertexIdx( int i ) const 
	{
		return (uint32 *)(((ubyte *)this) + indicesoffset) + i; 
	};

	// primitive type for EGF. If model is optimized, you can use it.
	int8				primitiveType;
};

ALIGNED_TYPE(modelgroupdesc_s, 4) modelgroupdesc_t;

// model descriptor
struct studiomodeldesc_s
{
	uint8				numgroups;
	int					groupsoffset;		// Groups data index
	uint8				lod_index;

	inline modelgroupdesc_t *pGroup( int i ) const 
	{
		return (modelgroupdesc_t *)(((ubyte *)this) + groupsoffset) + i; 
	};
};

ALIGNED_TYPE(studiomodeldesc_s, 4) studiomodeldesc_t;

// bone descriptor
struct bonedesc_s
{
	char					name[44];	//Bone name

    int8					parent;		//Parent bone index

    Vector3D				rotation;	//Starting rotation
    Vector3D				position;	//Starting position
};

ALIGNED_TYPE(bonedesc_s, 4) bonedesc_t;

// Model
struct modelheader_s
{
	PPMEM_MANAGED_OBJECT();

	int					ident;					// main header
	uint8				version;

	int					flags;					// model flags

	int					length;					// model size

	char				modelname[256];			// extra model name

//---------------------------------------------------------------
// Model descriptors
//---------------------------------------------------------------

	uint8				nummodels;				// model references count
	int					modelsoffset;			// model reference data offset

	inline studiomodeldesc_t*	pModelDesc( int i ) const 
	{
		return (studiomodeldesc_t *)(((ubyte *)this) + modelsoffset) + i; 
	};

//---------------------------------------------------------------
// body groups
//---------------------------------------------------------------

	uint8				numbodygroups;			// body groups count
	int					bodygroupsoffset;		// body groups offset

	inline studiobodygroup_t*	pBodyGroups( int i ) const 
	{
		return (studiobodygroup_t *)(((ubyte *)this) + bodygroupsoffset) + i; 
	};

//---------------------------------------------------------------
// lod models
//---------------------------------------------------------------

	uint8				numlods;				// LOD models count
	int					lodsoffset;				// LOD models offset

	inline studiolodmodel_t*	pLodModel( int i ) const 
	{
		return (studiolodmodel_t *)(((ubyte *)this) + lodsoffset) + i; 
	};

//---------------------------------------------------------------
// lod parameters
//---------------------------------------------------------------

	uint8				numlodparams;			// LOD params count
	int					lodparamsoffset;		// LOD params offset

	inline studiolodparams_t*	pLodParams( int i ) const 
	{
		return (studiolodparams_t *)(((ubyte *)this) + lodparamsoffset) + i; 
	};

//---------------------------------------------------------------
// materials
//---------------------------------------------------------------

	uint8				nummaterials;			// Material count
	int					materialsoffset;		// Materials offset

	inline studiomaterialdesc_t*	pMaterial( int i ) const 
	{
		return (studiomaterialdesc_t *)(((ubyte *)this) + materialsoffset) + i; 
	};

//---------------------------------------------------------------
// motion packages
//---------------------------------------------------------------

	uint8				nummotionpackages;			// motion package count
	int					packagesoffset;				// motion package offset

	inline motionpackagedesc_t*	pPackage( int i ) const 
	{
		return (motionpackagedesc_t *)(((ubyte *)this) + packagesoffset) + i; 
	};

//---------------------------------------------------------------
// material search pathes
//---------------------------------------------------------------

	uint8				numsearchpathdescs;		// Material path count
	int					searchpathdescsoffset;	// Material pathes offset

	inline materialpathdesc_t*	pMaterialSearchPath( int i ) const 
	{
		return (materialpathdesc_t *)(((ubyte *)this) + searchpathdescsoffset) + i; 
	};

//---------------------------------------------------------------
// model bones
//---------------------------------------------------------------

	uint8				numbones;				// bones count
	int					bonesoffset;			// bone data offset

	inline bonedesc_t*	pBone( int i ) const 
	{
		return (bonedesc_t *)(((ubyte *)this) + bonesoffset) + i; 
	};

//---------------------------------------------------------------
// bone attachments
//---------------------------------------------------------------

	uint8				numattachments;				// attachment count
	int					attachmentsoffset;			// attachment data offset

	inline studioattachment_t*	pAttachment( int i ) const 
	{
		return (studioattachment_t *)(((ubyte *)this) + attachmentsoffset) + i; 
	};

//---------------------------------------------------------------
// IK chains
//---------------------------------------------------------------

	uint8				numikchains;				// ik chain count
	int					ikchainsoffset;				// ik chain data offset

	inline studioikchain_t*	pIkChain( int i ) const 
	{
		return (studioikchain_t *)(((ubyte *)this) + ikchainsoffset) + i; 
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
	for(int i = 0; i < pStudioHdr->numbones; i++)
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
	for(int i = 0; i < pStudioHdr->numbones; i++)
	{
		if(!stricmp(pStudioHdr->pBone(i)->name, pszName))
			return pStudioHdr->pBone(i);
	}

	return NULL;
}

//---------------------------------------------------------------
// Searches for body group
//---------------------------------------------------------------
inline studiobodygroup_t* Studio_FindBodyGroup(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numbodygroups; i++)
	{
		if(!stricmp(pStudioHdr->pBodyGroups(i)->name, pszName))
			return pStudioHdr->pBodyGroups(i);
	}

	return NULL;
}

//---------------------------------------------------------------
// Searches for body group
//---------------------------------------------------------------
inline int Studio_FindBodyGroupId(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numbodygroups; i++)
	{
		if(!stricmp(pStudioHdr->pBodyGroups(i)->name, pszName))
			return i;
	}

	return -1;
}

//---------------------------------------------------------------
// Searches for IK chain
//---------------------------------------------------------------
inline studioikchain_t* Studio_FindIkChain(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numikchains; i++)
	{
		if(!stricmp(pStudioHdr->pIkChain(i)->name, pszName))
			return pStudioHdr->pIkChain(i);
	}

	return NULL;
}

//---------------------------------------------------------------
// Searches for attachment, returns id
//---------------------------------------------------------------
inline int Studio_FindAttachmentID(studiohdr_t* pStudioHdr, const char* pszName)
{
	for(int i = 0; i < pStudioHdr->numattachments; i++)
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
	for(int i = 0; i < pStudioHdr->numattachments; i++)
	{
		if(!stricmp(pStudioHdr->pAttachment(i)->name, pszName))
			return pStudioHdr->pAttachment(i);
	}

	return NULL;
}

#endif //EQMODEL_H