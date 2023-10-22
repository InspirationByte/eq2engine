//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rendering library base interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "core/InterfaceManager.h"

struct ShaderAPIParams;
class IEqSwapChain;
class IShaderAPI;
class CImage;
class IRenderLibrary;

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

	virtual void			BeginFrame(IEqSwapChain* swapChain = nullptr) = 0;
	virtual void			EndFrame() = 0;

	virtual IShaderAPI*		GetRenderer() const = 0;
	virtual void			SetBackbufferSize(int w, int h) = 0;

	virtual void			SetFocused(bool inFocus) = 0;

	virtual bool			SetWindowed(bool enabled) = 0;
	virtual bool			IsWindowed() const = 0;

	virtual	bool			CaptureScreenshot(CImage &img) = 0;

	virtual IEqSwapChain*	CreateSwapChain(void* window, bool windowed = true) = 0;
	virtual void			DestroySwapChain(IEqSwapChain* swapChain) = 0;
};
