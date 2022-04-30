//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI through SDL
//////////////////////////////////////////////////////////////////////////////////


#include "CGLRenderLib_SDL.h"
#include "GLSwapChain.h"

#include "core/IConsoleCommands.h"
#include "core/IDkCore.h"
#include "core/DebugInterface.h"
#include "core/platform/MessageBox.h"
#include "utils/strtools.h"
#include "imaging/ImageLoader.h"
#include "VertexFormatGL.h"

#include "gl_loader.h"

extern bool GLCheckError(const char* op);

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

ShaderAPIGL g_shaderApi;
IShaderAPI* g_pShaderAPI = &g_shaderApi;

CGLRenderLib_SDL g_library;

CGLRenderLib_SDL::CGLRenderLib_SDL()
{
	GetCore()->RegisterInterface(RENDERER_INTERFACE_VERSION, this);

	m_glSharedContext = 0;
	m_windowed = true;
	m_mainThreadId = Threading::GetCurrentThreadID();
	m_asyncOperationActive = false;
}

CGLRenderLib_SDL::~CGLRenderLib_SDL()
{
	GetCore()->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

IShaderAPI* CGLRenderLib_SDL::GetRenderer() const
{
	return &g_shaderApi;
}

bool CGLRenderLib_SDL::InitCaps()
{
#if !defined(PLAT_ANDROID)
	if (!gladLoaderLoadEGL(EGL_DEFAULT_DISPLAY))
	{
		ErrorMsg("EGL loading failed!");
		return false;
	}
#endif // PLAT_ANDROID
	return true;
}


bool CGLRenderLib_SDL::InitAPI(shaderAPIParams_t& params)
{
	ASSERTMSG(params.windowHandle != NULL, "you must specify window handle!");
	m_window = (SDL_Window*)params.windowHandle;

	Msg("Initializing SDL GL context...\n");

// #ifdef USE_GLES2
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
// #else
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
// 	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
// #endif // USE_GLES2


	m_glSharedContext = SDL_GL_CreateContext(m_window);
	if (m_glSharedContext == nullptr)
	{
		CrashMsg("Could not create SDL GL shared context (attributes missing?)\n");
		return false;
	}

	m_glContext = SDL_GL_CreateContext(m_window);
	if (m_glContext == nullptr)
	{
		CrashMsg("Could not create SDL GL context\n");
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

	//-------------------------------------------
	// init caps
	//-------------------------------------------
	ShaderAPICaps_t& caps = g_shaderApi.m_caps;

	memset(&caps, 0, sizeof(caps));

	caps.maxTextureAnisotropicLevel = 1;

	caps.isHardwareOcclusionQuerySupported = true;
	caps.isInstancingSupported = true; // GL ES 3

#ifndef USE_GLES2
	if (GLAD_GL_ARB_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.maxTextureAnisotropicLevel);
#endif // USE_GLES2

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &caps.maxTextureSize);

	caps.maxRenderTargets = MAX_MRTS;

	caps.maxVertexGenericAttributes = MAX_GL_GENERIC_ATTRIB;
	caps.maxVertexTexcoordAttributes = MAX_TEXCOORD_ATTRIB;

	caps.maxTextureUnits = 1;
	caps.maxVertexStreams = MAX_VERTEXSTREAM;
	caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &caps.maxVertexTextureUnits);
	caps.maxVertexTextureUnits = min(caps.maxVertexTextureUnits, MAX_VERTEXTEXTURES);

	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &caps.maxVertexGenericAttributes);

	// limit by the MAX_GL_GENERIC_ATTRIB defined by ShaderAPI
	caps.maxVertexGenericAttributes = min(MAX_GL_GENERIC_ATTRIB, caps.maxVertexGenericAttributes);

	caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &caps.maxTextureUnits);

	if(caps.maxTextureUnits > MAX_TEXTUREUNIT)
		caps.maxTextureUnits = MAX_TEXTUREUNIT;

	caps.maxRenderTargets = 1;
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &caps.maxRenderTargets);

	if (caps.maxRenderTargets > MAX_MRTS)
		caps.maxRenderTargets = MAX_MRTS;

	// get texture capabilities
	{
		caps.INTZSupported = true;
		caps.INTZFormat = FORMAT_D16;

		caps.NULLSupported = true;
		caps.NULLFormat = FORMAT_NONE;

		for (int i = FORMAT_R8; i <= FORMAT_RGBA16; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}
		
		for (int i = FORMAT_D16; i <= FORMAT_D24S8; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}
#ifndef USE_GLES2
		caps.textureFormatsSupported[FORMAT_D32F] = 
			caps.renderTargetFormatsSupported[FORMAT_D32F] = GLAD_GL_ARB_depth_buffer_float;

		if (GLAD_GL_EXT_texture_compression_s3tc)
		{
			for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
				caps.textureFormatsSupported[i] = true;

			caps.textureFormatsSupported[FORMAT_ATI1N] = false;
		}
#endif // USE_GLES2

#ifdef USE_GLES2
		for (int i = FORMAT_ETC1; i <= FORMAT_PVRTC_A_4BPP; i++)
			caps.textureFormatsSupported[i] = true;
		
#endif // USE_GLES3
	}

	GLCheckError("caps check");

	return true;
}

void CGLRenderLib_SDL::ExitAPI()
{
	SDL_GL_DeleteContext(m_glContext);
	SDL_GL_DeleteContext(m_glSharedContext);
}


void CGLRenderLib_SDL::BeginFrame()
{
	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	g_shaderApi.m_nViewportWidth = m_width;
	g_shaderApi.m_nViewportHeight = m_height;

	g_shaderApi.SetViewport(0, 0, m_width, m_height);
}

void CGLRenderLib_SDL::EndFrame(IEqSwapChain* schain)
{
	SDL_GL_SetSwapInterval(g_shaderApi.m_params->verticalSyncEnabled ? 1 : 0);
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

	ASSERTMSG(IsMainThread(thisThreadId) == false, "BeginAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == false);

	Msg("BeginAsyncOperation\n");

	SDL_GL_MakeCurrent(m_window, m_glSharedContext); // could be invalid

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_SDL::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERTMSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	//glFinish();

	Msg("EndAsyncOperation\n");

	SDL_GL_MakeCurrent(m_window, nullptr); // could be invalid

	m_asyncOperationActive = false;
}

bool CGLRenderLib_SDL::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}