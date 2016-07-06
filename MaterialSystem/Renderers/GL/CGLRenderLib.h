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

#ifdef USE_GLES2
#include <EGL/egl.h>
#endif // USE_GLES2

class ShaderAPIGL;

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

	void*					GetSharedContext();		// this is called in other thread
	void					DropSharedContext();

	void					RestoreSharedContext();	// this is often called in main thread
	

protected:

	ShaderAPIGL*			m_Renderer;

	DkList<IEqSwapChain*>	m_swapChains;

#ifdef USE_GLES2
    EGLNativeDisplayType	hdc;
    EGLNativeWindowType		hwnd;
    EGLDisplay				eglDisplay;
    EGLSurface				eglSurface;
    EGLContext				glContext;
	EGLContext				glContext2;

	EGLConfig				eglConfig;

#elif defined(_WIN32)
	DISPLAY_DEVICE			device;
	DEVMODE					dm;

	HDC						hdc;
	HGLRC					glContext;
	HGLRC					glContext2;
	HWND					hwnd;

#elif defined(LINUX)
	GLXContext				glContext;
	GLXContext				glContext2;
    XF86VidModeModeInfo**	dmodes;
    Display*                display;
    int                     m_screen;
#elif defined(__APPLE__)
	AGLContext				glContext;
	AGLContext				glContext2;
	CFArrayRef				dmodes;
	CFDictionaryRef			initialMode;
#endif // _WIN32

	int						m_width;
	int						m_height;

	bool					m_bResized;
};
#endif //CGLRENDERLIB_H
