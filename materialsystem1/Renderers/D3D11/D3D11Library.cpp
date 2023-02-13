//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>

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
ShaderAPID3DX10 s_shaderApi;
IShaderAPI* g_pShaderAPI = &s_shaderApi;
CD3D11RenderLib g_library;

IShaderAPI* g_pShaderAPI = NULL;

DECLARE_CMD(r_info, "Prints renderer info", 0)
{
	g_pShaderAPI->PrintAPIInfo();
}

CD3D11RenderLib::CD3D11RenderLib()
{
	g_eqCore->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
}

CD3D11RenderLib::~CD3D11RenderLib()
{
	g_eqCore->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

bool CD3D11RenderLib::InitCaps()
{
	// D3D10 capabilities are strictly defined and vendors support them all.

	return true;
}

bool CD3D11RenderLib::InitAPI(const shaderAPIParams_t& sparams)
{
	m_savedParams = sparams;

	const int depthBits = m_savedParams.depthBits;
	//int stencilBits = 0;


	// set window
	m_hwnd = (HWND)m_savedParams.windowHandle;

	// get window parameters
	RECT windowRect;
	GetClientRect(m_hwnd, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	if(m_savedParams.screenFormat == FORMAT_RGB8)
		m_savedParams.screenFormat = FORMAT_RGBA8;

	m_backBufferFormat = formats[m_savedParams.screenFormat];
	m_depthBufferFormat = DXGI_FORMAT_UNKNOWN;

	switch (depthBits)
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
	}

//	if (screen >= GetSystemMetrics(SM_CMONITORS)) screen = 0;

	if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void **) &m_dxgiFactory)))
	{
		MsgError("Couldn't create DXGIFactory");
		return false;
	}

	IDXGIAdapter* dxgiAdapter;
	if (m_dxgiFactory->EnumAdapters(0, &dxgiAdapter) == DXGI_ERROR_NOT_FOUND)
	{
		MsgError("No adapters found");
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
		MsgError("No outputs found");
		return false;
	}
	
	DXGI_OUTPUT_DESC oDesc;
	dxgiOutput->GetDesc(&oDesc);

	int targetHz = m_savedParams.screenRefreshRateHZ;
	int fsRefresh = 60;

	m_fullScreenRefresh.Numerator = fsRefresh;
	m_fullScreenRefresh.Denominator = 1;

	uint nModes = 0;
	dxgiOutput->GetDisplayModeList(m_backBufferFormat, 0, &nModes, NULL);

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
	if (FAILED(D3D10CreateDevice1(dxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, D3D10_FEATURE_LEVEL_10_1, D3D10_1_SDK_VERSION, &m_rhi)))
#else
	if (FAILED(D3D10CreateDevice(dxgiAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, deviceFlags, D3D10_SDK_VERSION, &m_rhi)))
#endif
	{
		const char* err_token = "Error";

		const char* driver_upd = "Couldn't create Direct3D10 device interface!\n\nCheck your system configuration and/or install latest video drivers!";
		MessageBoxA(m_hwnd, driver_upd, err_token, MB_OK | MB_ICONWARNING);

		return false;
	}

	m_msaaSamples = m_savedParams.multiSamplingMode+1;

	while (m_msaaSamples > 0)
	{
		UINT nQuality;
		if (SUCCEEDED(m_rhi->CheckMultisampleQualityLevels(m_backBufferFormat, m_msaaSamples, &nQuality)) && nQuality > 0)
			break;
		else
			m_msaaSamples -= 2;
	}

	Msg("creating swap chain and renderer\n");

	s_shaderApi.SetD3DDevice(m_rhi);

	// create our swap chain
	m_defaultSwapChain = CreateSwapChain(m_savedParams.windowHandle, true);

	// We'll handle Alt-Enter ourselves thank you very much ...
	m_dxgiFactory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER);

	dxgiOutput->Release();
	dxgiAdapter->Release();

	if(!s_shaderApi.CreateBackbufferDepth(m_width, m_height, m_depthBufferFormat, m_msaaSamples))
	{
		ErrorMsg("Cannot create backbuffer surfaces!\n");
		MsgError("Cannot create backbuffer surfaces!\n");
		return false;
	}

	UpdateWindow((HWND)m_savedParams.windowHandle);

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

		m_rhi = NULL;
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
	if (!s_shaderApi.IsDeviceActive())
	{
		// to pause engine
		s_shaderApi.m_bDeviceIsLost = false;
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
	return &s_shaderApi;
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
	s_shaderApi.m_bDeviceIsLost = true;

	if (m_rhi != NULL && (m_width != w || m_height != h))
	{
		s_shaderApi.ReleaseBackbufferDepth();

		// resize backbuffer
		if(!m_defaultSwapChain->SetBackbufferSize(w, h))
		{
			MsgError("Cannot create backbuffer surfaces!\n");
		}

		m_width = w;
		m_height = h;

		s_shaderApi.CreateBackbufferDepth(m_width, m_height, m_depthBufferFormat, m_msaaSamples);
		s_shaderApi.ChangeRenderTargetToBackBuffer();
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
	if (SUCCEEDED(m_rhi->CreateTexture2D(&desc, NULL, &texture)))
	{
		ID3D10Resource* backbufferTex = ((CD3D10SwapChain*)m_defaultSwapChain)->m_backbuffer->m_textures[0];

		if (m_savedParams.multiSamplingMode > 1)
		{
			ID3D10Texture2D *resolved = NULL;
			desc.Usage = D3D10_USAGE_DEFAULT;
			desc.CPUAccessFlags = 0;

			if (SUCCEEDED(m_rhi->CreateTexture2D(&desc, NULL, &resolved)))
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
	
	if(!pNewChain->Initialize((HWND)window, m_msaaSamples, m_backBufferFormat, m_savedParams.verticalSyncEnabled, m_fullScreenRefresh, windowed, m_dxgiFactory, m_rhi ))
	{
		MsgError("ERROR: Can't create D3D10 swapchain!\n");
		delete pNewChain;
		return NULL;
	}

	m_swapChains.append(pNewChain);
	return pNewChain;
}

// returns default swap chain
IEqSwapChain* CD3D11RenderLib::GetDefaultSwapchain()
{
	return m_defaultSwapChain;
}

// destroys a swapchain
void CD3D11RenderLib::DestroySwapChain(IEqSwapChain* swapChain)
{
	m_swapChains.remove(swapChain);
	delete swapChain;
}