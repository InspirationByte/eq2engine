//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Renderer FAKE swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9SwapChain.h"

CD3D9SwapChain::~CD3D9SwapChain()
{

}

CD3D9SwapChain::CD3D9SwapChain()
{
	//m_backbuffer = NULL;
}

bool CD3D9SwapChain::Initialize( HWND window, bool vSync, bool windowed)
{
	// init basic
	m_window = window;
	m_vSyncEnabled = vSync;

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

	return true;
}

/*
bool CD3D9SwapChain::CreateOrUpdateBackbuffer()
{
	if(!m_RHIChain)
		return false;

	if(!m_backbuffer)
	{
		m_backbuffer = PPNew CD3D10Texture;
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
*/
// retrieves backbuffer size for this swap chain
void CD3D9SwapChain::GetBackbufferSize(int& wide, int& tall)
{
	wide = m_width;
	tall = m_height;
}

// sets backbuffer size for this swap chain
bool CD3D9SwapChain::SetBackbufferSize(int wide, int tall)
{
	m_width = wide;
	m_height = tall;

	return true;
}

// individual swapbuffers call
bool CD3D9SwapChain::SwapBuffers()
{
	return true;
}