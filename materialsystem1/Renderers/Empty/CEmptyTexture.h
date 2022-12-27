//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CTexture.h"

class CEmptyTexture : public CTexture
{
public:
	friend class ShaderAPIEmpty;

	// initializes texture from image array of images
	bool				Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0)
	{
		return true;
	}

	// initializes render target texture
	bool				InitRenderTarget( const SamplerStateParam_t& sampler,
											ETextureFormat format,
											int width, int height,
											int flags = 0
	) 
	{
		return true;
	}

	// locks texture for modifications, etc
	bool				Lock(LockInOutData& data)
	{
		m_lockData = &data;

		data.lockData = PPAlloc(1024*1024);
		data.lockData = (ubyte*)m_lockData;
		return data;
	}
	
	// unlocks texture for modifications, etc
	void				Unlock()
	{
		if (!m_lockData)
			return;

		PPFree(m_lockData->lockData); 
		m_lockData->lockData = nullptr;

		m_lockData = nullptr;
	}
};
