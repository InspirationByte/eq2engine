//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ES ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_ES_H
#define CGLRENDERLIB_ES_H

#include "../Shared/IRenderLibrary.h"
#include "ShaderAPIGL.h"
#include <EGL/egl.h>

class ShaderAPIGL;

#define GL_CONTEXT EGLContext
#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_ES : public IRenderLibrary
{
	friend class			ShaderAPIGL;

public:

	CGLRenderLib_ES();
	~CGLRenderLib_ES();

	bool					InitCaps();

	bool					InitAPI( shaderAPIParams_t &params );
	void					ExitAPI();
	void					ReleaseSwapChains();
	void					ReleaseSurface();

	// frame begin/end
	void					BeginFrame();
	void					EndFrame(IEqSwapChain* swapChain = NULL);

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

	Array<IEqSwapChain*>	m_swapChains;
	uintptr_t				m_mainThreadId;
	bool					m_asyncOperationActive;

	GL_CONTEXT				m_glContext;
	GL_CONTEXT				m_glSharedContext;

    EGLNativeDisplayType	m_hdc;
    EGLNativeWindowType		m_hwnd;
    EGLDisplay				m_eglDisplay;
    EGLSurface				m_eglSurface;
	EGLConfig				m_eglConfig;

#ifdef PLAT_ANDROID
	bool					m_lostSurface;
#endif // PLAT_ANDROID

	int						m_width;
	int						m_height;

	bool					m_bResized;
	bool					m_windowed;
};
#endif //CGLRENDERLIB_H
