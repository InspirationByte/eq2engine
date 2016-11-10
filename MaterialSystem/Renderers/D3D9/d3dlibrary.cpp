//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "IDkCore.h"
#include "IConCommandFactory.h"
#include "D3DLibrary.h"
#include "ShaderAPID3DX9.h"
#include "d3dx9_def.h"

#include "D3D9SwapChain.h"

#include "Imaging/ImageLoader.h"


HOOK_TO_CVAR(r_screen);

//IRenderer* renderer = NULL;
/*
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData){
	if (*(int *) dwData == 0){
		monInfo.cbSize = sizeof(monInfo);
		GetMonitorInfo(hMonitor, &monInfo);
		return FALSE;
	}
	(*(int *) dwData)--;

	return TRUE;
}
*/

// make library
CD3DRenderLib g_library;

IShaderAPI* g_pShaderAPI = NULL;

CD3DRenderLib::CD3DRenderLib()
{
	GetCore()->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
}

CD3DRenderLib::~CD3DRenderLib()
{
	GetCore()->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

bool CD3DRenderLib::InitCaps()
{
	m_Renderer = NULL;
#ifdef USE_D3DEX
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION,&d3d)))
#else
	if ((m_d3dFactory = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
#endif
	{
		const char* driver_upd = "Couldn't initialize Direct3D\nMake sure you have DirectX 9.0c or later version installed.";
		ErrorMsg(driver_upd);
		return false;
	}

	m_d3dFactory->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dCaps);

	return true;
}

DECLARE_CMD(r_info, "Prints renderer info", 0)
{
	g_pShaderAPI->PrintAPIInfo();
}

DWORD ComputeDeviceFlags( const D3DCAPS9& caps, bool bSoftwareVertexProcessing )
{
	// Find out what type of device to make
	bool bPureDeviceSupported = (caps.DevCaps & D3DDEVCAPS_PUREDEVICE) != 0;

	DWORD nDeviceCreationFlags = 0;

	if ( !bSoftwareVertexProcessing )
	{
		nDeviceCreationFlags = D3DCREATE_HARDWARE_VERTEXPROCESSING;

		if ( bPureDeviceSupported )
			nDeviceCreationFlags |= D3DCREATE_PUREDEVICE;
	}
	else
		nDeviceCreationFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	nDeviceCreationFlags |= D3DCREATE_FPU_PRESERVE;
	nDeviceCreationFlags |= D3DCREATE_MULTITHREADED;

	return nDeviceCreationFlags;
}

bool CD3DRenderLib::InitAPI( shaderapiinitparams_t &params )
{
	// get device information
	D3DADAPTER_IDENTIFIER9 dai;
	if (!FAILED(m_d3dFactory->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &dai)))
	{
		Msg(" \n*Detected video adapter: %s\n", dai.Description, dai.Driver);
	}

	// clear present parameters
	memset(&m_d3dpp, 0, sizeof(m_d3dpp));

	// set window
	hwnd = (HWND)params.hWindow;

	// get window parameters
	RECT windowRect;
	GetClientRect(hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	m_d3dpp.Windowed = params.bIsWindowed;

	// set backbuffer bits
	int depthBits = params.nDepthBits;
	int stencilBits = 0;

	// setup backbuffer format
	m_d3dpp.BackBufferFormat = formats[ params.nScreenFormat ];

	// Find a suitable fullscreen format
	//int fullScreenRefresh = 60;
	//int targetHz = params.nRefreshRateHZ;

	int nModes = m_d3dFactory->GetAdapterModeCount(D3DADAPTER_DEFAULT, m_d3dpp.BackBufferFormat);

	m_d3dpp.BackBufferWidth  = m_width;
	m_d3dpp.BackBufferHeight = m_height;
	m_d3dpp.BackBufferCount  = 1;

	DevMsg(DEVMSG_SHADERAPI, "Initial backbuffer size: %d %d\n", m_width, m_height);

	if(params.bEnableVerticalSync)
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	else
		m_d3dpp.PresentationInterval	= D3DPRESENT_INTERVAL_IMMEDIATE;

	// get
	D3DDISPLAYMODE d3ddm;
	HRESULT hr = m_d3dFactory->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm);
	if (FAILED(hr))
	{
		ErrorMsg("D3D error: Could not get Adapter Display mode.");
		return false;
	}

	// setup windowed parameters
	if(params.bIsWindowed)
	{
		m_d3dpp.BackBufferFormat	= d3ddm.Format;
		m_d3dpp.SwapEffect			= params.nMultisample > 0 ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	}
	else
		m_d3dpp.SwapEffect			= D3DSWAPEFFECT_DISCARD;

	m_d3dpp.hDeviceWindow	= hwnd;

	m_d3dpp.MultiSampleQuality = 0;
	m_d3dpp.EnableAutoDepthStencil = (depthBits > 0);
	m_d3dpp.AutoDepthStencilFormat = (depthBits > 16)? ((stencilBits > 0)? D3DFMT_D24S8 : D3DFMT_D24X8) : D3DFMT_D16;

#ifdef USE_D3DEX
	mode.Size = sizeof(D3DDISPLAYMODEEX);
	mode.Width = width;
	mode.Height = height;
	mode.RefreshRate = 60;
	mode.Format = d3dpp.BackBufferFormat;
	mode.ScanLineOrdering = D3DSCANLINEORDERING_INTERLACED;
#endif

	D3DDEVTYPE devtype = D3DDEVTYPE_HAL;
	bool bSoftwareVertexProcessing = false;

	int debugres = g_cmdLine->FindArgument("-debugd3d");
	if(debugres != -1)
	{
		devtype = D3DDEVTYPE_REF;
		bSoftwareVertexProcessing = true;
	}

	// compute device dlags
	DWORD deviceFlags = ComputeDeviceFlags(m_d3dCaps, bSoftwareVertexProcessing);

	// try to create device and check the multisample types
	int multiSample = params.nMultisample;

	while (true)
	{
		m_d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE) multiSample;

#ifdef USE_D3DEX
		uint result = m_d3dFactory->CreateDeviceEx(r_screen->GetInt(), devtype, hwnd, deviceFlags, &m_d3dpp, &m_d3dMode, &m_rhi);
#else
		uint result = m_d3dFactory->CreateDevice(r_screen->GetInt(), devtype, hwnd, deviceFlags, &m_d3dpp, &m_rhi);

		// get d3d mode
		m_d3dFactory->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_d3dMode);
#endif

		if (result == D3D_OK)
		{
			
			m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
			break;
		} 
		else 
		{
			if (multiSample > 0)
			{
				
				multiSample -= 2;
			} 
			else 
			{
				MessageBox(hwnd, "Couldn't create Direct3D9 device interface!\n\nCheck your system configuration and/or install latest video drivers!", "Error", MB_OK | MB_ICONWARNING);

				return false;
			}
		}
	}

	if(params.nMultisample != multiSample)
		MsgWarning("MSAA fallback from %d to %d\n", params.nMultisample, multiSample);

	params.nMultisample = multiSample;

	m_Renderer = new ShaderAPID3DX9();
	m_Renderer->SetD3DDevice(m_rhi, m_d3dCaps);

	g_pShaderAPI = m_Renderer;

	UpdateWindow(hwnd);

	return true;
}

void CD3DRenderLib::ExitAPI()
{
	if (!m_Renderer->m_params->bIsWindowed)
	{
		// Reset display mode to default
		ChangeDisplaySettingsEx(m_dispDev.DeviceName, NULL, NULL, 0, NULL);
	}

	delete m_Renderer;

    if (m_rhi != NULL)
	{
		ULONG numUnreleased = m_rhi->Release();

		MsgWarning("D3D warning: unreleased objects: %d\n", numUnreleased);

		m_rhi = NULL;
	}

    if (m_d3dFactory != NULL)
	{
		m_d3dFactory->Release();
		m_d3dFactory = NULL;
	}

	DestroyWindow(hwnd);
}

void CD3DRenderLib::BeginFrame()
{
	if (!m_Renderer->IsDeviceActive())
	{
		if(!m_bResized)
		{
			HRESULT hr;

			if(FAILED(hr = m_rhi->TestCooperativeLevel()))
			{
				if (hr == D3DERR_DEVICELOST)
					return;

				if (hr == D3DERR_DEVICENOTRESET)
					m_Renderer->ResetDevice(m_d3dpp);

				return;
			}
		}
		else
		{
			m_Renderer->ResetDevice(m_d3dpp);
			m_bResized = false;
		}
	}

	m_rhi->BeginScene();
}

void CD3DRenderLib::EndFrame(IEqSwapChain* swapChain)
{
	m_rhi->EndScene();

	HRESULT hr;

	HWND pHWND = hwnd;

	if(swapChain != NULL)
		pHWND = (HWND)swapChain->GetWindow();

	if(!m_Renderer->m_params->bIsWindowed)
	{
		// fullscreen present
		hr = m_rhi->Present(NULL, NULL, pHWND, NULL);
		m_Renderer->CheckDeviceResetOrLost(hr);
		return;
	}

	RECT destRect;
	GetClientRect( pHWND, &destRect );

	int x,y,w,h;

	m_Renderer->GetViewport(x,y,w,h);

	RECT srcRect;
	srcRect.left = x;
	srcRect.right = x + w;
	srcRect.top = y;
	srcRect.bottom = y + h;

	hr = m_rhi->Present(&srcRect, &destRect, pHWND, NULL);

	m_Renderer->CheckDeviceResetOrLost(hr);
}

void CD3DRenderLib::SetBackbufferSize(const int w, const int h)
{
	if (m_rhi != NULL && (m_width != w || m_height != h))
	{
		m_d3dpp.BackBufferWidth  = w;
		m_d3dpp.BackBufferHeight = h;

		m_bResized = true;

		((ShaderAPID3DX9 *) m_Renderer)->m_bDeviceIsLost = true;

		m_width = w;
		m_height = h;
	}
}

bool CD3DRenderLib::CaptureScreenshot(CImage& img)
{
	/*
	if(m_Renderer->m_params->bIsWindowed)
	{
		MsgWarning("Go to fullscreen mode to capture screenshots\n");
		return false;
	}*/

	POINT topLeft = { 0, 0 };
	ClientToScreen(hwnd, &topLeft);

	bool result = false;

	LPDIRECT3DSURFACE9 surface;
	if (m_rhi->CreateOffscreenPlainSurface(m_d3dMode.Width, m_d3dMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, NULL) == D3D_OK)
	{
		if (m_rhi->GetFrontBufferData(0, surface) == D3D_OK)
		{
			D3DLOCKED_RECT lockedRect;

			if (surface->LockRect(&lockedRect, NULL, D3DLOCK_READONLY) == D3D_OK)
			{
				ubyte* dst = img.Create(FORMAT_RGB8, m_width, m_height, 1, 1);

				for (int y = 0; y < m_height; y++)
				{
					ubyte *src = ((ubyte *) lockedRect.pBits) + 4 * ((topLeft.y + y) * m_d3dMode.Width + topLeft.x);

					for (int x = 0; x < m_width; x++)
					{
						dst[0] = src[2];
						dst[1] = src[1];
						dst[2] = src[0];
						dst += 3;
						src += 4;
					}
				}

				surface->UnlockRect();
				result = true;
			}
		}

		surface->Release();
	}

	return result;
}

// creates swap chain
IEqSwapChain* CD3DRenderLib::CreateSwapChain(void* window, bool windowed)
{
	CD3D9SwapChain* pNewChain = new CD3D9SwapChain();
	
	if(!pNewChain->Initialize((HWND)window, m_Renderer->m_params->bEnableVerticalSync, windowed))
	{
		MsgError("ERROR: Can't create D3D9 swapchain!\n");
		delete pNewChain;
		return NULL;
	}

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// returns default swap chain
IEqSwapChain* CD3DRenderLib::GetDefaultSwapchain()
{
	return NULL;
}

// destroys a swapchain
void CD3DRenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}

void CD3DRenderLib::ReleaseSwapChains()
{
	for(int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}