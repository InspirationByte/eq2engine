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
	return m_winInfo.get(RenderWindowInfo::WINDOW);
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
		WGPUSurfaceDescriptor desc = {};

		WGPUSurfaceDescriptorFromWindowsHWND windowsSurfDesc = {};
#ifdef _WIN32
		windowsSurfDesc.chain.sType = WGPUSType_SurfaceDescriptorFromWindowsHWND;
		windowsSurfDesc.hinstance = m_winInfo.get(RenderWindowInfo::TOPLEVEL);
		windowsSurfDesc.hwnd = m_winInfo.get(RenderWindowInfo::WINDOW);
		desc.nextInChain = &windowsSurfDesc.chain;
#else
		// TODO: other platforms
#endif

		m_surface = wgpuInstanceCreateSurface(m_host->m_instance, &desc);
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

	//while (m_completedFrames < m_requestedFrame)
	//	wgpuDeviceTick(m_host->m_rhiDevice);

	wgpuSwapChainPresent(m_swapChain);
	//wgpuQueueOnSubmittedWorkDone(queue, 0, OnWGPUSwapChainWorkSubmittedCallback, this);
	return true;
}