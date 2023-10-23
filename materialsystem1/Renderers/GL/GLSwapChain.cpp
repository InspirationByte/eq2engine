//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: OpenGL Renderer FAKE swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "core/core_common.h"
#include "GLSwapChain.h"
#include "renderers/ShaderAPI_defs.h"

CGLSwapChain::~CGLSwapChain()
{
#ifdef PLAT_WIN
	if(m_window)
		ReleaseDC(m_window, m_windowDC);
#endif
}

bool CGLSwapChain::Initialize(const RenderWindowInfo& windowInfo)
{
	// init basic
#ifdef PLAT_WIN
	m_window = (HWND)windowInfo.get(windowInfo.userData, RenderWindowInfo::WINDOW);
	m_width = 0;
	m_height = 0;

	// get window parameters
	RECT windowRect;
	GetClientRect(m_window, &windowRect);

	m_width = windowRect.right - windowRect.left;
	m_height = windowRect.bottom - windowRect.top;

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