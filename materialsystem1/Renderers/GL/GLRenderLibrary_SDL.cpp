//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI through SDL
//////////////////////////////////////////////////////////////////////////////////

#include <SDL.h>
#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "shaderapigl_def.h"

#include "GLRenderLibrary_SDL.h"

#include "GLSwapChain.h"
#include "ShaderAPIGL.h"

#include "gl_loader.h"

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


HOOK_TO_CVAR(r_screen);


CGLRenderLib_SDL::CGLRenderLib_SDL()
{
	m_glSharedContext = 0;
	m_windowed = true;
	m_mainThreadId = Threading::GetCurrentThreadID();
	m_asyncOperationActive = false;
}

CGLRenderLib_SDL::~CGLRenderLib_SDL()
{
}

IShaderAPI* CGLRenderLib_SDL::GetRenderer() const
{
	return &g_shaderApi;
}

bool CGLRenderLib_SDL::InitCaps()
{
/*
#if !defined(PLAT_ANDROID)
	if (!gladLoaderLoadEGL(EGL_DEFAULT_DISPLAY))
	{
		ErrorMsg("EGL loading failed!");
		return false;
	}
#endif // PLAT_ANDROID
*/
	return true;
}


bool CGLRenderLib_SDL::InitAPI(const shaderAPIParams_t& params)
{
	ASSERT_MSG(params.windowHandleType == RHI_WINDOW_HANDLE_SDL, "Not SDL window handle");

	ASSERT_MSG(params.windowHandle != NULL, "you must specify window handle!");
	m_window = (SDL_Window*)params.windowHandle;

	Msg("Initializing SDL GL context...\n");

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	// Use OpenGL render driver.
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

	// Enable depth buffer.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	
	// Set color channel depth.
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

#ifdef USE_GLES2
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif // USE_GLES2

	m_glContext = SDL_GL_CreateContext(m_window);
	if (m_glContext == nullptr)
	{
		CrashMsg("Could not create SDL GL context\n");
		return false;
	}

	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
	m_glSharedContext = SDL_GL_CreateContext(m_window);
	if (m_glSharedContext == nullptr)
	{
		CrashMsg("Could not create SDL GL shared context (attributes missing?)\n");
		return false;
	}

	Msg("Initializing GL extensions...\n");

#ifdef USE_GLES2
	// load GLES2 GL 3.0 extensions using glad
	if (!gladLoadGLES2Loader(SDL_GL_GetProcAddress))
	{
		ErrorMsg("Cannot load OpenGL ES 2 functionality!\n");
		return false;
	}
#else
	// load OpenGL extensions using glad
	if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
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

#ifndef USE_GLES2
		const char majorStr[2] = {versionStr[0], '\0'};
		const char minorStr[2] = {versionStr[2], '\0'};

		int verMajor = atoi(majorStr);
		int verMinor = atoi(minorStr);

		if(verMajor < 2)
		{
			ErrorMsg("OpenGL major version must be at least 2!\n\nPlease update your drivers or hardware.");
			return false;
		}

		if(verMajor == 3 && verMinor < 3)
		{
			ErrorMsg("OpenGL 3.x version must be at least 3.3!\n\nPlease update your drivers or hardware.");
			return false;
		}
#else
		// TODO: VERSION CHECK
#endif // USE_GLES2
	}

#ifndef USE_GLES2 // TEMPORARILY DISABLED
	if (/*GLAD_GL_ARB_multisample &&*/ params.multiSamplingMode > 0)
		glEnable(GL_MULTISAMPLE);
#endif // USE_GLES2

	InitGLHardwareCapabilities(g_shaderApi.m_caps);
	g_glWorker.Init(this);

	return true;
}

void CGLRenderLib_SDL::ExitAPI()
{
	SDL_GL_DeleteContext(m_glContext);
	SDL_GL_DeleteContext(m_glSharedContext);
}


void CGLRenderLib_SDL::BeginFrame(IEqSwapChain* swapChain)
{
	m_currentSwapChain = swapChain;

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	g_shaderApi.m_nViewportWidth = m_width;
	g_shaderApi.m_nViewportHeight = m_height;

	g_shaderApi.SetViewport(0, 0, m_width, m_height);
}

void CGLRenderLib_SDL::EndFrame()
{
	SDL_GL_SetSwapInterval(g_shaderApi.m_params.verticalSyncEnabled ? 1 : 0);
	SDL_GL_SwapWindow(m_window);
}

// changes fullscreen mode
bool CGLRenderLib_SDL::SetWindowed(bool enabled)
{
	m_windowed = enabled;

	// SDL_SetWindowFullscreen(m_window);
	
	return true;
}

// speaks for itself
bool CGLRenderLib_SDL::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib_SDL::SetBackbufferSize(const int w, const int h)
{
	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;

	SetWindowed(m_windowed);

	if (m_glContext != NULL)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib_SDL::SetFocused(bool inFocus)
{

}

bool CGLRenderLib_SDL::CaptureScreenshot(CImage &img)
{
	glFinish();

	ubyte *pixels = img.Create(FORMAT_RGB8, m_width, m_height, 1, 1);
	ubyte *flipped = new ubyte[m_width * m_height * 3];

	glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, flipped);
	for (int y = 0; y < m_height; y++)
		memcpy(pixels + y * m_width * 3, flipped + (m_height - y - 1) * m_width * 3, m_width * 3);

	delete [] flipped;

	return true;
}

void CGLRenderLib_SDL::ReleaseSwapChains()
{
	for (int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

// creates swap chain
IEqSwapChain* CGLRenderLib_SDL::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = new CGLSwapChain();

#ifdef PLAT_WIN
	if (!pNewChain->Initialize((HWND)window, g_shaderApi.m_params->verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create OpenGL swapchain!\n");
		delete pNewChain;
		return NULL;
	}
#endif

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void  CGLRenderLib_SDL::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

// returns default swap chain
IEqSwapChain* CGLRenderLib_SDL::GetDefaultSwapchain()
{
	return NULL;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib_SDL::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);

	const int result = SDL_GL_MakeCurrent(m_window, m_glSharedContext);
	ASSERT_MSG(result == 0, "BeginAsyncOperation - SDL_GL_MakeCurrent failed (%s, code %d)", SDL_GetError(), result);

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_SDL::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	const int result = SDL_GL_MakeCurrent(nullptr, nullptr);
	ASSERT_MSG(result == 0, "EndAsyncOperation - SDL_GL_MakeCurrent failed (%s, code %d)", SDL_GetError(), result);

	m_asyncOperationActive = false;
}

bool CGLRenderLib_SDL::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}