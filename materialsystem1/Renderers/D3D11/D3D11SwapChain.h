//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 swapchain impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/ISwapChain.h"
#include "D3D11Texture.h"

class CD3D10SwapChain : public ISwapChain
{
	friend class		CD3D11RenderLib;
public:
	~CD3D10SwapChain();

	bool			Initialize(	HWND window,
								int numMSAASamples, 
								DXGI_FORMAT backFmt,
								bool vSync, 
								DXGI_RATIONAL refreshParams,
								bool windowed,
								IDXGIFactory* dxFactory,
								ID3D10Device* rhi);

	void*			GetWindow() const {return m_window;}
	int				GetMSAASamples() const {return m_numMSAASamples;}

	ITexturePtr		GetBackbuffer() const {return ITexturePtr(m_backbuffer);}

	// retrieves backbuffer size for this swap chain
	void			GetBackbufferSize(int& wide, int& tall) const;

	// sets backbuffer size for this swap chain
	bool			SetBackbufferSize(int wide, int tall);

	// individual swapbuffers call
	bool			SwapBuffers();

	bool			CreateOrUpdateBackbuffer();

protected:
	CRefPtr<CD3D10Texture>	m_backbuffer;

	IDXGISwapChain*			m_swapChain{ nullptr };
	HWND					m_window{ nullptr };

	DXGI_FORMAT				m_backBufferFormat{ DXGI_FORMAT_UNKNOWN };

	int						m_numMSAASamples{ 0 };
	int						m_width{ 0 };
	int						m_height{ 0 };
	bool					m_vSyncEnabled{ false };
};