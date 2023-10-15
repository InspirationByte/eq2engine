//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#ifdef USE_GLES2
static_assert(false, "this file should NOT BE included when GLES version is built")
#endif // USE_GLES2

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "imaging/ImageLoader.h"
#include "shaderapigl_def.h"

#include "glx_caps.hpp"

#include "GLRenderLibrary_GLX.h"

#include "GLSwapChain.h"
#include "GLWorker.h"
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

struct DispRes 
{
	int w, h, index;
};

static struct DispRes newRes(const int w, const int h, const int i) 
{
	DispRes dr = { w, h, i };
	return dr;
}

static int dComp(const DispRes& d0, const DispRes& d1) 
{
	if (d0.w != d1.w) return (d0.w - d1.w); else return (d0.h - d1.h);
}

HOOK_TO_CVAR(r_screen);

IShaderAPI* CGLRenderLib_GLX::GetRenderer() const
{
	return &s_renderApi;
}

bool CGLRenderLib_GLX::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

	m_display = XOpenDisplay(0);
	m_screen = DefaultScreen( m_display );

	glX::exts::LoadTest didLoad = glX::sys::LoadFunctions(m_display, m_screen);
	if(!didLoad)
	{
		MsgError("OpenGL load errors: %i\n", didLoad.GetNumMissing());
		return false;
	}

	return true;
}

void CGLRenderLib_GLX::DestroySharedContexts()
{
	glXDestroyContext(m_display, m_glSharedContext);
}


void CGLRenderLib_GLX::InitSharedContexts()
{
	const int iAttribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 	3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 	3,
		GLX_CONTEXT_FLAGS_ARB, 			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	m_glSharedContext = glX::CreateContextAttribsARB( m_display, m_bestFbc, m_glContext, True, iAttribs );;
}

bool CGLRenderLib_GLX::InitAPI(const shaderAPIParams_t& params)
{
	int multiSamplingMode = params.multiSamplingMode;

	// chose display modes
	int nModes;
    XF86VidModeGetAllModeLines(m_display, m_screen, &nModes, &m_dmodes);

	for (int i = 0; i < nModes; i++)
	{
		XF86VidModeModeInfo* dmode = m_dmodes[i];
		if (dmode->hdisplay < 640 || dmode->vdisplay < 480)
			continue;

		if (dmode->hdisplay == m_width && dmode->vdisplay == m_width)
			m_fullScreenMode = dmode;
	}

	m_window = (Window)params.windowInfo.get(shaderAPIWindowInfo_t::WINDOW);

    XWindowAttributes winAttrib;
    XGetWindowAttributes(m_display, m_window, &winAttrib);

	m_width = winAttrib.width;
	m_height = winAttrib.height;

    int colorBits = 16;
    int depthBits = 24;
    int stencilBits = 1;

	// Figure display format to use
	switch(params.screenFormat)
	{
		case FORMAT_RGBA8:
			colorBits = 32;
			break;
		case FORMAT_RGB8:
			colorBits = 24;
			break;
		case FORMAT_RGB565:
			colorBits = 16;
			break;
		default:
			ASSERT_FAIL("Incorrect screenFormat");
			break;
	}

	int fbCount = 0;
	GLXFBConfig* fbConfig = nullptr;

	while (true)
	{
		static int visual_attribs[] = {
			GLX_X_RENDERABLE, 		True,
			GLX_DRAWABLE_TYPE, 		GLX_WINDOW_BIT,
			GLX_RENDER_TYPE, 		GLX_RGBA_BIT,
			GLX_X_VISUAL_TYPE, 		GLX_TRUE_COLOR,
			GLX_DOUBLEBUFFER,  		True,
			GLX_RED_SIZE,      		8,
			GLX_GREEN_SIZE,    		8,
			GLX_BLUE_SIZE,     		8,
			GLX_ALPHA_SIZE,    		(colorBits > 24) ? 8 : 0,
			GLX_DEPTH_SIZE,    		depthBits,
			GLX_STENCIL_SIZE,  		stencilBits,
			// GLX_SAMPLE_BUFFERS,		(multiSamplingMode > 0),
			// GLX_SAMPLES,         	multiSamplingMode,
			None,
		};

		fbConfig = glXChooseFBConfig(m_display, m_screen, visual_attribs, &fbCount);
		if (fbConfig != nullptr)
			break;

		multiSamplingMode -= 2;
		if (multiSamplingMode < 0)
		{
			ErrorMsg("No Visual matching colorBits=%d, depthBits=%d and stencilBits=%d", colorBits, depthBits, stencilBits);
			return false;
		}
	}

	int best_fbc = -1;
	int worst_fbc = -1;
	int best_num_samp = -1;
	int worst_num_samp = 999;
	for (int i = 0; i < fbCount; ++i)
	{
		XVisualInfo* vi = glXGetVisualFromFBConfig( m_display, fbConfig[i] );
		if ( !vi )
			continue;

		int samp_buf;
		int samples;
		glXGetFBConfigAttrib(m_display, fbConfig[i], GLX_SAMPLE_BUFFERS, &samp_buf );
		glXGetFBConfigAttrib(m_display, fbConfig[i], GLX_SAMPLES, &samples );
		
		if ( best_fbc < 0 || samp_buf && samples > best_num_samp )
		{
			best_fbc = i;
			best_num_samp = samples;
		}

		if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
		{
			worst_fbc = i;
			worst_num_samp = samples;
		}

		XFree( vi );
	}

	m_bestFbc = fbConfig[best_fbc];
	XFree( fbConfig );

	m_xvi = glXGetVisualFromFBConfig(m_display, m_bestFbc);

	// create context
	{
		const int iAttribs[] =
		{
			GLX_CONTEXT_MAJOR_VERSION_ARB, 	3,
			GLX_CONTEXT_MINOR_VERSION_ARB, 	3,
			GLX_CONTEXT_FLAGS_ARB, 			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
			None
		};

		m_glContext = glX::CreateContextAttribsARB( m_display, m_bestFbc, 0, True, iAttribs );

		if(!m_glContext)
		{
			ErrorMsg("Cannot initialize OpenGL context");
			return false;
		}

		MsgInfo("Direct GLX rendering context: %s\n", glXIsDirect(m_display, m_glContext) ? "YES" : "no");
	}

	InitSharedContexts();
	XSync(m_display, False);

	const bool makeCurrentSuccess = glXMakeCurrent(m_display, (GLXDrawable)m_window, m_glContext);

	ASSERT_MSG(makeCurrentSuccess, "gl context MakeCurrent failed");

	Msg("Initializing GL extensions...\n");

	const bool gladGLIsLoaded = gladLoadGLLoader( gladHelperLoaderFunction );

	{
		const char* rend = (const char *) glGetString(GL_RENDERER);
		const char* vendor = (const char *) glGetString(GL_VENDOR);
		Msg("*Detected video adapter: %s by %s\n", rend, vendor);

		const char* versionStr = (const char *) glGetString(GL_VERSION);
		Msg("*OpenGL version is: %s\n", versionStr);

		const char majorStr[2] = {versionStr[0], '\0'};
		const char minorStr[2] = {versionStr[2], '\0'};

		int verMajor = atoi(majorStr);
		int verMinor = atoi(minorStr);

		if(verMajor < 3)
		{
			ErrorMsg("OpenGL major version must be at least 3!\n\nPlease update your drivers or hardware.");
			return false;
		}

		if(verMajor == 3 && verMinor < 3)
		{
			ErrorMsg("OpenGL 3.x version must be at least 3.3!\n\nPlease update your drivers or hardware.");
			return false;
		}
	}

	// load OpenGL extensions using glad
	if(!gladGLIsLoaded)
	{
		MsgError("Cannot load OpenGL extensions or several functions!\n");
		return false;
	}

	if(g_cmdLine->FindArgument("-glext") != -1)
		PrintGLExtensions();

	if (/*GLAD_GL_ARB_multisample &&*/ multiSamplingMode > 0)
		glEnable(GL_MULTISAMPLE);

	InitGLHardwareCapabilities(s_renderApi.m_caps);
	g_glWorker.Init(this);

	return true;
}

void CGLRenderLib_GLX::ExitAPI()
{
    glXMakeCurrent(m_display, None, nullptr);
    glXDestroyContext(m_display, m_glContext);

	if(!m_windowed)
	{
		if (XF86VidModeSwitchToMode(m_display, m_screen, m_dmodes[0]))
			XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
	}

    XFree(m_dmodes);
	XSync(m_display, False);
    XCloseDisplay(m_display);
}


void CGLRenderLib_GLX::BeginFrame(IEqSwapChain* swapChain)
{
	s_renderApi.m_deviceLost = false;
	s_renderApi.StepProgressiveLodTextures();

	int width = m_width;
	int height = m_height;

/*
	TODO: linux window swapping

	HDC drawContext = m_hdc;
	if (swapChain)
	{
		CGLSwapChain* glSwapChain = (CGLSwapChain*)swapChain;
		drawContext = glSwapChain->m_windowDC;

		width = glSwapChain->m_width;
		height = glSwapChain->m_height;
	}

	if(m_curSwapChain != swapChain)
	{
		wglMakeCurrent(nullptr, nullptr);
		wglMakeCurrent(drawContext, m_glContext);
	}
*/

	m_curSwapChain = swapChain;

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	s_renderApi.m_backbufferSize = IVector2D(m_width, m_height);
}

void CGLRenderLib_GLX::EndFrame()
{
	if (glX::exts::var_EXT_swap_control)
	{
		glX::SwapIntervalEXT(m_display, m_window, s_renderApi.m_params.verticalSyncEnabled ? 1 : 0);
	}

	glXSwapBuffers(m_display, m_window);
}

// changes fullscreen mode
bool CGLRenderLib_GLX::SetWindowed(bool enabled)
{
	XF86VidModeModeInfo* desiredMode = enabled ? m_dmodes[0] : m_fullScreenMode;

	if(!desiredMode || !XF86VidModeSwitchToMode(m_display, m_screen, desiredMode))
	{
		MsgError("Couldn't switch to %s mode", enabled ? "windowed" : "fullscreen");
		enabled = m_windowed;
	}

	if(m_windowed != enabled)
		XF86VidModeSetViewPort(m_display, m_screen, 0, 0);

	m_windowed = enabled;
	
	return true;
}

// speaks for itself
bool CGLRenderLib_GLX::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib_GLX::SetBackbufferSize(const int w, const int h)
{
	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;
	s_renderApi.m_deviceLost = true; // needed for triggering matsystem callbacks

	SetWindowed(m_windowed);

	if (m_glContext != nullptr)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib_GLX::SetFocused(bool inFocus)
{

}

bool CGLRenderLib_GLX::CaptureScreenshot(CImage &img)
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

void CGLRenderLib_GLX::ReleaseSwapChains()
{
	for (int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

// creates swap chain
IEqSwapChain* CGLRenderLib_GLX::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = PPNew CGLSwapChain();

	if (!pNewChain->Initialize(window, s_renderApi.m_params.verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create OpenGL swapchain!\n");
		delete pNewChain;
		return nullptr;
	}

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void  CGLRenderLib_GLX::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

// returns default swap chain
IEqSwapChain*  CGLRenderLib_GLX::GetDefaultSwapchain()
{
	return nullptr;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib_GLX::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);

	while (glXMakeCurrent(m_display, m_window, m_glSharedContext) == false) {}

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_GLX::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	while (glXMakeCurrent(m_display, None, nullptr) == false) {}

	m_asyncOperationActive = false;
}

bool CGLRenderLib_GLX::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}