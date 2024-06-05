//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF Model cache
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterial.h"

class CEqJobManager;
class CEqStudioGeom;
class IVertexFormat;
struct StudioMotionData;

static constexpr const int CACHE_INVALID_MODEL = -1;

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CStudioCache
{
	friend class			CEqStudioGeom;

public:
	CStudioCache() = default;

	void					Init(CEqJobManager* jobMng);
	void					Shutdown();

	CEqJobManager*			GetJobMng() const;

	// caches model and returns it's index
	int						PrecacheModel(const char* modelName);
	int						GetCachedModelCount() const;

	CEqStudioGeom*			GetModel(int index) const;
	const char*				GetModelFilename(CEqStudioGeom* pModel) const;
	int						GetModelIndex(const char* modelName) const;
	int						GetModelIndex(CEqStudioGeom* pModel) const;

	void					FreeCachedModel(CEqStudioGeom* pModel);

	void					ReleaseCache();

	IVertexFormat*			GetEGFVertexFormat() const;
	IMaterialPtr			GetErrorMaterial();

	void					PrintLoadedModels() const;

private:
	void					InitErrorMaterial();

	CEqJobManager*				m_jobMng{ nullptr };

	Map<int, int>				m_geomCacheIndex{ PP_SL };
	Array<CEqStudioGeom*>		m_geomCachedList{ PP_SL };
	Array<int>					m_geomFreeCacheSlots{ PP_SL };

	//Map<int, int>				m_motionCacheIndex{ PP_SL };
	//Array<StudioMotionData*>	m_motionCachedList{ PP_SL };
	//Array<int>					m_motionFreeCacheSlots{ PP_SL };

	IVertexFormat*				m_egfFormat{ nullptr };
	IMaterialPtr				m_errorMaterial;
};

// model cache manager
extern CStaticAutoPtr<CStudioCache> g_studioModelCache;

#define PrecacheStudioModel(mod) g_studioModelCache->PrecacheModel(mod)