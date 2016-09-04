//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

#include "../IRenderLibrary.h"
#include "ShaderAPIGL.h"
#include <map>

#ifdef USE_GLES2
#include <EGL/egl.h>
#endif // USE_GLES2

class ShaderAPIGL;

#ifdef USE_GLES2
#define GL_CONTEXT EGLContext
#elif _WIN32
#define GL_CONTEXT HGLRC
#elif defined(LINUX)
#define GL_CONTEXT GLXContext
#elif defined(__APPLE__)
#define GL_CONTEXT GLXContext
#endif // _WIN32

struct glsCtx_t
{
	glsCtx_t() {}
	glsCtx_t(GL_CONTEXT ctx) : context(ctx), isAqquired(false), threadId(0)
	{
	}

	GL_CONTEXT	context;
	bool		isAqquired;
	uintptr_t	threadId;
};

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib : public IRenderLibrary
{
	friend class			ShaderAPIGL;

public:

							CGLRenderLib();
							~CGLRenderLib();

	bool					InitCaps();

	bool					InitAPI( shaderapiinitparams_t &params );
	void					ExitAPI();
	void					ReleaseSwapChains();

	// frame begin/end
	void					BeginFrame();
	void					EndFrame(IEqSwapChain* swapChain = NULL);

	// renderer interface
	IShaderAPI*				GetRenderer() const {return (IShaderAPI*)m_Renderer;}

	// sets backbuffer size for default swap chain
	void					SetBackbufferSize(int w, int h);

	// captures screenshot, outputs image to 'img'
	bool					CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*			CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*			GetDefaultSwapchain();

	GL_CONTEXT				GetFreeSharedContext(uintptr_t threadId);

	GL_CONTEXT				CreateSharedContext(GL_CONTEXT shareWith);

	void					InitSharedContexts();
	void					DestroySharedContexts();
	
protected:

	ShaderAPIGL*			m_Renderer;

	DkList<IEqSwapChain*>	m_swapChains;

#ifdef USE_GLES2
    EGLNativeDisplayType	hdc;
    EGLNativeWindowType		hwnd;
    EGLDisplay				eglDisplay;
    EGLSurface				eglSurface;
    EGLContext				glContext;

	EGLConfig				eglConfig;

#elif defined(_WIN32)
	DISPLAY_DEVICE			device;
	DEVMODE					dm;

	HDC						hdc;
	HGLRC					glContext;
	HWND					hwnd;

#elif defined(LINUX)
	GLXContext				glContext;
    XF86VidModeModeInfo**	dmodes;
    Display*				display;
    int						m_screen;
#elif defined(__APPLE__)
	AGLContext				glContext;
	CFArrayRef				dmodes;
	CFDictionaryRef			initialMode;
#endif // _WIN32

	int						m_width;
	int						m_height;

	bool					m_bResized;

	glsCtx_t				m_contexts[MAX_SHARED_CONTEXTS];
};
#endif //CGLRENDERLIB_H
