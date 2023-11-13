//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ES ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <EGL/egl.h>

#include "../IRenderLibrary.h"
#include "../RenderWorker.h"
#include "renderers/IShaderAPI.h"

class ShaderAPIGL;

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_EGL : public IRenderLibrary, public RenderWorkerHandler
{
	friend class			ShaderAPIGL;

public:

	CGLRenderLib_EGL() = default;
	~CGLRenderLib_EGL() = default;

	bool					InitCaps();

	bool					InitAPI(const ShaderAPIParams &params);
	void					ExitAPI();
	ITexturePtr				GetCurrentBackbuffer() const { return nullptr; }

	void					ReleaseSurface();
	bool					CreateSurface();

	// frame begin/end
	void					BeginFrame(ISwapChain* swapChain = nullptr);
	void					EndFrame();

	IShaderAPI*				GetRenderer() const;

	void					SetBackbufferSize(int w, int h);

	void					SetFocused(bool inFocus);
	bool					SetWindowed(bool enabled);
	bool					IsWindowed() const;

	bool					CaptureScreenshot(CImage &img);

	ISwapChain*			CreateSwapChain(const RenderWindowInfo& windowInfo);
	void					DestroySwapChain(ISwapChain* swapChain);

	// start capturing GL commands from specific thread id
	void					BeginAsyncOperation(uintptr_t threadId);
	void					EndAsyncOperation();
	bool					IsMainThread(uintptr_t threadId) const;
protected:

	void					InitSharedContexts();
	void					DestroySharedContexts();

	Array<ISwapChain*>	m_swapChains{ PP_SL };
	ISwapChain*			m_curSwapChain{ nullptr };

	uintptr_t				m_mainThreadId{ 0 };
	bool					m_asyncOperationActive{ false };

	EGLContext				m_glContext{ nullptr };
	EGLContext				m_glSharedContext{ nullptr };

	RenderWindowInfo 	m_windowInfo;

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
