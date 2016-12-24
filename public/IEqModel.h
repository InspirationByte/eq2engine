//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine model
//////////////////////////////////////////////////////////////////////////////////

#ifndef IEQMODEL_H
#define IEQMODEL_H

#include "model.h"
#include "ppmem.h"
#include "Decals.h"

class IMaterial;
class IMaterialSystem;
class IVertexFormat;
class IEqModelInstancer;

enum EngineModelType_e
{
	ENGINEMODEL_INVALID = -1,

	ENGINEMODEL_STUDIO,			// egf models with dynamics
	ENGINEMODEL_WORLD_BRUSH,	// brush model list of surfaces
	ENGINEMODEL_WORLD_SURFACE,	// simply the list of surfaces
};

enum EModelLoadingState
{
	EQMODEL_LOAD_ERROR = -1,
	EQMODEL_LOAD_IN_PROGRESS = 0,
	EQMODEL_LOAD_OK,
};

// structure for world models
struct worldhwdata_t // (as studiohwdata_t)
{
	eqlevelsurf_t** surfaces;
	int				numSurfaces;

	physmodeldata_t	m_physmodel;
};

#define MOD_STUDIOHWDATA(m) (studiohwdata_t*)m->GetHWData()
#define MOD_WORLDHWDATA(m)	(worldhwdata_t*)m->GetHWData()

//-------------------------------------------------------
//
// IEqModelInstancer - model instancer for optimized rendering
//
//-------------------------------------------------------

class IEqModel;

class IEqModelInstancer
{
public:
	PPMEM_MANAGED_OBJECT();

				virtual	~IEqModelInstancer() {}

	virtual void		ValidateAssert() = 0;

	virtual void		Draw( int renderFlags, IEqModel* model ) = 0;
	virtual bool		HasInstances() const = 0;
};


//-------------------------------------------------------
//
// IEqModel - The model interface
//
//-------------------------------------------------------

class IEqModel
{
public:

	PPMEM_MANAGED_OBJECT();

						virtual ~IEqModel() {}

//------------------------------------
// shared model usage
//------------------------------------

	// destroys model - cache-use only if from cache.
	virtual void				DestroyModel() = 0;

	// returns name (real patch to model)
	virtual const char*			GetName() const = 0;

	// selects lod index
	virtual int					SelectLod(float fDistance) = 0;

	virtual const BoundingBox&	GetAABB() const = 0;

	// makes dynamic temporary decal
	virtual studiotempdecal_t*	MakeTempDecal( const decalmakeinfo_t& info, Matrix4x4* jointMatrices) = 0;

	// instancing
	virtual void				SetInstancer( IEqModelInstancer* instancer ) = 0;
	virtual IEqModelInstancer*	GetInstancer() const = 0;

//------------------------------------
// Rendering
//------------------------------------

	// draws single texture group
	// preSetVBO - if you don't use SetupVBOStream
	virtual void				DrawGroup(int nModel, int nGroup, bool preSetVBO = true) = 0;

	// sets vertex buffer streams in RHI
	virtual void				SetupVBOStream( int nStream ) = 0;

	// draw debug information (skeletons, etc.)
	virtual void				DrawDebug() = 0;

	// prepares model for skinning, return value indicates the hardware skinning flag
	virtual bool				PrepareForSkinning( Matrix4x4* jointMatrices ) = 0;

	// returns material assigned to the group
	// materialIndex = <studiohwdata_t>->pStudioHdr->pModelDesc(nModel)->pGroup(nTexGroup)->materialIndex;
	virtual IMaterial*			GetMaterial(int materialIdx) = 0;

	// loads materials for studio
	virtual	void				LoadMaterials() = 0;

	// studio model hardware data pointer for information
	virtual studiohwdata_t*		GetHWData() const = 0;

	// loading state
	virtual int					GetLoadingState() const = 0;
};


//-------------------------------------------------------
//
// IModelCache - model cache interface
//
//-------------------------------------------------------

#define CACHE_INVALID_MODEL -1

class IModelCache
{
public:
	// caches model and returns it's index
	virtual int					PrecacheModel(const char* modelName) = 0;

	// returns count of cached models
	virtual int					GetCachedModelCount() const = 0;

	// returns model by the index
	virtual IEqModel*			GetModel(int index) const = 0;

	// returns model file name
	virtual const char* 		GetModelFilename(IEqModel* pModel) const = 0;

	// returns model index from file name
	virtual int					GetModelIndex(const char* modelName) const = 0;

	// returns model index from pointer
	virtual int					GetModelIndex(IEqModel* pModel) const = 0;

	// releases all models
	virtual void				ReleaseCache() = 0;

	// reloads all models
	virtual void				ReloadModels() = 0;

	// returns EGF vertex format
	virtual IVertexFormat*		GetEGFVertexFormat() const = 0;

	// prints loaded models to console
	virtual void				PrintLoadedModels() const = 0;
};

// model cache manager
extern IModelCache* g_pModelCache;

#endif // IEQMODEL_H
