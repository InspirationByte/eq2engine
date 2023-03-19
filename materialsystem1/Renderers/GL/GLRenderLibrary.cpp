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

#ifdef PLAT_WIN
#	include <dwmapi.h>
#	include <VersionHelpers.h>
#	include "wgl_caps.hpp"
#elif defined(PLAT_LINUX)
#	include "glx_caps.hpp"
#else
#	include "agl_caps.hpp"
#endif

#include "GLRenderLibrary.h"

#include "GLSwapChain.h"
#include "GLWorker.h"
#include "ShaderAPIGL.h"

#include "gl_loader.h"

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

#ifdef PLAT_LINUX

struct DispRes {
	int w, h, index;
};

struct DispRes newRes(const int w, const int h, const int i) {
	DispRes dr = { w, h, i };
	return dr;
}
int dComp(const DispRes& d0, const DispRes& d1) {
	if (d0.w != d1.w) return (d0.w - d1.w); else return (d0.h - d1.h);
}

#endif // PLAT_LINUX


HOOK_TO_CVAR(r_screen);

ShaderAPIGL g_shaderApi;
IShaderAPI* g_pShaderAPI = &g_shaderApi;

CGLRenderLib g_library;

CGLRenderLib::CGLRenderLib()
{
	g_eqCore->RegisterInterface(RENDERER_INTERFACE_VERSION, this);

	m_glSharedContext = 0;
	m_windowed = true;
	m_mainThreadId = Threading::GetCurrentThreadID();
	m_asyncOperationActive = false;
}

CGLRenderLib::~CGLRenderLib()
{
	g_eqCore->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

IShaderAPI* CGLRenderLib::GetRenderer() const
{
	return &g_shaderApi;
}

#ifdef PLAT_WIN
MONITORINFO monInfo;

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData){
	if (*(int *) dwData == 0){
		monInfo.cbSize = sizeof(monInfo);
		GetMonitorInfo(hMonitor, &monInfo);
		return FALSE;
	}
	(*(int *) dwData)--;

	return TRUE;
}

LRESULT CALLBACK PFWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, message, wParam, lParam);
};

#endif // PLAT_WIN

bool CGLRenderLib::InitCaps()
{
#if defined(PLAT_WIN)
	HINSTANCE modHandle = GetModuleHandle(nullptr);

	//Unregister PFrmt if
	UnregisterClass(L"PFrmt", modHandle);

	int depthBits = 24;
	int stencilBits = 0;

	PIXELFORMATDESCRIPTOR pfd = {
        sizeof (PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, 24,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		depthBits, stencilBits,
		0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

	WNDCLASS wincl;
	wincl.hInstance = modHandle;
	wincl.lpszClassName = L"PFrmt";
	wincl.lpfnWndProc = PFWinProc;
	wincl.style = 0;
	wincl.hIcon = nullptr;
	wincl.hCursor = nullptr;
	wincl.lpszMenuName = nullptr;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = nullptr;

	RegisterClass(&wincl);

	HWND hPFwnd = CreateWindow(L"PFrmt", L"PFormat", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, nullptr, modHandle, nullptr);
	if(hPFwnd)
	{
		HDC hdc = GetDC(hPFwnd);

		int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, nPixelFormat, &pfd);

		HGLRC hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);

		wgl::exts::LoadTest didLoad = wgl::sys::LoadFunctions(hdc);
		if(!didLoad)
		{
			MsgError("OpenGL load errors: %i\n", didLoad.GetNumMissing());
			return false;
		}

		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(hglrc);
		ReleaseDC(m_hwnd, hdc);

		SendMessage(hPFwnd, WM_CLOSE, 0, 0);
	}
	else
	{
		MsgError("Renderer fault!!!\n");
		return false;
	}
#elif PLAT_LINUX
	m_display = XOpenDisplay(0);
	m_screen = DefaultScreen( m_display );

	glX::exts::LoadTest didLoad = glX::sys::LoadFunctions(m_display, m_screen);
	if(!didLoad)
	{
		MsgError("OpenGL load errors: %i\n", didLoad.GetNumMissing());
		return false;
	}
#endif // PLAT_WIN
	return true;
}

void CGLRenderLib::DestroySharedContexts()
{
#ifdef PLAT_WIN
	wglDeleteContext(m_glSharedContext);
#elif defined(PLAT_LINUX)
	glXDestroyContext(m_display, m_glSharedContext);
#endif // PLAT_WIN || PLAT_LINUX
}


void CGLRenderLib::InitSharedContexts()
{
#ifdef PLAT_WIN
	const int iAttribs[] = {
		wgl::CONTEXT_MAJOR_VERSION_ARB,		3,
		wgl::CONTEXT_MINOR_VERSION_ARB,		3,
		wgl::CONTEXT_PROFILE_MASK_ARB,		wgl::CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	HGLRC context = wgl::CreateContextAttribsARB(m_hdc, m_glContext, iAttribs);
	DWORD error = GetLastError();
	if (!context)
	{
		char* errorStr = "Unknown error";
		if (error == wgl::ERROR_INVALID_PROFILE_ARB)
			errorStr = "ERROR_INVALID_PROFILE_ARB";
		else if (error == wgl::ERROR_INVALID_VERSION_ARB)
			errorStr = "ERROR_INVALID_VERSION_ARB";

		ErrorMsg("InitSharedContexts failed to initialize OpenGL context - %s (%x)\n", errorStr, error);
	}

	//if (wglShareLists(context, glContext) == FALSE)
	//	ASSERT_FAIL("wglShareLists - Failed to share (err=%d, ctx=%d)!", GetLastError(), context);

#elif defined(PLAT_LINUX)
	const int iAttribs[] =
	{
		GLX_CONTEXT_MAJOR_VERSION_ARB, 	3,
		GLX_CONTEXT_MINOR_VERSION_ARB, 	3,
		GLX_CONTEXT_FLAGS_ARB, 			GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		None
	};

	GLXContext context = glX::CreateContextAttribsARB( m_display, m_bestFbc, m_glContext, True, iAttribs );
#endif // PLAT_WIN || PLAT_LINUX

	m_glSharedContext = context;
}

bool CGLRenderLib::InitAPI(const shaderAPIParams_t& params)
{
	int multiSamplingMode = params.multiSamplingMode;

#ifdef PLAT_WIN
	if (r_screen->GetInt() >= GetSystemMetrics(SM_CMONITORS))
		r_screen->SetValue("0");

	m_hwnd = (HWND)params.windowHandle;
	m_hdc = GetDC(m_hwnd);

	// Enumerate display devices
	int monitorCounter = r_screen->GetInt();
	EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM) &monitorCounter);

	m_dispDevice.cb = sizeof(m_dispDevice);
	EnumDisplayDevicesA(nullptr, r_screen->GetInt(), &m_dispDevice, 0);
	
	// get window parameters

	RECT windowRect;
	GetClientRect(m_hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	ZeroMemory(&m_devMode,sizeof(m_devMode));
	m_devMode.dmSize = sizeof(m_devMode);

	// Figure display format to use
	if(params.screenFormat == FORMAT_RGBA8)
	{
		m_devMode.dmBitsPerPel = 32;
	}
	else if(params.screenFormat == FORMAT_RGB8)
	{
		m_devMode.dmBitsPerPel = 24;
	}
	else if(params.screenFormat == FORMAT_RGB565)
	{
		m_devMode.dmBitsPerPel = 16;
	}
	else
	{
		m_devMode.dmBitsPerPel = 24;
	}

	m_devMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_DISPLAYFREQUENCY;
	m_devMode.dmPelsWidth = m_width;
	m_devMode.dmPelsHeight = m_height;
	//dm.dmDisplayFrequency = 60;

	// change display settings
	SetWindowed(true);

	// choose the best format
	m_pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    m_pfd.nVersion = 1;
    m_pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    m_pfd.dwLayerMask = PFD_MAIN_PLANE;
    m_pfd.iPixelType = PFD_TYPE_RGBA;
    m_pfd.cColorBits = m_devMode.dmBitsPerPel;
    m_pfd.cDepthBits = 24;
    m_pfd.cAccumBits = 0;
    m_pfd.cStencilBits = 0;

	int iPFDAttribs[] = {
		wgl::DRAW_TO_WINDOW_ARB,			GL_TRUE,
		wgl::ACCELERATION_ARB,				wgl::FULL_ACCELERATION_ARB,
		wgl::DOUBLE_BUFFER_ARB,				GL_TRUE,
		wgl::RED_BITS_ARB,					8,
		wgl::GREEN_BITS_ARB,				8,
		wgl::BLUE_BITS_ARB,					8,
		wgl::ALPHA_BITS_ARB,				(m_devMode.dmBitsPerPel > 24) ? 8 : 0,
		wgl::DEPTH_BITS_ARB,				24,
		wgl::STENCIL_BITS_ARB,				1,
		0
	};

	int pixelFormats[256];
	int bestFormat = 0;
	int bestSamples = 0;
	uint nPFormats;

	if (wgl::exts::var_ARB_pixel_format && wgl::ChoosePixelFormatARB(m_hdc, iPFDAttribs, nullptr, elementsOf(pixelFormats), pixelFormats, &nPFormats) && nPFormats > 0)
	{
		int minDiff = 0x7FFFFFFF;
		int attrib = wgl::SAMPLES_ARB;
		int samples;

		// Find a multisample format as close as possible to the requested
		for (uint i = 0; i < nPFormats; i++)
		{
			wgl::GetPixelFormatAttribivARB(m_hdc, pixelFormats[i], 0, 1, &attrib, &samples);
			int diff = abs(multiSamplingMode - samples);
			if (diff < minDiff)
			{
				minDiff = diff;
				bestFormat = i;
				bestSamples = samples;
			}
		}

		multiSamplingMode = bestSamples;
	}
	else
	{
		pixelFormats[0] = ChoosePixelFormat(m_hdc, &m_pfd);
	}

	m_bestPixelFormat = pixelFormats[bestFormat];
	SetPixelFormat(m_hdc, m_bestPixelFormat, &m_pfd);

	int iAttribs[] = {
		wgl::CONTEXT_MAJOR_VERSION_ARB,		3,
		wgl::CONTEXT_MINOR_VERSION_ARB,		3,
		wgl::CONTEXT_PROFILE_MASK_ARB,		wgl::CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	m_glContext = wgl::CreateContextAttribsARB(m_hdc, nullptr, iAttribs);
	DWORD error = GetLastError();
	if(!m_glContext)
	{
		char* errorStr = "Unknown error";
		if (error == wgl::ERROR_INVALID_PROFILE_ARB)
			errorStr = "ERROR_INVALID_PROFILE_ARB";
		else if (error == wgl::ERROR_INVALID_VERSION_ARB)
			errorStr = "ERROR_INVALID_VERSION_ARB";

		ErrorMsg("Cannot initialize OpenGL context - %s (%x)\n", errorStr, error);
		return false;
	}

	InitSharedContexts();

	const bool makeCurrentSuccess = wglMakeCurrent(m_hdc, m_glContext);

#elif defined(PLAT_LINUX)

	int nModes;
    XF86VidModeGetAllModeLines(m_display, m_screen, &nModes, &m_dmodes);

	Array<DispRes> modes(PP_SL);

	int foundMode = -1;
	for (int i = 0; i < nModes; i++)
	{
		if (m_dmodes[i]->hdisplay >= 640 && m_dmodes[i]->vdisplay >= 480)
		{
			modes.append(newRes(m_dmodes[i]->hdisplay, m_dmodes[i]->vdisplay, i));

			if (m_dmodes[i]->hdisplay == m_width && m_dmodes[i]->vdisplay == m_width)
				foundMode = i;
		}
	}

	m_window = (Window)params.windowHandle;

    XWindowAttributes winAttrib;
    XGetWindowAttributes(m_display, m_window, &winAttrib);

	m_width = winAttrib.width;
	m_height = winAttrib.height;

    int colorBits = 16;
    int depthBits = 24;
    int stencilBits = 0;

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

    if (!m_windowed)
    {
		if (foundMode >= 0 && XF86VidModeSwitchToMode(m_display, m_screen, m_dmodes[foundMode]))
		{
			XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
		}
		else
		{
			MsgError("Couldn't set fullscreen at %dx%d.", m_width, m_height);
			m_windowed = true;
		}
	}

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
#endif //PLAT_WIN

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

	//-------------------------------------------
	// init caps
	//-------------------------------------------
	ShaderAPICaps_t& caps = g_shaderApi.m_caps;
	caps.maxTextureAnisotropicLevel = 1;

	caps.isHardwareOcclusionQuerySupported = true;
	caps.isInstancingSupported = true; // GL ES 3

	if (GLAD_GL_ARB_texture_filter_anisotropic)
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &caps.maxTextureAnisotropicLevel);

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

	caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED | SHADER_CAPS_GEOMETRY_SUPPORTED;
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

		caps.textureFormatsSupported[FORMAT_D32F] = 
			caps.renderTargetFormatsSupported[FORMAT_D32F] = GLAD_GL_ARB_depth_buffer_float;

		if (GLAD_GL_EXT_texture_compression_s3tc)
		{
			for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
				caps.textureFormatsSupported[i] = true;

			caps.textureFormatsSupported[FORMAT_ATI1N] = false;
		}
	}

	GLCheckError("caps check");

	return true;
}

void CGLRenderLib::ExitAPI()
{
#ifdef PLAT_WIN
	wglMakeCurrent(nullptr, nullptr);

	DestroySharedContexts();

	wglDeleteContext(m_glContext);

	ReleaseDC(m_hwnd, m_hdc);

	if (!m_windowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsExA((const char *) m_dispDevice.DeviceName, nullptr, nullptr, 0, nullptr);
	}
#elif defined(PLAT_LINUX)

    glXMakeCurrent(m_display, None, nullptr);
    glXDestroyContext(m_display, m_glContext);

	if(!m_windowed)
	{
		if (XF86VidModeSwitchToMode(m_display, m_screen, m_dmodes[0]))
			XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
	}

    XFree(m_dmodes);
	//XFreeCursor(display, blankCursor);
	XSync(m_display, False);
    XCloseDisplay(m_display);
#endif
}


void CGLRenderLib::BeginFrame(IEqSwapChain* swapChain)
{
	g_shaderApi.m_deviceLost = false;

	g_shaderApi.StepProgressiveLodTextures();

	int width = m_width;
	int height = m_height;

#ifdef PLAT_WIN
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
#endif

	m_curSwapChain = swapChain;

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	g_shaderApi.m_nViewportWidth = width;
	g_shaderApi.m_nViewportHeight = height;
}

void CGLRenderLib::EndFrame()
{
#ifdef PLAT_WIN
	int swapInterval = g_shaderApi.m_params.verticalSyncEnabled ? 1 : 0;

	HDC drawContext = m_hdc;
	if (m_curSwapChain)
	{
		CGLSwapChain* glSwapChain = (CGLSwapChain*)m_curSwapChain;
		drawContext = glSwapChain->m_windowDC;
	}

    // HACK: Use DwmFlush when desktop composition is enabled on Windows Vista and 7
    if (m_windowed && !IsWindows8OrGreater() && IsWindowsVistaOrGreater())
    {
        BOOL enabled = FALSE;

        if (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled)
        {
            int count = swapInterval;
            while (count--)
                DwmFlush();

			// HACK: Disable WGL swap interval when desktop composition is enabled on Windows
			//       Vista and 7 to avoid interfering with DWM vsync
			swapInterval = 0;
        }
    }

	if (wgl::exts::var_EXT_swap_control)
		wgl::SwapIntervalEXT(swapInterval);

	SwapBuffers(drawContext);

#elif defined(PLAT_LINUX)

	if (glX::exts::var_EXT_swap_control)
	{
		glX::SwapIntervalEXT(m_display, m_window, g_shaderApi.m_params.verticalSyncEnabled ? 1 : 0);
	}

	glXSwapBuffers(m_display, m_window);

#endif // PLAT_WIN
}

// changes fullscreen mode
bool CGLRenderLib::SetWindowed(bool enabled)
{
	m_windowed = enabled;

	if (!enabled)
	{
#if defined(PLAT_LINUX)
		ASSERT_FAIL("CGLRenderLib::SetWindowed - Not implemented for Linux");
		/*
		if (foundMode >= 0 && XF86VidModeSwitchToMode(display, m_screen, m_dmodes[foundMode]))
		{
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
		}
		else
		{
			MsgError("Couldn't set fullscreen at %dx%d.", m_width, m_height);
			params.windowedMode = true;
		}*/
#elif defined(PLAT_WIN)
		m_devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_DISPLAYFREQUENCY;
		m_devMode.dmPelsWidth = m_width;
		m_devMode.dmPelsHeight = m_height;

		LONG dispChangeStatus = ChangeDisplaySettingsExA((const char*)m_dispDevice.DeviceName, &m_devMode, nullptr, CDS_FULLSCREEN, nullptr);
		if (dispChangeStatus != DISP_CHANGE_SUCCESSFUL)
		{
			MsgError("ChangeDisplaySettingsEx - couldn't set fullscreen mode %dx%d on %s (%d)\n", m_width, m_height, m_dispDevice.DeviceName, dispChangeStatus);			
			return false;
		}
#endif
	}
	else
	{
#if defined(PLAT_LINUX)
		if (XF86VidModeSwitchToMode(m_display, m_screen, m_dmodes[0]))
			XF86VidModeSetViewPort(m_display, m_screen, 0, 0);
#elif defined(PLAT_WIN)
		ChangeDisplaySettingsExA((const char*)m_dispDevice.DeviceName, nullptr, nullptr, 0, nullptr);
#endif
	}
	
	return true;
}

// speaks for itself
bool CGLRenderLib::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib::SetBackbufferSize(const int w, const int h)
{
	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;
	g_shaderApi.m_deviceLost = true;

	SetWindowed(m_windowed);

	if (m_glContext != nullptr)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib::SetFocused(bool inFocus)
{

}

bool CGLRenderLib::CaptureScreenshot(CImage &img)
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

void CGLRenderLib::ReleaseSwapChains()
{
	for (int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

// creates swap chain
IEqSwapChain* CGLRenderLib::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = PPNew CGLSwapChain();

#ifdef PLAT_WIN
	if (!pNewChain->Initialize((HWND)window, g_shaderApi.m_params.verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create OpenGL swapchain!\n");
		delete pNewChain;
		return nullptr;
	}

	SetPixelFormat(pNewChain->m_windowDC, m_bestPixelFormat, &m_pfd);
#endif

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void  CGLRenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

// returns default swap chain
IEqSwapChain*  CGLRenderLib::GetDefaultSwapchain()
{
	return nullptr;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);

#ifdef PLAT_WIN
	while (wglMakeCurrent(m_hdc, m_glSharedContext) == false) {}
#elif defined(PLAT_LINUX)
	while (glXMakeCurrent(m_display, m_window, m_glSharedContext) == false) {}
#elif defined(PLAT_OSX)
	//aglMakeCurrent TODO
#endif

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

#ifdef PLAT_WIN
	while (wglMakeCurrent(nullptr, nullptr) == false) {}
#elif defined(PLAT_LINUX)
	while (glXMakeCurrent(m_display, None, nullptr) == false) {}
#elif defined(PLAT_OSX)
	// aglMakeCurrent TODO
#endif

	m_asyncOperationActive = false;
}

bool CGLRenderLib::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}