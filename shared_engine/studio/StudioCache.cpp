//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: EGF Model cache
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "utils/KeyValues.h"

#include "materialsystem1/IMaterialSystem.h"

#include "StudioCache.h"
#include "StudioGeom.h"


DECLARE_CVAR(job_modelLoader, "0", "Load models in parallel threads", CV_ARCHIVE);

CStaticAutoPtr<CStudioCache> g_studioModelCache;

DECLARE_CMD(egf_info, "Print loaded EGF info", CV_CHEAT)
{
	MsgInfo("Models loaded: %d\n", g_studioModelCache->GetCachedModelCount());
	for (int i = 0; i < g_studioModelCache->GetCachedModelCount(); ++i)
	{
		CEqStudioGeom* geom = g_studioModelCache->GetModel(i);
		Msg("  %d: %s\n", i, geom->GetName());
	}
}

void CStudioCache::Init(CEqJobManager* jobMng)
{
	m_jobMng = jobMng;
}

void CStudioCache::Shutdown()
{
	ReleaseCache();
	m_jobMng = nullptr;
	m_errorMaterial = nullptr;
}

CEqJobManager* CStudioCache::GetJobMng() const
{
	ASSERT_MSG(m_jobMng, "Studio cache is not initialized");
	return m_jobMng; 
}

IMaterialPtr CStudioCache::GetErrorMaterial()
{
	InitErrorMaterial();
	return m_errorMaterial;
}

void CStudioCache::InitErrorMaterial()
{
	if (m_errorMaterial)
		return;

	KVSection overdrawParams;
	overdrawParams.SetName("Skinned"); // set shader
	overdrawParams.SetKey("BaseTexture", "error");

	m_errorMaterial = g_matSystem->CreateMaterial("_studioErrorMaterial", &overdrawParams);
	m_errorMaterial->LoadShaderAndTextures();
}

// caches model and returns it's index
int CStudioCache::PrecacheModel(const char* modelName)
{
	if (strlen(modelName) <= 0)
		return CACHE_INVALID_MODEL;

	if (!m_egfFormat)
	{
		FixedArray<VertexLayoutDesc, 4> egfVertexLayout;
		egfVertexLayout.append(EGFHwVertex::PositionUV::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::TBN::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::BoneWeights::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::Color::GetVertexLayoutDesc());
		m_egfFormat = g_renderAPI->CreateVertexFormat("EGFVertex", egfVertexLayout);
	}
	
	const int idx = GetModelIndex(modelName);
	if (idx == CACHE_INVALID_MODEL)
	{
		EqString str(modelName);
		fnmPathFixSeparators(str);

		DevMsg(DEVMSG_CORE, "Loading model '%s'\n", str.ToCString());

		CEqStudioGeom* model = PPNew CEqStudioGeom();
		if (model->LoadModel(str, job_modelLoader.GetBool()))
		{
			if (m_freeCacheSlots.numElem())
			{
				const int newIdx = m_freeCacheSlots.popBack();
				m_cachedList[newIdx] = model;
				model->m_cacheIdx = newIdx;
			}
			else
			{
				const int newIdx = m_cachedList.append(model);
				model->m_cacheIdx = newIdx;
			}

			const int nameHash = StringToHash(str, true);
			m_cacheIndex.insert(nameHash, model->m_cacheIdx);
		}
		else
		{
			SAFE_DELETE(model);
		}

		return model ? model->m_cacheIdx : 0;
	}

	return idx;
}

// returns count of cached models
int	CStudioCache::GetCachedModelCount() const
{
	return m_cachedList.numElem();
}

CEqStudioGeom* CStudioCache::GetModel(int index) const
{
	if (index <= CACHE_INVALID_MODEL)
		return m_cachedList[0];

	CEqStudioGeom* model = m_cachedList[index];

	if (model && model->GetLoadingState() != MODEL_LOAD_ERROR)
		return model;

	// return default error model
	return m_cachedList[0];
}

const char* CStudioCache::GetModelFilename(CEqStudioGeom* model) const
{
	return model->GetName();
}

int CStudioCache::GetModelIndex(const char* modelName) const
{
	EqString str(modelName);
	fnmPathFixSeparators(str);

	const int nameHash = StringToHash(str, true);
	auto found = m_cacheIndex.find(nameHash);
	if (!found.atEnd())
	{
		return *found;
	}

	return CACHE_INVALID_MODEL;
}

int CStudioCache::GetModelIndex(CEqStudioGeom* model) const
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i] == model)
			return i;
	}

	return CACHE_INVALID_MODEL;
}

// decrements reference count and deletes if it's zero
void CStudioCache::FreeCachedModel(CEqStudioGeom* model)
{
	const int modelIndex = arrayFindIndex(m_cachedList, model);
	if (modelIndex == -1)
		return;

	const int nameHash = StringToHash(model->GetName(), true);

	m_cacheIndex.remove(nameHash);
	m_freeCacheSlots.append(modelIndex);

	// wait for loading completion
	m_cachedList[modelIndex]->GetStudioHdr();

	SAFE_DELETE(m_cachedList[modelIndex]);
}

void CStudioCache::ReleaseCache()
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i])
		{
			// wait for loading completion
			m_cachedList[i]->GetStudioHdr();
			SAFE_DELETE(m_cachedList[i]);
		}
	}

	m_cachedList.clear(true);
	m_cacheIndex.clear(true);
	m_freeCacheSlots.clear(true);

	g_renderAPI->DestroyVertexFormat(m_egfFormat);
	m_egfFormat = nullptr;
	m_errorMaterial = nullptr;
}

IVertexFormat* CStudioCache::GetEGFVertexFormat() const
{
	return m_egfFormat;
}

// prints loaded models to console
void CStudioCache::PrintLoadedModels() const
{
	Msg("---MODELS---\n");
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i])
			Msg("%s\n", m_cachedList[i]->GetName());
	}
	Msg("---END MODELS---\n");
}
