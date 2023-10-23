//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D9 Renderer FAKE swapchain for using to draw in multiple windows
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9SwapChain.h"

bool CD3D9SwapChain::Initialize(const RenderWindowInfo& windowInfo)
{
	// init basic
	m_window = (HWND)windowInfo.get(windowInfo.userData, RenderWindowInfo::WINDOW);

	// get window parameters
	RECT windowRect;
	GetClientRect(m_window, &windowRect);

	m_width = windowRect.right - windowRect.left;
	m_height = windowRect.bottom - windowRect.top;

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