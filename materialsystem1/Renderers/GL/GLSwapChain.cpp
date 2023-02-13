//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer FAKE swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "GLSwapChain.h"

CGLSwapChain::~CGLSwapChain()
{
#ifdef PLAT_WIN
	if(m_window)
		ReleaseDC(m_window, m_windowDC);
#endif
}

bool CGLSwapChain::Initialize(void* window, bool vSync, bool windowed)
{
	// init basic
#ifdef PLAT_WIN
	m_window = (HWND)window;
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
#endif
	return true;
}

void* CGLSwapChain::GetWindow() const
{
#ifdef PLAT_WIN
	return m_window;
#else
	return nullptr;
#endif
}

// retrieves backbuffer size for this swap chain
void CGLSwapChain::GetBackbufferSize(int& wide, int& tall) const
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