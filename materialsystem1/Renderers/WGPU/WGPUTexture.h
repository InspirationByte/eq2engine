//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "CTexture.h"

class CWGPUTexture : public CTexture
{
public:
	friend class WGPURenderAPI;

	// initializes texture from image array of images
	bool Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	// locks texture for modifications, etc
	bool Lock(LockInOutData& data);
	
	// unlocks texture for modifications, etc
	void Unlock();

	void Ref_DeleteObject();
};
