//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D10 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D10SWAPCHAIN_H
#define D3D10SWAPCHAIN_H

#include "IEqSwapChain.h"

#include <d3d10.h>
#include <d3dx10.h>

#include "D3D10Texture.h"

class CD3D10SwapChain : public IEqSwapChain
{
friend class		CD3DRenderLib;
public:


	CD3D10SwapChain();
	~CD3D10SwapChain();

	bool			Initialize(	HWND window,
								int numMSAASamples, 
								DXGI_FORMAT backFmt,
								bool vSync, 
								DXGI_RATIONAL refreshParams,
								bool windowed,
								IDXGIFactory* dxFactory,
								ID3D10Device* rhi);

	void*			GetWindow() {return m_window;}
	int				GetMSAASamples() {return m_numMSAASamples;}

	ITexture*		GetBackbuffer() {return m_backbuffer;}

	// retrieves backbuffer size for this swap chain
	void			GetBackbufferSize(int& wide, int& tall);

	// sets backbuffer size for this swap chain
	bool			SetBackbufferSize(int wide, int tall);

	// individual swapbuffers call
	bool			SwapBuffers();

	bool			CreateOrUpdateBackbuffer();

protected:
	IDXGISwapChain*	m_RHIChain;
	HWND			m_window;

	DXGI_FORMAT		m_backBufferFormat;

	int				m_numMSAASamples;

	CD3D10Texture*	m_backbuffer;
	bool			m_vSyncEnabled;
	int				m_width;
	int				m_height;

	ID3D10Device*	m_rhi;
};

#endif // D3D10SWAPCHAIN_H