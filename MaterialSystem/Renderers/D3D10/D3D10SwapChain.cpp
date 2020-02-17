//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D10 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include "D3D10SwapChain.h"
#include "ShaderAPID3DX10.h"
#include "DebugInterface.h"
#include "d3dx10_def.h"

CD3D10SwapChain::~CD3D10SwapChain()
{
	// back to normal screen
	if(m_RHIChain)
	{
		m_RHIChain->SetFullscreenState(false, NULL);
		m_RHIChain->Release();
	}

	m_rhi->Release();

	if(m_backbuffer)
		g_pShaderAPI->FreeTexture(m_backbuffer);
}

CD3D10SwapChain::CD3D10SwapChain()
{
	m_backbuffer = NULL;
	m_rhi = NULL;
	m_RHIChain = NULL;
}

bool CD3D10SwapChain::Initialize(	HWND window, int numMSAASamples, 
									DXGI_FORMAT backFmt, 
									bool vSync, 
									DXGI_RATIONAL refreshParams, 
									bool windowed,
									IDXGIFactory* dxFactory, ID3D10Device* rhi)
{
	// init basic
	m_window = window;
	m_numMSAASamples = numMSAASamples;
	m_vSyncEnabled = vSync;
	
	m_backBufferFormat = backFmt;

	m_width = 0;
	m_height = 0;

	m_rhi = rhi;
	m_rhi->AddRef();

	// init datas

	// set window
	HWND windowHandle = m_window;

	// get window parameters
	RECT windowRect;
	GetClientRect(windowHandle, &windowRect);

	m_width = windowRect.right;
	m_height = windowRect.bottom;

	// Create device and swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));

	sd.BufferDesc.Width  = m_width;
	sd.BufferDesc.Height = m_height;
	sd.BufferCount = 1;

	sd.BufferDesc.Format = backFmt;

	sd.BufferDesc.RefreshRate = refreshParams;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
	
	sd.OutputWindow = windowHandle;
	sd.Windowed = (BOOL)windowed;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	sd.SampleDesc.Count = m_numMSAASamples;
	sd.SampleDesc.Quality = 0;

	Msg("Creating DXGI swapchain W=%d H=%d\n", m_width, m_height);

	HRESULT res = dxFactory->CreateSwapChain(m_rhi, &sd, &m_RHIChain);

	if (FAILED(res))
	{
		MsgError("ERROR:! Couldn't create DX10 swapchain (0x%p)", res);
		return false;
	}

	return CreateOrUpdateBackbuffer();
}

bool CD3D10SwapChain::CreateOrUpdateBackbuffer()
{
	if(!m_RHIChain)
		return false;

	if(!m_backbuffer)
	{
		m_backbuffer = new CD3D10Texture;
		m_backbuffer->SetName("_rt_backbuffer");
		m_backbuffer->SetDimensions(m_width, m_height);
		m_backbuffer->SetFlags(TEXFLAG_RENDERTARGET | TEXFLAG_FOREIGN | TEXFLAG_NOQUALITYLOD);

		m_backbuffer->Ref_Grab();

		((ShaderAPID3DX10*)g_pShaderAPI)->m_TextureList.append(m_backbuffer);
	}

	ID3D10Texture2D*		backbufferTex;
	ID3D10RenderTargetView* backbufferRTV;

	if (FAILED(m_RHIChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID *) &backbufferTex))) 
		return false;

	if (FAILED(m_rhi->CreateRenderTargetView(backbufferTex, NULL, &backbufferRTV)))
		return false;

	m_backbuffer->textures.append( backbufferTex );
	m_backbuffer->rtv.append( backbufferRTV );
	m_backbuffer->srv.append( ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(backbufferTex) );

	Msg("CreateOrUpdateBackbuffer OK\n");

	return true;
}

// retrieves backbuffer size for this swap chain
void CD3D10SwapChain::GetBackbufferSize(int& wide, int& tall)
{
	wide = m_backbuffer->GetWidth();
	tall = m_backbuffer->GetHeight();
}

// sets backbuffer size for this swap chain
bool CD3D10SwapChain::SetBackbufferSize(int wide, int tall)
{
	m_width = wide;
	m_height = tall;

	m_rhi->OMSetRenderTargets(0, NULL, NULL);

	if(m_backbuffer)
	{
		// release textures
		for(int i = 0; i < m_backbuffer->textures.numElem(); i++)
			m_backbuffer->textures[i]->Release();

		m_backbuffer->textures.clear();

		for(int i = 0; i < m_backbuffer->rtv.numElem(); i++)
			m_backbuffer->rtv[i]->Release();

		m_backbuffer->rtv.clear();

		for(int i = 0; i < m_backbuffer->srv.numElem(); i++)
			m_backbuffer->srv[i]->Release();

		m_backbuffer->srv.clear();
	}

	Msg("SetBackbufferSize: %dx%d\n", m_width, m_height);

	HRESULT hr = m_RHIChain->ResizeBuffers(1, m_width, m_height, m_backBufferFormat, /*DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH*/ 0);

	if (FAILED(hr))
	{
		MsgError("ERROR: CD3D10SwapChain SetBackbufferSize failed (%d)\n", hr);
		return false;
	}

	m_backbuffer->SetDimensions(m_width, m_height);

	return CreateOrUpdateBackbuffer();
}

// individual swapbuffers call
bool CD3D10SwapChain::SwapBuffers()
{
	if (FAILED(m_RHIChain->Present( (int)m_vSyncEnabled, 0 )))
		return false;

	return true;
}