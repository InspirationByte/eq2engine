//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI texture
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include "Texture.h"

class CNVRHITexture : public CTexture
{
	friend class CNVRHIRenderAPI;
	friend class CNVRHISwapChain;
public:
	~CNVRHITexture();

	bool			Init(const CRefPtr<CImage> image, const SamplerStateParams& sampler, int flags = 0);
	void			Release();

	bool			Lock(LockInOutData& data);
	void			Unlock(IGPUCommandRecorder* writeCmdRecorder = nullptr);

	nvrhi::TextureHandle				GetNVRHITextureHandle() const { return m_rhiTexture; }
	int									GetNVRHITextureViewCount() const { return m_rhiViews.numElem(); }
	const nvrhi::TextureSubresourceSet&	GetNVRHITextureView(int idx) const { return m_rhiViews[idx]; }

protected:
	void			Ref_DeleteObject();

	Array<nvrhi::TextureSubresourceSet>	m_rhiViews{ PP_SL };
	nvrhi::TextureHandle	m_rhiTexture{ nullptr };
	EImageType				m_imgType{ IMAGE_TYPE_INVALID };
	int						m_texSize{ 0 };
};
