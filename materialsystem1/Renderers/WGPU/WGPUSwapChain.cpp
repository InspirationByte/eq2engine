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
#include "WGPURenderDefs.h"

constexpr int TOGGLE_BIT = 0x80000000;

CWGPUSwapChain::CWGPUSwapChain(CWGPURenderLib* host, const RenderWindowInfo& windowInfo, ITexturePtr swapChainTexture)
	: m_host(host)
	, m_winInfo(windowInfo)
{
	m_textureRef = CRefPtr<CWGPUTexture>(static_cast<CWGPUTexture*>(swapChainTexture.Ptr()));
	m_textureRef->SetFormat(MakeTexFormat(FORMAT_RGBA8, TEXFORMAT_FLAG_SWAP_RB));

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
	return ITexturePtr(m_textureRef);
}

void CWGPUSwapChain::UpdateBackbufferView() const
{
	if (!m_swapChain)
		return;

	{
		if (m_textureRef->m_rhiTextures.numElem())
			wgpuTextureRelease(m_textureRef->m_rhiTextures[0]);
		else
			m_textureRef->m_rhiTextures.setNum(1);
		m_textureRef->m_rhiTextures[0] = wgpuSwapChainGetCurrentTexture(m_swapChain);
	}

	{
		if (m_textureRef->m_rhiViews.numElem())
			wgpuTextureViewRelease(m_textureRef->m_rhiViews[0]);
		else
			m_textureRef->m_rhiViews.setNum(1);
		m_textureRef->m_rhiViews[0] = wgpuSwapChainGetCurrentTextureView(m_swapChain);
	}
}

void CWGPUSwapChain::GetBackbufferSize(int& wide, int& tall) const
{	 
	wide = m_backbufferSize.x;
	tall = m_backbufferSize.y;
}

void CWGPUSwapChain::SetVSync(bool enable)
{
	if((m_vSync > 0) != enable)
		m_vSync = TOGGLE_BIT | (int)enable;
}

bool CWGPUSwapChain::UpdateResize()
{
	if ((m_vSync & TOGGLE_BIT) == 0 && m_textureRef->GetWidth() == m_backbufferSize.x && m_textureRef->GetHeight() == m_backbufferSize.y)
		return true;

	m_vSync &= ~TOGGLE_BIT;

	m_textureRef->SetDimensions(m_backbufferSize.x, m_backbufferSize.y);
	if (m_swapChain)
	{
		m_textureRef->Release();
		wgpuSwapChainRelease(m_swapChain);
	}

	if (!m_surface)
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
			windowsSurfDesc.chain.sType = WGPUSType_SurfaceDescriptorFromXlibWindow;
			x11SurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			x11SurfDesc.window = (uint32_t)m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &x11SurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_WAYLAND:
			windowsSurfDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWaylandSurface;
			waylandSurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			waylandSurfDesc.surface = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::SURFACE);
			surfDesc.nextInChain = &waylandSurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_ANDROID:
			windowsSurfDesc.chain.sType = WGPUSType_SurfaceDescriptorFromAndroidNativeWindow;
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
	swapChainDesc.format = GetWGPUTextureFormat(m_textureRef->GetFormat());
	swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
	swapChainDesc.presentMode = m_vSync ? WGPUPresentMode_Fifo : WGPUPresentMode_Mailbox;

	m_swapChain = wgpuDeviceCreateSwapChain(m_host->m_rhiDevice, m_surface, &swapChainDesc);
	return m_swapChain;
}

bool CWGPUSwapChain::SetBackbufferSize(int wide, int tall)
{	 
	m_backbufferSize = IVector2D(wide, tall);
	return true;
}
	 
bool CWGPUSwapChain::SwapBuffers()
{
	if (!m_swapChain)
		return false;

	wgpuSwapChainPresent(m_swapChain);
	return true;
}