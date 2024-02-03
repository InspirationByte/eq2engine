//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../IRenderLibrary.h"

class CEmptySwapChain;

class CEmptyRenderLib : public IRenderLibrary
{
public:
	CEmptyRenderLib();
	~CEmptyRenderLib();

	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams &params);
	void			ExitAPI();

	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();
	ITexturePtr		GetCurrentBackbuffer() const;

	IShaderAPI*		GetRenderer() const;

	void			SetVSync(bool enable) {}
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChain*		CreateSwapChain(const RenderWindowInfo& windowInfo);
	void			DestroySwapChain(ISwapChain* swapChain);

protected:
	EQWNDHANDLE		hwnd;

	int				m_swapChainCounter{ 0 };
	ISwapChain*		m_currentSwapChain{ nullptr };
	Array<CEmptySwapChain*>	m_swapChains{ PP_SL };

	int				width, height;
	bool			bHasWireframeRendering;
	bool			m_bActive;

	bool			m_bResized;
	bool			m_windowed;
};

