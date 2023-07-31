//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D10 Renderer swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include "core/core_common.h"
#include "D3D11SwapChain.h"
#include "ShaderAPID3D11.h"

#include "d3dx11_def.h"

extern ShaderAPID3DX10 s_shaderApi;

CD3D10SwapChain::~CD3D10SwapChain()
{
	// back to normal screen
	if(m_swapChain)
	{
		m_swapChain->SetFullscreenState(false, nullptr);
		m_swapChain->Release();
	}

	if(m_backbuffer)
		g_pShaderAPI->FreeTexture(m_backbuffer);
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

	HRESULT res = dxFactory->CreateSwapChain(rhi, &sd, &m_swapChain);

	if (FAILED(res))
	{
		MsgError("ERROR:! Couldn't create DX10 swapchain (0x%p)", res);
		return false;
	}

	return CreateOrUpdateBackbuffer();
}

bool CD3D10SwapChain::CreateOrUpdateBackbuffer()
{
	if(!m_swapChain)
		return false;

	if(!m_backbuffer)
	{
		m_backbuffer.Assign((CD3D10Texture*)s_shaderApi.CreateTextureResource("_rt_backbuffer").Ptr());

		m_backbuffer->SetDimensions(m_width, m_height);
		m_backbuffer->SetFlags(TEXFLAG_RENDERTARGET | TEXFLAG_NOQUALITYLOD);
	}

	ID3D10Texture2D*		backbufferTex;
	ID3D10RenderTargetView* backbufferRTV;

	if (FAILED(m_swapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID *) &backbufferTex))) 
		return false;

	if (FAILED(s_shaderApi.m_pD3DDevice->CreateRenderTargetView(backbufferTex, nullptr, &backbufferRTV)))
		return false;

	m_backbuffer->m_textures.append( backbufferTex );
	m_backbuffer->m_rtv.append( backbufferRTV );
	m_backbuffer->m_srv.append( ((ShaderAPID3DX10*)g_pShaderAPI)->TexResource_CreateShaderResourceView(backbufferTex) );

	return true;
}

// retrieves backbuffer size for this swap chain
void CD3D10SwapChain::GetBackbufferSize(int& wide, int& tall) const
{
	wide = m_backbuffer->GetWidth();
	tall = m_backbuffer->GetHeight();
}

// sets backbuffer size for this swap chain
bool CD3D10SwapChain::SetBackbufferSize(int wide, int tall)
{
	m_width = wide;
	m_height = tall;

	s_shaderApi.m_pD3DDevice->OMSetRenderTargets(0, nullptr, nullptr);

	if(m_backbuffer)
		m_backbuffer->Release();

	Msg("SetBackbufferSize: %dx%d\n", m_width, m_height);

	HRESULT hr = m_swapChain->ResizeBuffers(1, m_width, m_height, m_backBufferFormat, /*DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH*/ 0);

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
	if (FAILED(m_swapChain->Present( (int)m_vSyncEnabled, 0 )))
		return false;

	return true;
}