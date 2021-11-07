//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer FAKE swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include "GLSwapChain.h"

#include "core/DebugInterface.h"

CGLSwapChain::~CGLSwapChain()
{
	if(m_window)
		ReleaseDC(m_window, m_windowDC);
}

CGLSwapChain::CGLSwapChain()
{
	//m_backbuffer = NULL;
}

bool CGLSwapChain::Initialize( HWND window, bool vSync, bool windowed)
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

	m_windowDC = GetDC(m_window);

	return true;
}

// retrieves backbuffer size for this swap chain
void CGLSwapChain::GetBackbufferSize(int& wide, int& tall)
{
	wide = m_width;
	tall = m_height;
}

// sets backbuffer size for this swap chain
bool CGLSwapChain::SetBackbufferSize(int wide, int tall)
{
	m_width = wide;
	m_height = tall;

	return true;
}

// individual swapbuffers call
bool CGLSwapChain::SwapBuffers()
{
	return true;
}