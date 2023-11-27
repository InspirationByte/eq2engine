//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"

class ShaderAPIGL;

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmd.h>
#include <X11/extensions/xf86vmode.h>

typedef XID GLXContextID;
typedef XID GLXPixmap;
typedef XID GLXDrawable;
typedef XID GLXPbuffer;
typedef XID GLXWindow;
typedef XID GLXFBConfigID;
typedef struct __GLXcontextRec* GLXContext;
typedef struct __GLXFBConfigRec* GLXFBConfig;

#define MAX_SHARED_CONTEXTS 1 // thank you, OpenGL, REALLY FUCKED ME with having multiple context, works perfect btw it crashes

class CGLRenderLib_GLX : public IRenderLibrary, public RenderWorkerHandler
{
	friend class			ShaderAPIGL;

public:

	CGLRenderLib_GLX() = default;
	~CGLRenderLib_GLX() = default;

	bool					InitCaps();

	bool					InitAPI(const ShaderAPIParams &params);
	void					ExitAPI();

	void					BeginFrame(ISwapChain* swapChain = nullptr);
	void					EndFrame();
	ITexturePtr				GetCurrentBackbuffer() const { return nullptr; }

	IShaderAPI*				GetRenderer() const;

	void					SetVSync(bool enable) { m_vSync = enable; }
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

	GLXContext				m_glContext{ nullptr };
	GLXContext				m_glSharedContext{ nullptr };

    XF86VidModeModeInfo*	m_fullScreenMode{ nullptr };
	XF86VidModeModeInfo**	m_dmodes{ nullptr };
    Display*				m_display{ nullptr };
	XVisualInfo*			m_xvi{ nullptr };
	Window					m_window;
    int						m_screen;
	GLXFBConfig 			m_bestFbc;

	int						m_width{ 0 };
	int						m_height{ 0 };
	bool					m_windowed{ true };
	bool					m_vSync{ false };
};
