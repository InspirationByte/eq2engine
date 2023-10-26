//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU window surface swap chain
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "WGPUBackend.h"
#include "WGPUSwapChain.h"
#include "WGPULibrary.h"
#include "renderers/ITexture.h"

// TODO: determine properly?
const WGPUTextureFormat kSwapChainFormat = WGPUTextureFormat_BGRA8Unorm;

CWGPUSwapChain::CWGPUSwapChain(CWGPURenderLib* host, const RenderWindowInfo& windowInfo)
	: m_host(host)
	, m_winInfo(windowInfo)
{

	m_swapFmt = kSwapChainFormat;
}

CWGPUSwapChain::~CWGPUSwapChain()
{
	if (m_swapChain)
	{
		wgpuSwapChainRelease(m_swapChain);
		wgpuSurfaceRelease(m_surface);
	}
}

void* CWGPUSwapChain::GetWindow() const
{
	return m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
}

ITexturePtr CWGPUSwapChain::GetBackbuffer() const
{
	return nullptr;
}

void CWGPUSwapChain::GetBackbufferSize(int& wide, int& tall) const
{	 
	wide = m_backbufferSize.x;
	tall = m_backbufferSize.y;
}

bool CWGPUSwapChain::SetBackbufferSize(int wide, int tall)
{	 
	const IVector2D newSize(wide, tall);

	if (m_backbufferSize == newSize)
		return false;

	m_backbufferSize = newSize;

	if (m_swapChain)
		wgpuSwapChainRelease(m_swapChain);

	if(!m_surface)
	{
		WGPUSurfaceDescriptor surfDesc = {};

		WGPUSurfaceDescriptorFromWindowsHWND windowsSurfDesc = {};
		WGPUSurfaceDescriptorFromXlibWindow x11SurfDesc = {};
		WGPUSurfaceDescriptorFromWaylandSurface waylandSurfDesc = {};
		WGPUSurfaceDescriptorFromAndroidNativeWindow androidWindowSurfDesc = {};

		switch (m_winInfo.windowType)
		{
		case RHI_WINDOW_HANDLE_NATIVE_WINDOWS:
			windowsSurfDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
			windowsSurfDesc.hinstance = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::TOPLEVEL);
			windowsSurfDesc.hwnd = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &windowsSurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_X11:
			x11SurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			x11SurfDesc.window = (uint32_t)m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &x11SurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_WAYLAND:
			waylandSurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			waylandSurfDesc.surface = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::SURFACE);
			surfDesc.nextInChain = &waylandSurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_ANDROID:
			androidWindowSurfDesc.window = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &androidWindowSurfDesc.chain;
			break;
		default:
			ASSERT_FAIL("Unsupported RHI_WINDOW_HANDLE value!");
		}

		m_surface = wgpuInstanceCreateSurface(m_host->m_instance, &surfDesc);
	}

	WGPUSwapChainDescriptor swapChainDesc = {};
	swapChainDesc.width = m_backbufferSize.x;
	swapChainDesc.height = m_backbufferSize.y;
	swapChainDesc.format = m_swapFmt;
	swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
	swapChainDesc.presentMode = WGPUPresentMode_Fifo; // TODO: other modes

	m_swapChain = wgpuDeviceCreateSwapChain(m_host->m_rhiDevice, m_surface, &swapChainDesc);

	return m_swapChain != nullptr;
}
	 
bool CWGPUSwapChain::SwapBuffers()
{
	if (!m_swapChain)
		return false;
	wgpuSwapChainPresent(m_swapChain);
	return true;
}