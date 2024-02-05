//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>
#include "renderers/ISwapChain.h"
#include "renderers/ShaderAPI_defs.h"
#include "WGPUTexture.h"

class CWGPURenderLib;

class CWGPUSwapChain : public ISwapChain
{
public:
	friend class CWGPURenderLib;

	~CWGPUSwapChain();
	CWGPUSwapChain(CWGPURenderLib* host, const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture);

	void			SetVSync(bool enable);

	void*			GetWindow() const;
	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();

	bool			UpdateResize();
	
protected:

	void			UpdateBackbufferView() const;

	CRefPtr<CWGPUTexture>	m_textureRef;

	CWGPURenderLib*		m_host{ nullptr };
	RenderWindowInfo	m_winInfo;

	WGPUSurface			m_surface{ nullptr };
	WGPUSwapChain		m_swapChain{ nullptr };
	IVector2D			m_backbufferSize{ 0 };
	int					m_vSync{ -1 };
};