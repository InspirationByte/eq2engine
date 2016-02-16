//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Rendering library base interface
//				VEERY OLD (MUSCLE ENGINE TIMES) INTERFACE//////////////////////////////////////////////////////////////////////////////////

#ifndef IRENDERLIB_H
#define IRENDERLIB_H

#include "IShaderAPI.h"
#include "IEqSwapChain.h"
#include "DebugInterface.h"

#include "InterfaceManager.h"

#define RENDERER_INTERFACE_VERSION	"DkRenderer_009"

class IRenderLibrary : public ICoreModuleInterface
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

	bool					IsInitialized() const {return true;};
	const char*				GetInterfaceName() const {return RENDERER_INTERFACE_VERSION;}
};

#endif //IRENDERLIB_H
