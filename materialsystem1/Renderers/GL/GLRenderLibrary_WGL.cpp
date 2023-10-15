//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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

#include <dwmapi.h>
#include <VersionHelpers.h>
#include "wgl_caps.hpp"

#include "GLRenderLibrary_WGL.h"

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

static MONITORINFO monInfo;

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData){
	if (*(int *) dwData == 0){
		monInfo.cbSize = sizeof(monInfo);
		GetMonitorInfo(hMonitor, &monInfo);
		return FALSE;
	}
	(*(int *) dwData)--;

	return TRUE;
}

static LRESULT CALLBACK PFWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, message, wParam, lParam);
};


HOOK_TO_CVAR(r_screen);

IShaderAPI* CGLRenderLib_WGL::GetRenderer() const
{
	return &s_renderApi;
}

bool CGLRenderLib_WGL::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

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

	return true;
}

void CGLRenderLib_WGL::DestroySharedContexts()
{
	wglDeleteContext(m_glSharedContext);
}


void CGLRenderLib_WGL::InitSharedContexts()
{
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

	m_glSharedContext = context;
}

bool CGLRenderLib_WGL::InitAPI(const ShaderAPIParams& params)
{
	int multiSamplingMode = params.multiSamplingMode;

	if (r_screen->GetInt() >= GetSystemMetrics(SM_CMONITORS))
		r_screen->SetValue("0");

	m_hwnd = (HWND)params.windowInfo.get(RenderWindowInfo::WINDOW);
	m_hdc = (HDC)params.windowInfo.get(RenderWindowInfo::DISPLAY);

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
	switch(params.screenFormat)
	{
		case FORMAT_RGBA8:
			m_devMode.dmBitsPerPel = 32;
			break;
		case FORMAT_RGB8:
			m_devMode.dmBitsPerPel = 24;
			break;
		case FORMAT_RGB565:
			m_devMode.dmBitsPerPel = 16;
			break;
		default:
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

void CGLRenderLib_WGL::ExitAPI()
{
	wglMakeCurrent(nullptr, nullptr);

	DestroySharedContexts();

	wglDeleteContext(m_glContext);

	ReleaseDC(m_hwnd, m_hdc);

	if (!m_windowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsExA((const char *) m_dispDevice.DeviceName, nullptr, nullptr, 0, nullptr);
	}
}


void CGLRenderLib_WGL::BeginFrame(IEqSwapChain* swapChain)
{
	s_renderApi.m_deviceLost = false;
	s_renderApi.StepProgressiveLodTextures();

	int width = m_width;
	int height = m_height;

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

	m_curSwapChain = swapChain;

	// ShaderAPIGL uses m_nViewportWidth/Height as backbuffer size
	s_renderApi.m_backbufferSize = IVector2D(width, height);
}

void CGLRenderLib_WGL::EndFrame()
{
	int swapInterval = s_renderApi.m_params.verticalSyncEnabled ? 1 : 0;

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
}

// changes fullscreen mode
bool CGLRenderLib_WGL::SetWindowed(bool enabled)
{
	m_windowed = enabled;

	if (!enabled)
	{
		m_devMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_DISPLAYFREQUENCY;
		m_devMode.dmPelsWidth = m_width;
		m_devMode.dmPelsHeight = m_height;

		LONG dispChangeStatus = ChangeDisplaySettingsExA((const char*)m_dispDevice.DeviceName, &m_devMode, nullptr, CDS_FULLSCREEN, nullptr);
		if (dispChangeStatus != DISP_CHANGE_SUCCESSFUL)
		{
			MsgError("ChangeDisplaySettingsEx - couldn't set fullscreen mode %dx%d on %s (%d)\n", m_width, m_height, m_dispDevice.DeviceName, dispChangeStatus);			
			return false;
		}
	}
	else
	{
		ChangeDisplaySettingsExA((const char*)m_dispDevice.DeviceName, nullptr, nullptr, 0, nullptr);
	}
	
	return true;
}

// speaks for itself
bool CGLRenderLib_WGL::IsWindowed() const
{
	return m_windowed;
}

void CGLRenderLib_WGL::SetBackbufferSize(const int w, const int h)
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
void CGLRenderLib_WGL::SetFocused(bool inFocus)
{

}

bool CGLRenderLib_WGL::CaptureScreenshot(CImage &img)
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

void CGLRenderLib_WGL::ReleaseSwapChains()
{
	for (int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

// creates swap chain
IEqSwapChain* CGLRenderLib_WGL::CreateSwapChain(void* window, bool windowed)
{
	CGLSwapChain* pNewChain = PPNew CGLSwapChain();

	if (!pNewChain->Initialize((HWND)window, s_renderApi.m_params.verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create OpenGL swapchain!\n");
		delete pNewChain;
		return nullptr;
	}

	SetPixelFormat(pNewChain->m_windowDC, m_bestPixelFormat, &m_pfd);

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void  CGLRenderLib_WGL::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

// returns default swap chain
IEqSwapChain*  CGLRenderLib_WGL::GetDefaultSwapchain()
{
	return nullptr;
}

//----------------------------------------------------------------------------------------
// OpenGL multithreaded context switching
//----------------------------------------------------------------------------------------

// prepares async operation
void CGLRenderLib_WGL::BeginAsyncOperation(uintptr_t threadId)
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	if (thisThreadId == m_mainThreadId) // not required for main thread
		return;

	ASSERT(m_asyncOperationActive == false);

	while (wglMakeCurrent(m_hdc, m_glSharedContext) == false) {}

	m_asyncOperationActive = true;
}

// completes async operation
void CGLRenderLib_WGL::EndAsyncOperation()
{
	uintptr_t thisThreadId = Threading::GetCurrentThreadID();

	ASSERT_MSG(IsMainThread(thisThreadId) == false , "EndAsyncOperation() cannot be called from main thread!");
	ASSERT(m_asyncOperationActive == true);

	while (wglMakeCurrent(nullptr, nullptr) == false) {}

	m_asyncOperationActive = false;
}

bool CGLRenderLib_WGL::IsMainThread(uintptr_t threadId) const
{
	return threadId == m_mainThreadId;
}