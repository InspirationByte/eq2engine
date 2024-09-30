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
}

CWGPUSwapChain::~CWGPUSwapChain()
{
	if(m_surface)
		wgpuSurfaceRelease(m_surface);
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
	WGPUSurfaceTexture rhiSurfTex{};
	wgpuSurfaceGetCurrentTexture(m_surface, &rhiSurfTex);

	switch (rhiSurfTex.status)
	{
    case WGPUSurfaceGetCurrentTextureStatus_Success:
		// All good, could check for `surfTex.suboptimal` here.
		break;
	case WGPUSurfaceGetCurrentTextureStatus_Timeout:
	case WGPUSurfaceGetCurrentTextureStatus_Outdated:
	case WGPUSurfaceGetCurrentTextureStatus_Lost:
	case WGPUSurfaceGetCurrentTextureStatus_Error:
		return;
	case WGPUSurfaceGetCurrentTextureStatus_OutOfMemory:
	case WGPUSurfaceGetCurrentTextureStatus_DeviceLost:
	case WGPUSurfaceGetCurrentTextureStatus_Force32:
		// Fatal error
		ASSERT_FAIL("wgpuSurfaceGetCurrentTexture status=%#.8x\n", rhiSurfTex.status);
		return;
    }

	ASSERT_MSG(rhiSurfTex.texture != nullptr, "Swapchain texture has not been created");

	{
		if (m_textureRef->m_rhiTextures.numElem())
			wgpuTextureRelease(m_textureRef->m_rhiTextures[0]);
		else
			m_textureRef->m_rhiTextures.setNum(1);
		m_textureRef->m_rhiTextures[0] = rhiSurfTex.texture;
	}

	{
		if (m_textureRef->m_rhiViews.numElem())
			wgpuTextureViewRelease(m_textureRef->m_rhiViews[0]);
		else
			m_textureRef->m_rhiViews.setNum(1);
		m_textureRef->m_rhiViews[0] = wgpuTextureCreateView(rhiSurfTex.texture, nullptr);
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
	m_textureRef->Release();

	if (!m_surface)
	{
		WGPUSurfaceDescriptor surfDesc = {};

		WGPUSurfaceSourceWindowsHWND windowsSurfDesc = {};
		WGPUSurfaceSourceXlibWindow x11SurfDesc = {};
		WGPUSurfaceSourceWaylandSurface waylandSurfDesc = {};
		WGPUSurfaceSourceAndroidNativeWindow androidWindowSurfDesc = {};

		switch (m_winInfo.windowType)
		{
		case RHI_WINDOW_HANDLE_NATIVE_WINDOWS:
			windowsSurfDesc.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
			windowsSurfDesc.hinstance = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::TOPLEVEL);
			windowsSurfDesc.hwnd = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &windowsSurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_X11:
			x11SurfDesc.chain.sType = WGPUSType_SurfaceSourceXlibWindow;
			x11SurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			x11SurfDesc.window = (uint64_t)m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &x11SurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_WAYLAND:
			waylandSurfDesc.chain.sType = WGPUSType_SurfaceSourceWaylandSurface;
			waylandSurfDesc.display = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::DISPLAY);
			waylandSurfDesc.surface = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::SURFACE);
			surfDesc.nextInChain = &waylandSurfDesc.chain;
			break;
		case RHI_WINDOW_HANDLE_NATIVE_ANDROID:
			androidWindowSurfDesc.chain.sType = WGPUSType_SurfaceSourceAndroidNativeWindow;
			androidWindowSurfDesc.window = m_winInfo.get(m_winInfo.userData, RenderWindowInfo::WINDOW);
			surfDesc.nextInChain = &androidWindowSurfDesc.chain;
			break;
		default:
			ASSERT_FAIL("Unsupported RHI_WINDOW_HANDLE value!");
		}
	
		m_surface = wgpuInstanceCreateSurface(m_host->m_instance, &surfDesc);
	}

	WGPUSurfaceCapabilities surfCapabilities;
	wgpuSurfaceGetCapabilities(m_surface, m_host->m_rhiAdapter, &surfCapabilities);

	ASSERT_MSG(surfCapabilities.formatCount > 0, "Surface capabilities format count is 0");

	WGPUTextureFormat rhiSurfaceFormat = surfCapabilities.formats[0];
	for(int i = 1; i < FORMAT_COUNT; ++i)
	{
		const ETextureFormat format = static_cast<ETextureFormat>(i);
		const ETextureFormat srgbFormat = MakeTexFormat(format, TEXFORMAT_FLAG_SRGB);
		const ETextureFormat rbSwapFormat = MakeTexFormat(format, TEXFORMAT_FLAG_SWAP_RB);
		const ETextureFormat rbSwapSrgbFormat = MakeTexFormat(format, TEXFORMAT_FLAG_SWAP_RB | TEXFORMAT_FLAG_SRGB);

		if(GetWGPUTextureFormat(format) == rhiSurfaceFormat)
		{
			m_textureRef->SetFormat(format);
			break;
		}

		if(GetWGPUTextureFormat(srgbFormat) == rhiSurfaceFormat)
		{
			m_textureRef->SetFormat(srgbFormat);
			break;
		}

		if(GetWGPUTextureFormat(rbSwapFormat) == rhiSurfaceFormat)
		{	
			m_textureRef->SetFormat(rbSwapFormat);
			break;
		}

		if(GetWGPUTextureFormat(rbSwapSrgbFormat) == rhiSurfaceFormat)
		{	
			m_textureRef->SetFormat(rbSwapSrgbFormat);
			break;
		}
	}

	WGPUSurfaceConfiguration rhiSurfaceConfig{};
	rhiSurfaceConfig.device = m_host->m_rhiDevice;
	rhiSurfaceConfig.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;	// requires SurfaceCapabilities feature
	rhiSurfaceConfig.format = rhiSurfaceFormat;
	rhiSurfaceConfig.presentMode = m_vSync ? WGPUPresentMode_Fifo : WGPUPresentMode_Mailbox;
	rhiSurfaceConfig.alphaMode = WGPUCompositeAlphaMode_Opaque;	// TODO: get from surface capabilities
	rhiSurfaceConfig.width = m_backbufferSize.x;
	rhiSurfaceConfig.height = m_backbufferSize.y;

	wgpuSurfaceConfigure(m_surface, &rhiSurfaceConfig);
	return m_surface;
}

bool CWGPUSwapChain::SetBackbufferSize(int wide, int tall)
{	 
	m_backbufferSize = IVector2D(wide, tall);
	return true;
}
	 
bool CWGPUSwapChain::SwapBuffers()
{
	wgpuSurfacePresent(m_surface);
	return true;
}