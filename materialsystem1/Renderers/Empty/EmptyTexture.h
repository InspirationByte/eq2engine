//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	bool Init(const SamplerStateParam_t& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	// locks texture for modifications, etc
	bool Lock(LockInOutData& data);
	
	// unlocks texture for modifications, etc
	void Unlock();

	void Ref_DeleteObject();
};
