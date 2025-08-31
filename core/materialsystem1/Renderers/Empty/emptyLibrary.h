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
	void			EndFrame() {}
	ITexturePtr		GetCurrentBackbuffer() const;

	IShaderAPI*		GetRenderer() const;

	void			SetVSync(bool enable) {}
	void			SetBackbufferSize(int w, int h) {}
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);

protected:
	EQWNDHANDLE		hwnd;

	int				m_swapChainCounter{ 0 };
	ISwapChainPtr	m_currentSwapChain;
	ISwapChainPtr	m_defaultSwapChain;

	int				width, height;
	bool			bHasWireframeRendering;
	bool			m_bActive;

	bool			m_bResized;
	bool			m_windowed;
};

