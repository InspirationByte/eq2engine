//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI texture
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include "Texture.h"
#include "ResourcePool.h"

class CNVRHITexture : public CTexture
{
	friend class CNVRHIRenderAPI;
	friend class CNVRHISwapChainDXGI;
	friend class CNVRHISwapChainVK;

public:
	DECLARE_RENDER_RESOURCE(CNVRHITexture);

	~CNVRHITexture();

	bool			Init(const CRefPtr<CImage> image, const SamplerStateParams& sampler, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock(IGPUCommandRecorder* writeCmdRecorder = nullptr);

	nvrhi::TextureHandle				GetNVRHITextureHandle() const { return m_rhiTexture; }
	int									GetNVRHITextureViewCount() const { return m_rhiViews.numElem(); }
	const nvrhi::TextureSubresourceSet&	GetNVRHITextureView(int idx) const { return m_rhiViews[idx]; }
	nvrhi::TextureDimension				GetNVRHIDimension() const { return m_rhiDimension; }

protected:

	Array<nvrhi::TextureSubresourceSet>	m_rhiViews{ PP_SL };
	nvrhi::TextureHandle	m_rhiTexture{ nullptr };
	nvrhi::TextureDimension	m_rhiDimension{ nvrhi::TextureDimension::Unknown };
	EImageType				m_imgType{ IMAGE_TYPE_INVALID };
	int						m_texSize{ 0 };
	int						m_transientHeapIdx{ -1 };
};
