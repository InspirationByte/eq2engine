//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rendering library base interface
//				VEERY OLD (MUSCLE ENGINE TIMES) INTERFACE//////////////////////////////////////////////////////////////////////////////////

#ifndef IRENDERLIB_H
#define IRENDERLIB_H

#include "ishaderapi.h"
#include "IEqSwapChain.h"
#include "DebugInterface.h"

#include "InterfaceManager.h"

#define RENDERER_INTERFACE_VERSION	"DkRenderer_0115"

class IRenderLibrary
{
public:
	virtual bool			InitCaps() = 0;

	virtual bool			InitAPI(const shaderapiinitparams_t &params) = 0;
	virtual void			ExitAPI() = 0;
	virtual void			ReleaseSwapChains() = 0;

	// frame begin/end
	virtual void			BeginFrame() = 0;
	virtual void			EndFrame(IEqSwapChain* swapChain = NULL) = 0;

	// renderer interface
	virtual IShaderAPI*		GetRenderer() = 0;

	// sets backbuffer size for default swap chain
	virtual void			SetBackbufferSize(int w, int h) = 0;

	// captures screenshot, outputs image to 'img'
	virtual	bool			CaptureScreenshot(CImage &img) = 0;

	// creates swap chain
	virtual IEqSwapChain*	CreateSwapChain(void* window, bool windowed = true) = 0;

	// destroys a swapchain
	virtual void			DestroySwapChain(IEqSwapChain* swapChain) = 0;

	// returns default swap chain
	virtual IEqSwapChain*	GetDefaultSwapchain() = 0;
};

#endif //IRENDERLIB_H
