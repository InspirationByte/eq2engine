//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Empty texture class
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>
#include "CTexture.h"

class CWGPUTexture : public CTexture
{
	friend class WGPURenderAPI;
public:
	~CWGPUTexture();

	bool			Init(const SamplerStateParams& sampler, const ArrayCRef<CRefPtr<CImage>> images, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock();

protected:
	void			Ref_DeleteObject();

	Array<WGPUTexture>		m_rhiTextures{ PP_SL };
	Array<WGPUTextureView>	m_rhiViews{ PP_SL };
	int						m_texSize{ 0 };
};
