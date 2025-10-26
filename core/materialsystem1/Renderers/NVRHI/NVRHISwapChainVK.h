//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/vulkan.h>
#include "renderers/ISwapChain.h"
#include "renderers/ShaderAPI_defs.h"
#include "NVRHITexture.h"

class CNVRHIRenderLibVK;
using nvrhi::RefCountPtr;

class CNVRHISwapChainVK : public ISwapChain
{
public:
	friend class CNVRHIRenderLibVK;

	CNVRHISwapChainVK(const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture);

	void			SetVSync(bool enable);

	void* GetWindow() const;
	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();

	bool			UpdateResize();

protected:

	void			UpdateBackbufferView() const;

	//DXGI_SWAP_CHAIN_DESC1				m_dxgiSwapChainDesc{};
	//Array<nvrhi::TextureHandle>			m_rhiSwapChainTextures{ PP_SL };
	//Array<RefCountPtr<ID3D12Resource>>	m_d3d12SwapChainBuffers{ PP_SL };
	//RefCountPtr<IDXGISwapChain3>		m_dxgiSwapChain;

	CRefPtr<CNVRHITexture>		m_textureRef;
	nvrhi::Format				m_swapChainFormat{ nvrhi::Format::RGBA8_UNORM };

	RenderWindowInfo			m_winInfo;
	int							m_vSync{ 0 };
};