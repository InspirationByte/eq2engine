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
#include "materialsystem1/VertexFormatBuilder.h"

#include "StudioCache.h"
#include "StudioGeom.h"


DECLARE_CVAR(job_modelLoader, "0", "Load models in parallel threads", CV_ARCHIVE);

CAutoPtr<CStudioCache> g_studioModelCache;

DECLARE_CMD(egf_info, "Print loaded EGF info", CV_CHEAT)
{
	MsgInfo("Models loaded: %d\n", g_studioModelCache->GetCachedModelCount());
	for (int i = 0; i < g_studioModelCache->GetCachedModelCount(); ++i)
	{
		CEqStudioGeom* geom = g_studioModelCache->GetModel(i);
		Msg("  %d: %s\n", i, geom->GetName());
	}
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
	overdrawParams.SetName("Skinned"); // set shader 'BaseUnlit'
	overdrawParams.SetKey("BaseTexture", "error");

	m_errorMaterial = materials->CreateMaterial("_studioErrorMaterial", &overdrawParams);
	m_errorMaterial->LoadShaderAndTextures();
}

// caches model and returns it's index
int CStudioCache::PrecacheModel(const char* modelName)
{
	if (strlen(modelName) <= 0)
		return CACHE_INVALID_MODEL;

	if (m_egfFormat[0] == nullptr)
	{
		CVertexFormatBuilder fmtBuilder;
		fmtBuilder.SetStream(0, EGFHwVertex::PositionUV::GetVertexFormatDesc(), "PosUVs");
		fmtBuilder.SetStream(1, EGFHwVertex::TBN::GetVertexFormatDesc(), "TBN");

		{
			ArrayCRef<VertexFormatDesc_t> genFmt = fmtBuilder.Build();
			m_egfFormat[0] = g_pShaderAPI->CreateVertexFormat("EGFVertex", genFmt);
		}

		fmtBuilder.SetStream(2, EGFHwVertex::BoneWeights::GetVertexFormatDesc(), "BoneWeight");
		{
			ArrayCRef<VertexFormatDesc_t> genFmt = fmtBuilder.Build();
			m_egfFormat[1] = g_pShaderAPI->CreateVertexFormat("EGFVertexSkinned", genFmt);
		}
	}
	
	const int idx = GetModelIndex(modelName);
	if (idx == CACHE_INVALID_MODEL)
	{
		EqString str(modelName);
		str.Path_FixSlashes();
		const int nameHash = StringToHash(str, true);
		
		DevMsg(DEVMSG_CORE, "Loading model '%s'\n", str.ToCString());

		const int cacheIdx = m_cachedList.numElem();

		CEqStudioGeom* pModel = PPNew CEqStudioGeom();
		if (pModel->LoadModel(str, job_modelLoader.GetBool()))
		{
			int newIdx = m_cachedList.append(pModel);
			ASSERT(newIdx == cacheIdx);

			pModel->m_cacheIdx = cacheIdx;
			m_cacheIndex.insert(nameHash, cacheIdx);
		}
		else
		{
			delete pModel;
			pModel = nullptr;
		}

		return pModel ? cacheIdx : 0;
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

const char* CStudioCache::GetModelFilename(CEqStudioGeom* pModel) const
{
	return pModel->GetName();
}

int CStudioCache::GetModelIndex(const char* modelName) const
{
	EqString str(modelName);
	str.Path_FixSlashes();

	const int hash = StringToHash(str, true);
	auto found = m_cacheIndex.find(hash);
	if (!found.atEnd())
	{
		return *found;
	}

	return CACHE_INVALID_MODEL;
}

int CStudioCache::GetModelIndex(CEqStudioGeom* pModel) const
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i] == pModel)
			return i;
	}

	return CACHE_INVALID_MODEL;
}

// decrements reference count and deletes if it's zero
void CStudioCache::FreeCachedModel(CEqStudioGeom* pModel)
{

}

void CStudioCache::ReleaseCache()
{
	for (int i = 0; i < m_cachedList.numElem(); i++)
	{
		if (m_cachedList[i])
		{
			// wait for loading completion
			m_cachedList[i]->GetStudioHdr();
			delete m_cachedList[i];
		}
	}

	m_cachedList.clear(true);
	m_cacheIndex.clear(true);

	for(int i = 0; i < 2; ++i)
	{
		g_pShaderAPI->DestroyVertexFormat(m_egfFormat[i]);
		m_egfFormat[i] = nullptr;
	}
	m_errorMaterial = nullptr;
}

IVertexFormat* CStudioCache::GetEGFVertexFormat(bool skinned) const
{
	return m_egfFormat[skinned];
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
