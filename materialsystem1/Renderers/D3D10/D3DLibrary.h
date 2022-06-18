//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

#include "IShaderAPI.h"
#include "../Shared/IRenderLibrary.h"

#include <d3d10.h>
#include <d3dx10.h>

class CD3DRenderLib : public IRenderLibrary
{
public:
	friend class	ShaderAPID3DX10;

	CD3DRenderLib();
	~CD3DRenderLib();

	bool			InitCaps();

	void			ExitAPI();
	void			ReleaseSwapChains();

	bool			InitAPI(const shaderAPIParams_t &params);

	// frame begin/end
	void			BeginFrame();
	void			EndFrame(IEqSwapChain* swapChain = nullptr);

	// renderer interface
	IShaderAPI*		GetRenderer() {return m_Renderer;}

	// sets backbuffer size for default swap chain
	void			SetBackbufferSize(int w, int h);

	// captures screenshot, outputs image to 'img'
	bool			CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*	CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void			DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*	GetDefaultSwapchain();

protected:

	IShaderAPI*		m_Renderer;

	HWND			m_hwnd;
	IDXGIFactory*	m_dxgiFactory;

#ifdef COMPILE_D3D_10_1
	ID3D10Device1*	m_rhi;
#else
	ID3D10Device*	m_rhi;
#endif

	IEqSwapChain*			m_swapChain;
	Array<IEqSwapChain*>	m_swapChains{ PP_SL };

	DXGI_RATIONAL	m_fullScreenRefresh;

	DXGI_FORMAT		m_nBackBufferFormat;
	DXGI_FORMAT		m_nDepthBufferFormat;
	int				m_nMsaaSamples;

	int				m_width;
	int				m_height;

	bool			m_bResized;

	shaderAPIParams_t	savedParams;
};

#endif //CGLRENDERLIB_H