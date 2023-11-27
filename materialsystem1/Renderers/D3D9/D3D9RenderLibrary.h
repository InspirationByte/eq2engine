//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

	bool					InitAPI( const ShaderAPIParams &params );

	void					BeginFrame(ISwapChain* swapChain = nullptr);
	void					EndFrame();
	ITexturePtr				GetCurrentBackbuffer() const { return nullptr; }

	IShaderAPI*				GetRenderer() const;

	void					SetVSync(bool enable) { m_vSync = enable; }
	void					SetBackbufferSize(int w, int h);

	void					SetFocused(bool inFocus);

	bool					SetWindowed(bool enabled);
	bool					IsWindowed() const;

	bool					CaptureScreenshot(CImage &img);


	ISwapChain*				CreateSwapChain(const RenderWindowInfo& windowInfo);
	void					DestroySwapChain(ISwapChain* swapChain);

protected:

	void					SetupSwapEffect(const ShaderAPIParams& params);

	Array<ISwapChain*>		m_swapChains{ PP_SL };
	ISwapChain*				m_curSwapChain{ nullptr };

	HWND					m_hwnd;
	DISPLAY_DEVICE			m_dispDev;
	D3DCAPS9				m_d3dCaps;
	D3DPRESENT_PARAMETERS	m_d3dpp;

	D3DDISPLAYMODE			m_d3dMode;
	LPDIRECT3D9				m_d3dFactory{ nullptr };
	LPDIRECT3DDEVICE9		m_rhi{ nullptr };

	int						m_width{ 0 };
	int						m_height{ 0 };
	bool					m_vSync{ false };
};
