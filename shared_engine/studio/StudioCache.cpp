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
	g_studioModelCache->PrintLoadedModels();
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
			if (m_geomFreeCacheSlots.numElem())
			{
				const int newIdx = m_geomFreeCacheSlots.popBack();
				m_geomCachedList[newIdx] = model;
				model->m_cacheIdx = newIdx;
			}
			else
			{
				const int newIdx = m_geomCachedList.append(model);
				model->m_cacheIdx = newIdx;
			}

			const int nameHash = StringToHash(str, true);
			m_geomCacheIndex.insert(nameHash, model->m_cacheIdx);
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
	return m_geomCachedList.numElem();
}

CEqStudioGeom* CStudioCache::GetModel(int index) const
{
	if (index <= CACHE_INVALID_MODEL)
		return m_geomCachedList[0];

	CEqStudioGeom* model = m_geomCachedList[index];

	if (model && model->GetLoadingState() != MODEL_LOAD_ERROR)
		return model;

	// return default error model
	return m_geomCachedList[0];
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
	auto found = m_geomCacheIndex.find(nameHash);
	if (!found.atEnd())
	{
		return *found;
	}

	return CACHE_INVALID_MODEL;
}

int CStudioCache::GetModelIndex(CEqStudioGeom* model) const
{
	for (int i = 0; i < m_geomCachedList.numElem(); i++)
	{
		if (m_geomCachedList[i] == model)
			return i;
	}

	return CACHE_INVALID_MODEL;
}

// decrements reference count and deletes if it's zero
void CStudioCache::FreeCachedModel(CEqStudioGeom* model)
{
	const int modelIndex = arrayFindIndex(m_geomCachedList, model);
	if (modelIndex == -1)
		return;

	const int nameHash = StringToHash(model->GetName(), true);

	m_geomCacheIndex.remove(nameHash);
	m_geomFreeCacheSlots.append(modelIndex);

	// wait for loading completion
	m_geomCachedList[modelIndex]->GetStudioHdr();

	SAFE_DELETE(m_geomCachedList[modelIndex]);
}

void CStudioCache::ReleaseCache()
{
	for (int i = 0; i < m_geomCachedList.numElem(); i++)
	{
		if (m_geomCachedList[i])
		{
			// wait for loading completion
			m_geomCachedList[i]->GetStudioHdr();
			SAFE_DELETE(m_geomCachedList[i]);
		}
	}

	m_geomCachedList.clear(true);
	m_geomCacheIndex.clear(true);
	m_geomFreeCacheSlots.clear(true);

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
	MsgInfo("Cached geom count: %d\n", m_geomCachedList.numElem());
	for (int i = 0; i < m_geomCachedList.numElem(); ++i)
	{
		CEqStudioGeom* geom = m_geomCachedList[i];
		if(geom)
			Msg("  %d: %s\n", i, geom->GetName());
	}
	MsgInfo("--- end\n");
}
