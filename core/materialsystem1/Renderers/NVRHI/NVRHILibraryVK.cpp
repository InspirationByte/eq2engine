//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include <nvrhi/nvrhi.h>
#include <nvrhi/validation.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "imaging/ImageLoader.h"

#include "NVRHIBackend.h"
#include "NVRHILibraryVK.h"
#include "NVRHISwapChainVK.h"
#include "NVRHIRenderAPI.h"

DECLARE_CVAR(vulkan_validation, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR(vulkan_break_on_error, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR_G(vulkan_fastSync, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR_F(nvrhi_validation);
DECLARE_CVAR_F(nvrhi_breakOnError);

// Triple buffering for NVRHI with command queue event query sync method
static constexpr uint32 SWAP_CHAIN_BUFFERS = 3;

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

CNVRHIRenderLibVK* CNVRHIRenderLibVK::Instance = nullptr;

static VKAPI_ATTR vk::Bool32 VKAPI_CALL vulkanDebugCallback(
	vk::DebugReportFlagsEXT flags,
	vk::DebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	if (flags & vk::DebugReportFlagBitsEXT::eError)
	{
		MsgError("VULKAN ERROR %s:%d:0x%zx: %s\n", layerPrefix, code, location, msg);
		if (vulkan_break_on_error.GetBool())
		{
			_DEBUG_BREAK;
		}
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		MsgError("VULKAN WARNING %s:%d:0x%zx: %s\n", layerPrefix, code, location, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		MsgWarning("VULKAN PERFORMANCE %s:%d:0x%zx: %s\n", layerPrefix, code, location, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eInformation)
	{
		MsgError("VULKAN %s:%d:0x%zx: %s\n", layerPrefix, code, location, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eDebug)
	{
		MsgError("VULKAN DEBUG %s:%d:0x%zx: %s\n", layerPrefix, code, location, msg);
	}

	return VK_FALSE;
}

IShaderAPI* CNVRHIRenderLibVK::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibVK::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId;
}

bool CNVRHIRenderLibVK::InitCaps()
{
	CNVRHIRenderLibVK::Instance = this;
	CNVRHIRenderAPI::Instance.m_rhiBackendType = NVRHI_BACKEND_VULKAN;

#if VK_HEADER_VERSION >= 301
	using VulkanDynamicLoader = vk::detail::DynamicLoader;
#else
	using VulkanDynamicLoader = vk::DynamicLoader;
#endif

#if defined(__APPLE__) && defined( USE_MoltenVK )
	// use libMoltenVK on Apple devices
	static const VulkanDynamicLoader rhiDynamicLoader("libMoltenVK.dylib");
#else
	static const VulkanDynamicLoader rhiDynamicLoader;
#endif

	m_vkGetInstanceProcAddr = rhiDynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkGetInstanceProcAddr);

#ifdef NVRHI_WITH_VALIDATION
	g_consoleCommands->RegisterCommand(&vulkan_validation);
	g_consoleCommands->RegisterCommand(&vulkan_break_on_error);
#endif
	g_consoleCommands->RegisterCommand(&vulkan_fastSync);

	return true;
}

bool CNVRHIRenderLibVK::InitAPI(const ShaderAPIParams& params)
{
#ifdef NVRHI_WITH_VALIDATION
	const bool isDeviceValidationEnabled = (g_cmdLine->Find("-rhivalidation") != -1);
	if (isDeviceValidationEnabled)
	{
		vulkan_validation.SetBool(true);
		nvrhi_validation.SetBool(true);
		nvrhi_breakOnError.SetBool(true);
	}
#endif

	{
		m_enabledExtensions.instance.append(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		m_enabledExtensions.device.append(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		m_enabledExtensions.device.append(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
		m_enabledExtensions.device.append(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
#if defined(__APPLE__) && defined( VK_KHR_portability_subset )
		// This is required for using the MoltenVK portability subset implementation on macOS
		m_enabledExtensions.device.append(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif
		
#ifdef VK_USE_PLATFORM_WIN32_KHR
		m_enabledExtensions.instance.append(VK_KHR_SURFACE_EXTENSION_NAME);
		m_enabledExtensions.instance.append(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VULKAN_USE_PLATFORM_SDL)
		const RenderWindowInfo& windowInfo = params.windowInfo;
		if(windowInfo.parent && windowInfo.parent->windowType == RHI_WINDOW_HANDLE_SDL)
		{
			Array<const char*>& instanceExts = *(Array<const char*>*)windowInfo.parent->get(RenderWindowInfo::EXTENSIONS);
			for(const char* ext : instanceExts )
				m_enabledExtensions.instance.insert(ext);
		}
#endif
	}

	{

#if defined(__APPLE__)
#if defined( VK_KHR_portability_enumeration )
		// needed for enumerating the MoltenVK portability implementation on macOS
		m_optionalExtensions.instance.append(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
		m_optionalExtensions.instance.append(VK_EXT_LAYER_SETTINGS_EXTENSION_NAME);
#endif
		m_optionalExtensions.instance.append(VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME);
		m_optionalExtensions.instance.append(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		m_optionalExtensions.instance.append(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
        m_optionalExtensions.instance.append(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		m_optionalExtensions.device.append(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		m_optionalExtensions.device.append(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
		m_optionalExtensions.device.append(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
		m_optionalExtensions.device.append(VK_NV_MESH_SHADER_EXTENSION_NAME);
		m_optionalExtensions.device.append(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
#if USE_OPTICK
		m_optionalExtensions.device.append(VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME);
#endif
#if defined( VK_KHR_format_feature_flags2 )
		m_optionalExtensions.device.append(VK_KHR_FORMAT_FEATURE_FLAGS_2_EXTENSION_NAME);
#endif
		m_optionalExtensions.device.append(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
		m_optionalExtensions.device.append(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
	}

#ifdef NVRHI_WITH_VALIDATION
	if (vulkan_validation.GetBool())
	{
		m_enabledExtensions.layers.append("VK_LAYER_KHRONOS_validation");

		/*
		Suppress specific [ WARNING-Shader-OutputNotConsumed ] validation warnings which are by design:
		0xc81ad50e: vkCreateGraphicsPipelines(): pCreateInfos[0].pVertexInputState Vertex attribute at location X not consumed by vertex shader.
		0x9805298c: vkCreateGraphicsPipelines(): pCreateInfos[0] fragment shader writes to output location X with no matching attachment.
		Suppress similar [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ] warnings for older Vulkan SDKs:
		0x609a13b: vertex shader writes to output location X.0 which is not consumed by fragment shader...
		0x609a13b: Vertex attribute at location X not consumed by vertex shader.
		0x609a13b: fragment shader writes to output location X with no matching attachment.
		0x46877e3e: Inside the fragment shader, it writes to output Location 0 but there is no VkRenderingInfo::pColorAttachments[0] and this write is unused.

		Suppress image view compatibility 2D <-> 2D Array
		0x6174abc7: the sampled image descriptor VkImageViewType is VK_IMAGE_VIEW_TYPE_2D_ARRAY but the OpTypeImage has (Dim = 2D) and (Arrayed = 0)
		0x1c95b84c: the sampled image descriptor VkImageViewType is VK_IMAGE_VIEW_TYPE_2D but the OpTypeImage has (Dim = 2D) and (Arrayed = 1)

		Suppress texture loading transfer messages
		0x46582f7b: command buffer VkCommandBuffer expects VkImage to be in layout VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL--instead, current layout is VK_IMAGE_LAYOUT_UNDEFINED.
		
		Suppress RGBA/BRGA validation warning for now
		0x13365b2: the sampled image descriptor is accessed by a OpTypeImage that has a Format operand Rgba8 (equivalent to VK_FORMAT_R8G8B8A8_UNORM) which doesn't match the VkImageView format (VK_FORMAT_B8G8R8A8_UNORM)

		Suppress false-positive descriptor count warning for MipMapGen pass which is by design:
		0x3af3126a: vkCreateComputePipelines(): pCreateInfos[0].stage uses ... with a descriptorCount of X, but requires at least Y in the SPIR-V.
		*/

		EqStringRef suppressedMsgs[] = { "0x6174abc7", "0x1c95b84c", "0xc81ad50e", "0x9805298c", "0x609a13b", "0x46877e3e", "0x3af3126a", "0x13365b2", "0x46582f7b"};
#ifdef _WIN32
#define FMT_ENV_DELIM ";"
#else
#define FMT_ENV_DELIM ":"
#endif

		EqString vkIdsStr;
		bool first = true;
		for (EqStringRef msgId : ArrayCRef(suppressedMsgs))
		{
			vkIdsStr.AppendFmt("%s%s", first ? "" : FMT_ENV_DELIM, msgId);
			first = false;
		}
#ifdef _WIN32
		SetEnvironmentVariableA("VK_LAYER_MESSAGE_ID_FILTER", vkIdsStr.ToCString());
#else
		setenv("VK_LAYER_MESSAGE_ID_FILTER", vkIdsStr.ToCString(), 1);
#endif
#undef FMT_ENV_DELIM
	}
#endif

	Array<const char*> vkInstExtNames(PP_SL);
	Array<const char*> vkLayerNames(PP_SL);

	// create instance
	{
		{
			Array<EqString> requiredExtensions = m_enabledExtensions.instance;

			// figure out which optional extensions are supported
			for (const auto& instanceExt : vk::enumerateInstanceExtensionProperties())
			{
				const EqString name = instanceExt.extensionName.data();
				if (arrayFindIndex(m_optionalExtensions.instance, name) != -1)
					m_enabledExtensions.instance.append(name);

				requiredExtensions.remove(name);
			}

			if (!requiredExtensions.isEmpty())
			{
				EqString errorStr;
				errorStr.Append("Cannot create a Vulkan instance because the following required extension(s) are not supported:");
				for (const auto& ext : requiredExtensions)
					errorStr.AppendFmt("  - %s\n", ext);

				MsgError(errorStr);
				return false;
			}

			DevMsg(DEVMSG_RENDER, "Enabled Vulkan instance extensions:\n");
			for (const auto& ext : m_enabledExtensions.instance)
				DevMsg(DEVMSG_RENDER, "    %s\n", ext.ToCString());

			Array<EqString> requiredLayers = m_enabledExtensions.layers;

			auto instanceVersion = vk::enumerateInstanceVersion();
			for (const auto& layer : vk::enumerateInstanceLayerProperties())
			{
				const EqString name = layer.layerName.data();
				if (arrayFindIndex(m_optionalExtensions.layers, name) != -1)
					m_enabledExtensions.layers.append(name);

				requiredLayers.remove(name);
			}

			if (!requiredLayers.isEmpty())
			{
				EqString errorStr;
				errorStr.Append("Cannot create a Vulkan instance because the following required layer(s) are not supported:\n");
				for (const auto& ext : requiredLayers)
					errorStr.AppendFmt("  - %s\n", ext);

				MsgError(errorStr);
				return false;
			}

			DevMsg(DEVMSG_RENDER, "Enabled Vulkan layers:\n");
			for (const auto& layer : m_enabledExtensions.layers)
				DevMsg(DEVMSG_RENDER, "    %s\n", layer);
		}

		for (const char* ext : m_enabledExtensions.instance)
			vkInstExtNames.append(ext);

		for (const char* ext : m_enabledExtensions.layers)
			vkLayerNames.append(ext);

		auto applicationInfo = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_VERSION(1, 2, 0))
			.setPApplicationName(g_eqCore->GetApplicationName())
			.setPEngineName("eq2");

		// create the vulkan instance
		vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
			.setEnabledLayerCount(vkLayerNames.numElem())
			.setPpEnabledLayerNames(vkLayerNames.ptr())
			.setEnabledExtensionCount(vkInstExtNames.numElem())
			.setPpEnabledExtensionNames(vkInstExtNames.ptr())
			.setPApplicationInfo(&applicationInfo);

		const vk::Result res = vk::createInstance(&info, nullptr, &m_vkInstance);
		if (res != vk::Result::eSuccess)
		{
			MsgError("Failed to create a Vulkan instance, error code = %s\n", nvrhi::vulkan::resultToString((VkResult)res));
			return false;
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(m_vkInstance);
	}

#ifdef NVRHI_WITH_VALIDATION
	if (vulkan_validation.GetBool())
	{
		auto info = vk::DebugReportCallbackCreateInfoEXT()
			.setFlags(vk::DebugReportFlagBitsEXT::eError |
				vk::DebugReportFlagBitsEXT::eWarning |
				//   vk::DebugReportFlagBitsEXT::eInformation |
				vk::DebugReportFlagBitsEXT::ePerformanceWarning)
#if VK_HEADER_VERSION >= 304
			.setPfnCallback(vulkanDebugCallback)
#else
			.setPfnCallback(reinterpret_cast<PFN_vkDebugReportCallbackEXT>(vulkanDebugCallback))
#endif
			.setPUserData(this);

		const vk::Result res = m_vkInstance.createDebugReportCallbackEXT(&info, nullptr, &m_vkDebugReportCallback);
		ASSERT_MSG(res == vk::Result::eSuccess, "createDebugReportCallbackEXT failed");
	}
#endif

	{
		// fill default supposedly supported ShaderAPI capabilities
		ShaderAPICapabilities& caps = CNVRHIRenderAPI::Instance.m_caps;
		caps.minUniformBufferOffsetAlignment = nvrhi::c_ConstantBufferOffsetSizeAlignment;
		caps.minStorageBufferOffsetAlignment = 256;
		caps.maxDynamicUniformBuffersPerPipelineLayout = 1000;
		caps.maxDynamicStorageBuffersPerPipelineLayout = 1000;
		caps.maxVertexStreams = MAX_VERTEXSTREAM;
		caps.maxVertexAttributes = MAX_GENERIC_ATTRIB * 4;
		caps.maxTextureSize = 32768;
		caps.maxTextureArrayLayers = 1024;
		caps.maxTextureUnits = MAX_TEXTUREUNIT;
		caps.maxVertexTextureUnits = MAX_TEXTUREUNIT;
		caps.maxBindGroups = MAX_BINDGROUPS;
		caps.maxBindingsPerBindGroup = 1000;
		caps.maxTextureAnisotropicLevel = 16;
		caps.maxRenderTargets = MAX_RENDERTARGETS;

		caps.maxComputeInvocationsPerWorkgroup = 1024;
		caps.maxComputeWorkgroupSizeX = 1024;
		caps.maxComputeWorkgroupSizeY = 1024;
		caps.maxComputeWorkgroupSizeZ = 64;
		caps.maxComputeWorkgroupsPerDimension = 65535;
		caps.multiDrawIndirectSupport = false;	// NVRHI doesn't support this currently

		caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED
			| SHADER_CAPS_PIXEL_SUPPORTED
			| SHADER_CAPS_COMPUTE_SUPPORTED;

		for (int i = FORMAT_R8; i <= FORMAT_RGBA32F; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}

		for (int i = FORMAT_D16; i <= FORMAT_D32F; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}

		for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
			caps.textureFormatsSupported[i] = true;

		caps.textureFormatsSupported[FORMAT_ATI1N] = false;
	}

	// create default swap chain
	m_defaultSwapChain = CRefPtr<CNVRHISwapChainVK>(static_cast<CNVRHISwapChainVK*>(CreateSwapChain(params.windowInfo).Ptr()));
	if (!m_defaultSwapChain)
		return false;

	if(!PickPhysicalDevice())
		return false;

	if(!FindQueueFamilies(m_vkPhysicalDevice, m_defaultSwapChain->m_vkWindowSurface))
		return false;

	if(!CreateDevice())
		return false;

	// init device swapchain capabilities
	{
		auto surfaceCaps = m_vkPhysicalDevice.getSurfaceCapabilitiesKHR(m_defaultSwapChain->m_vkWindowSurface);
		uint32 swapBufferCount = SWAP_CHAIN_BUFFERS;
		swapBufferCount = max(surfaceCaps.minImageCount, swapBufferCount);
		swapBufferCount = surfaceCaps.maxImageCount > 0 ? min(swapBufferCount, surfaceCaps.maxImageCount) : swapBufferCount;
		m_swapChainBufferCount = swapBufferCount;
	}

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &CNVRHIMessageCallback::Instance;
	deviceDesc.instance = m_vkInstance;
	deviceDesc.physicalDevice = m_vkPhysicalDevice;
	deviceDesc.device = m_vkDevice;
	deviceDesc.graphicsQueue = m_vkGraphicsQueue;
	deviceDesc.graphicsQueueIndex = m_vkGraphicsQueueFamily;
	if (m_vkComputeQueueFamily != -1)
	{
		deviceDesc.computeQueue = m_vkComputeQueue;
		deviceDesc.computeQueueIndex = m_vkComputeQueueFamily;
	}
	if (m_vkTransferQueueFamily != -1)
	{
		deviceDesc.transferQueue = m_vkTransferQueue;
		deviceDesc.transferQueueIndex = m_vkTransferQueueFamily;
	}
	deviceDesc.instanceExtensions = vkLayerNames.ptr();
	deviceDesc.numInstanceExtensions = vkLayerNames.numElem();
	deviceDesc.deviceExtensions = vkInstExtNames.ptr();
	deviceDesc.numDeviceExtensions = vkInstExtNames.numElem();

	m_nvrhiDevice = nvrhi::vulkan::createDevice(deviceDesc);
#ifdef NVRHI_WITH_VALIDATION
	if (nvrhi_validation.GetBool())
	{
		CNVRHIRenderAPI::Instance.m_rhiDevice = nvrhi::validation::createValidationLayer(m_nvrhiDevice);
	}
	else
#endif
	{
		CNVRHIRenderAPI::Instance.m_rhiDevice = m_nvrhiDevice;
	}
	constexpr int jobQueueSize = 1024;

	g_renderWorker.Init(this, nullptr, jobQueueSize);

	m_nvrhiFrameWaitQuery = m_nvrhiDevice->createEventQuery();
	m_nvrhiDevice->setEventQuery(m_nvrhiFrameWaitQuery, nvrhi::CommandQueue::Graphics);

	return true;
}

bool CNVRHIRenderLibVK::PickPhysicalDevice()
{
	int width, height;
	m_defaultSwapChain->GetBackbufferSize(width, height);

	const vk::Format vkRequestedFormat = vk::Format(nvrhi::vulkan::convertFormat(m_defaultSwapChain->m_swapChainFormat));
	const vk::Extent2D vkRequestedExtent(width, height);

	auto devices = m_vkInstance.enumeratePhysicalDevices();

	// Start building an error message in case we cannot find a device.
	EqString errorStream;
	errorStream.Append("Cannot find suitable Vulkan device that supports required extensions & properties.\n");

	// build a list of GPUs
	Array<vk::PhysicalDevice> discreteGPUs(PP_SL);
	Array<vk::PhysicalDevice> otherGPUs(PP_SL);
	for (const auto& dev : devices)
	{
		auto prop = dev.getProperties();
		errorStream.AppendFmt("%s:\n", prop.deviceName.data());

		// check that all required device extensions are present
		Array<EqString> requiredExtensions = m_enabledExtensions.device;
		auto deviceExtensions = dev.enumerateDeviceExtensionProperties();
		for (const auto& ext : deviceExtensions)
		{
			const int idx = arrayFindIndex(requiredExtensions, ext.extensionName.data());
			if(idx != -1)
				requiredExtensions.fastRemoveIndex(idx);
		}

		bool deviceIsGood = true;

		if (!requiredExtensions.isEmpty())
		{
			// device is missing one or more required extensions
			for (const auto& ext : requiredExtensions)
				errorStream.AppendFmt("- missing ext %s:\n", ext);

			deviceIsGood = false;
		}

		auto deviceFeatures = dev.getFeatures();
		if (!deviceFeatures.samplerAnisotropy)
		{
			errorStream.Append("  - samplerAnisotropy unsupported\n");
			deviceIsGood = false;
		}
		if (!deviceFeatures.textureCompressionBC)
		{
			errorStream.Append("  - textureCompressionBC unsupported \n");
			deviceIsGood = false;
		}

		// check that this device supports our intended swap chain creation parameters
		auto surfaceFmts = dev.getSurfaceFormatsKHR(m_defaultSwapChain->m_vkWindowSurface);
		auto surfacePModes = dev.getSurfacePresentModesKHR(m_defaultSwapChain->m_vkWindowSurface);

		bool surfaceFormatPresent = false;
		for (const vk::SurfaceFormatKHR& surfaceFmt : surfaceFmts)
		{
			if (surfaceFmt.format == vkRequestedFormat)
			{
				surfaceFormatPresent = true;
				break;
			}
		}

		if (!surfaceFormatPresent)
		{
			errorStream.Append("  - does not support the requested swap chain format\n");
			deviceIsGood = false;
		}

		if (find(surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifo) == surfacePModes.end())
		{
			errorStream.Append("  - does not support the required surface present modes\n");
			deviceIsGood = false;
		}

		if (!FindQueueFamilies(dev, m_defaultSwapChain->m_vkWindowSurface))
		{
			errorStream.Append("  - does not support the necessary queue types\n");
			deviceIsGood = false;
		}

		// check that we can present from the graphics queue
		uint32_t canPresent = dev.getSurfaceSupportKHR(m_vkGraphicsQueueFamily, m_defaultSwapChain->m_vkWindowSurface);
		if (!canPresent)
		{
			errorStream.Append("  - surface cannot present\n");
			deviceIsGood = false;
		}

		if (!deviceIsGood)
			continue;

		if (prop.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
			discreteGPUs.append(dev);
		else
			otherGPUs.append(dev);
	}

	// pick the first discrete GPU if it exists, otherwise the first integrated GPU
	if (!discreteGPUs.isEmpty())
	{
		m_vkPhysicalDevice = discreteGPUs[0];
		return true;
	}

	if (!otherGPUs.isEmpty())
	{
		m_vkPhysicalDevice = otherGPUs[0];
		return true;
	}

	MsgError("%s", errorStream.ToCString());

	return false;
}

bool CNVRHIRenderLibVK::FindQueueFamilies(vk::PhysicalDevice vkPhysicalDevice, vk::SurfaceKHR vkSurface)
{
	// TODO: ShaderAPIParams
	const bool enableComputeQueue = false;
	const bool enableCopyQueue = false;

	auto queueFamiliyProps = vkPhysicalDevice.getQueueFamilyProperties();

	for (int i = 0; i < int(queueFamiliyProps.size()); i++)
	{
		const auto& queueFamily = queueFamiliyProps[i];

		if (m_vkGraphicsQueueFamily == -1)
		{
			if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
			{
				m_vkGraphicsQueueFamily = i;
			}
		}

		if (m_vkComputeQueueFamily == -1 && enableComputeQueue)
		{
			if (queueFamily.queueCount > 0 &&
				(queueFamily.queueFlags & vk::QueueFlagBits::eCompute) && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics))
			{
				m_vkComputeQueueFamily = i;
			}
		}

		if (m_vkTransferQueueFamily == -1 && enableCopyQueue)
		{
			if (queueFamily.queueCount > 0 &&
				(queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) && !(queueFamily.queueFlags & (vk::QueueFlagBits::eCompute | vk::QueueFlagBits::eGraphics)))
			{
				m_vkTransferQueueFamily = i;
			}
		}

		if (m_vkPresentQueueFamily == -1 && queueFamily.queueCount > 0)
		{
			vk::Bool32 presentSupported;

			// Use portable implmentation for detecting presentation support vs. Windows-specific Vulkan call
			if (vkPhysicalDevice.getSurfaceSupportKHR(i, vkSurface, &presentSupported) == vk::Result::eSuccess && presentSupported)
			{
				m_vkPresentQueueFamily = i;
			}
		}
	}

	if (m_vkGraphicsQueueFamily == -1 || m_vkPresentQueueFamily == -1)
		return false;

	return true;
}

bool CNVRHIRenderLibVK::CreateDevice()
{
	ASSERT_MSG(m_vkPhysicalDevice, "No Vulkan physical device");

	// figure out which optional extensions are supported
	auto deviceExtensions = m_vkPhysicalDevice.enumerateDeviceExtensionProperties();
	for( const auto& ext : deviceExtensions )
	{
		const EqStringRef name = ext.extensionName.data();
		if(arrayFindIndex(m_optionalExtensions.device, name) == -1 )
			m_optionalExtensions.device.insert( name );
	}

	bool accelStructSupported = false;
	bool bufferAddressSupported = false;
	bool rayPipelineSupported = false;
	bool rayQuerySupported = false;
	bool meshletsSupported = false;
	bool vrsSupported = false;
	bool sync2Supported = false;
	bool dynamicRenderingSupported = false;

	DevMsg(DEVMSG_RENDER, "Enabled Vulkan device extensions:\n");
	for( const auto& ext : m_enabledExtensions.device )
	{
		DevMsg(DEVMSG_RENDER, "    %s\n", ext.ToCString() );

		if( ext == VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME )
			accelStructSupported = true;
		else if( ext == VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME )
			bufferAddressSupported = true;
		else if( ext == VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME )
			rayPipelineSupported = true;
		else if( ext == VK_KHR_RAY_QUERY_EXTENSION_NAME )
			rayQuerySupported = true;
		else if( ext == VK_NV_MESH_SHADER_EXTENSION_NAME )
			meshletsSupported = true;
		else if( ext == VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME )
			vrsSupported = true;
		else if( ext == VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME )
			sync2Supported = true;
		else if (ext == VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
			dynamicRenderingSupported = true;
		else if( ext == VK_GOOGLE_DISPLAY_TIMING_EXTENSION_NAME )
			m_displayTimingEnabled = true;
	}

	Set<int> uniqueQueueFamilies(PP_SL);
	uniqueQueueFamilies.insert(m_vkGraphicsQueueFamily);
	uniqueQueueFamilies.insert(m_vkPresentQueueFamily);

	if(m_vkComputeQueueFamily != -1)
		uniqueQueueFamilies.insert( m_vkComputeQueueFamily );

	if(m_vkTransferQueueFamily != -1)
		uniqueQueueFamilies.insert( m_vkTransferQueueFamily );

	float priority = 1.0f;
	Array<vk::DeviceQueueCreateInfo> queueDesc(PP_SL);
	for( auto it = uniqueQueueFamilies.begin(); it; ++it)
	{
		queueDesc.append( vk::DeviceQueueCreateInfo()
							 .setQueueFamilyIndex( it.key() )
							 .setQueueCount( 1 )
							 .setPQueuePriorities( &priority ) );
	}

	auto accelStructFeatures = vk::PhysicalDeviceAccelerationStructureFeaturesKHR()
							   .setAccelerationStructure( true );
	auto rayPipelineFeatures = vk::PhysicalDeviceRayTracingPipelineFeaturesKHR()
							   .setRayTracingPipeline( true )
							   .setRayTraversalPrimitiveCulling( true );
	auto rayQueryFeatures = vk::PhysicalDeviceRayQueryFeaturesKHR()
							.setRayQuery( true );
	auto meshletFeatures = vk::PhysicalDeviceMeshShaderFeaturesNV()
						   .setTaskShader( true )
						   .setMeshShader( true );

	// get/set shading rate features which are detected individually by nvrhi (not just at extension level)
	vk::PhysicalDeviceFeatures2 actualDeviceFeatures2;
	vk::PhysicalDeviceFragmentShadingRateFeaturesKHR fragmentShadingRateFeatures;
	actualDeviceFeatures2.pNext = &fragmentShadingRateFeatures;
	m_vkPhysicalDevice.getFeatures2( &actualDeviceFeatures2 );

	auto vrsFeatures = vk::PhysicalDeviceFragmentShadingRateFeaturesKHR()
					   .setPipelineFragmentShadingRate( fragmentShadingRateFeatures.pipelineFragmentShadingRate )
					   .setPrimitiveFragmentShadingRate( fragmentShadingRateFeatures.primitiveFragmentShadingRate )
					   .setAttachmentFragmentShadingRate( fragmentShadingRateFeatures.attachmentFragmentShadingRate );

	auto sync2Features = vk::PhysicalDeviceSynchronization2FeaturesKHR()
						.setSynchronization2( true );

	auto dynamicRenderingFeatures = vk::PhysicalDeviceDynamicRenderingFeatures()
						.setDynamicRendering(true);

#if defined(__APPLE__) && defined( VK_KHR_portability_subset )
	auto portabilityFeatures = vk::PhysicalDevicePortabilitySubsetFeaturesKHR()
#if USE_OPTICK
							   .setEvents( true )
#endif
							   .setImageViewFormatSwizzle( true );

	void* pNext = &portabilityFeatures;
#else
	void* pNext = nullptr;
#endif
#define APPEND_EXTENSION(condition, desc) \
	if (condition) { (desc).pNext = pNext; pNext = &(desc); }

	APPEND_EXTENSION( accelStructSupported, accelStructFeatures )
	APPEND_EXTENSION( rayPipelineSupported, rayPipelineFeatures )
	APPEND_EXTENSION( rayQuerySupported, rayQueryFeatures )
	APPEND_EXTENSION( meshletsSupported, meshletFeatures )
	APPEND_EXTENSION( vrsSupported, vrsFeatures )
	APPEND_EXTENSION( sync2Supported, sync2Features )
	APPEND_EXTENSION( dynamicRenderingSupported, dynamicRenderingFeatures )
#undef APPEND_EXTENSION

	auto deviceFeatures = vk::PhysicalDeviceFeatures()
						  .setShaderImageGatherExtended( true )
						  .setShaderStorageImageReadWithoutFormat( actualDeviceFeatures2.features.shaderStorageImageReadWithoutFormat )
						  .setSamplerAnisotropy( true )
						  .setTessellationShader( true )
						  .setTextureCompressionBC( true )
#if !defined(__APPLE__)
						  .setGeometryShader( true )
#endif
						  .setFillModeNonSolid( true )
						  .setImageCubeArray( true )
						  .setDualSrcBlend( true );

	auto vulkan12features = vk::PhysicalDeviceVulkan12Features()
							.setDescriptorIndexing( true )
							.setRuntimeDescriptorArray( true )
							.setDescriptorBindingPartiallyBound( true )
							.setDescriptorBindingVariableDescriptorCount( true )
							.setTimelineSemaphore( true )
							.setShaderSampledImageArrayNonUniformIndexing( true )
							.setBufferDeviceAddress( bufferAddressSupported )
							.setPNext( pNext );

	Array<const char*> vkDevExtNames(PP_SL);
	Array<const char*> vkLayerNames(PP_SL);

	for (const char* ext : m_enabledExtensions.device)
		vkDevExtNames.append(ext);

	for (const char* ext : m_enabledExtensions.layers)
		vkLayerNames.append(ext);

	auto deviceDesc = vk::DeviceCreateInfo()
		.setPQueueCreateInfos(queueDesc.ptr())
		.setQueueCreateInfoCount( queueDesc.numElem() )
		.setPEnabledFeatures( &deviceFeatures )
		.setEnabledExtensionCount( vkDevExtNames.numElem() )
		.setPpEnabledExtensionNames(vkDevExtNames.ptr() )
		.setEnabledLayerCount( vkLayerNames.numElem() )
		.setPpEnabledLayerNames(vkLayerNames.ptr() )
		.setPNext( &vulkan12features );

	const vk::Result res = m_vkPhysicalDevice.createDevice( &deviceDesc, nullptr, &m_vkDevice );
	if( res != vk::Result::eSuccess )
	{
		MsgError( "Failed to create a Vulkan physical device, error code = %s\n", nvrhi::vulkan::resultToString( ( VkResult )res ) );
		return false;
	}

	m_vkDevice.getQueue( m_vkGraphicsQueueFamily, 0, &m_vkGraphicsQueue );
	if(m_vkComputeQueueFamily != -1)
		m_vkDevice.getQueue( m_vkComputeQueueFamily, 0, &m_vkComputeQueue );

	if(m_vkTransferQueueFamily != -1)
		m_vkDevice.getQueue( m_vkTransferQueueFamily, 0, &m_vkTransferQueue );

	m_vkDevice.getQueue( m_vkPresentQueueFamily, 0, &m_vkPresentQueue );

	VULKAN_HPP_DEFAULT_DISPATCHER.init( m_vkDevice );

	// Determine if preferred image depth/stencil format D24S8 is supported (issue with Vulkan on AMD GPUs)
	vk::ImageFormatProperties imageFormatProperties;
	const vk::Result ret = m_vkPhysicalDevice.getImageFormatProperties( vk::Format::eD24UnormS8Uint,
						   vk::ImageType::e2D,
						   vk::ImageTiling::eOptimal,
						   vk::ImageUsageFlags( vk::ImageUsageFlagBits::eDepthStencilAttachment ),
						   vk::ImageCreateFlags( 0 ),
						   &imageFormatProperties );

	ShaderAPICapabilities& caps = CNVRHIRenderAPI::Instance.m_caps;
	caps.renderTargetFormatsSupported[FORMAT_D24S8] = 
		caps.renderTargetFormatsSupported[FORMAT_D24] = (ret == vk::Result::eSuccess);

	// Determine which Vulkan surface present modes are supported by device and surface
	auto surfacePModes = m_vkPhysicalDevice.getSurfacePresentModesKHR( m_defaultSwapChain->m_vkWindowSurface );
	m_enablePModeMailbox = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eMailbox ) != surfacePModes.end();
	m_enablePModeImmediate = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eImmediate ) != surfacePModes.end();
	m_enablePModeFifoRelaxed = find( surfacePModes.begin(), surfacePModes.end(), vk::PresentModeKHR::eFifoRelaxed ) != surfacePModes.end();

	// stash the device renderer string and api version
	auto prop = m_vkPhysicalDevice.getProperties();
	Msg("* NVRHI Vulkan Adapter: %s\n", prop.deviceName.data());

	m_vkApiVersion = prop.apiVersion;

	DevMsg(DEVMSG_RENDER, "Created Vulkan device: %s\n", prop.deviceName.data());

	return true;
}

void CNVRHIRenderLibVK::ExitAPI()
{
	g_renderWorker.Shutdown();

	//if (m_defaultSwapChain)
	//	m_defaultSwapChain->m_dxgiSwapChain->SetFullscreenState(false, nullptr);

	if (m_nvrhiDevice)
	{
		m_nvrhiDevice->waitForIdle();
		m_nvrhiDevice->runGarbageCollection();
	}

	if (m_vkDevice)
		m_vkDevice.waitIdle();

	CNVRHIRenderAPI::Instance.m_rhiDevice = nullptr;
	m_nvrhiDevice = nullptr;
	m_nvrhiFrameWaitQuery = nullptr;

	if (m_vkDebugReportCallback)
		m_vkInstance.destroyDebugReportCallbackEXT(m_vkDebugReportCallback);

	m_defaultSwapChain = nullptr;
	m_currentSwapChain = nullptr;

	if (m_vkDevice)
	{
		m_vkDevice.destroy();
		m_vkDevice = nullptr;
	}

#ifdef NVRHI_WITH_VALIDATION
	g_consoleCommands->UnregisterCommand(&vulkan_validation);
	g_consoleCommands->UnregisterCommand(&vulkan_break_on_error);
#endif
	g_consoleCommands->UnregisterCommand(&vulkan_fastSync);
}

void CNVRHIRenderLibVK::BeginFrame(ISwapChain* swapChain)
{
	CNVRHIRenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain.Assign(swapChain ? static_cast<CNVRHISwapChainVK*>(swapChain) : m_defaultSwapChain);

	// must obtain valid texture view upon Present
	g_renderWorker.WaitForExecute(__func__, [this]() {
		m_currentSwapChain->UpdateResize();
		m_currentSwapChain->UpdateBackbufferView();
		return 0;
	});

	g_renderWorker.Execute(__func__, [this]() {
		m_nvrhiDevice->runGarbageCollection();
		return 0;
	});
}

void CNVRHIRenderLibVK::EndFrame()
{
	ShaderAPIStats& stats = CNVRHIRenderAPI::Instance.GetStatsMutable();
	stats.drawCount = 0;
	stats.dispatchCount = 0;
	stats.indirectDrawCount = 0;
	stats.indirectDispatchCount = 0;
	stats.bufferUpdateCount = 0;
	stats.textureUpdateCount = 0;

	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();

		const int bufferCount = 3;// m_currentSwapChain->m_rhiSwapChainTextures.numElem();
		if (bufferCount > 2)
		{
			// sync on previous frame's command queue completion
			m_nvrhiDevice->waitEventQuery(m_nvrhiFrameWaitQuery);
		}

		m_nvrhiDevice->resetEventQuery(m_nvrhiFrameWaitQuery);
		m_nvrhiDevice->setEventQuery(m_nvrhiFrameWaitQuery, nvrhi::CommandQueue::Graphics);

		if (bufferCount < 3)
		{
			// sync on current frame's command queue completion for double buffering
			m_nvrhiDevice->waitEventQuery(m_nvrhiFrameWaitQuery);
		}
		return 0;
	});
}

ITexturePtr	CNVRHIRenderLibVK::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChainPtr CNVRHIRenderLibVK::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	EqStringRef texName = EqString::Format("swapChain%d", m_swapChainCounter);
	++m_swapChainCounter;

	bool justCreated = false;
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CRefPtr<CNVRHISwapChainVK> swapChain = CRefPtr_new(CNVRHISwapChainVK, windowInfo, swapChainTexture);
	{
		// Create the platform-specific surface
#if defined( VULKAN_USE_PLATFORM_SDL )
		VkResult surfaceCreateRes = VkResult::VK_ERROR_SURFACE_LOST_KHR;

		// Support generic SDL platform for linux and macOS
		if(windowInfo.parent && windowInfo.parent->windowType == RHI_WINDOW_HANDLE_SDL)
		{
			swapChain->m_vkWindowSurface = (vk::SurfaceKHR)windowInfo.parent->get(RenderWindowInfo::SURFACE, m_vkInstance);
			if(swapChain->m_vkWindowSurface)
			{
				surfaceCreateRes = VkResult::VK_SUCCESS;
			}
		}
		else
		{
			if(windowInfo.parent)
				MsgError("ERROR - Not supported window type %d of parent\n", windowInfo.parent->windowType);
			else
				MsgError("ERROR - Not supported window type %d\n", windowInfo.windowType);
			ASSERT_FAIL("Not supported window type");
		}
		auto res = vk::Result(surfaceCreateRes);

#elif defined( VK_USE_PLATFORM_WIN32_KHR )
		auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
			.setHinstance((HINSTANCE)windowInfo.get(RenderWindowInfo::TOPLEVEL))
			.setHwnd((HWND)windowInfo.get(RenderWindowInfo::WINDOW));

		auto res = m_vkInstance.createWin32SurfaceKHR(&surfaceCreateInfo, nullptr, &swapChain->m_vkWindowSurface);
#endif

		if (res != vk::Result::eSuccess)
		{
			MsgError("Failed to create a Vulkan window surface, error code = %s\n", nvrhi::vulkan::resultToString((VkResult)res));
			return nullptr;
		}
	}

	return ISwapChainPtr(swapChain);
}

void CNVRHIRenderLibVK::SetVSync(bool enable)
{
	m_defaultSwapChain->SetVSync(enable);
}

void CNVRHIRenderLibVK::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_defaultSwapChain->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CNVRHIRenderAPI::Instance.m_deviceLost = true;

	m_defaultSwapChain->SetBackbufferSize(w, h);
}

// changes fullscreen mode
bool CNVRHIRenderLibVK::SetWindowed(bool enabled)
{
	// FIXME: currently switching to exclusive fullscreen will guarantee device lost
	// need to handle it somehow...
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CNVRHIRenderLibVK::IsWindowed() const
{
	return m_windowed;
}

bool CNVRHIRenderLibVK::CaptureScreenshot(CImage &img)
{
	return nvrhiCaptureBackbufferImage(m_currentSwapChain->GetBackbuffer(), img);
}
