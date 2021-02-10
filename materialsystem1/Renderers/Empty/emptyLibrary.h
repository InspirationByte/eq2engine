//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

#include "renderers/IShaderAPI.h"
#include "IRenderLibrary.h"

class CEmptyRenderLib : public IRenderLibrary
{
public:
	CEmptyRenderLib();
	~CEmptyRenderLib();

	bool			InitCaps();

	bool			InitAPI( shaderAPIParams_t &params);
	void			ExitAPI();
	void			ReleaseSwapChains() {}

	// frame begin/end
	void			BeginFrame();
	void			EndFrame(IEqSwapChain* swapChain = NULL);

	// renderer interface
	IShaderAPI*		GetRenderer() const {return (IShaderAPI*)m_Renderer;}

	// sets backbuffer size for default swap chain
	void			SetBackbufferSize(int w, int h);

	// reports focus state
	void			SetFocused(bool inFocus) {}

	// captures screenshot, outputs image to 'img'
	bool			CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*	CreateSwapChain(void* window, bool windowed = true) {return NULL;}

	// destroys a swapchain
	void			DestroySwapChain(IEqSwapChain* swapChain) {}

	// returns default swap chain
	IEqSwapChain*	GetDefaultSwapchain() {return NULL;}

protected:

	IShaderAPI*				m_Renderer;

	EQWNDHANDLE				hwnd;

	int						width, height;
	bool					bHasWireframeRendering;
	bool					m_bActive;

	bool					m_bResized;
};
#endif //CGLRENDERLIB_H
