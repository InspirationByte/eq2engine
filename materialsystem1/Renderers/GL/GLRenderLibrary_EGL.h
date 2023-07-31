//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ES ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <EGL/egl.h>

#include "../IRenderLibrary.h"
#include "renderers/IShaderAPI.h"
#include "GLWorker.h"

class ShaderAPIGL;

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_EGL : public IRenderLibrary, public GLLibraryWorkerHandler
{
	friend class			ShaderAPIGL;

public:

	CGLRenderLib_EGL() = default;
	~CGLRenderLib_EGL() = default;

	bool					InitCaps();

	bool					InitAPI(const shaderAPIParams_t &params);
	void					ExitAPI();
	void					ReleaseSwapChains();

	void					ReleaseSurface();
	bool					CreateSurface();

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

	uintptr_t				m_mainThreadId{ 0 };
	bool					m_asyncOperationActive{ false };

	EGLContext				m_glContext{ nullptr };
	EGLContext				m_glSharedContext{ nullptr };

	shaderAPIWindowInfo_t 	m_windowInfo;

	EGLNativeWindowType		m_hwnd{ 0 };
    EGLDisplay				m_eglDisplay{ nullptr };
    EGLSurface				m_eglSurface{ nullptr };
	EGLConfig				m_eglConfig{ nullptr };

	ETextureFormat 			m_backbufferFormat{ FORMAT_NONE };
	int						m_multiSamplingMode{ 0 };
	int						m_width{ 0 };
	int						m_height{ 0 };

	bool					m_bResized{ false };
	bool					m_windowed{ true };
};
