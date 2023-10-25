//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../IRenderLibrary.h"

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

	IShaderAPI*		GetRenderer() const;

	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChain*	CreateSwapChain(const RenderWindowInfo& windowInfo) {return nullptr;}
	void			DestroySwapChain(ISwapChain* swapChain) {}

protected:
	EQWNDHANDLE				hwnd;

	int						width, height;
	bool					bHasWireframeRendering;
	bool					m_bActive;

	bool					m_bResized;
	bool					m_windowed;
};

