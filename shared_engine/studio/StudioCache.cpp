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
#include "studiofile/StudioLoader.h"

using namespace Threading;

CStaticAutoPtr<CStudioCache> g_studioCache;
static CEqReadWriteLock s_studioCacheRWLock;

DECLARE_CVAR(job_modelLoader, "0", "Load models in parallel threads", CV_ARCHIVE);
DECLARE_CMD(studio_cacheInfo, "Print loaded EGF and motion packages", CV_CHEAT)
{
	g_studioCache->PrintLoaded();
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

IMaterialPtr CStudioCache::GetErrorMaterial() const
{
	if (!m_errorMaterial)
	{
		KVSection overdrawParams;
		overdrawParams.SetName("Skinned"); // set shader
		overdrawParams.SetKey("BaseTexture", "error");

		m_errorMaterial = g_matSystem->CreateMaterial("_studioErrorMaterial", &overdrawParams);
		m_errorMaterial->LoadShaderAndTextures();
	}

	return m_errorMaterial;
}

IVertexFormat* CStudioCache::GetGeomVertexFormat() const
{
	if (!m_egfFormat)
	{
		FixedArray<VertexLayoutDesc, 4> egfVertexLayout;
		egfVertexLayout.append(EGFHwVertex::PositionUV::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::TBN::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::BoneWeights::GetVertexLayoutDesc());
		egfVertexLayout.append(EGFHwVertex::Color::GetVertexLayoutDesc());
		m_egfFormat = g_renderAPI->CreateVertexFormat("EGFVertex", egfVertexLayout);
	}
	return m_egfFormat;
}

// caches model and returns it's index
int CStudioCache::PrecacheModel(const char* fileName)
{
	if(!fileName || *fileName == 0)
		return STUDIOCACHE_INVALID_IDX;

	EqString nameStr(fileName);
	fnmPathFixSeparators(nameStr);

	if (!fnmPathHasExt(nameStr))
		fnmPathApplyExt(nameStr, s_egfGeomExt);

	const int nameHash = StringToHash(nameStr, true);
	{
		CScopedReadLocker m(s_studioCacheRWLock);
		auto foundIt = m_geomCacheIndex.find(nameHash);

		if (!foundIt.atEnd())
			return *foundIt;
	}

	DevMsg(DEVMSG_CORE, "Loading model '%s'\n", nameStr.ToCString());

	CEqStudioGeom* model = PPNew CEqStudioGeom();
	if (!model->LoadModel(nameStr, job_modelLoader.GetBool()))
	{
		SAFE_DELETE(model);
		return STUDIOCACHE_INVALID_IDX;
	}

	int newCacheIdx = STUDIOCACHE_INVALID_IDX;
	{
		CScopedWriteLocker m(s_studioCacheRWLock);
		if (m_geomFreeCacheSlots.numElem())
		{
			newCacheIdx = m_geomFreeCacheSlots.popBack();
			m_geomCachedList[newCacheIdx] = model;
		}
		else
		{
			newCacheIdx = m_geomCachedList.append(model);
		}

		model->m_nameHash = nameHash;
		model->m_cacheIdx = newCacheIdx;
	}

	m_geomCacheIndex.insert(nameHash, newCacheIdx);

	return newCacheIdx;
}

// returns count of cached models
int	CStudioCache::GetModelCount() const
{
	return m_geomCachedList.numElem();
}

CEqStudioGeom* CStudioCache::GetModel(int index) const
{
	if (index <= STUDIOCACHE_INVALID_IDX)
		return m_geomCachedList[0];

	CEqStudioGeom* model = m_geomCachedList[index];
	if (model && model->GetLoadingState() != MODEL_LOAD_ERROR)
		return model;

	// return default error model
	return m_geomCachedList[0];
}

// decrements reference count and deletes if it's zero
void CStudioCache::FreeModel(int index)
{
	CEqStudioGeom* model = m_geomCachedList[index];
	if (!model)
		return;

	m_geomCacheIndex.remove(model->m_nameHash);
	m_geomFreeCacheSlots.append(index);

	// wait for loading completion
	m_geomCachedList[index]->GetStudioHdr();

	// TODO: since it's refcounted, it should not be here
	SAFE_DELETE(m_geomCachedList[index]);
}

int	CStudioCache::GetMotionDataCount() const
{
	return m_motionCachedList.numElem();
}

const StudioMotionData* CStudioCache::GetMotionData(int index) const
{
	if (index <= STUDIOCACHE_INVALID_IDX)
		return nullptr;

	return &m_motionCachedList[index];
}

int	CStudioCache::PrecacheMotionData(const char* fileName, const char* requestedBy /*= nullptr*/)
{
	if (!fileName || *fileName == 0)
		return STUDIOCACHE_INVALID_IDX;

	EqString nameStr(fileName);
	fnmPathFixSeparators(nameStr);

	if (!fnmPathHasExt(nameStr))
		fnmPathApplyExt(nameStr, s_egfMotionPackageExt);

	const int nameHash = StringToHash(nameStr, true);
	{
		CScopedReadLocker m(s_studioCacheRWLock);
		auto foundIt = m_motionCacheIndex.find(nameHash);

		if (!foundIt.atEnd())
			return *foundIt;
	}

	DevMsg(DEVMSG_CORE, "Loading motion package '%s'\n", nameStr.ToCString());

	StudioMotionData motionData;
	if(!Studio_LoadMotionData(nameStr, motionData))
	{
		if(requestedBy)
			MsgError("Cannot open motion data package '%s' requested by '%s'!\n", nameStr.ToCString(), requestedBy);
		return STUDIOCACHE_INVALID_IDX;
	}

	int newCacheIdx = STUDIOCACHE_INVALID_IDX;
	{
		CScopedWriteLocker m(s_studioCacheRWLock);
		
		if (m_motionFreeCacheSlots.numElem())
		{
			newCacheIdx = m_motionFreeCacheSlots.popBack();
			m_motionCachedList[newCacheIdx] = std::move(motionData);
		}
		else
		{
			newCacheIdx = m_motionCachedList.append(std::move(motionData));
		}

		m_motionCachedList[newCacheIdx].nameHash = nameHash;
		m_motionCachedList[newCacheIdx].cacheIdx = newCacheIdx;

		m_motionCacheIndex.insert(nameHash, newCacheIdx);
	}

	return newCacheIdx;
}

void CStudioCache::FreeMotionData(int index)
{
	StudioMotionData& motionData = m_motionCachedList[index];
	if (motionData.cacheIdx == STUDIOCACHE_INVALID_IDX)
		return;

	m_motionCacheIndex.remove(motionData.nameHash);
	m_motionFreeCacheSlots.append(index);

	Studio_FreeMotionData(m_motionCachedList[index]);
	motionData.cacheIdx = STUDIOCACHE_INVALID_IDX;
}

void CStudioCache::ReleaseCache()
{
	// TODO: since it's refcounted, it should not be here
	for (CEqStudioGeom* model : m_geomCachedList)
	{
		if (!model)
			continue;

		// wait for loading completion
		model->GetStudioHdr();
		delete model;
	}

	for (StudioMotionData& motionData : m_motionCachedList)
		Studio_FreeMotionData(motionData);

	m_geomCachedList.clear(true);
	m_geomCacheIndex.clear(true);
	m_geomFreeCacheSlots.clear(true);

	m_motionCacheIndex.clear(true);
	m_motionCachedList.clear(true);
	m_motionFreeCacheSlots.clear(true);

	g_renderAPI->DestroyVertexFormat(m_egfFormat);
	m_egfFormat = nullptr;
	m_errorMaterial = nullptr;
}

// prints loaded models to console
void CStudioCache::PrintLoaded() const
{
	MsgInfo("Cached geom count: %d\n", m_geomCachedList.numElem());
	for (int i = 0; i < m_geomCachedList.numElem(); ++i)
	{
		CEqStudioGeom* geom = m_geomCachedList[i];
		if(geom)
			Msg("  %d: %s\n", i, geom->GetName());
	}
	MsgInfo("Cached motion package count: %d\n", m_motionCachedList.numElem());
	for (int i = 0; i < m_motionCachedList.numElem(); ++i)
	{
		const StudioMotionData& motionData = m_motionCachedList[i];
		if (motionData.cacheIdx >= 0)
			Msg("  %d: %s\n", i, motionData.name.ToCString());
	}
	MsgInfo("--- end\n");
}
