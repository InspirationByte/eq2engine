//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF Model cache
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CEqStudioGeom;
class IVertexFormat;

static constexpr const int CACHE_INVALID_MODEL = -1;

//-------------------------------------------------------------------------------------
// Model cache implementation
//-------------------------------------------------------------------------------------

class CStudioCache
{
	friend class			CEqStudioGeom;

public:
	CStudioCache() = default;

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

	void					PrintLoadedModels() const;

private:
	Map<int, int>			m_cacheIndex{ PP_SL };
	Array<CEqStudioGeom*>	m_cachedList{ PP_SL };

	IVertexFormat*			m_egfFormat{ nullptr };
};

// model cache manager
extern CStudioCache* g_studioModelCache;

#define PrecacheStudioModel(mod) g_studioModelCache->PrecacheModel(mod)