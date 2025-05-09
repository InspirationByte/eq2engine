//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF Model cache
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "materialsystem1/IMaterial.h"
#include "egf/model.h"

class CEqJobManager;
class CEqStudioGeom;
class IVertexFormat;

static constexpr const int STUDIOCACHE_INVALID_IDX = -1;

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CStudioCache
{
public:
	CStudioCache() = default;

	void					Init(CEqJobManager* jobMng);
	void					Shutdown();

	CEqJobManager*			GetJobMng() const;

	int						GetModelCount() const;
	CEqStudioGeom*			GetModel(int index) const;
	int						PrecacheModel(const char* modelName);
	void					FreeModel(int index);

	int						GetMotionDataCount() const;
	const StudioMotionData*	GetMotionData(int index) const;
	int						PrecacheMotionData(const char* fileName, const char* requestedBy = nullptr);
	void					FreeMotionData(int index);

	void					ReleaseCache();

	IVertexFormat*			GetGeomVertexFormat() const;
	IMaterialPtr			GetErrorMaterial() const;

	void					PrintLoaded() const;

private:

	CEqJobManager*				m_jobMng{ nullptr };

	Map<int, int>				m_geomCacheIndex{ PP_SL };
	Array<CEqStudioGeom*>		m_geomCachedList{ PP_SL };
	Array<int>					m_geomFreeCacheSlots{ PP_SL };

	Map<int, int>				m_motionCacheIndex{ PP_SL };
	Array<StudioMotionData>		m_motionCachedList{ PP_SL };
	Array<int>					m_motionFreeCacheSlots{ PP_SL };

	mutable IVertexFormat*		m_egfFormat{ nullptr };
	mutable IMaterialPtr		m_errorMaterial;
};

// model cache manager
extern CStaticAutoPtr<CStudioCache> g_studioCache;

#define PrecacheStudioModel(mod) g_studioCache->PrecacheModel(mod)