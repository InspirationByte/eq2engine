//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "WGPUTexture.h"
#include "WGPURenderAPI.h"

extern WGPURenderAPI s_renderApi;

// initializes texture from image array of images
bool CWGPUTexture::Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags)
{
	return true;
}

// locks texture for modifications, etc
bool CWGPUTexture::Lock(LockInOutData& data)
{
	m_lockData = &data;

	data.lockData = PPAlloc(16 * 1024 * 1024);
	data.lockPitch = 16;
	return true;
}

// unlocks texture for modifications, etc
void CWGPUTexture::Unlock()
{
	if (!m_lockData)
		return;

	PPFree(m_lockData->lockData);
	m_lockData->lockData = nullptr;

	m_lockData = nullptr;
}

void CWGPUTexture::Ref_DeleteObject()
{
	s_renderApi.FreeTexture(this);
	RefCountedObject::Ref_DeleteObject();
}