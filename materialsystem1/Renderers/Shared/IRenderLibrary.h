//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rendering library base interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once

struct shaderAPIParams_t;
class IEqSwapChain;
class IShaderAPI;
class CImage;

#define RENDERER_INTERFACE_VERSION	"E1Renderer_023"

class IRenderLibrary : public IEqCoreModule
{
public:
	virtual bool			InitCaps() = 0;

	virtual bool			InitAPI(const shaderAPIParams_t &params ) = 0;
	virtual void			ExitAPI() = 0;
	virtual void			ReleaseSwapChains() = 0;

	// frame begin/end
	virtual void			BeginFrame(IEqSwapChain* swapChain = nullptr) = 0;
	virtual void			EndFrame() = 0;

	// renderer interface
	virtual IShaderAPI*		GetRenderer() const = 0;

	// sets backbuffer size for default swap chain
	virtual void			SetBackbufferSize(int w, int h) = 0;

	// reports focus state
	virtual void			SetFocused(bool inFocus) = 0;

	// changes windowed/fullscreen mode; returns false if failed
	virtual bool			SetWindowed(bool enabled) = 0;

	// speaks for itself
	virtual bool			IsWindowed() const = 0;

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
