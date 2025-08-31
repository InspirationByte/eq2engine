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

class CNVRHISwapChainVK : public ISwapChain
{
public:
	friend class CNVRHIRenderLib;

	~CNVRHISwapChainVK();
	CNVRHISwapChainVK(CNVRHIRenderLib* host, const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture);

	void			SetVSync(bool enable);

	void*			GetWindow() const;
	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();

	bool			UpdateResize();
	
protected:

	void			UpdateBackbufferView() const;

	CRefPtr<CNVRHITexture>	m_textureRef;

	CNVRHIRenderLib*	m_host{ nullptr };
	RenderWindowInfo	m_winInfo;

	//WGPUSurface			m_surface{ nullptr };
	IVector2D			m_backbufferSize{ 0 };
	int					m_vSync{ -1 };
};