//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ES ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifndef USE_GLES2
static_assert(false, "this file should NOT BE included when non-GLES version is built")
#endif // USE_GLES2

#include <EGL/egl.h>
#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "shaderapigl_def.h"

#include "GLRenderLibrary_ES.h"

#include "GLSwapChain.h"
#include "GLWorker.h"
#include "ShaderAPIGL.h"

#include "gl_loader.h"

#	ifndef EGL_OPENGL_ES3_BIT
#		define EGL_OPENGL_ES3_BIT 0x00000040
#	endif // EGL_OPENGL_ES3_BIT

#	ifdef PLAT_ANDROID
#		include <android/native_window.h>
#	endif // PLAT_ANDROID

extern bool GLCheckError(const char* op, ...);

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

ShaderAPIGL g_shaderApi;
IShaderAPI* g_pShaderAPI = &g_shaderApi;

CGLRenderLib_ES g_library;

CGLRenderLib_ES::CGLRenderLib_ES()
{
	g_eqCore->RegisterInterface(RENDERER_INTERFACE_VERSION, this);

	m_glSharedContext = 0;
	m_windowed = true;
	m_mainThreadId = Threading::GetCurrentThreadID();
	m_asyncOperationActive = false;
}

CGLRenderLib_ES::~CGLRenderLib_ES()
{
	g_eqCore->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

IShaderAPI* CGLRenderLib_ES::GetRenderer() const
{
	return &g_shaderApi;
}

bool CGLRenderLib_ES::InitCaps()
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


void CGLRenderLib_ES::DestroySharedContexts()
{
	eglDestroyContext(m_eglDisplay, m_glSharedContext);
}


void CGLRenderLib_ES::InitSharedContexts()
{
	const EGLint contextAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE, EGL_NONE
	};

	EGLContext context = eglCreateContext(m_eglDisplay, m_eglConfig, m_glContext, contextAttr);
	if (context == EGL_NO_CONTEXT)
		ASSERT_FAIL("Failed to create context for share!");

	m_glSharedContext = context;
}

bool CGLRenderLib_ES::InitAPI(const shaderAPIParams_t& params)
{
	EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY;

	ASSERT_MSG(params.windowHandle != nullptr, "you must specify window handle!");
#ifdef PLAT_ANDROID
	m_lostSurface = false;
	externalWindowDisplayParams_t* winParams = (externalWindowDisplayParams_t*)params.windowHandle;
	m_hwnd = (EGLNativeWindowType)winParams->window;
#else
	// other EGL
	m_hwnd = (EGLNativeWindowType)params.windowHandle;
	nativeDisplay = (EGLNativeDisplayType)GetDC((HWND)m_hwnd);
#endif // PLAT_ANDROID

	Msg("Initializing EGL context...\n");

	// get egl display handle
	m_eglDisplay = eglGetDisplay(nativeDisplay);

	if (m_eglDisplay == EGL_NO_DISPLAY)
	{
		ErrorMsg("OpenGL ES init error: Could not get EGL display (%d)", m_eglDisplay);
		return false;
	}

	// Initialize the display
	{
		EGLint major = 0;
		EGLint minor = 0;
		if (eglInitialize(m_eglDisplay, &major, &minor) == EGL_FALSE)
		{
			ErrorMsg("OpenGL ES init error: Could not initialize EGL display!");
			return false;
		}

		if (major < 1)
		{
			// Does not support EGL 1.0
			ErrorMsg("OpenGL ES init error: System does not support at least EGL 1.0");
			return false;
		}
	}

	{
		// Obtain the first configuration with a depth buffer
		const EGLint attrs[] = {
			EGL_RED_SIZE,       5,
			EGL_GREEN_SIZE,     6,
			EGL_BLUE_SIZE,      5,
			EGL_ALPHA_SIZE,     EGL_DONT_CARE,
			EGL_DEPTH_SIZE,     24,
			EGL_STENCIL_SIZE,   EGL_DONT_CARE,
			EGL_SAMPLE_BUFFERS, params.multiSamplingMode ? 1 : 0,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,

			EGL_NONE
		};

		EGLint numOfConfigs = 0;

		// Get the number of possible EGLConfigs
		if (eglGetConfigs(m_eglDisplay, nullptr, 0, &numOfConfigs) == EGL_FALSE)
		{
			ErrorMsg("OpenGL ES init error: eglGetConfigs failed");
			return false;
		}

		Array<EGLConfig> configList(PP_SL);
		configList.setNum(numOfConfigs);

		EGLint configNumber = 0;
		if (eglChooseConfig(m_eglDisplay, attrs, configList.ptr(), numOfConfigs, &configNumber) == EGL_FALSE)
		{
			ErrorMsg("OpenGL ES init error: Could not find valid EGL config");
			return false;
		}

		if (numOfConfigs < 1)
		{
			ErrorMsg("OpenGL ES init error: no configurations.");
			return false;
		}

		// set config
		m_eglConfig = configList[0];
	}

#ifdef PLAT_ANDROID
	{
		// Get the native visual id
		EGLint nativeVid;
		if (eglGetConfigAttrib(m_eglDisplay, m_eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVid) == EGL_FALSE)
		{
			ErrorMsg("OpenGL ES init error: Could not get native visual id");
			return false;
		}

		// On Android, EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is guaranteed to be accepted by ANativeWindow_setBuffersGeometry
		ANativeWindow_setBuffersGeometry(m_hwnd, 0, 0, nativeVid);
	}
#endif
	
	// Create a surface for the main window
	m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_hwnd, nullptr);
	if (m_eglSurface == EGL_NO_SURFACE)
	{
		ErrorMsg("OpenGL ES init error: Could not create EGL surface\n");
		return false;
	}

	{
		// context attribute list
		const EGLint contextAttr[] = {
			EGL_CONTEXT_CLIENT_VERSION, 3,
			EGL_NONE,
		};

		MsgInfo("eglCreateContext...\n");

		// Create two OpenGL ES contexts
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

	// load GLES2 GL 3.0 extensions using glad
	if(!gladLoadGLES2Loader( gladHelperLoaderFunction ))
	{
		ErrorMsg("Cannot load OpenGL ES 2 functionality!\n");
		return false;
	}

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

	//-------------------------------------------
	// init caps
	//-------------------------------------------
	ShaderAPICaps_t& caps = g_shaderApi.m_caps;

	memset(&caps, 0, sizeof(caps));

	caps.maxTextureAnisotropicLevel = 1;

	caps.isHardwareOcclusionQuerySupported = true;
	caps.isInstancingSupported = true; // GL ES 3

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

		// all mobile compressed formats should be supported... except few specific that not included here
		for (int i = FORMAT_ETC1; i <= FORMAT_PVRTC_A_4BPP; i++)
			caps.textureFormatsSupported[i] = true;
	}

	GLCheckError("caps check");

	return true;
}

void CGLRenderLib_ES::ExitAPI()
{
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(m_eglDisplay, m_glContext);
	eglDestroySurface(m_eglDisplay, m_eglSurface);
	eglTerminate(m_eglDisplay);
}


void CGLRenderLib_ES::BeginFrame(IEqSwapChain* swapChain)
{
#ifdef PLAT_ANDROID
	if (m_lostSurface && m_glContext != nullptr)
	{
		MsgInfo("Creating surface...\n");
		m_eglSurface = EGL_NO_SURFACE;
		m_eglSurface = eglCreateWindowSurface(m_eglDisplay, m_eglConfig, m_hwnd, nullptr);

		if (m_eglSurface == EGL_NO_SURFACE)
		{
			ErrorMsg("Could not create EGL surface\n");
			return;
		}

		MsgInfo("Attaching surface...\n");
		if (!eglMakeCurrent(m_eglDisplay, m_eglSurface, m_eglSurface, m_glContext))
		{
			CrashMsg("eglMakeCurrent - error\n");
			return;
		}

		m_lostSurface = false;
	}
#endif // PLAT_ANDROID

	g_glWorker.Execute("StepProgressiveTextures", []() {
		g_shaderApi.StepProgressiveLodTextures();
		return 0;
	});

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	g_shaderApi.m_nViewportWidth = m_width;
	g_shaderApi.m_nViewportHeight = m_height;

	g_shaderApi.SetViewport(0, 0, m_width, m_height);
}

void CGLRenderLib_ES::EndFrame()
{
#ifdef PLAT_ANDROID
	eglSwapInterval(m_eglDisplay, g_shaderApi.m_params.verticalSyncEnabled ? 1 : 0);

	const GLenum attachments[] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
	glInvalidateFramebuffer(GL_FRAMEBUFFER, 2, attachments);
	GLCheckError("invalidate buffer");
#endif

	eglSwapBuffers(m_eglDisplay, m_eglSurface);
	GLCheckError("swap buffers");
}

// changes fullscreen mode
bool CGLRenderLib_ES::SetWindowed(bool enabled)
{
	m_windowed = enabled;

	// OpenGL ES has it unimplemented

	return true;
}

// speaks for itself
bool CGLRenderLib_ES::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib_ES::SetBackbufferSize(const int w, const int h)
{
	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;

	SetWindowed(m_windowed);

	if (m_glContext != EGL_NO_CONTEXT)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib_ES::SetFocused(bool inFocus)
{
	if(!inFocus)
		ReleaseSurface();
}

void CGLRenderLib_ES::ReleaseSurface()
{
#ifdef PLAT_ANDROID
	MsgInfo("Detaching surface...\n");
	eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (m_eglSurface != EGL_NO_SURFACE)
	{
		MsgInfo("Destroying surface...\n");
		eglDestroySurface(m_eglDisplay, m_eglSurface);
		m_eglSurface = EGL_NO_SURFACE;
	}

	m_lostSurface = true;
#endif // PLAT_ANDROID
}

bool CGLRenderLib_ES::CaptureScreenshot(CImage &img)
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

void CGLRenderLib_ES::ReleaseSwapChains()
{
	for (int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

// creates swap chain
IEqSwapChain* CGLRenderLib_ES::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = PPNew CGLSwapChain();

#ifdef PLAT_WIN
	if (!pNewChain->Initialize((HWND)window, g_shaderApi.m_params.verticalSyncEnabled, windowed))
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
void  CGLRenderLib_ES::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

// returns default swap chain
IEqSwapChain* CGLRenderLib_ES::GetDefaultSwapchain()
{
	return nullptr;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib_ES::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);
	eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_glSharedContext);

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_ES::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	//glFinish();
	eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	m_asyncOperationActive = false;
}

bool CGLRenderLib_ES::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}
