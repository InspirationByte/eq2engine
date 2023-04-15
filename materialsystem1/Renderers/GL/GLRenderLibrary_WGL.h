//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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
	friend class			ShaderAPIGL;

public:

							CGLRenderLib_WGL();
							~CGLRenderLib_WGL();

	bool					InitCaps();

	bool					InitAPI(const shaderAPIParams_t &params);
	void					ExitAPI();
	void					ReleaseSwapChains();

	// frame begin/end
	void					BeginFrame(IEqSwapChain* swapChain = nullptr);
	void					EndFrame();

	// renderer interface
	IShaderAPI*				GetRenderer() const;

	// sets backbuffer size for default swap chain
	void					SetBackbufferSize(int w, int h);

	// reports focus state
	void					SetFocused(bool inFocus);

	// changes fullscreen mode
	bool					SetWindowed(bool enabled);

	// speaks for itself
	bool					IsWindowed() const;

	// captures screenshot, outputs image to 'img'
	bool					CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*			CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*			GetDefaultSwapchain();

	// start capturing GL commands from specific thread id
	void					BeginAsyncOperation(uintptr_t threadId);
	void					EndAsyncOperation();
	bool					IsMainThread(uintptr_t threadId) const;
protected:

	void					InitSharedContexts();
	void					DestroySharedContexts();

	Array<IEqSwapChain*>	m_swapChains{ PP_SL };
	IEqSwapChain*			m_curSwapChain{ nullptr };

	uintptr_t				m_mainThreadId;
	bool					m_asyncOperationActive;

	HGLRC					m_glContext;
	HGLRC					m_glSharedContext;

	int						m_bestPixelFormat{ 0 };
	PIXELFORMATDESCRIPTOR	m_pfd;
	DISPLAY_DEVICEA			m_dispDevice;
	DEVMODEA				m_devMode;
	HDC						m_hdc{ nullptr };
	HWND					m_hwnd{ nullptr };

	int						m_width{ 0 };
	int						m_height{ 0 };
	bool					m_windowed{ false };
};
