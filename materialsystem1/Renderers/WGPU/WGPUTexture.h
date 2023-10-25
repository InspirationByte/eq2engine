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

	bool		Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);

	bool		Lock(LockInOutData& data);
	void		Unlock();

protected:
	void		Ref_DeleteObject();


};
