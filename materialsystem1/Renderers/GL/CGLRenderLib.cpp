//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "CGLRenderLib.h"

#include "core/IConsoleCommands.h"
#include "core/IDkCore.h"
#include "core/DebugInterface.h"

#include "utils/strtools.h"

#include "gl_loader.h"

#ifdef USE_GLES2

#ifndef EGL_OPENGL_ES3_BIT
#define EGL_OPENGL_ES3_BIT 0x00000040
#endif // EGL_OPENGL_ES3_BIT

#elif PLAT_WIN
#include "wgl_caps.hpp"
#elif PLAT_LINUX
#include "glx_caps.hpp"
#else
#include "agl_caps.hpp"
#endif // USE_GLES2

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

#include "imaging/ImageLoader.h"

HOOK_TO_CVAR(r_screen);

ShaderAPIGL g_shaderApi;
IShaderAPI* g_pShaderAPI = &g_shaderApi;

CGLRenderLib g_library;

CGLRenderLib::CGLRenderLib()
{
	GetCore()->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
	glSharedContext = 0;
	m_windowed = true;
}

CGLRenderLib::~CGLRenderLib()
{
	GetCore()->UnregisterInterface(RENDERER_INTERFACE_VERSION);
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
#if PLAT_WIN
	HINSTANCE modHandle = GetModuleHandle(NULL);

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
	wincl.hIcon = NULL;
	wincl.hCursor = NULL;
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = NULL;

	RegisterClass(&wincl);

	HWND hPFwnd = CreateWindow(L"PFrmt", L"PFormat", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, NULL, modHandle, NULL);
	if(hPFwnd)
	{
		HDC hdc = GetDC(hPFwnd);

		int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, nPixelFormat, &pfd);

		HGLRC hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);

		wgl::exts::LoadTest didLoad = wgl::sys::LoadFunctions(hdc);
		if(!didLoad)
			MsgError("OpenGL load errors: %i\n", didLoad.GetNumMissing());

		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(hglrc);
		ReleaseDC(hwnd, hdc);

		SendMessage(hPFwnd, WM_CLOSE, 0, 0);
	}
	else
	{
		MsgError("Renderer fault!!!\n");
		return false;
	}
#endif // PLAT_WIN
	return true;
}

void PrintGLExtensions()
{
	const char *ver = (const char *) glGetString(GL_VERSION);
	Msg("OpenGL version: %s\n \n",ver);
	const char *exts = (const char *) glGetString(GL_EXTENSIONS);

	DkList<EqString> splExts;
	xstrsplit(exts," ",splExts);

	MsgWarning("Supported OpenGL extensions:\n");
	int i;
	for (i = 0; i < splExts.numElem();i++)
	{
		MsgInfo("%s\n",splExts[i].ToCString());
	}
	MsgWarning("Total extensions supported: %i\n",i);
}

DECLARE_CMD(gl_extensions,"Print supported OpenGL extensions",0)
{
	PrintGLExtensions();
}

#ifdef PLAT_LINUX

struct DispRes {
	int w, h, index;
};

struct DispRes newRes(const int w, const int h, const int i){
	DispRes dr = { w, h, i };
	return dr;
}
int dComp(const DispRes &d0, const DispRes &d1){
	if (d0.w != d1.w) return (d0.w - d1.w); else return (d0.h - d1.h);
}

#endif // PLAT_LINUX

void CGLRenderLib::DestroySharedContexts()
{
#ifdef USE_GLES2
	eglDestroyContext(eglDisplay, glSharedContext);
#else

#ifdef PLAT_WIN
	wglDeleteContext(glSharedContext);
#elif PLAT_LINUX
	glXDestroyContext(display, glSharedContext);
#endif // PLAT_WIN || PLAT_LINUX

#endif // USE_GLES2
}


void CGLRenderLib::InitSharedContexts()
{
#ifdef USE_GLES2
	// context attribute list
	EGLint contextAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};

	EGLContext context = eglCreateContext(eglDisplay, eglConfig, glContext, contextAttr);
	if (context == EGL_NO_CONTEXT)
		ASSERTMSG(false, "Failed to create context for share!");
#else

#ifdef PLAT_WIN
	int iAttribs[] = {
		wgl::CONTEXT_MAJOR_VERSION_ARB,		3,
		wgl::CONTEXT_MINOR_VERSION_ARB,		3,
		wgl::CONTEXT_PROFILE_MASK_ARB,		wgl::CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	HGLRC context = wgl::CreateContextAttribsARB(hdc, glContext, iAttribs);
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
	//	ASSERTMSG(false, EqString::Format("wglShareLists - Failed to share (err=%d, ctx=%d)!", GetLastError(), context).ToCString());

#elif PLAT_LINUX
	GLXContext context = glXCreateContext(display, vi, glContext, True);
#endif // PLAT_WIN || PLAT_LINUX

#endif // USE_GLES2

	glSharedContext = context;
}

GL_CONTEXT CGLRenderLib::GetSharedContext()
{
	return glSharedContext;
}

bool CGLRenderLib::InitAPI(shaderAPIParams_t& params)
{
#ifdef USE_GLES2
	eglSurface = EGL_NO_SURFACE;


#ifdef ANDROID
	lostSurface = false;
	externalWindowDisplayParams_t* winParams = (externalWindowDisplayParams_t*)params.windowHandle;

	ASSERT(winParams != NULL);

	hwnd = (EGLNativeWindowType)winParams->window;
#else
	// other EGL
	hwnd = (EGLNativeWindowType)params.windowHandle;
#endif // ANDROID

	Msg("Initializing EGL context...\n");

	EGLBoolean bsuccess;

	// create native window
#ifdef ANDROID
	EGLNativeDisplayType nativeDisplay = EGL_DEFAULT_DISPLAY;
#else
	EGLNativeDisplayType nativeDisplay = (EGLNativeDisplayType)GetDC((HWND)hwnd);
#endif // #ifdef ANDROID

	// get egl display handle
	eglDisplay = eglGetDisplay(nativeDisplay);

	if (eglDisplay == EGL_NO_DISPLAY)
	{
		ErrorMsg("OpenGL ES init error: Could not get EGL display (%d)", eglDisplay);
		return false;
	}

#ifndef ANDROID
	// Initialize the display
	EGLint major = 0;
	EGLint minor = 0;
	bsuccess = eglInitialize(eglDisplay, &major, &minor);
	if (!bsuccess)
	{
		ErrorMsg("OpenGL ES init error: Could not initialize EGL display!");
		return false;
	}

	MsgInfo("eglInitialize: %d.%d\n", major, minor);

	if (major < 1)
	{
		// Does not support EGL 1.0
		ErrorMsg("OpenGL ES init error: System does not support at least EGL 1.0");
		return false;
	}
#endif // ANDROID

	eglBindAPI(EGL_OPENGL_ES_API);

	// Obtain the first configuration with a depth buffer
	EGLint attrs[] = {
		EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE, EGL_WINDOW_BIT,

		EGL_DEPTH_SIZE, 16,

		EGL_RED_SIZE, 5,
		EGL_GREEN_SIZE, 6,
		EGL_BLUE_SIZE, 5,

		EGL_NONE
	};

	EGLint numConfig = 0;
	bsuccess = eglChooseConfig(eglDisplay, attrs, &eglConfig, 1, &numConfig);
	if (!bsuccess)
	{
		ErrorMsg("OpenGL ES init error: Could not find valid EGL config");
		return false;
	}

	// Get the native visual id
	int nativeVid;
	if (!eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVid))
	{
		ErrorMsg("OpenGL ES init error: Could not get native visual id");
		return false;
	}

	// Create a surface for the main window
	if (eglSurface == EGL_NO_SURFACE)
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, hwnd, NULL);

	if (eglSurface == EGL_NO_SURFACE)
	{
		ErrorMsg("OpenGL ES init error: Could not create EGL surface\n");
		return false;
	}

	// context attribute list
	EGLint contextAttr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 3,
		EGL_NONE
	};

	MsgInfo("eglCreateContext...\n");

	// Create two OpenGL ES contexts
	glContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, contextAttr);
	if (glContext == EGL_NO_CONTEXT)
	{
		CrashMsg("Could not create EGL context\n");
		return false;
	}

	//bool result = eglQueryContext();

	InitSharedContexts();

	// assign to this thread
	if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, glContext))
	{
		CrashMsg("eglMakeCurrent - error\n");
		return false;
	}

#elif defined(PLAT_WIN)

	if (r_screen->GetInt() >= GetSystemMetrics(SM_CMONITORS))
		r_screen->SetValue("0");

	hwnd = (HWND)params.windowHandle;
	hdc = GetDC(hwnd);

	// Enumerate display devices
	int monitorCounter = r_screen->GetInt();
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) &monitorCounter);

	device.cb = sizeof(device);
	EnumDisplayDevicesA(NULL, r_screen->GetInt(), &device, 0);
	
	// get window parameters

	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	ZeroMemory(&dm,sizeof(dm));
	dm.dmSize = sizeof(dm);

	// Figure display format to use
	if(params.screenFormat == FORMAT_RGBA8)
	{
		dm.dmBitsPerPel = 32;
	}
	else if(params.screenFormat == FORMAT_RGB8)
	{
		dm.dmBitsPerPel = 24;
	}
	else if(params.screenFormat == FORMAT_RGB565)
	{
		dm.dmBitsPerPel = 16;
	}
	else
	{
		dm.dmBitsPerPel = 24;
	}

	dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_DISPLAYFREQUENCY;
	dm.dmPelsWidth = m_width;
	dm.dmPelsHeight = m_height;
	//dm.dmDisplayFrequency = 60;

	// change display settings
	SetWindowed(true);

	// choose the best format
	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.dwLayerMask = PFD_MAIN_PLANE;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = dm.dmBitsPerPel;
    pfd.cDepthBits = 24;
    pfd.cAccumBits = 0;
    pfd.cStencilBits = 0;

	int iPFDAttribs[] = {

		wgl::DRAW_TO_WINDOW_ARB,			GL_TRUE,
		wgl::ACCELERATION_ARB,				wgl::FULL_ACCELERATION_ARB,
		wgl::DOUBLE_BUFFER_ARB,				GL_TRUE,
		wgl::RED_BITS_ARB,					8,
		wgl::GREEN_BITS_ARB,				8,
		wgl::BLUE_BITS_ARB,					8,
		wgl::ALPHA_BITS_ARB,				(dm.dmBitsPerPel > 24) ? 8 : 0,
		wgl::DEPTH_BITS_ARB,				24,
		wgl::STENCIL_BITS_ARB,				1,
		0
	};

	int pixelFormats[256];
	int bestFormat = 0;
	int bestSamples = 0;
	uint nPFormats;

	if (wgl::exts::var_ARB_pixel_format && wgl::ChoosePixelFormatARB(hdc, iPFDAttribs, NULL, elementsOf(pixelFormats), pixelFormats, &nPFormats) && nPFormats > 0)
	{
		int minDiff = 0x7FFFFFFF;
		int attrib = wgl::SAMPLES_ARB;
		int samples;

		// Find a multisample format as close as possible to the requested
		for (uint i = 0; i < nPFormats; i++)
		{
			wgl::GetPixelFormatAttribivARB(hdc, pixelFormats[i], 0, 1, &attrib, &samples);
			int diff = abs(params.multiSamplingMode - samples);
			if (diff < minDiff)
			{
				minDiff = diff;
				bestFormat = i;
				bestSamples = samples;
			}
		}

		params.multiSamplingMode = bestSamples;
	}
	else
	{
		pixelFormats[0] = ChoosePixelFormat(hdc, &pfd);
	}

	SetPixelFormat(hdc, pixelFormats[bestFormat], &pfd);

	int iAttribs[] = {
		wgl::CONTEXT_MAJOR_VERSION_ARB,		3,
		wgl::CONTEXT_MINOR_VERSION_ARB,		3,
		wgl::CONTEXT_PROFILE_MASK_ARB,		wgl::CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
		0
	};

	glContext = wgl::CreateContextAttribsARB(hdc, NULL, iAttribs);
	DWORD error = GetLastError();
	if(!glContext)
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

	wglMakeCurrent(hdc, glContext);

#elif PLAT_LINUX

    display = XOpenDisplay(0);

	m_screen = DefaultScreen( display );

	int nModes;
    XF86VidModeGetAllModeLines(display, m_screen, &nModes, &dmodes);

	DkList<DispRes> modes;

	char str[64];
	int foundMode = -1;
	for (int i = 0; i < nModes; i++)
	{
		if (dmodes[i]->hdisplay >= 640 && dmodes[i]->vdisplay >= 480)
		{
			modes.append(newRes(dmodes[i]->hdisplay, dmodes[i]->vdisplay, i));

			if (dmodes[i]->hdisplay == m_width && dmodes[i]->vdisplay == m_width)
				foundMode = i;
		}
	}

    Window xwin = (Window)params.windowHandle;

    XWindowAttributes winAttrib;
    XGetWindowAttributes(display,xwin,&winAttrib);

	m_width = winAttrib.width;
	m_height = winAttrib.height;

    int colorBits = 16;
    int depthBits = 24;
    int stencilBits = 0;

	// Figure display format to use
	if(params.screenFormat == FORMAT_RGBA8)
	{
		colorBits = 32;
	}
	else if(params.screenFormat == FORMAT_RGB8)
	{
		colorBits = 24;
	}
	else if(params.screenFormat == FORMAT_RGB565)
	{
		colorBits = 16;
	}
	else
	{
		colorBits = 24;
	}

	while (true)
	{
		int attribs[] = {
			GLX_RGBA,
			GLX_DOUBLEBUFFER,
			GLX_RED_SIZE,      8,
			GLX_GREEN_SIZE,    8,
			GLX_BLUE_SIZE,     8,
			GLX_ALPHA_SIZE,    (colorBits > 24)? 8 : 0,
			GLX_DEPTH_SIZE,    depthBits,
			GLX_STENCIL_SIZE,  stencilBits,
			GLX_SAMPLE_BUFFERS, (params.multiSamplingMode > 0),
			GLX_SAMPLES,         params.multiSamplingMode,
			GLX_CONTEXT_MAJOR_VERSION_ARB,		3,
			GLX_CONTEXT_MINOR_VERSION_ARB,		3,
			None,
		};

		vi = glXChooseVisual(display, m_screen, attribs);
		if (vi != NULL) break;

		params.multiSamplingMode -= 2;
		if (params.multiSamplingMode < 0){
			char str[256];
			sprintf(str, "No Visual matching colorBits=%d, depthBits=%d and stencilBits=%d", colorBits, depthBits, stencilBits);
			ErrorMsg(str);
			return false;
		}
	}

    if (!params.windowedMode)
    {
		if (foundMode >= 0 && XF86VidModeSwitchToMode(display, m_screen, dmodes[foundMode]))
		{
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
		}
		else
		{
			MsgError("Couldn't set fullscreen at %dx%d.", m_width, m_height);
			params.windowedMode = true;
		}
	}

	glContext = glXCreateContext(display, vi, None, True);
	InitSharedContexts();

	glXMakeCurrent(display, (GLXDrawable)params.windowHandle, glContext);
#endif //PLAT_WIN

	Msg("Initializing GL extensions...\n");

#ifdef USE_GLES2
	// load GLES2 GL 3.0 extensions using glad
	if(!gladLoadGLES2Loader( gladHelperLoaderFunction ))
	{
		ErrorMsg("Cannot load OpenGL ES 2 functionality!\n");
		return false;
	}
#else
	// load OpenGL extensions using glad
	if(!gladLoadGLLoader( gladHelperLoaderFunction ))
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

#ifdef USE_GLES2
	g_shaderApi.m_display = this->eglDisplay;
	//g_shaderApi.m_eglSurface = this->eglSurface;
	g_shaderApi.m_hdc = this->hdc;
#elif PLAT_WIN
	g_shaderApi.m_hdc = this->hdc;
#elif PLAT_LINUX
	g_shaderApi.m_display = this->display;
#endif //PLAT_WIN

	g_shaderApi.m_glContext = this->glContext;

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

void CGLRenderLib::ExitAPI()
{
#ifdef PLAT_WIN

#ifdef USE_GLES2
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(eglDisplay, glContext);
	eglDestroySurface(eglDisplay, eglSurface);
	eglTerminate(eglDisplay);
#else
	wglMakeCurrent(NULL, NULL);

	DestroySharedContexts();

	wglDeleteContext(glContext);

	ReleaseDC(hwnd, hdc);

	if (!m_windowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsExA((const char *) device.DeviceName, NULL, NULL, 0, NULL);
	}
#endif // USE_GLES2

#else

#ifdef PLAT_LINUX

    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glContext);

	if(!g_shaderApi.m_params->windowedMode)
	{
		if (XF86VidModeSwitchToMode(display, m_screen, dmodes[0]))
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
	}

    XFree(dmodes);
	//XFreeCursor(display, blankCursor);

	XSync(display, False);

    XCloseDisplay(display);
#endif // PLAT_LIUX

#endif // PLAT_WIN

}


void CGLRenderLib::BeginFrame()
{
#ifdef ANDROID
	if (lostSurface && glContext != NULL)
	{
		MsgInfo("Creating surface...\n");
		eglSurface = EGL_NO_SURFACE;
		eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, hwnd, NULL);

		if (eglSurface == EGL_NO_SURFACE)
		{
			ErrorMsg("Could not create EGL surface\n");
			return;
		}

		MsgInfo("Attaching surface...\n");
		if (!eglMakeCurrent(eglDisplay, eglSurface, eglSurface, glContext))
		{
			CrashMsg("eglMakeCurrent - error\n");
			return;
		}

		lostSurface = false;
	}
#endif // ANDROID

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	g_shaderApi.m_nViewportWidth = m_width;
	g_shaderApi.m_nViewportHeight = m_height;

	g_shaderApi.SetViewport(0, 0, m_width, m_height);
}

void CGLRenderLib::EndFrame(IEqSwapChain* schain)
{
#ifdef USE_GLES2

	eglSwapInterval(eglDisplay, g_shaderApi.m_params->verticalSyncEnabled ? 1 : 0);

	eglSwapBuffers(eglDisplay, eglSurface);
	GLCheckError("swap buffers");

#elif PLAT_WIN
	if (wgl::exts::var_EXT_swap_control)
	{
		wgl::SwapIntervalEXT(g_shaderApi.m_params->verticalSyncEnabled ? 1 : 0);
	}

	SwapBuffers(hdc);

#elif defined(PLAT_LINUX)

	if (glX::exts::var_EXT_swap_control)
	{
		glX::SwapIntervalEXT(display, (Window)g_shaderApi.m_params->windowHandle, g_shaderApi.m_params->verticalSyncEnabled ? 1 : 0);
	}

	glXSwapBuffers(display, (Window)g_shaderApi.m_params->windowHandle);

#endif // PLAT_WIN
}

// changes fullscreen mode
bool CGLRenderLib::SetWindowed(bool enabled)
{
	m_windowed = enabled;

	if (!enabled)
	{
#if defined(PLAT_LINUX)
		ASSERTMSG(false, "CGLRenderLib::SetWindowed - Not implemented for Linux");
		/*
		if (foundMode >= 0 && XF86VidModeSwitchToMode(display, m_screen, dmodes[foundMode]))
		{
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
		}
		else
		{
			MsgError("Couldn't set fullscreen at %dx%d.", m_width, m_height);
			params.windowedMode = true;
		}*/
#elif defined(PLAT_WIN)
		dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_DISPLAYFREQUENCY;
		dm.dmPelsWidth = m_width;
		dm.dmPelsHeight = m_height;

		LONG dispChangeStatus = ChangeDisplaySettingsExA((const char*)device.DeviceName, &dm, NULL, CDS_FULLSCREEN, NULL);
		if (dispChangeStatus != DISP_CHANGE_SUCCESSFUL)
		{
			MsgError("ChangeDisplaySettingsEx - couldn't set fullscreen mode %dx%d on %s (%d)\n", m_width, m_height, device.DeviceName, dispChangeStatus);			
			return false;
		}
#endif
	}
	else
	{
#if defined(PLAT_LINUX)
		if (XF86VidModeSwitchToMode(display, m_screen, dmodes[0]))
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
#elif defined(PLAT_WIN)
		ChangeDisplaySettingsExA((const char*)device.DeviceName, NULL, NULL, 0, NULL);
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

#if defined(PLAT_WIN) && !defined(USE_GLES2)
	SetWindowed(m_windowed);
#endif // PLAT_WIN && !USE_GLES2

	if (glContext != NULL)
	{
		glViewport(0, 0, m_width, m_height);
		GLCheckError("set viewport");
	}
}

// reports focus state
void CGLRenderLib::SetFocused(bool inFocus)
{
	if(!inFocus)
		ReleaseSurface();
}

void CGLRenderLib::ReleaseSurface()
{
#ifdef ANDROID
	MsgInfo("Detaching surface...\n");
	eglMakeCurrent(eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

	if (eglSurface != EGL_NO_SURFACE)
	{
		MsgInfo("Destroying surface...\n");
		eglDestroySurface(eglDisplay, eglSurface);
		eglSurface = EGL_NO_SURFACE;
	}

	lostSurface = true;
#endif 
}

bool CGLRenderLib::CaptureScreenshot(CImage &img)
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

void CGLRenderLib::ReleaseSwapChains()
{

}

// creates swap chain
IEqSwapChain* CGLRenderLib::CreateSwapChain(void* window, bool windowed)
{
	return NULL;
}

// destroys a swapchain
void  CGLRenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{

}

// returns default swap chain
IEqSwapChain*  CGLRenderLib::GetDefaultSwapchain()
{
	return NULL;
}


DECLARE_CMD(r_info, "Prints renderer info", 0)
{
	g_pShaderAPI->PrintAPIInfo();
}
