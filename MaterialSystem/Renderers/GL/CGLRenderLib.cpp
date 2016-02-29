//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech OpenGL ShaderAPI
//////////////////////////////////////////////////////////////////////////////////

#include "Platform.h"
#include "IDkCore.h"
#include "CGLRenderLib.h"

#include "gl_loader.h"

#if !defined(IS_OPENGL)
#error "IS_OPENGL'' Should in your project!!!"
#endif // PLAT_LINUX && !IS_OPENGL

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

// make library
CGLRenderLib g_library;

IShaderAPI* g_pShaderAPI = NULL;

CGLRenderLib::CGLRenderLib()
{
	GetCore()->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
}

CGLRenderLib::~CGLRenderLib()
{
	GetCore()->UnregisterInterface(RENDERER_INTERFACE_VERSION);
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
#if 0
	HINSTANCE modHandle = GetModuleHandle(NULL);

	//Unregister PFrmt if
	UnregisterClass("PFrmt", modHandle);

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
	wincl.lpszClassName = "PFrmt";
	wincl.lpfnWndProc = PFWinProc;
	wincl.style = 0;
	wincl.hIcon = NULL;
	wincl.hCursor = NULL;
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = NULL;

	RegisterClass(&wincl);

	HWND hPFwnd = CreateWindow("PFrmt", "PFormat", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, NULL, modHandle, NULL);
	if(hPFwnd)
	{
		HDC hdc = GetDC(hwnd);

		// Application Verifier will break here, this is AMD OpenGL driver issue and still not fixed over years.
		int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
		SetPixelFormat(hdc, nPixelFormat, &pfd);

		HGLRC hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);

		gl::exts::LoadTest didLoad = gl::sys::LoadFunctions();
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
		MsgInfo("%s\n",splExts[i].c_str());
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

#if defined(USE_GLES2) // && defined(PLAT_WIN)
bool OpenNativeDisplay(EGLNativeDisplayType* nativedisp_out)
{
	*nativedisp_out = EGL_DEFAULT_DISPLAY;
    return true;
}

void CloseNativeDisplay(EGLNativeDisplayType nativedisp)
{
}
#endif // USE_GLES2 && PLAT_WIN

bool CGLRenderLib::InitAPI( const shaderapiinitparams_t& params )
{
	savedParams = params;

#ifdef USE_GLES2

    eglSurface = EGL_NO_SURFACE;

#ifdef ANDROID
    externalWindowDisplayParams_t* winParams = (externalWindowDisplayParams_t*)params.hWindow;

    ASSERT(winParams != NULL);

    eglSurface = (EGLSurface)winParams->paramArray[0];

	hwnd = (EGLNativeWindowType)winParams->window;
#else
	// other EGL
	hwnd = (EGLNativeWindowType)params.hWindow;
#endif // ANDROID

	Msg("Initializing EGL context...\n");

    EGLBoolean bsuccess;

    // create native window
    EGLNativeDisplayType nativeDisplay;
    if(!OpenNativeDisplay(&nativeDisplay))
    {
        ErrorMsg("OpenGL ES init error: Cannot open native display!");
        return false;
    }

    // get egl display handle
    eglDisplay = eglGetDisplay(nativeDisplay);
    if(eglDisplay == EGL_NO_DISPLAY)
    {
        ErrorMsg("OpenGL ES init error: Could not get EGL display (%d)", eglDisplay);
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

    // Initialize the display
    EGLint major = 0;
    EGLint minor = 0;
    bsuccess = eglInitialize(eglDisplay, &major, &minor);
    if (!bsuccess)
    {
        ErrorMsg("OpenGL ES init error: Could not initialize EGL display!");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }
    if (major < 1)
    {
        // Does not support EGL 1.4
        ErrorMsg("OpenGL ES init error: System does not support at least EGL 3.0");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

    // Obtain the first configuration with a depth buffer
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

    EGLint numConfig =0;
    EGLConfig eglConfig = 0;
    bsuccess = eglChooseConfig(eglDisplay, attrs, &eglConfig, 1, &numConfig);
    if (!bsuccess)
    {
        ErrorMsg("OpenGL ES init error: Could not find valid EGL config");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

    // Get the native visual id
    int nativeVid;
    if (!eglGetConfigAttrib(eglDisplay, eglConfig, EGL_NATIVE_VISUAL_ID, &nativeVid))
    {
        ErrorMsg("OpenGL ES init error: Could not get native visual id");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

    // Create a surface for the main window
    if(eglSurface == EGL_NO_SURFACE)
        eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, hwnd, NULL);

    if (eglSurface == EGL_NO_SURFACE)
    {
        ErrorMsg("OpenGL ES init error: Could not create EGL surface\n");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

	eglBindAPI(EGL_OPENGL_ES_API);

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
        ErrorMsg("OpenGL ES init error: Could not create EGL context\n");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

    glContext2 = eglCreateContext(eglDisplay, eglConfig, glContext, contextAttr);
    if (glContext == EGL_NO_CONTEXT)
    {
         ErrorMsg("OpenGL ES init error: Could not create EGL context\n");
        CloseNativeDisplay(nativeDisplay);
        return false;
    }

	eglMakeCurrent(eglDisplay, eglSurface, eglSurface, glContext);

#elif defined(PLAT_WIN)
	if (r_screen->GetInt() >= GetSystemMetrics(SM_CMONITORS))
		r_screen->SetValue("0");

	hwnd = (HWND)params.hWindow;

	// Enumerate display devices
	int monitorCounter = r_screen->GetInt();
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) &monitorCounter);

	device.cb = sizeof(device);
	EnumDisplayDevices(NULL, r_screen->GetInt(), &device, 0);

	// get window parameters

	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	ZeroMemory(&dm,sizeof(dm));
	dm.dmSize = sizeof(dm);

	// Figure display format to use
	if(params.nScreenFormat == FORMAT_RGBA8)
	{
		dm.dmBitsPerPel = 32;
	}
	else if(params.nScreenFormat == FORMAT_RGB8)
	{
		dm.dmBitsPerPel = 24;
	}
	else if(params.nScreenFormat == FORMAT_RGB565)
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
	if (!params.bIsWindowed)
	{
		int dispChangeStatus = ChangeDisplaySettings(&dm, CDS_FULLSCREEN);

		if (dispChangeStatus != DISP_CHANGE_SUCCESSFUL)
		{
			MsgError("ChangeDisplaySettingsEx - couldn't set fullscreen mode %dx%d on %s (%d)\n", m_width, m_height, device.DeviceName, dispChangeStatus);

			DWORD lastErr = GetLastError();

			char err[256] = {'\0'};

			if(lastErr != 0)
			{
				FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM,
								NULL,
								lastErr,
								MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
								err,
								255,
								NULL);

				MsgError("Couldn't set fullscreen mode:\n\n%s (0x%p)", err, lastErr);
			}

		}
	}

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

	hdc = GetDC(hwnd);

	int iAttribs[] = {
		wgl::DRAW_TO_WINDOW_ARB,	GL_TRUE,
		wgl::ACCELERATION_ARB,		wgl::FULL_ACCELERATION_ARB,
		wgl::DOUBLE_BUFFER_ARB,		GL_TRUE,
		wgl::RED_BITS_ARB,			8,
		wgl::GREEN_BITS_ARB,		8,
		wgl::BLUE_BITS_ARB,			8,
		wgl::ALPHA_BITS_ARB,		(dm.dmBitsPerPel > 24) ? 8 : 0,
		wgl::DEPTH_BITS_ARB,		24,
		wgl::STENCIL_BITS_ARB,		0,
		0
	};

	int pixelFormats[256];
	int bestFormat = 0;
	int bestSamples = 0;
	uint nPFormats;

	if (wgl::exts::var_ARB_pixel_format && wgl::ChoosePixelFormatARB(hdc, iAttribs, NULL, elementsOf(pixelFormats), pixelFormats, &nPFormats) && nPFormats > 0)
	{
		int minDiff = 0x7FFFFFFF;
		int attrib = wgl::SAMPLES_ARB;
		int samples;

		// Find a multisample format as close as possible to the requested
		for (uint i = 0; i < nPFormats; i++)
		{
			wgl::GetPixelFormatAttribivARB(hdc, pixelFormats[i], 0, 1, &attrib, &samples);
			int diff = abs(params.nMultisample - samples);
			if (diff < minDiff)
			{
				minDiff = diff;
				bestFormat = i;
				bestSamples = samples;
			}
		}

		savedParams.nMultisample = bestSamples;
	}
	else
	{
		pixelFormats[0] = ChoosePixelFormat(hdc, &pfd);
	}

	SetPixelFormat(hdc, pixelFormats[bestFormat], &pfd);

	glContext = wglCreateContext(hdc);
	glContext2 = wglCreateContext(hdc);

	wglShareLists(glContext2, glContext);

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

    Window xwin = (Window)savedParams.hWindow;

    XWindowAttributes winAttrib;
    XGetWindowAttributes(display,xwin,&winAttrib);

	m_width = winAttrib.width;
	m_height = winAttrib.height;

    int colorBits = 16;
    int depthBits = 24;
    int stencilBits = 0;

	// Figure display format to use
	if(params.nScreenFormat == FORMAT_RGBA8)
	{
		colorBits = 32;
	}
	else if(params.nScreenFormat == FORMAT_RGB8)
	{
		colorBits = 24;
	}
	else if(params.nScreenFormat == FORMAT_RGB565)
	{
		colorBits = 16;
	}
	else
	{
		colorBits = 24;
	}

	XVisualInfo *vi;
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
			GLX_SAMPLE_BUFFERS, (savedParams.nMultisample > 0),
			GLX_SAMPLES,         savedParams.nMultisample,
			None,
		};

		vi = glXChooseVisual(display, m_screen, attribs);
		if (vi != NULL) break;

		savedParams.nMultisample -= 2;
		if (savedParams.nMultisample < 0){
			char str[256];
			sprintf(str, "No Visual matching colorBits=%d, depthBits=%d and stencilBits=%d", colorBits, depthBits, stencilBits);
			ErrorMsg(str);
			return false;
		}
	}

    if (!savedParams.bIsWindowed)
    {
		if (foundMode >= 0 && XF86VidModeSwitchToMode(display, m_screen, dmodes[foundMode]))
		{
			XF86VidModeSetViewPort(display, m_screen, 0, 0);
		}
		else
		{
			MsgError("Couldn't set fullscreen at %dx%d.", m_width, m_height);
			savedParams.bIsWindowed = true;
		}
	}

	glContext = glXCreateContext(display, vi, None, True);
	glContext2 = glXCreateContext(display, vi, glContext, True);

	glXMakeCurrent(display, (GLXDrawable)savedParams.hWindow, glContext);
#endif //PLAT_WIN

	Msg("Initializing GL extensions...\n");

#ifdef USE_GLES2
	// load GLES2 GL 3.0 extensions using glad
	if(!gladLoadGLES2Loader( gladHelperLoaderFunction ))
	{
		MsgError("Cannot load OpenGL ES 2.0!\n");
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
		const char *rend = (const char *) glGetString(GL_RENDERER);
		const char *vendor = (const char *) glGetString(GL_VENDOR);
		Msg("*Detected video adapter: %s by %s\n",rend,vendor);

		const char *version = (const char *) glGetString(GL_VERSION);
		Msg("*OpenGL version is: %s\n",version);
	}

	m_Renderer = new ShaderAPIGL();
	g_pShaderAPI = m_Renderer;

#ifdef USE_GLES2
	m_Renderer->m_display = this->eglDisplay;
	m_Renderer->m_eglSurface = this->eglSurface;
	m_Renderer->m_hdc = this->hdc;
#elif PLAT_WIN
	m_Renderer->m_hdc = this->hdc;
#elif PLAT_LINUX
    m_Renderer->m_display = this->display;
#endif //PLAT_WIN

	m_Renderer->m_glContext = this->glContext;
	m_Renderer->m_glContext2 = this->glContext2;

#ifndef USE_GLES2 // TEMPORARILY DISABLED
	if (GLAD_GL_ARB_multisample && params.nMultisample > 0)
		glEnable(GL_MULTISAMPLE_ARB);
#endif // USE_GLES2

	return true;
}

void CGLRenderLib::ExitAPI()
{
	m_Renderer->GL_CRITICAL();
	m_Renderer->GL_END_CRITICAL();

	delete m_Renderer;

#ifdef PLAT_WIN

#ifdef USE_GLES2
	eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(eglDisplay, glContext);
	eglDestroyContext(eglDisplay, glContext2);
	eglDestroySurface(eglDisplay, eglSurface);
	eglTerminate(eglDisplay);
#else
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(glContext);
	wglDeleteContext(glContext2);

	ReleaseDC(hwnd, hdc);

	if (!savedParams.bIsWindowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsEx((const char *) device.DeviceName, NULL, NULL, 0, NULL);
	}
#endif // USE_GLES2

#else

#ifdef PLAT_LINUX

    glXMakeCurrent(display, None, NULL);
    glXDestroyContext(display, glContext);

	if(!savedParams.bIsWindowed)
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
	m_Renderer->GL_CRITICAL();
	m_Renderer->SetViewport(0, 0, m_width, m_height);
	m_Renderer->GL_END_CRITICAL();
}

void CGLRenderLib::EndFrame(IEqSwapChain* schain)
{
	m_Renderer->GL_CRITICAL();

#ifdef USE_GLES2

	eglSwapBuffers(eglDisplay, eglSurface);

#elif PLAT_WIN
	if (wgl::exts::var_EXT_swap_control)
	{
		wgl::SwapIntervalEXT(savedParams.bEnableVerticalSync ? 1 : 0);
	}

	SwapBuffers(hdc);

#elif defined(PLAT_LINUX)

	if (glX::exts::var_EXT_swap_control)
	{
		glX::SwapIntervalEXT(display, (Window)savedParams.hWindow, savedParams.bEnableVerticalSync ? 1 : 0);
	}

	glXSwapBuffers(display, (Window)savedParams.hWindow);

#endif // PLAT_WIN

    m_Renderer->GL_END_CRITICAL();
}

void CGLRenderLib::SetBackbufferSize(const int w, const int h)
{
	if(m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;

	if(!savedParams.bIsWindowed)
	{
#if defined(PLAT_WIN) && !defined(USE_GLES2)
		dm.dmPelsWidth = m_width;
		dm.dmPelsHeight = m_height;

		if (ChangeDisplaySettingsEx((const char *) device.DeviceName, &dm, NULL, CDS_FULLSCREEN, NULL) == DISP_CHANGE_FAILED)
		{
			MsgError("Couldn't set fullscreen mode\n");
			WarningMsg("Couldn't set fullscreen mode");
		}
#endif // PLAT_WIN && !USE_GLES2
	}

	if (glContext != NULL)
	{
		m_Renderer->GL_CRITICAL();
		glViewport(0, 0, m_width, m_height);
		m_Renderer->GL_END_CRITICAL();
	}
}

bool CGLRenderLib::CaptureScreenshot(CImage &img)
{
	m_Renderer->GL_CRITICAL();

	glFinish();

	ubyte *pixels = img.Create(FORMAT_RGB8, m_width, m_height, 1, 1);
	ubyte *flipped = new ubyte[m_width * m_height * 3];

	glReadPixels(0, 0, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, flipped);
	for (int y = 0; y < m_height; y++)
		memcpy(pixels + y * m_width * 3, flipped + (m_height - y - 1) * m_width * 3, m_width * 3);

	delete [] flipped;

	m_Renderer->GL_END_CRITICAL();

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
