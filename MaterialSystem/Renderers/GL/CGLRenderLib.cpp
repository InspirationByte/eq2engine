//***********Copyright (C) Two Dark Interactive Software 2007-2009************
//
// Description: OpenGL Rendering library interface
//
//****************************************************************************

#include "Platform.h"
#include "CGLRenderLib.h"
#include "gl_caps.hpp"
#include "wgl_caps.hpp"

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

#ifdef _WIN32
void InitGLEntryPoints(HWND hwnd, const PIXELFORMATDESCRIPTOR &pfd)
{
	HDC hdc = GetDC(hwnd);

	// Application Verifier will break here, this is AMD OpenGL driver issue and still not fixed over years.
	int nPixelFormat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, nPixelFormat, &pfd);

	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);

	gl::exts::LoadTest didLoad = gl::sys::LoadFunctions();
	if(!didLoad)
		MsgError("OpenGL: %i\n", didLoad.GetNumMissing());

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK PFWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hwnd, message, wParam, lParam);
};
#endif // _WIN32

bool CGLRenderLib::InitCaps()
{
#ifdef _WIN32

	m_renderInstance = GetModuleHandle(NULL);

	//Unregister PFrmt if
	UnregisterClass("PFrmt", m_renderInstance);

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
	wincl.hInstance = m_renderInstance;
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

	HWND hPFwnd = CreateWindow("PFrmt", "PFormat", WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 8, 8, HWND_DESKTOP, NULL, m_renderInstance, NULL);
	if(hPFwnd)
	{
		InitGLEntryPoints(hPFwnd, pfd);

		SendMessage(hPFwnd, WM_CLOSE, 0, 0);
	}
	else
	{
		MsgError("Renderer fault!!!\n");
		return false;
	}
#endif // _WIN32
	return true;
}

#ifdef _WIN32
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
#endif // _WIN32

DECLARE_CMD(gl_extensions,"Print supported OpenGL extensions",0)
{
	const char *ver = (const char *) gl::GetString(gl::VERSION);
	Msg("OpenGL version: %s\n \n",ver);
	const char *exts = (const char *) gl::GetString(gl::EXTENSIONS);

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

bool CGLRenderLib::InitAPI( const shaderapiinitparams_t& params )
{
	savedParams = params;

	g_localizer->AddTokensFile("api_gl");

#ifdef _WIN32
	if (r_screen->GetInt() >= GetSystemMetrics(SM_CMONITORS))
		r_screen->SetValue("0");

	hwnd = (HWND)params.hWindow;

	int monitorCounter = r_screen->GetInt();
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM) &monitorCounter);

	device.cb = sizeof(device);
	EnumDisplayDevices(NULL, r_screen->GetInt(), &device, 0);



	// get window parameters

	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;
#else
	// TODO: SDL window handle or X11
#endif // _WIN32

#ifdef _WIN32
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

	PIXELFORMATDESCRIPTOR pfd = {
        sizeof (PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA, dm.dmBitsPerPel,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		24, 0,
		0, PFD_MAIN_PLANE, 0, 0, 0, 0
    };

	hdc = GetDC(hwnd);

	int iAttribs[] = {
		wgl::DRAW_TO_WINDOW_ARB, gl::TRUE_,
		wgl::ACCELERATION_ARB,  wgl::FULL_ACCELERATION_ARB,
		wgl::DOUBLE_BUFFER_ARB,  gl::TRUE_,
		wgl::RED_BITS_ARB,       8,
		wgl::GREEN_BITS_ARB,     8,
		wgl::BLUE_BITS_ARB,      8,
		wgl::ALPHA_BITS_ARB,     (dm.dmBitsPerPel > 24)? 8 : 0,
		wgl::DEPTH_BITS_ARB,     24,
		wgl::STENCIL_BITS_ARB,   0,
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
	}
	else
	{
		pixelFormats[0] = ChoosePixelFormat(hdc, &pfd);
	}
	savedParams.nMultisample = bestSamples;

	SetPixelFormat(hdc, pixelFormats[bestFormat], &pfd);

	glContext = wglCreateContext(hdc);
	glContext2 = wglCreateContext(hdc);

	wglShareLists(glContext2, glContext);

	wglMakeCurrent(hdc, glContext);

	{
		const char *rend = (const char *) gl::GetString(gl::RENDERER);
		const char *vendor = (const char *) gl::GetString(gl::VENDOR);
		Msg("*Detected video adapter: %s by %s\n",rend,vendor);

		const char *version = (const char *) gl::GetString(gl::VERSION);
		Msg("*OpenGL version is: %s\n",version);
	}

	if(gl::sys::GetMajorVersion() < 2)
	{
		WarningMsg("Cannot initialize OpenGL due driver is only supports version 1.x.x\n\nPlease update your video drivers!");
		exit(-5);
	}
#else

#endif //_WIN32

	m_Renderer = new ShaderAPIGL();
	g_pShaderAPI = m_Renderer;

#ifdef _WIN32
	m_Renderer->m_hdc = this->hdc;
#endif //_WIN32
	m_Renderer->m_glContext = this->glContext;
	m_Renderer->m_glContext2 = this->glContext2;

	m_Renderer->Init(savedParams);

	m_Renderer->SetViewport(0, 0, m_width, m_height);

	BeginFrame();
	m_Renderer->Clear(true,true,true,ColorRGBA(0.5,0.5,0.5, 0.0f));
	EndFrame();

	if (gl::exts::var_ARB_multisample && params.nMultisample > 0)
	{
		gl::Enable(gl::MULTISAMPLE_ARB);
	}

	return true;
}

void CGLRenderLib::ExitAPI()
{
	//m_Renderer->GL_CRITICAL();

	delete m_Renderer;

#ifdef _WIN32

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(glContext);
	wglDeleteContext(glContext2);
	ReleaseDC(hwnd, hdc);

	if (!savedParams.bIsWindowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsEx((const char *) device.DeviceName, NULL, NULL, 0, NULL);
	}
#else

#endif // _WIN32
}
	/*
void CGLRenderLib::SetActive(bool bActive)
{
	m_bActive = bActive;



	if(m_bActive)
	{
		if (rs_fullscreen->GetBool())
		{
			glViewport(0, 0, dm.dmPelsWidth, dm.dmPelsHeight);

			if (ChangeDisplaySettingsEx((const char *) device.DeviceName, &dm, NULL, CDS_FULLSCREEN, NULL) == DISP_CHANGE_FAILED)
			{
				MsgError("Couldn't set fullscreen mode\n");
				ErrorMsg("Couldn't set fullscreen mode");
				rs_fullscreen->SetValue("0");
			}
		}
	}
	else
	{
		ChangeDisplaySettingsEx((const char *) device.DeviceName, NULL, NULL, 0, NULL);
	}



}*/


void CGLRenderLib::BeginFrame()
{
	//glColor3f(1, 1, 1);

	m_Renderer->SetViewport(0, 0, m_width, m_height);
}

void CGLRenderLib::EndFrame(IEqSwapChain* schain)
{
	m_Renderer->GL_CRITICAL();

#ifdef _WIN32
	if (wgl::exts::var_EXT_swap_control)
	{
		wgl::SwapIntervalEXT(savedParams.bEnableVerticalSync ? 1 : 0);
	}

	m_Renderer->GL_END_CRITICAL();

	SwapBuffers(hdc);

#ifdef DEBUG
	CheckForErrors();
#endif
#else

#endif // _WIN32
}

/*
bool CGLRenderLib::CheckForErrors()
{
	int error = glGetError();
	if (error != GL_NO_ERROR)
	{
		if (error == GL_INVALID_ENUM)
			Msg("OpenGL Renderer error: GL_INVALID_ENUM");
		else if (error == GL_INVALID_VALUE)
			Msg("OpenGL Renderer error: GL_INVALID_VALUE");
		else if (error == GL_INVALID_OPERATION)
			Msg("OpenGL Renderer error: GL_INVALID_OPERATION");
		else if (error == GL_STACK_OVERFLOW)
			Msg("OpenGL Renderer error: GL_STACK_OVERFLOW");
		else if (error == GL_STACK_UNDERFLOW)
			Msg("OpenGL Renderer error: GL_STACK_UNDERFLOW");
		else if (error == GL_OUT_OF_MEMORY)
			Msg("OpenGL Renderer error: GL_OUT_OF_MEMORY");
		else if (error == GL_INVALID_FRAMEBUFFER_OPERATION_EXT)
			Msg("OpenGL Renderer error: GL_INVALID_FRAMEBUFFER_OPERATION_EXT");
		else
			Msg("OpenGL Renderer error: Unrecognized OpenGL error");

		return true;
	}

	return false;
}*/

void CGLRenderLib::SetBackbufferSize(const int w, const int h)
{
	m_width = w;
	m_height = h;

	if(!savedParams.bIsWindowed)
	{
#ifdef _WIN32
		dm.dmPelsWidth = m_width;
		dm.dmPelsHeight = m_height;

		if (ChangeDisplaySettingsEx((const char *) device.DeviceName, &dm, NULL, CDS_FULLSCREEN, NULL) == DISP_CHANGE_FAILED)
		{
			MsgError("Couldn't set fullscreen mode\n");
			WarningMsg("Couldn't set fullscreen mode");
		}
#endif // _WIN32
	}

	//Msg("Resize to: %i,%i\n",w,h);
	if (glContext != NULL)
	{
		m_Renderer->GL_CRITICAL();
		gl::Viewport(0, 0, w, h);
		m_Renderer->GL_END_CRITICAL();
	}
}

bool CGLRenderLib::CaptureScreenshot(CImage &img)
{
	m_Renderer->GL_CRITICAL();

	ubyte *pixels = img.Create(FORMAT_RGB8, m_width, m_height, 1, 1);
	ubyte *flipped = new ubyte[m_width * m_height * 3];

	gl::ReadPixels(0, 0, m_width, m_height, gl::RGB, gl::UNSIGNED_BYTE, flipped);
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
