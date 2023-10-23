//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"
#include "core/IDkCore.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"

#include "D3D11Library.h"
#include "ShaderAPID3D11.h"
#include "D3D11SwapChain.h"
#include "d3dx11_def.h"
#include "Imaging/ImageLoader.h"

HOOK_TO_CVAR(r_screen);

// make library
ShaderAPID3DX10 s_renderApi;
IShaderAPI* g_renderAPI = &s_renderApi;

CD3D11RenderLib::CD3D11RenderLib()
{
}

CD3D11RenderLib::~CD3D11RenderLib()
{
}

bool CD3D11RenderLib::InitCaps()
{
	// D3D10 capabilities are strictly defined and vendors support them all.

	return true;
}

bool CD3D11RenderLib::InitAPI(const ShaderAPIParams& params)
{
	// set window
	m_hwnd = (HWND)params.windowInfo.get(RenderWindowInfo::WINDOW);
	m_verticalSyncEnabled = params.verticalSyncEnabled;

	// get window parameters
	RECT windowRect;
	GetClientRect(m_hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	ETextureFormat screenFormat = params.screenFormat;
	if (screenFormat == FORMAT_RGB8)
		screenFormat = FORMAT_RGBA8;
	m_backBufferFormat = formats[screenFormat];

	switch (params.depthBits)
	{
		case 16:
			m_depthBufferFormat = DXGI_FORMAT_D16_UNORM;
			break;
		case 24:
			m_depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
			break;
		case 32:
			m_depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
			break;
		default:
			m_depthBufferFormat = DXGI_FORMAT_UNKNOWN;
			break;
	}

	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &m_dxgiFactory)))
	{
		CrashMsg("Couldn't create DXGIFactory");
		return false;
	}

	IDXGIAdapter* dxgiAdapter;
	if (m_dxgiFactory->EnumAdapters(0, &dxgiAdapter) == DXGI_ERROR_NOT_FOUND)
	{
		CrashMsg("No adapters found");
		return false;
	}

	DXGI_ADAPTER_DESC adapterDesc;
	dxgiAdapter->GetDesc(&adapterDesc);

	char pszAdapterName[128];
	wcstombs(pszAdapterName, adapterDesc.Description, 128);
	Msg("\n*Detected Video adapter: %s\n", pszAdapterName);

	IDXGIOutput *dxgiOutput;
	if (dxgiAdapter->EnumOutputs(0, &dxgiOutput) == DXGI_ERROR_NOT_FOUND)
	{
		CrashMsg("No outputs displays found");
		return false;
	}
	
	DXGI_OUTPUT_DESC oDesc;
	dxgiOutput->GetDesc(&oDesc);

	int targetHz = params.screenRefreshRateHZ;
	int fsRefresh = 60;

	m_fullScreenRefresh.Numerator = fsRefresh;
	m_fullScreenRefresh.Denominator = 1;

	uint nModes = 0;
	dxgiOutput->GetDisplayModeList(m_backBufferFormat, 0, &nModes, nullptr);

	DXGI_MODE_DESC *modes = new DXGI_MODE_DESC[nModes];
	dxgiOutput->GetDisplayModeList(m_backBufferFormat, 0, &nModes, modes);

	for (uint i = 0; i < nModes; i++)
	{
		if (modes[i].Width >= 640 && modes[i].Height >= 480)
		{
			if (int(modes[i].Width) == m_width && int(modes[i].Height) == m_width)
			{
				int refresh = modes[i].RefreshRate.Numerator / modes[i].RefreshRate.Denominator;
				if (abs(refresh - targetHz) < abs(fsRefresh - targetHz))
				{
					fsRefresh = refresh;
					m_fullScreenRefresh = modes[i].RefreshRate;
				}
			}
		}
	}
	delete [] modes;

	DWORD deviceFlags = 0;//D3D10_CREATE_DEVICE_SINGLETHREADED;

	int debugres = g_cmdLine->FindArgument("-debugd3d");
	if(debugres != -1)
	{
		deviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
	}

#ifdef COMPILE_D3D_10_1
	if (FAILED(D3D10CreateDevice1(dxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION, &m_rhi)))
#else
	if (FAILED(D3D10CreateDevice(dxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, nullptr, deviceFlags, D3D10_SDK_VERSION, &m_rhi)))
#endif
	{
		ErrorMsg("Couldn't create Direct3D10 device interface!\n\nCheck your system configuration and/or install latest video drivers!");
		return false;
	}

	m_msaaSamples = params.multiSamplingMode + 1;

	while (m_msaaSamples > 0)
	{
		UINT nQuality;
		if (SUCCEEDED(m_rhi->CheckMultisampleQualityLevels(m_backBufferFormat, m_msaaSamples, &nQuality)) && nQuality > 0)
			break;
		else
			m_msaaSamples -= 2;
	}

	s_renderApi.SetD3DDevice(m_rhi);

	// create our swap chain
	m_defaultSwapChain = CreateSwapChain(m_hwnd, true);

	// We'll handle Alt-Enter ourselves thank you very much ...
	m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

	dxgiOutput->Release();
	dxgiAdapter->Release();

	if(!s_renderApi.CreateBackbufferDepth(m_width, m_height, m_depthBufferFormat, m_msaaSamples))
	{
		CrashMsg("Cannot create backbuffer surfaces!\n");
		return false;
	}

	UpdateWindow(m_hwnd);

	//-------------------------------------------
	// init caps
	//-------------------------------------------
	ShaderAPICaps& caps = s_renderApi.m_caps;

	HOOK_TO_CVAR(r_anisotropic);

	caps.isHardwareOcclusionQuerySupported = true;
	caps.isInstancingSupported = true;

	caps.maxTextureAnisotropicLevel = clamp(r_anisotropic->GetInt(), 1, 16);
	caps.maxTextureSize = 65535;
	caps.maxRenderTargets = MAX_RENDERTARGETS;

	caps.maxVertexGenericAttributes = MAX_GENERIC_ATTRIB;
	caps.maxVertexTexcoordAttributes = MAX_TEXCOORD_ATTRIB;
	caps.maxVertexStreams = MAX_VERTEXSTREAM;

	caps.maxTextureUnits = MAX_TEXTUREUNIT;
	caps.maxVertexTextureUnits = MAX_VERTEXTEXTURES;

	caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED | SHADER_CAPS_PIXEL_SUPPORTED | SHADER_CAPS_GEOMETRY_SUPPORTED;

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
		caps.renderTargetFormatsSupported[FORMAT_D32F] = true;

	for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
		caps.textureFormatsSupported[i] = true;

	caps.textureFormatsSupported[FORMAT_ATI1N] = false;

	return true;
}

void CD3D11RenderLib::ExitAPI()
{
	if(m_dxgiFactory)
		m_dxgiFactory->Release();

	if (m_rhi)
	{
		ULONG count = m_rhi->Release();

		if (count)
			Msg("D3D10: Unreleased objects after release: %d\n", count);

		m_rhi = nullptr;
	}
}

void CD3D11RenderLib::ReleaseSwapChains()
{
	for(int i = 0; i < m_swapChains.numElem(); i++)
	{
		delete m_swapChains[i];
	}
}

void CD3D11RenderLib::BeginFrame(IEqSwapChain* swapChain)
{
	// "restore"
	if (!s_renderApi.IsDeviceActive())
	{
		// to pause engine
		s_renderApi.m_deviceIsLost = false;
	}
}

void CD3D11RenderLib::EndFrame()
{
	if(m_currentSwapChain)	// swap on other window
		m_currentSwapChain->SwapBuffers();
	else
		m_defaultSwapChain->SwapBuffers();
}

IShaderAPI* CD3D11RenderLib::GetRenderer() const
{
	return &s_renderApi;
}

// reports focus state
void CD3D11RenderLib::SetFocused(bool inFocus)
{
	ASSERT_FAIL("unimplemented");
}

// changes windowed/fullscreen mode; returns false if failed
bool CD3D11RenderLib::SetWindowed(bool enabled)
{
	ASSERT_FAIL("unimplemented");
	return false;
}

// speaks for itself
bool CD3D11RenderLib::IsWindowed() const
{
	ASSERT_FAIL("unimplemented");
	return false;
}

void CD3D11RenderLib::SetBackbufferSize(int w, int h)
{
	// to pause engine
	s_renderApi.m_deviceIsLost = true;

	if (m_rhi != nullptr && (m_width != w || m_height != h))
	{
		s_renderApi.ReleaseBackbufferDepth();

		// resize backbuffer
		if(!m_defaultSwapChain->SetBackbufferSize(w, h))
		{
			MsgError("Cannot create backbuffer surfaces!\n");
		}

		m_width = w;
		m_height = h;

		s_renderApi.CreateBackbufferDepth(m_width, m_height, m_depthBufferFormat, m_msaaSamples);
		s_renderApi.ChangeRenderTargetToBackBuffer();
	}
}

bool CD3D11RenderLib::CaptureScreenshot(CImage &img)
{
	D3D10_TEXTURE2D_DESC desc;
	int bwidth, bheight;
	m_defaultSwapChain->GetBackbufferSize(bwidth, bheight);

	desc.Width = bwidth;
	desc.Height = bheight;
	desc.Format = m_backBufferFormat;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.BindFlags = 0;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.Usage = D3D10_USAGE_STAGING;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;

	bool result = false;

	ID3D10Texture2D *texture;
	if (SUCCEEDED(m_rhi->CreateTexture2D(&desc, nullptr, &texture)))
	{
		ID3D10Resource* backbufferTex = ((CD3D10SwapChain*)m_defaultSwapChain)->m_backbuffer->m_textures[0];

		if (m_msaaSamples > 1)
		{
			ID3D10Texture2D *resolved = nullptr;
			desc.Usage = D3D10_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;

			if (SUCCEEDED(m_rhi->CreateTexture2D(&desc, nullptr, &resolved)))
			{
				m_rhi->ResolveSubresource(resolved, 0, backbufferTex, 0, desc.Format);
				m_rhi->CopyResource(texture, resolved);
				resolved->Release();
			}
		}
		else
		{
			m_rhi->CopyResource(texture, backbufferTex);
		}

		D3D10_MAPPED_TEXTURE2D map;
		if (SUCCEEDED(texture->Map(0, D3D10_MAP_READ, 0, &map)))
		{
			if (m_backBufferFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
			{
				uint32 *dst = (uint32 *) img.Create(FORMAT_RGB10A2, bwidth, bheight, 1, 1);
				ubyte *src = (ubyte *) map.pData;

				for (int y = 0; y < bheight; y++)
				{
					for (int x = 0; x < bwidth; x++)
					{
						dst[x] = ((uint32 *) src)[x] | 0xC0000000;
					}
					dst += bwidth;
					src += map.RowPitch;
				}
			}
			else
			{
				ubyte *dst = img.Create(FORMAT_RGB8, bwidth, bheight, 1, 1);
				ubyte *src = (ubyte *) map.pData;

				for (int y = 0; y < bheight; y++)
				{
					for (int x = 0; x < bwidth; x++)
					{
						dst[3 * x + 0] = src[4 * x + 0];
						dst[3 * x + 1] = src[4 * x + 1];
						dst[3 * x + 2] = src[4 * x + 2];
					}
					dst += bwidth * 3;
					src += map.RowPitch;
				}
			}
			result = true;

			texture->Unmap(0);
		}

		texture->Release();
	}

	return result;
}


// creates swap chain
IEqSwapChain* CD3D11RenderLib::CreateSwapChain(void* window, bool windowed)
{
	CD3D10SwapChain* pNewChain = new CD3D10SwapChain();
	
	if(!pNewChain->Initialize((HWND)window, m_msaaSamples, m_backBufferFormat, m_verticalSyncEnabled, m_fullScreenRefresh, windowed, m_dxgiFactory, m_rhi ))
	{
		MsgError("ERROR: Can't create D3D10 swapchain!\n");
		delete pNewChain;
		return nullptr;
	}

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// destroys a swapchain
void CD3D11RenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}