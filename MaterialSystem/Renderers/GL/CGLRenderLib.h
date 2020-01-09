//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

#include "../Shared/IRenderLibrary.h"
#include "ShaderAPIGL.h"
#include <map>

#ifdef USE_GLES2
#include <EGL/egl.h>
#endif // USE_GLES2

class ShaderAPIGL;

typedef void* (*PFNGetEGLSurfaceFromSDL)();

#ifdef USE_GLES2
#define GL_CONTEXT EGLContext
#elif _WIN32
#define GL_CONTEXT HGLRC
#elif defined(LINUX)
#define GL_CONTEXT GLXContext
#elif defined(__APPLE__)
#define GL_CONTEXT GLXContext
#endif // _WIN32

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib : public IRenderLibrary
{
	friend class			ShaderAPIGL;

public:

							CGLRenderLib();
							~CGLRenderLib();

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

	// captures screenshot, outputs image to 'img'
	bool					CaptureScreenshot(CImage &img);

	// creates swap chain
	IEqSwapChain*			CreateSwapChain(void* window, bool windowed = true);

	// destroys a swapchain
	void					DestroySwapChain(IEqSwapChain* swapChain);

	// returns default swap chain
	IEqSwapChain*			GetDefaultSwapchain();

	GL_CONTEXT				GetSharedContext();
protected:

	void					InitSharedContexts();
	void					DestroySharedContexts();

	DkList<IEqSwapChain*>	m_swapChains;

	GL_CONTEXT				glContext;
	GL_CONTEXT				glSharedContext;

#ifdef USE_GLES2
    EGLNativeDisplayType	hdc;
    EGLNativeWindowType		hwnd;
    EGLDisplay				eglDisplay;
    EGLSurface				eglSurface;
	EGLConfig				eglConfig;

#ifdef ANDROID
	bool					lostSurface;
#endif // ANDROID

#elif defined(_WIN32)
	DISPLAY_DEVICE			device;
	DEVMODE					dm;

	HDC						hdc;
	
	HWND					hwnd;

#elif defined(LINUX)
    XF86VidModeModeInfo**	dmodes;
    Display*				display;
    XVisualInfo*            vi;
    int						m_screen;
#elif defined(__APPLE__)
	CFArrayRef				dmodes;
	CFDictionaryRef			initialMode;
#endif // _WIN32

	int						m_width;
	int						m_height;

	bool					m_bResized;
};
#endif //CGLRENDERLIB_H
