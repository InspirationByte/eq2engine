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

	bool			Init(const ArrayCRef<CImagePtr> images, const SamplerStateParams& sampler, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock(IGPUCommandRecorder* writeCmdRecorder = nullptr);

	WGPUTexture		GetWGPUTexture() const { return m_rhiTextures[0]; }
	WGPUTextureView	GetWGPUTextureView(int viewIdx = -1) const { return m_rhiViews[viewIdx >= 0 ? viewIdx : m_animFrame]; }
	int				GetWGPUTextureViewCount() const { return m_rhiViews.numElem(); }

protected:
	void			Ref_DeleteObject();

	Array<WGPUTexture>		m_rhiTextures{ PP_SL };
	Array<WGPUTextureView>	m_rhiViews{ PP_SL };
	EImageType				m_imgType{ IMAGE_TYPE_INVALID };
	int						m_texSize{ 0 };
};
