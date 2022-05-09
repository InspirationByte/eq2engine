//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3DLIBRARY_H
#define D3DLIBRARY_H

#include <d3d9.h>

#include "renderers/IShaderAPI.h"
#include "IRenderLibrary.h"

class CD3DRenderLib : public IRenderLibrary
{
public:
	friend class	ShaderAPID3DX9;

	CD3DRenderLib();
	~CD3DRenderLib();

	bool					InitCaps();

	void					ExitAPI();
	void					ReleaseSwapChains();

	bool					InitAPI( const shaderAPIParams_t &params );

	// frame begin/end
	void					BeginFrame();
	void					EndFrame(IEqSwapChain* swapChain = NULL);

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

	Array<IEqSwapChain*>	m_swapChains;

	HWND					m_hwnd;
	DISPLAY_DEVICE			m_dispDev;
	D3DCAPS9				m_d3dCaps;
	D3DPRESENT_PARAMETERS	m_d3dpp;

#ifdef USE_D3DEX
	D3DDISPLAYMODEEX		m_d3dMode;
	LPDIRECT3D9EX			m_d3dFactory;
	LPDIRECT3DDEVICE9EX		m_rhi;
#else
	D3DDISPLAYMODE			m_d3dMode;
	LPDIRECT3D9				m_d3dFactory;
	LPDIRECT3DDEVICE9		m_rhi;
#endif // USE_D3DEX

	int						m_width;
	int						m_height;

	bool					m_bResized;
};
#endif //D3DLIBRARY_H