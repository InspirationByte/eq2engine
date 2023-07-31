//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
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
	//m_backbuffer = nullptr;
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

// retrieves backbuffer size for this swap chain
void CD3D9SwapChain::GetBackbufferSize(int& wide, int& tall) const
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