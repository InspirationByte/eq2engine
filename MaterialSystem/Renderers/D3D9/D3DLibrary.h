//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

#include <d3d9.h>
#include <d3dx9.h>

#include "IShaderAPI.h"
#include "../IRenderLibrary.h"

class CD3DRenderLib : public IRenderLibrary
{
public:
	friend class	ShaderAPID3DX9;

	CD3DRenderLib();
	~CD3DRenderLib();

	bool					InitCaps();

	void					ExitAPI();
	void					ReleaseSwapChains();

	bool					InitAPI( shaderapiinitparams_t &params );

	// frame begin/end
	void					BeginFrame();
	void					EndFrame(IEqSwapChain* swapChain = NULL);

	// renderer interface
	IShaderAPI*				GetRenderer() const {return (IShaderAPI*)m_Renderer;}

	// sets backbuffer size for default swap chain
	void					SetBackbufferSize(int w, int h);

	// captures screenshot, outputs image to 'img'
	bool					CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*			CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*			GetDefaultSwapchain();

protected:

	ShaderAPID3DX9*			m_Renderer;

	DkList<IEqSwapChain*>	m_swapChains;

	HWND					hwnd;
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
#endif //CGLRENDERLIB_H