//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IRenderLibrary.h"

class CEmptyRenderLib : public IRenderLibrary
{
public:
	CEmptyRenderLib();
	~CEmptyRenderLib();

	bool			InitCaps();

	bool			InitAPI(const shaderAPIParams_t &params);
	void			ExitAPI();
	void			ReleaseSwapChains() {}

	// frame begin/end
	void			BeginFrame(IEqSwapChain* swapChain = nullptr);
	void			EndFrame();

	// renderer interface
	IShaderAPI*		GetRenderer() const;

	// sets backbuffer size for default swap chain
	void			SetBackbufferSize(int w, int h);

	// reports focus state
	void			SetFocused(bool inFocus) {}

	// changes fullscreen mode
	bool			SetWindowed(bool enabled);

	// speaks for itself
	bool			IsWindowed() const;

	// captures screenshot, outputs image to 'img'
	bool			CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*	CreateSwapChain(void* window, bool windowed = true) {return nullptr;}

	// destroys a swapchain
	void			DestroySwapChain(IEqSwapChain* swapChain) {}

	// returns default swap chain
	IEqSwapChain*	GetDefaultSwapchain() {return nullptr;}

protected:
	EQWNDHANDLE				hwnd;

	int						width, height;
	bool					bHasWireframeRendering;
	bool					m_bActive;

	bool					m_bResized;
	bool					m_windowed;
};

