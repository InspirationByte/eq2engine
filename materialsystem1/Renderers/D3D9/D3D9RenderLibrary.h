//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <d3d9.h>

#include "renderers/IShaderAPI.h"
#include "IRenderLibrary.h"

class CD3D9RenderLib : public IRenderLibrary
{
public:
	friend class	ShaderAPID3D9;

	CD3D9RenderLib();
	~CD3D9RenderLib();

	bool					InitCaps();

	void					ExitAPI();
	void					ReleaseSwapChains();

	bool					InitAPI( const shaderAPIParams_t &params );

	// frame begin/end
	void					BeginFrame(IEqSwapChain* swapChain = nullptr);
	void					EndFrame();

	// renderer interface
	IShaderAPI*				GetRenderer() const;

	// sets backbuffer size for default swap chain
	void					SetBackbufferSize(int w, int h);

	// reports focus state
	void					SetFocused(bool inFocus);

	// changes windowed/fullscreen mode; returns false if failed
	bool					SetWindowed(bool enabled);

	// speaks for itself
	bool					IsWindowed() const;

	// captures screenshot, outputs image to 'img'
	bool					CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*			CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*			GetDefaultSwapchain();

protected:

	void					CheckResetDevice();
	void					SetupSwapEffect(const shaderAPIParams_t& params);

	Array<IEqSwapChain*>	m_swapChains{ PP_SL };
	IEqSwapChain*			m_curSwapChain{ nullptr };

	HWND					m_hwnd;
	DISPLAY_DEVICE			m_dispDev;
	D3DCAPS9				m_d3dCaps;
	D3DPRESENT_PARAMETERS	m_d3dpp;

	D3DDISPLAYMODE			m_d3dMode;
	LPDIRECT3D9				m_d3dFactory{ nullptr };
	LPDIRECT3DDEVICE9		m_rhi{ nullptr };

	int						m_width{ 0 };
	int						m_height{ 0 };

	bool					m_resized{ 0 };
};
