//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "shaderapigl_def.h"

#ifdef PLAT_WIN
#	include <glad_egl.h>
#else
#	include "loaders/EGL/egl.h"
#	include "loaders/EGL/eglext.h"
#endif

#ifdef PLAT_LINUX
#include <wayland-egl.h>
#endif // PLAT_LINUX

#ifdef PLAT_ANDROID
#	include <android/native_window.h>
#endif // PLAT_ANDROID

#include "GLRenderLibrary_EGL.h"

#include "GLSwapChain.h"
#include "GLWorker.h"
#include "ShaderAPIGL.h"
#include "gl_loader.h"

#ifndef EGL_OPENGL_ES3_BIT
#	define EGL_OPENGL_ES3_BIT 0x00000040
#endif // EGL_OPENGL_ES3_BIT



/*

OpenGL extensions used to generate gl_caps.hpp/cpp

EXT_texture_compression_s3tc
EXT_texture_sRGB
EXT_texture_filter_anisotropic
ARB_draw_instanced
ARB_instanced_arrays
ARB_multisample
ARB_fragment_shader
ARB_shader_objects
ARB_vertex_shader
ARB_framebuffer_object
EXT_framebuffer_object
ARB_draw_buffers
EXT_draw_buffers2
ARB_half_float_pixel
ARB_depth_buffer_float
EXT_packed_depth_stencil
ARB_texture_rectangle
ARB_texture_float
ARB_occlusion_query

*/

static const char* GetEGLErrorString(EGLint errorId)
{
#define CASE(x) case x: return #x
	switch(errorId)
	{
		CASE(EGL_SUCCESS);
		CASE(EGL_NOT_INITIALIZED);
		CASE(EGL_BAD_ACCESS);
		CASE(EGL_BAD_ALLOC);
		CASE(EGL_BAD_ATTRIBUTE);
		CASE(EGL_BAD_CONTEXT);
		CASE(EGL_BAD_CONFIG);
		CASE(EGL_BAD_CURRENT_SURFACE);
		CASE(EGL_BAD_DISPLAY);
		CASE(EGL_BAD_SURFACE);
		CASE(EGL_BAD_MATCH);
		CASE(EGL_BAD_PARAMETER);
		CASE(EGL_BAD_NATIVE_PIXMAP);
		CASE(EGL_BAD_NATIVE_WINDOW);
		CASE(EGL_CONTEXT_LOST);
	}
#undef CASE
	return nullptr;
}


IShaderAPI* CGLRenderLib_EGL::GetRenderer() const
{
	return &s_renderApi;
}

bool CGLRenderLib_EGL::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

#ifdef PLAT_WIN
	if (!gladLoaderLoadEGL(EGL_DEFAULT_DISPLAY))
	{
		ErrorMsg("EGL loading failed!");
		return false;
	}
#endif
	return true;
}


void CGLRenderLib_EGL::DestroySharedContexts()
{
	eglDestroyContext(m_eglDisplay, m_glSharedContext);
}


void CGLRenderLib_EGL::InitSharedContexts()
{
	const EGLint contextAttr[] = {
#ifdef USE_GLES2
		EGL_CONTEXT_CLIENT_VERSION, 3,
#else
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
#endif
		EGL_NONE, EGL_NONE
	};

	EGLContext context = eglCreateContext(m_eglDisplay, m_eglConfig, m_glContext, contextAttr);
	if (context == EGL_NO_CONTEXT)
		ASSERT_FAIL("Failed to create context for share!");

	m_glSharedContext = context;
}

bool CGLRenderLib_EGL::InitAPI(const ShaderAPIParams& params)
{
	ASSERT_MSG(params.windowInfo.get != nullptr, "you must specify window handle!");

	m_windowInfo = params.windowInfo;
	EGLNativeDisplayType displayType = (EGLNativeDisplayType)m_windowInfo.get(RenderWindowInfo::DISPLAY);
	m_eglDisplay = eglGetDisplay(displayType);

	Msg("Initializing EGL context...\n");

	// get egl display handle
	m_multiSamplingMode = params.multiSamplingMode;

	if (m_eglDisplay == EGL_NO_DISPLAY)
	{
		ErrorMsg("OpenGL init error: Could not get EGL display (%d)", m_eglDisplay);
		return false;
	}

	// Initialize the display
	{
		EGLint major = 0;
		EGLint minor = 0;
		if (eglInitialize(m_eglDisplay, &major, &minor) == EGL_FALSE)
		{
			ErrorMsg("OpenGL init error: Could not initialize EGL display!");
			return false;
		}

		if (major < 1)
		{
			// Does not support EGL 1.0
			ErrorMsg("OpenGL init error: System does not support at least EGL 1.0");
			return false;
		}
	}

	// HACK: call loader again to initialize rest of EGL
#ifdef PLAT_WIN
	if (!gladLoaderLoadEGL(m_eglDisplay))
	{
		ErrorMsg("EGL loading failed!");
		return false;
	}
#endif

#ifdef USE_GLES2
	eglBindAPI(EGL_OPENGL_ES_API);
#else
	eglBindAPI(EGL_OPENGL_API);
#endif
	
	m_backbufferFormat = params.screenFormat;
	if (!CreateSurface())
	{
		ErrorMsg("OpenGL init error: Could not create EGL surface\n");
		return false;
	}

	{
		// context attribute list
		const EGLint contextAttr[] = {
#ifdef USE_GLES2
			EGL_CONTEXT_CLIENT_VERSION, 3,
#else
			EGL_CONTEXT_MAJOR_VERSION, 3,
			EGL_CONTEXT_MINOR_VERSION, 3,
			EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT,
#endif
			EGL_NONE, EGL_NONE
		};

		// Create two OpenGL contexts
		m_glContext = eglCreateContext(m_eglDisplay, m_eglConfig, EGL_NO_CONTEXT, contextAttr);
		if (m_glContext == EGL_NO_CONTEXT)
		{
			CrashMsg("Could not create EGL context\n");
			return false;
		}
	}

	InitSharedContexts();

	// assign to this thread
	if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_glContext))
	{
		CrashMsg("eglMakeCurrent - error\n");
		return false;
	}

	Msg("Initializing GL extensions...\n");

#ifdef USE_GLES2
	// load GLES2 GL 3.0 extensions using glad
	if (!gladLoadGLES2Loader(gladHelperLoaderFunction))
	{
		ErrorMsg("Cannot load OpenGL ES 2 functionality!\n");
		return false;
	}
#else
	// load OpenGL extensions using glad
	if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress))
	{
		MsgError("Cannot load OpenGL extensions or several functions!\n");
		return false;
	}
#endif // USE_GLES2


	if(g_cmdLine->FindArgument("-glext") != -1)
		PrintGLExtensions();

	{
		const char* rend = (const char *) glGetString(GL_RENDERER);
		const char* vendor = (const char *) glGetString(GL_VENDOR);
		Msg("*Detected video adapter: %s by %s\n", rend, vendor);

		const char* versionStr = (const char *) glGetString(GL_VERSION);
		Msg("*OpenGL version is: %s\n", versionStr);
	}

#if 0 // TEMPORARILY DISABLED
	if (/*GLAD_GL_ARB_multisample &&*/ params.multiSamplingMode > 0)
		glEnable(GL_MULTISAMPLE);
#endif // USE_GLES2

	InitGLHardwareCapabilities(s_renderApi.m_caps);
	g_glWorker.Init(this);

	return true;
}

void CGLRenderLib_EGL::ExitAPI()
{	
	for (int i = 0; i < m_swapChains.numElem(); i++)
		delete m_swapChains[i];
	m_swapChains.clear();

	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(m_eglDisplay, m_glContext);
	eglDestroySurface(m_eglDisplay, m_eglSurface);
	eglTerminate(m_eglDisplay);

#ifdef PLAT_LINUX
	if (m_windowInfo.windowType == RHI_WINDOW_HANDLE_NATIVE_WAYLAND)
		wl_egl_window_destroy((wl_egl_window*)m_hwnd);
#endif // PLAT_LINUX
}


void CGLRenderLib_EGL::BeginFrame(IEqSwapChain* swapChain)
{
	s_renderApi.m_deviceLost = false;
	s_renderApi.StepProgressiveLodTextures();

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	s_renderApi.m_backbufferSize = IVector2D(m_width, m_height);

	// FIXME: is this needed?
	s_renderApi.SetViewport(IAARectangle(0, 0, m_width, m_height));
}

void CGLRenderLib_EGL::EndFrame()
{
	eglSwapInterval(m_eglDisplay, s_renderApi.m_params.verticalSyncEnabled ? 1 : 0);

#ifdef glInvalidateFramebuffer
	const GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
	glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, attachments);
	GLCheckError("invalidate buffer");
#endif

	eglSwapBuffers(m_eglDisplay, m_eglSurface);
	GLCheckError("swap buffers");
}

// changes fullscreen mode
bool CGLRenderLib_EGL::SetWindowed(bool enabled)
{
#ifdef PLAT_LINUX
	/*if (m_windowInfo.windowType == RHI_WINDOW_HANDLE_NATIVE_WAYLAND)
	{
		wl_surface* wlSurface = (wl_surface*)m_windowInfo.get(RenderWindowInfo::SURFACE);
		xdg_top* wlSurface = (wl_surface*)m_windowInfo.get(RenderWindowInfo::TOPLEVEL);

		if(enabled)
			xdg_toplevel_unset_fullscreen(m_xdg_toplevel);
		else
			xdg_toplevel_set_fullscreen(m_xdg_toplevel, NULL);

		if(m_windowed != enabled)
			wl_surface_commit(m_surface);

		m_windowed = enabled;
	}*/
#endif // PLAT_LINUX

	return true;
}

// speaks for itself
bool CGLRenderLib_EGL::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib_EGL::SetBackbufferSize(const int w, const int h)
{
#ifdef PLAT_ANDROID
	// since surface might be lost after unfocus, we create it
	CreateSurface();
#endif

	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;
	s_renderApi.m_deviceLost = true; // needed for triggering matsystem callbacks

#ifdef PLAT_LINUX
	// need for our hacked window
	if (m_windowInfo.windowType == RHI_WINDOW_HANDLE_NATIVE_WAYLAND)
		wl_egl_window_resize((wl_egl_window*)m_hwnd, m_width, m_height, 0, 0);
#endif // PLAT_LINUX

	SetWindowed(m_windowed);

	if (m_glContext != EGL_NO_CONTEXT)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib_EGL::SetFocused(bool inFocus)
{
#ifdef PLAT_ANDROID
	if (!inFocus)
		ReleaseSurface();
	else
		CreateSurface();
#endif // PLAT_ANDROID
}

void CGLRenderLib_EGL::ReleaseSurface()
{
	if (m_eglSurface == EGL_NO_SURFACE)
		return;

	// detach surface from context
	if (!eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_glContext))
		MsgError("eglMakeCurrent - error 1\n");

	// disable context
	if (!eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
		MsgError("eglMakeCurrent - error 2\n");

	MsgInfo("Destroying EGL surface...\n");
	if (!eglDestroySurface(m_eglDisplay, m_eglSurface))
		MsgError("Can't destroy EGL surface\n");

	m_eglSurface = EGL_NO_SURFACE;
}

bool CGLRenderLib_EGL::CreateSurface()
{
	if (m_eglSurface != EGL_NO_SURFACE)
		return true;

	// Figure display format to use
	ubyte bitsR = 8, bitsG = 8, bitsB = 8;
	switch(m_backbufferFormat)
	{
		case FORMAT_RGBA8:
		case FORMAT_RGB8:
			bitsR = 8;
			bitsG = 8;
			bitsB = 8;
			break;
		case FORMAT_RGB565:
			bitsR = 5;
			bitsG = 6;
			bitsB = 5;
			break;
		default:
			ASSERT_FAIL("Incorrect screenFormat");
			break;
	}

	// Obtain the first configuration with a depth buffer
	const EGLint attrs[] = {
		EGL_RED_SIZE,       bitsR,
		EGL_GREEN_SIZE,     bitsG,
		EGL_BLUE_SIZE,      bitsB,
		EGL_ALPHA_SIZE,     EGL_DONT_CARE,
		EGL_DEPTH_SIZE,     24,
		EGL_STENCIL_SIZE,   EGL_DONT_CARE,
		EGL_SAMPLE_BUFFERS, m_multiSamplingMode ? 1 : 0,
#ifdef USE_GLES2
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
#else
		EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
		EGL_SURFACE_TYPE,	 EGL_WINDOW_BIT,
#endif

		EGL_NONE
	};

	EGLint numConfigs = 0;
	if (eglChooseConfig(m_eglDisplay, attrs, &m_eglConfig, 1, &numConfigs) == EGL_FALSE)
	{
		ErrorMsg("OpenGL init error: Could not find valid EGL config");
		return false;
	}

	if (numConfigs < 1)
	{
		ErrorMsg("OpenGL init error: no configurations.");
		return false;
	}

#ifdef PLAT_LINUX
	if (m_windowInfo.windowType == RHI_WINDOW_HANDLE_NATIVE_WAYLAND)
	{
		// SDL2 does not maintain the window for Vulkan mode, so we have to create one
		wl_surface* wlSurface = (wl_surface*)m_windowInfo.get(RenderWindowInfo::SURFACE);
		m_hwnd = (EGLNativeWindowType)wl_egl_window_create(wlSurface, 512, 512);
	}
	else
#endif // PLAT_LINUX
	{
		m_hwnd = (EGLNativeWindowType)m_windowInfo.get(RenderWindowInfo::WINDOW);
	}

#ifdef PLAT_ANDROID
	// Get the native visual id
	EGLint nativeVid;
	if (eglGetConfigAttrib(m_eglDisplay, m_eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVid) == EGL_FALSE)
	{
		ErrorMsg("CreateSurface error: Could not get EGL native visual id");
		return false;
	}

	// On Android, EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is guaranteed to be accepted by ANativeWindow_setBuffersGeometry
	MsgInfo("Setting native window geometry\n");

	// NOTE: window must be updated since SDL may destroy old one
	ANativeWindow_setBuffersGeometry(m_hwnd, 0, 0, nativeVid);
#endif

	MsgInfo("Creating EGL surface...\n");

	// Create a surface for the main window
	m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_hwnd, nullptr);

	if (m_eglSurface == EGL_NO_SURFACE)
	{	
		MsgError("Cannot create EGL window surface - %s\n", GetEGLErrorString(eglGetError()));
		return false;
	}

	if (m_glContext && !eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_glContext))
		MsgError("eglMakeCurrent - error\n");

	return true;
}

bool CGLRenderLib_EGL::CaptureScreenshot(CImage &img)
{
	glFinish();

	ubyte *pixels = img.Create(FORMAT_RGB8, m_width, m_height, 1, 1);
	ubyte *flipped = PPNew ubyte[m_width * m_height * 3];

	glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, flipped);
	for (int y = 0; y < m_height; y++)
		memcpy(pixels + y * m_width * 3, flipped + (m_height - y - 1) * m_width * 3, m_width * 3);

	delete [] flipped;

	return true;
}

// creates swap chain
IEqSwapChain* CGLRenderLib_EGL::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = PPNew CGLSwapChain();

#ifdef PLAT_WIN
	if (!pNewChain->Initialize((HWND)window, s_renderApi.m_params.verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create OpenGL swapchain!\n");
		delete pNewChain;
		return nullptr;
	}
#endif

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void  CGLRenderLib_EGL::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib_EGL::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);

	// NOTE: Android emulator does not handle this properly, and so ANGLE.
	eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_glSharedContext);

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_EGL::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	//glFinish();
	eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	m_asyncOperationActive = false;
}

bool CGLRenderLib_EGL::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}
