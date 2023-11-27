//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rendering library base interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/InterfaceManager.h"

class ISwapChain;
class IShaderAPI;
class CImage;
class IRenderLibrary;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;
struct ShaderAPIParams;
struct RenderWindowInfo;

class IRenderManager : public IEqCoreModule
{
public:
	CORE_INTERFACE("E1RenderManager_001")

	virtual IRenderLibrary* CreateRenderer(const ShaderAPIParams &params) const = 0;

	bool					IsInitialized() const { return true; };
};


class IRenderLibrary
{
public:
	virtual bool			InitCaps() = 0;

	virtual bool			InitAPI(const ShaderAPIParams &params ) = 0;
	virtual void			ExitAPI() = 0;

	virtual void			BeginFrame(ISwapChain* swapChain = nullptr) = 0;
	virtual void			EndFrame() = 0;
	
	// returns backbuffer texture associated with swap chain used in BeginFrame
	virtual ITexturePtr		GetCurrentBackbuffer() const = 0;

	virtual IShaderAPI*		GetRenderer() const = 0;

	// set vsync on default swapchain
	virtual void			SetVSync(bool enable) = 0;

	// set backbuffer size on default swap chain
	virtual void			SetBackbufferSize(int w, int h) = 0;
	
	virtual void			SetFocused(bool inFocus) = 0;
	virtual bool			SetWindowed(bool enabled) = 0;
	virtual bool			IsWindowed() const = 0;

	virtual	bool			CaptureScreenshot(CImage &img) = 0;

	virtual ISwapChain*		CreateSwapChain(const RenderWindowInfo& windowInfo) = 0;
	virtual void			DestroySwapChain(ISwapChain* swapChain) = 0;
};
