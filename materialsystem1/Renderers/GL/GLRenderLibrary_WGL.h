//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../IRenderLibrary.h"
#include "GLWorker.h"

class ShaderAPIGL;

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_WGL : public IRenderLibrary, public GLLibraryWorkerHandler
{
	friend class ShaderAPIGL;

public:
	CGLRenderLib_WGL() = default;
	~CGLRenderLib_WGL() = default;

	bool					InitCaps();

	bool					InitAPI(const ShaderAPIParams &params);
	void					ExitAPI();

	void					BeginFrame(IEqSwapChain* swapChain = nullptr);
	void					EndFrame();

	IShaderAPI*				GetRenderer() const;

	void					SetBackbufferSize(int w, int h);
	void					SetFocused(bool inFocus);

	bool					SetWindowed(bool enabled);
	bool					IsWindowed() const;

	bool					CaptureScreenshot(CImage &img);

	IEqSwapChain*			CreateSwapChain(const RenderWindowInfo& windowInfo);
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// start capturing GL commands from specific thread id
	void					BeginAsyncOperation(uintptr_t threadId);
	void					EndAsyncOperation();
	bool					IsMainThread(uintptr_t threadId) const;
protected:

	void					InitSharedContexts();
	void					DestroySharedContexts();

	Array<IEqSwapChain*>	m_swapChains{ PP_SL };
	IEqSwapChain*			m_curSwapChain{ nullptr };

	uintptr_t				m_mainThreadId{ 0 };
	bool					m_asyncOperationActive{ false };

	HGLRC					m_glContext{ 0 };
	HGLRC					m_glSharedContext{ 0 };

	int						m_bestPixelFormat{ 0 };
	PIXELFORMATDESCRIPTOR	m_pfd;
	DISPLAY_DEVICEA			m_dispDevice;
	DEVMODEA				m_devMode;
	HDC						m_hdc{ nullptr };
	HWND					m_hwnd{ nullptr };

	int						m_width{ 0 };
	int						m_height{ 0 };
	bool					m_windowed{ true };
};
