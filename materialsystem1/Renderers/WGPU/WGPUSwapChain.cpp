#include "core/core_common.h"
#include "WGPUBackend.h"
#include "WGPUSwapChain.h"
#include "WGPULibrary.h"
#include "renderers/ITexture.h"

#ifdef DAWN_ENABLE_BACKEND_VULKAN

#include <vulkan/vulkan_win32.h>
#pragma comment(lib, "vulkan-1.lib")

// Helper to obtain a Vulkan surface from the supplied window
VkSurfaceKHR createVkSurface(WGPUDevice device, void* window) 
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;
#ifdef WIN32
	VkWin32SurfaceCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	info.hinstance = GetModuleHandle(NULL);
	info.hwnd = reinterpret_cast<HWND>(window);
	vkCreateWin32SurfaceKHR(dawn::native::vulkan::GetInstance(device), &info, nullptr, &surface);
#else // LINUX
	// TODO...
#endif
	return surface;
}
#endif

static bool InitSwapChain(WGPUDevice device, WGPUBackendType backend, void* window, DawnSwapChainImplementation& swapImpl, WGPUTextureFormat& swapFmt)
{
	if (swapImpl.userData)
		return false;

	switch (backend) {
#ifdef DAWN_ENABLE_BACKEND_D3D12
	case WGPUBackendType_D3D12:
		swapImpl = dawn::native::d3d12::CreateNativeSwapChainImpl(device, reinterpret_cast<HWND>(window));
		swapFmt = dawn::native::d3d12::GetNativeSwapChainPreferredFormat(&swapImpl);
		break;
#endif
#ifdef DAWN_ENABLE_BACKEND_VULKAN
	case WGPUBackendType_Vulkan:
		swapImpl = dawn::native::vulkan::CreateNativeSwapChainImpl(device, createVkSurface(device, window));
		swapFmt = dawn::native::vulkan::GetNativeSwapChainPreferredFormat(&swapImpl);
		break;
#endif
	default:
		swapImpl = dawn::native::null::CreateNativeSwapChainImpl();
		swapFmt = WGPUTextureFormat_Undefined;
		break;
	}
	return true;
}

CWGPUSwapChain::CWGPUSwapChain(CWGPURenderLib* host, void* window)
	: m_host(host), m_window(window)
{
	memset(&m_swapImpl, 0, sizeof(m_swapImpl));
	InitSwapChain(m_host->m_rhiDevice, m_host->m_backendType, m_window, m_swapImpl, m_swapFmt);
}

CWGPUSwapChain::~CWGPUSwapChain()
{
	if (m_swapChain)
		wgpuSwapChainRelease(m_swapChain);
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

	if (!m_swapChain)
	{
		WGPUSwapChainDescriptor swapChainDesc = {};
		//swapChainDesc.nextInChain = nullptr;
		//swapChainDesc.width = m_backbufferSize.x;
		//swapChainDesc.height = m_backbufferSize.y;
		//swapChainDesc.format = m_swapFmt;
		//swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
		//swapChainDesc.presentMode = WGPUPresentMode_Immediate; // TODO: other modes

		swapChainDesc.implementation = reinterpret_cast<uintptr_t>(&m_swapImpl);
		m_swapChain = wgpuDeviceCreateSwapChain(m_host->m_rhiDevice, nullptr, &swapChainDesc);
	}

	// NOTE: currently crashes on D3D12 and invalid on Vulkan, after resizing again it becomes valid
	wgpuSwapChainConfigure(m_swapChain, m_swapFmt, WGPUTextureUsage_RenderAttachment, m_backbufferSize.x, m_backbufferSize.y);

	return true;
}
	 
bool CWGPUSwapChain::SwapBuffers()
{
	if (!m_swapChain)
		return false;

	//while (m_completedFrames < m_requestedFrame)
	//	wgpuDeviceTick(m_host->m_rhiDevice);

	wgpuSwapChainPresent(m_swapChain);
	//wgpuQueueOnSubmittedWorkDone(queue, 0, swapChainWorkSubmittedCallback, this);
	return true;
}