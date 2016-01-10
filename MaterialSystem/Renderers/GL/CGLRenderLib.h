//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef CGLRENDERLIB_H
#define CGLRENDERLIB_H

//#include "IRenderer.h"
#include "../IRenderLibrary.h"
#include "ShaderAPIGL.h"

class ShaderAPIGL;

class CGLRenderLib : public IRenderLibrary
{
	friend class			ShaderAPIGL;

public:

							CGLRenderLib();
							~CGLRenderLib();

	bool					InitCaps();

	bool					InitAPI(const shaderapiinitparams_t &params);
	void					ExitAPI();
	void					ReleaseSwapChains();

	// frame begin/end
	void					BeginFrame();
	void					EndFrame(IEqSwapChain* swapChain = NULL);

	// renderer interface
	IShaderAPI*				GetRenderer() {return m_Renderer;}

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

protected:

	ShaderAPIGL*			m_Renderer;

	DkList<IEqSwapChain*>	m_swapChains;

#ifdef _WIN32
	HINSTANCE				m_renderInstance;
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
#elif defined(__APPLE__)
	AGLContext				glContext;
	AGLContext				glContext2;
	CFArrayRef				dmodes;
	CFDictionaryRef			initialMode;
#endif // _WIN32

	int						m_width;
	int						m_height;

	bool					m_bResized;

	shaderapiinitparams_t	savedParams;
};
#endif //CGLRENDERLIB_H
