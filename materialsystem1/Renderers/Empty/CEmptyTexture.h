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

	CEmptyTexture() : m_lockData(nullptr) {}

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

	// dummy class
	// locks texture for modifications, etc
	void	Lock(LockData* pLockData, Rectangle_t* pRect = nullptr, bool bDiscard = false, bool bReadOnly = false, int nLevel = 0, int nCubeFaceId = 0)
	{
		m_lockData = PPAlloc(1024*1024);
		pLockData->pData = (ubyte*)m_lockData;
	}
	
	// unlocks texture for modifications, etc
	void	Unlock() {PPFree(m_lockData); m_lockData = nullptr;}

	void*	m_lockData;
};
