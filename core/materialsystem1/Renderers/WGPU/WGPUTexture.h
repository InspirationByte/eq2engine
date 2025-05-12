//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU texture
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "WGPUBackend.h"
#include "Texture.h"

class CWGPUTexture : public CTexture
{
	friend class CWGPURenderAPI;
	friend class CWGPUSwapChain;
public:
	~CWGPUTexture();

	bool			Init(const CRefPtr<CImage> image, const SamplerStateParams& sampler, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock(IGPUCommandRecorder* writeCmdRecorder = nullptr);

	WGPUTexture		GetWGPUTexture() const { return m_rhiTexture; }
	WGPUTextureView	GetWGPUTextureView(int viewIdx = 0) const { return m_rhiViews[viewIdx]; }
	int				GetWGPUTextureViewCount() const { return m_rhiViews.numElem(); }

protected:

	WGPUTexture				m_rhiTexture{ nullptr };
	Array<WGPUTextureView>	m_rhiViews{ PP_SL };
	EImageType				m_imgType{ IMAGE_TYPE_INVALID };
	int						m_texSize{ 0 };
};
