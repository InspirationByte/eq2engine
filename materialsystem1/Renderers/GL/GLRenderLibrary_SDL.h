//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI through SDL
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../Shared/IRenderLibrary.h"


class ShaderAPIGL;

#define GL_CONTEXT SDL_GLContext

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_SDL : public IRenderLibrary
{
	friend class			ShaderAPIGL;

public:

	CGLRenderLib_SDL();
	~CGLRenderLib_SDL();

	bool					InitCaps();

	bool					InitAPI(const shaderAPIParams_t &params);
	void					ExitAPI();
	void					ReleaseSwapChains();

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

	Array<IEqSwapChain*>	m_swapChains{ PP_SL };
	uintptr_t				m_mainThreadId;
	bool					m_asyncOperationActive;

	GL_CONTEXT				m_glContext;
	GL_CONTEXT				m_glSharedContext;
	SDL_Window*				m_window;

	int						m_width;
	int						m_height;

	bool					m_bResized;
	bool					m_windowed;
};
#endif //CGLRENDERLIB_H
