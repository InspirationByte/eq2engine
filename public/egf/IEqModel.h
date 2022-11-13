//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine model
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "egf/model.h"

class IMaterial;
class IMaterialSystem;
class IVertexFormat;
class IEqModelInstancer;

struct decalmakeinfo_t;
struct tempdecal_t;

enum EModelType
{
	MODEL_TYPE_INVALID = -1,

	MODEL_TYPE_WORLD_LEVEL,		// big level model
	MODEL_TYPE_WORLD_SOLID,		// world solid
	MODEL_TYPE_STUDIO,			// EGF studio model
};

enum EModelLoadingState
{
	MODEL_LOAD_ERROR = -1,
	MODEL_LOAD_IN_PROGRESS = 0,
	MODEL_LOAD_OK,
};

#define EGF_LOADING_CRITICAL_SECTION(m)		while(m->GetLoadingState() != MODEL_LOAD_OK) {	g_parallelJobs->CompleteJobCallbacks(); Platform_Sleep(1); }
#define MOD_STUDIOHWDATA(m) (studioHwData_t*)m->GetHWData()
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

	virtual ~IEqModel() {}
						
	void						Ref_DeleteObject() {}

	// model type
	virtual EModelType			GetModelType() const = 0;

//------------------------------------
// shared model usage
//------------------------------------

	// destroys model - cache-use only if from cache.
	virtual void				DestroyModel() = 0;

	// returns name (real patch to model)
	virtual const char*			GetName() const = 0;

	// Bounding box
	virtual const BoundingBox&	GetAABB() const = 0;

	// studio model hardware data pointer for information
	virtual studioHwData_t*		GetHWData() const = 0;

	// loading state
	virtual int					GetLoadingState() const = 0;

	// loads materials
	virtual	void				LoadMaterials() = 0;

	// loads additional motion package
	virtual void				LoadMotionPackage(const char* filename) = 0;

//------------------------------------
// Rendering
//------------------------------------

	// makes dynamic temporary decal
	virtual tempdecal_t*		MakeTempDecal(const decalmakeinfo_t& info, Matrix4x4* jointMatrices) = 0;

	// instancing
	virtual void				SetInstancer(IEqModelInstancer* instancer) = 0;
	virtual IEqModelInstancer*	GetInstancer() const = 0;

	// selects lod index
	virtual int					SelectLod(float fDistance) const = 0;

	// draws single texture group
	// preSetVBO - if you don't use SetupVBOStream
	virtual void				DrawGroup(int nModel, int nGroup, bool preSetVBO = true) const = 0;

	// sets vertex buffer streams in RHI
	virtual void				SetupVBOStream( int nStream ) const = 0;

	// prepares model for skinning, return value indicates the hardware skinning flag
	virtual bool				PrepareForSkinning( Matrix4x4* jointMatrices ) = 0;

	// returns material assigned to the group
	// materialIndex = <studiohwdata_t>->studio->pModelDesc(nModel)->pGroup(nTexGroup)->materialIndex;
	virtual IMaterial*			GetMaterial(int materialIdx, int materialGroupIdx = 0) const = 0;
};


//-------------------------------------------------------
//
// IModelCache - model cache interface
//
//-------------------------------------------------------

#define CACHE_INVALID_MODEL -1

class IStudioModelCache
{
public:
	virtual ~IStudioModelCache() {}

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

	// decrements reference count and deletes if it's zero
	virtual void				FreeCachedModel(IEqModel* pModel) = 0;

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
extern IStudioModelCache* g_studioModelCache;

#define PrecacheStudioModel(mod) g_studioModelCache->PrecacheModel(mod)
