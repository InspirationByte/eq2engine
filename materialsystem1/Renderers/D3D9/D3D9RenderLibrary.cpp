//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "imaging/ImageLoader.h"
#include "shaderapid3d9_def.h"
#include "D3D9RenderLibrary.h"

#include "ShaderAPID3D9.h"
#include "D3D9SwapChain.h"

HOOK_TO_CVAR(r_screen);

// make library
ShaderAPID3DX9 s_shaderApi;
IShaderAPI* g_pShaderAPI = &s_shaderApi;
CD3DRenderLib g_library;

CD3DRenderLib::CD3DRenderLib()
{
	g_eqCore->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
}

CD3DRenderLib::~CD3DRenderLib()
{
	g_eqCore->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

IShaderAPI* CD3DRenderLib::GetRenderer() const 
{ 
	return &s_shaderApi;
}

bool CD3DRenderLib::InitCaps()
{
#ifdef USE_D3DEX
	if (FAILED(Direct3DCreate9Ex(D3D_SDK_VERSION,&d3d)))
#else
	if ((m_d3dFactory = Direct3DCreate9(D3D_SDK_VERSION)) == nullptr)
#endif
	{
		const char* driver_upd = "Couldn't initialize Direct3D\nMake sure you have DirectX 9.0c or later version installed.";
		ErrorMsg(driver_upd);
		return false;
	}

	m_d3dFactory->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &m_d3dCaps);

	return true;
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

bool CD3DRenderLib::InitAPI( const shaderAPIParams_t &params )
{
	int multiSamplingMode = params.multiSamplingMode;

	EGraphicsVendor vendor;
	
	// get device information
	D3DADAPTER_IDENTIFIER9 dai;
	if (!FAILED(m_d3dFactory->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &dai)))
	{
		Msg(" \n*Detected video adapter: %s\n", dai.Description, dai.Driver);
		
		if (xstristr(dai.Description, "nvidia"))
			vendor = VENDOR_NV;
		else if (xstristr(dai.Description, "ati") || xstristr(dai.Description, "amd") || xstristr(dai.Description, "radeon"))
			vendor = VENDOR_ATI;
		else if (xstristr(dai.Description, "intel"))
			vendor = VENDOR_INTEL;
		else
			vendor = VENDOR_OTHER;
	}

	// clear present parameters
	memset(&m_d3dpp, 0, sizeof(m_d3dpp));

	// set window
	m_hwnd = (HWND)params.windowHandle;

	// get window parameters
	RECT windowRect;
	GetClientRect(m_hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	// always initialize in windowed mode
	m_d3dpp.Windowed = true;

	// set backbuffer bits
	int depthBits = params.depthBits;
	int stencilBits = 1;

	// setup backbuffer format
	m_d3dpp.BackBufferFormat = formats[ params.screenFormat ];

	// Find a suitable fullscreen format
	//int fullScreenRefresh = 60;
	//int targetHz = params.screenRefreshRateHZ;

	m_d3dpp.BackBufferWidth = m_width;
	m_d3dpp.BackBufferHeight = m_height;
	m_d3dpp.BackBufferCount  = 1;

	DevMsg(DEVMSG_SHADERAPI, "Initial backbuffer size: %d %d\n", m_width, m_height);

	if(params.verticalSyncEnabled)
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	else
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	// get
	HRESULT hr = m_d3dFactory->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &m_d3dMode);
	if (FAILED(hr))
	{
		ErrorMsg("D3D error: Could not get Adapter Display mode.");
		return false;
	}

	m_d3dpp.hDeviceWindow	= m_hwnd;

	m_d3dpp.MultiSampleQuality = 0;
	m_d3dpp.EnableAutoDepthStencil = TRUE;// (depthBits > 0);
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
	int multiSample = params.multiSamplingMode;

	while (true)
	{
		m_d3dpp.MultiSampleType = (D3DMULTISAMPLE_TYPE) multiSample;

		SetupSwapEffect(params);
	
#ifdef USE_D3DEX
		uint result = m_d3dFactory->CreateDeviceEx(r_screen->GetInt(), devtype, hwnd, deviceFlags, &m_d3dpp, &m_d3dMode, &m_rhi);
#else
		uint result = m_d3dFactory->CreateDevice(r_screen->GetInt(), devtype, m_hwnd, deviceFlags, &m_d3dpp, &m_rhi);
#endif

		if (result == D3D_OK)
		{
			break;
		}
		
		if (multiSample > 0)
		{
			multiSample -= 2;
		}
		else
		{
			MessageBoxA(m_hwnd, "Couldn't create Direct3D9 device interface!\n\nCheck your system configuration and/or install latest video drivers!", "Error", MB_OK | MB_ICONWARNING);

			return false;
		}
	}

	DevMsg(DEVMSG_SHADERAPI, "[DEBUG] D3D9 device created successfully...\n");

	if(multiSamplingMode != multiSample)
		MsgWarning("MSAA fallback from %d to %d\n", params.multiSamplingMode, multiSample);

	multiSamplingMode = multiSample;

	s_shaderApi.SetD3DDevice(m_rhi, m_d3dCaps);
	s_shaderApi.m_vendor = vendor;

	//-------------------------------------------
	// init caps
	//-------------------------------------------
	DevMsg(DEVMSG_SHADERAPI, "[DEBUG] D3D9 Device capabilities...\n");
	ShaderAPICaps_t& caps = s_shaderApi.m_caps;

	memset(&caps, 0, sizeof(caps));

	caps.isHardwareOcclusionQuerySupported = m_d3dCaps.PixelShaderVersion >= D3DPS_VERSION(2, 0);
	caps.isInstancingSupported = m_d3dCaps.VertexShaderVersion >= D3DVS_VERSION(2, 0);

	caps.maxTextureAnisotropicLevel = m_d3dCaps.MaxAnisotropy;
	caps.maxTextureSize = min(m_d3dCaps.MaxTextureWidth, m_d3dCaps.MaxTextureHeight);
	caps.maxRenderTargets = m_d3dCaps.NumSimultaneousRTs;
	caps.maxVertexGenericAttributes = MAX_GENERIC_ATTRIB;
	caps.maxVertexTexcoordAttributes = MAX_TEXCOORD_ATTRIB;
	caps.maxVertexStreams = MAX_VERTEXSTREAM;
	caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED;

	if (m_d3dCaps.PixelShaderVersion >= D3DPS_VERSION(1, 0))
	{
		caps.maxTextureUnits = 4;

		if (m_d3dCaps.PixelShaderVersion >= D3DPS_VERSION(1, 4))
			caps.maxTextureUnits = 6;

		if (m_d3dCaps.PixelShaderVersion >= D3DPS_VERSION(2, 0))
			caps.maxTextureUnits = 16;
	}
	else
	{
		caps.maxTextureUnits = m_d3dCaps.MaxSimultaneousTextures;
	}

	HOOK_TO_CVAR(r_anisotropic);
	caps.maxTextureAnisotropicLevel = clamp(r_anisotropic->GetInt(), 1, caps.maxTextureAnisotropicLevel);

	// get texture caps
	for(int i = 0; i < FORMAT_COUNT; i++)
	{
		caps.textureFormatsSupported[i] = (m_d3dFactory->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dMode.Format, 0, D3DRTYPE_TEXTURE, formats[i]) == D3D_OK);
		caps.renderTargetFormatsSupported[i] = (m_d3dFactory->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, formats[i]) == D3D_OK);
	
		DevMsg(DEVMSG_SHADERAPI, "[DEBUG] texture format %d: %s %s\n", i, caps.textureFormatsSupported[i] ? "tex" : "", caps.renderTargetFormatsSupported[i] ? "rt" : "");
	}

	// Determine if INTZ is supported
	hr = m_d3dFactory->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE,FOURCC_INTZ);
	caps.INTZSupported = (hr == D3D_OK);

	// Determine if NULL is supported
	hr = m_d3dFactory->CheckDeviceFormat(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_d3dMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, FOURCC_NULL);
	caps.NULLSupported = (hr == D3D_OK);

	// allocate formats on caps and format Map as well
	caps.INTZFormat = FORMAT_R32UI;
	caps.NULLFormat = FORMAT_NONE;		// at actual NONE slot

	formats[caps.INTZFormat] = FOURCC_INTZ;
	formats[caps.NULLFormat] = FOURCC_NULL;

	return true;
}

void CD3DRenderLib::ExitAPI()
{
	if (!IsWindowed())
	{
		// Reset display mode to default
		ChangeDisplaySettingsEx(m_dispDev.DeviceName, nullptr, nullptr, 0, nullptr);
	}

    if (m_rhi != nullptr)
	{
		ULONG numUnreleased = m_rhi->Release();

		if(numUnreleased)
			MsgWarning("D3D warning: unreleased objects: %d\n", numUnreleased);

		m_rhi = nullptr;
	}

    if (m_d3dFactory != nullptr)
	{
		m_d3dFactory->Release();
		m_d3dFactory = nullptr;
	}

	//DestroyWindow(hwnd);
}

void CD3DRenderLib::CheckResetDevice()
{
	if (s_shaderApi.IsDeviceActive())
		return;

	if (!m_bResized)
	{
		HRESULT hr;

		if (FAILED(hr = m_rhi->TestCooperativeLevel()))
		{
			if (hr == D3DERR_DEVICELOST)
				return;

			if (hr == D3DERR_DEVICENOTRESET)
				s_shaderApi.ResetDevice(m_d3dpp);

			return;
		}
	}
	else
	{
		s_shaderApi.ResetDevice(m_d3dpp);
		m_bResized = false;
	}
}

void CD3DRenderLib::BeginFrame()
{
	CheckResetDevice();

	m_rhi->BeginScene();
}

void CD3DRenderLib::EndFrame(IEqSwapChain* swapChain)
{
	m_rhi->EndScene();

	HRESULT hr;

	HWND pHWND = m_hwnd;

	if(swapChain != nullptr)
		pHWND = (HWND)swapChain->GetWindow();

	if(!IsWindowed())
	{
		// fullscreen present
		hr = m_rhi->Present(nullptr, nullptr, pHWND, nullptr);
		s_shaderApi.CheckDeviceResetOrLost(hr);
		return;
	}

	RECT destRect;
	GetClientRect( pHWND, &destRect );

	int x,y,w,h;

	s_shaderApi.GetViewport(x,y,w,h);

	RECT srcRect;
	srcRect.left = x;
	srcRect.right = x + w;
	srcRect.top = y;
	srcRect.bottom = y + h;

	hr = m_rhi->Present(&srcRect, &destRect, pHWND, nullptr);

	s_shaderApi.CheckDeviceResetOrLost(hr);
}

void CD3DRenderLib::SetBackbufferSize(const int w, const int h)
{
	if (m_rhi == nullptr || m_width == w && m_height == h)
		return;

	m_width = w;
	m_height = h;

	m_d3dpp.BackBufferWidth = w;
	m_d3dpp.BackBufferHeight = h;
	m_d3dpp.EnableAutoDepthStencil = TRUE;

	SetupSwapEffect(s_shaderApi.m_params);
	
	m_bResized = true;
	s_shaderApi.m_bDeviceIsLost = true;
}

// reports focus state
void CD3DRenderLib::SetFocused(bool inFocus)
{

}

void CD3DRenderLib::SetupSwapEffect(const shaderAPIParams_t& params)
{
	if (m_d3dpp.Windowed)
	{
		m_d3dpp.BackBufferFormat = m_d3dMode.Format;
		m_d3dpp.SwapEffect = (m_d3dpp.MultiSampleType > D3DMULTISAMPLE_NONE) ? D3DSWAPEFFECT_DISCARD : D3DSWAPEFFECT_COPY;
	}
	else
	{
		m_d3dpp.BackBufferFormat = formats[params.screenFormat];
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	}
}


// changes fullscreen mode
bool CD3DRenderLib::SetWindowed(bool enabled)
{
	bool old = m_d3dpp.Windowed;

	m_d3dpp.BackBufferWidth = m_width;
	m_d3dpp.BackBufferHeight = m_height;
	m_d3dpp.BackBufferCount = 1;

	m_d3dpp.Windowed = enabled;

	SetupSwapEffect(s_shaderApi.m_params);
	
	m_bResized = false;

	if (!s_shaderApi.ResetDevice(m_d3dpp))
	{
		MsgError("SetWindowed(%d) - failed to reset device!\n", enabled);
		m_d3dpp.Windowed = old;
		return false;
	}

	return true;
}

// speaks for itself
bool CD3DRenderLib::IsWindowed() const
{
	return m_d3dpp.Windowed;
}

bool CD3DRenderLib::CaptureScreenshot(CImage& img)
{
	/*
	if(m_Renderer->m_params->windowedMode)
	{
		MsgWarning("Go to fullscreen mode to capture screenshots\n");
		return false;
	}*/

	POINT topLeft = { 0, 0 };
	ClientToScreen(m_hwnd, &topLeft);

	bool result = false;

	LPDIRECT3DSURFACE9 surface;
	if (m_rhi->CreateOffscreenPlainSurface(m_d3dMode.Width, m_d3dMode.Height, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &surface, nullptr) == D3D_OK)
	{
		if (m_rhi->GetFrontBufferData(0, surface) == D3D_OK)
		{
			D3DLOCKED_RECT lockedRect;

			if (surface->LockRect(&lockedRect, nullptr, D3DLOCK_READONLY) == D3D_OK)
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
	CD3D9SwapChain* pNewChain = PPNew CD3D9SwapChain();
	
	if(!pNewChain->Initialize((HWND)window, s_shaderApi.m_params.verticalSyncEnabled, windowed))
	{
		MsgError("ERROR: Can't create D3D9 swapchain!\n");
		delete pNewChain;
		return nullptr;
	}

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// returns default swap chain
IEqSwapChain* CD3DRenderLib::GetDefaultSwapchain()
{
	return nullptr;
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