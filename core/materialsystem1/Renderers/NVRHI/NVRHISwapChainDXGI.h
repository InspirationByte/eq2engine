//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <dxgi1_6.h>
#include <d3d11.h>
#include <d3d12.h>
#include "renderers/ISwapChain.h"
#include "renderers/ShaderAPI_defs.h"
#include "NVRHITexture.h"

class CNVRHIRenderLibDXGIBase;
using nvrhi::RefCountPtr;

class CNVRHISwapChainDXGI : public ISwapChain
{
public:
	friend class CNVRHIRenderLibDXGIBase;
	friend class CNVRHIRenderLibD3D11;
	friend class CNVRHIRenderLibD3D12;

	~CNVRHISwapChainDXGI();
	CNVRHISwapChainDXGI(CNVRHIRenderLibDXGIBase* host, const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture);

	void			SetVSync(bool enable);

	void*			GetWindow() const;
	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();

	bool			UpdateResize();
	
protected:

	void			UpdateBackbufferView() const;

	DXGI_SWAP_CHAIN_DESC1				m_dxgiSwapChainDesc{};
	Array<RefCountPtr<ID3D12Resource>>	m_d3d12SwapChainBuffers{ PP_SL };
	Array<nvrhi::TextureHandle>			m_rhiSwapChainTextures{ PP_SL };
	RefCountPtr<IDXGISwapChain3>		m_dxgiSwapChain;
	
	CRefPtr<CNVRHITexture>		m_textureRef;
	nvrhi::Format				m_swapChainFormat{ nvrhi::Format::RGBA8_UNORM };

	CNVRHIRenderLibDXGIBase*	m_host{ nullptr };
	RenderWindowInfo			m_winInfo;

	//WGPUSurface				m_surface{ nullptr };
	int							m_vSync{ -1 };
};