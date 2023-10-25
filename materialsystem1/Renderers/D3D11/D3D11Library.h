//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <d3d10.h>

#include "renderers/IShaderAPI.h"
#include "../IRenderLibrary.h"

class CD3D11RenderLib : public IRenderLibrary
{
	friend class	ShaderAPID3DX10;
public:
	CD3D11RenderLib();
	~CD3D11RenderLib();

	bool			InitCaps();

	void			ExitAPI();
	void			ReleaseSwapChains();

	bool			InitAPI(const ShaderAPIParams &params);

	// frame begin/end
	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();

	// renderer interface
	IShaderAPI*		GetRenderer() const;

	// reports focus state
	void			SetFocused(bool inFocus);

	// changes windowed/fullscreen mode; returns false if failed
	bool			SetWindowed(bool enabled);

	// speaks for itself
	bool			IsWindowed() const;

	// sets backbuffer size for default swap chain
	void			SetBackbufferSize(int w, int h);

	// captures screenshot, outputs image to 'img'
	bool			CaptureScreenshot(CImage &img);

	// creates swap chain
	ISwapChain*	CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void			DestroySwapChain(ISwapChain* swapChain);

protected:

	HWND					m_hwnd{ nullptr };
	IDXGIFactory*			m_dxgiFactory{ nullptr };

#ifdef COMPILE_D3D_10_1
	ID3D10Device1*			m_rhi{ nullptr };
#else
	ID3D10Device*			m_rhi{ nullptr };
#endif

	ISwapChain*			m_defaultSwapChain{ nullptr };
	ISwapChain*			m_currentSwapChain{ nullptr };
	Array<ISwapChain*>	m_swapChains{ PP_SL };

	DXGI_RATIONAL			m_fullScreenRefresh;

	// TODO: struct for those
	DXGI_FORMAT				m_backBufferFormat{ DXGI_FORMAT_UNKNOWN };
	DXGI_FORMAT				m_depthBufferFormat{ DXGI_FORMAT_UNKNOWN };
	int						m_msaaSamples{ 0 };
	int						m_width{ 0 };
	int						m_height{ 0 };

	bool					m_verticalSyncEnabled{ false };

	//ShaderAPIParams		m_savedParams;
};
