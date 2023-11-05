//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU texture
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>
#include "CTexture.h"

class CWGPUTexture : public CTexture
{
	friend class CWGPURenderAPI;
	friend class CWGPUSwapChain;
public:
	~CWGPUTexture();

	bool			Init(const SamplerStateParams& sampler, const ArrayCRef<CImagePtr> images, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock();

	WGPUTextureView	GetWGPUTextureView() const { return m_rhiViews[m_animFrame]; }

protected:
	void			Ref_DeleteObject();

	Array<WGPUTexture>		m_rhiTextures{ PP_SL };
	Array<WGPUTextureView>	m_rhiViews{ PP_SL };
	int						m_texSize{ 0 };
};
