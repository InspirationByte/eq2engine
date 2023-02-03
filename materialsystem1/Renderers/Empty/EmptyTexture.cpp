//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/core_common.h"
#include "EmptyTexture.h"
#include "ShaderAPIEmpty.h"

extern ShaderAPIEmpty s_shaderAPI;

// initializes texture from image array of images
bool CEmptyTexture::Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags)
{
	return true;
}

// locks texture for modifications, etc
bool CEmptyTexture::Lock(LockInOutData& data)
{
	m_lockData = &data;

	data.lockData = PPAlloc(1024 * 1024);
	data.lockData = (ubyte*)m_lockData;
	return data;
}

// unlocks texture for modifications, etc
void CEmptyTexture::Unlock()
{
	if (!m_lockData)
		return;

	PPFree(m_lockData->lockData);
	m_lockData->lockData = nullptr;

	m_lockData = nullptr;
}

void CEmptyTexture::Ref_DeleteObject()
{
	s_shaderAPI.FreeTexture(this);
	delete this;
}