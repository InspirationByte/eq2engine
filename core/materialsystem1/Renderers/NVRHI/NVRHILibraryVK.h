/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/vulkan.h>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

#include "../IRenderLibrary.h"
#include "../RenderWorker.h"

class CNVRHISwapChainVK;

class CNVRHIRenderLibVK
	: public IRenderLibrary
	, public RenderWorkerHandler
{
	friend class CNVRHISwapChainVK;
public:
	CNVRHIRenderLibVK();

	bool			InitCaps();
	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();
	
	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();

	IShaderAPI*		GetRenderer() const;
	ITexturePtr		GetCurrentBackbuffer() const;

	void			SetVSync(bool enable);
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage& img);

	virtual ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);
protected:
	bool			PickPhysicalDevice();
	bool			FindQueueFamilies(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface);
	bool			CreateDevice();

	const char*		GetAsyncThreadName() const { return "EqRenderThread"; }
	void			BeginAsyncOperation(uintptr_t threadId) {}
	void			EndAsyncOperation() {}
	bool			IsMainThread(uintptr_t threadId) const;

	struct VulkanExtensionSet
	{
		Array<EqString> instance{ PP_SL };
		Array<EqString> layers{ PP_SL };
		Array<EqString> device{ PP_SL };
	};

	// minimal set of required extensions
	VulkanExtensionSet			m_enabledExtensions;

	Array<CNVRHISwapChainVK*>	m_swapChains{ PP_SL };
	int							m_swapChainCounter{ 0 };

	CRefPtr<CNVRHISwapChainVK>	m_currentSwapChain;
	CRefPtr<CNVRHISwapChainVK>	m_defaultSwapChain;

	uint32_t					m_vkDeviceApiVersion = VK_HEADER_VERSION_COMPLETE;
	PFN_vkGetInstanceProcAddr	m_vkGetInstanceProcAddr = nullptr;

	vk::Instance				m_vkInstance;
	vk::DebugReportCallbackEXT	m_vkDebugReportCallback;

	vk::PhysicalDevice			m_vkPhysicalDevice;
	int							m_vkGraphicsQueueFamily = -1;
	int							m_vkComputeQueueFamily = -1;
	int							m_vkTransferQueueFamily = -1;
	int							m_vkPresentQueueFamily = -1;

	vk::Device					m_vkDevice;
	vk::Queue					m_vkGraphicsQueue;
	vk::Queue					m_vkComputeQueue;
	vk::Queue					m_vkTransferQueue;
	vk::Queue					m_vkPresentQueue;

	nvrhi::DeviceHandle			m_nvrhiDevice;
	nvrhi::EventQueryHandle		m_nvrhiFrameWaitQuery;

	bool						m_windowed{ false };
};

