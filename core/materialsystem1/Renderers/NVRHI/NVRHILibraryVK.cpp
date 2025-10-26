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
DECLARE_CVAR_F(nvrhi_validation);
DECLARE_CVAR_F(nvrhi_breakOnError);

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

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
		MsgError("VULKAN ERROR: location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eWarning)
	{
		MsgError("VULKAN WARNING: location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::ePerformanceWarning)
	{
		MsgWarning("VULKAN PERFORMANCE: location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eInformation)
	{
		MsgError("VULKANL location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg);
	}
	else if (flags & vk::DebugReportFlagBitsEXT::eDebug)
	{
		MsgError("VULKAN DEBUG: location=0x%zx code=%d, layerPrefix='%s'] %s\n", location, code, layerPrefix, msg);
	}

	return VK_FALSE;
}

CNVRHIRenderLibVK::CNVRHIRenderLibVK()
{
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
#endif

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

	Array<const char*> vkEnabledExts(PP_SL);
	Array<const char*> vkEnabledLayers(PP_SL);
#ifdef VK_USE_PLATFORM_WIN32_KHR
	vkEnabledExts.append(VK_KHR_SURFACE_EXTENSION_NAME);
	vkEnabledExts.append(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

#ifdef NVRHI_WITH_VALIDATION
	if (vulkan_validation.GetBool())
	{
		vkEnabledLayers.append("VK_LAYER_KHRONOS_validation");

		/*
		Suppress specific [ WARNING-Shader-OutputNotConsumed ] validation warnings which are by design:
		0xc81ad50e: vkCreateGraphicsPipelines(): pCreateInfos[0].pVertexInputState Vertex attribute at location X not consumed by vertex shader.
		0x9805298c: vkCreateGraphicsPipelines(): pCreateInfos[0] fragment shader writes to output location X with no matching attachment.
		Suppress similar [ UNASSIGNED-CoreValidation-Shader-OutputNotConsumed ] warnings for older Vulkan SDKs:
		0x609a13b: vertex shader writes to output location X.0 which is not consumed by fragment shader...
		0x609a13b: Vertex attribute at location X not consumed by vertex shader.
		0x609a13b: fragment shader writes to output location X with no matching attachment.

		Suppress false-positive descriptor count warning for MipMapGen pass which is by design:
		0x3af3126a: vkCreateComputePipelines(): pCreateInfos[0].stage uses ... with a descriptorCount of X, but requires at least Y in the SPIR-V.
		*/
#ifdef _WIN32
		//SetEnvironmentVariableA("VK_LAYER_MESSAGE_ID_FILTER", "0xc81ad50e;0x9805298c;0x609a13b;0x3af3126a");
#else
		setenv("VK_LAYER_MESSAGE_ID_FILTER", "0xc81ad50e:0x9805298c:0x609a13b:0x3af3126a", 1);
#endif
	}
#endif

	// create instance
	{
		MsgInfo("Enabled Vulkan instance extensions:\n");
		for (const char* ext : vkEnabledExts)
		{
			MsgInfo("    %s\n", ext);
		}

		auto applicationInfo = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_VERSION(1, 2, 0))
			.setPApplicationName(g_eqCore->GetApplicationName())
			.setPEngineName("eq2");

		// create the vulkan instance
		vk::InstanceCreateInfo info = vk::InstanceCreateInfo()
			.setEnabledLayerCount(vkEnabledLayers.numElem())
			.setPpEnabledLayerNames(vkEnabledLayers.ptr())
			.setEnabledExtensionCount(vkEnabledExts.numElem())
			.setPpEnabledExtensionNames(vkEnabledExts.ptr())
			.setPApplicationInfo(&applicationInfo);

		const vk::Result res = vk::createInstance(&info, nullptr, &m_vkInstance);
		if (res != vk::Result::eSuccess)
		{
			MsgError("Failed to create a Vulkan instance, error code = %s", nvrhi::vulkan::resultToString((VkResult)res));
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

	// create default swap chain
	m_defaultSwapChain = CRefPtr<CNVRHISwapChainVK>(static_cast<CNVRHISwapChainVK*>(CreateSwapChain(params.windowInfo).Ptr()));
	if (!m_defaultSwapChain)
		return false;

	// pick physical device
	{
		// TODO
	}

	nvrhi::vulkan::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &CNVRHIMessageCallback::Instance;
	/*
	deviceDesc.pDevice = m_rhiDevice12;
	deviceDesc.pAdapter = m_dxgiAdapter;
	deviceDesc.pGraphicsCommandQueue = m_rhiGraphicsQueue;
	deviceDesc.pComputeCommandQueue = m_rhiComputeQueue;
	deviceDesc.pCopyCommandQueue = m_rhiCopyQueue;
	*/
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

	{
		// fill ShaderAPI capabilities
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

		caps.textureFormatsSupported[FORMAT_D32F] =
			caps.renderTargetFormatsSupported[FORMAT_D32F] = true;

		for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
			caps.textureFormatsSupported[i] = true;

		caps.textureFormatsSupported[FORMAT_ATI1N] = false;
	}

	return true;
}

void CNVRHIRenderLibVK::ExitAPI()
{
	g_renderWorker.Shutdown();

	//if (m_defaultSwapChain)
	//	m_defaultSwapChain->m_dxgiSwapChain->SetFullscreenState(false, nullptr);

	m_nvrhiDevice->waitForIdle();
	m_nvrhiDevice->runGarbageCollection();

	m_defaultSwapChain = nullptr;
	m_currentSwapChain = nullptr;

	CNVRHIRenderAPI::Instance.m_rhiDevice = nullptr;
	m_nvrhiDevice = nullptr;
	m_nvrhiFrameWaitQuery = nullptr;
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
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CRefPtr<CNVRHISwapChainVK> swapChain = CRefPtr_new(CNVRHISwapChainVK, windowInfo, swapChainTexture);

	{
		
		// Create the platform-specific surface
#if defined( VULKAN_USE_PLATFORM_SDL )
		// Support generic SDL platform for linux and macOS
		auto res = vk::Result(CreateSDLWindowSurface((VkInstance)m_VulkanInstance, (VkSurfaceKHR*)&m_WindowSurface));

#elif defined( VK_USE_PLATFORM_WIN32_KHR )
		auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
			.setHinstance((HINSTANCE)windowInfo.get(windowInfo.userData, RenderWindowInfo::TOPLEVEL))
			.setHwnd((HWND)windowInfo.get(windowInfo.userData, RenderWindowInfo::WINDOW));

		auto res = m_vkInstance.createWin32SurfaceKHR(&surfaceCreateInfo, nullptr, &swapChain->m_vkWindowSurface);
#endif

		if (res != vk::Result::eSuccess)
		{
			MsgError("Failed to create a Vulkan window surface, error code = %s", nvrhi::vulkan::resultToString((VkResult)res));
			return nullptr;
		}
	}

	return ISwapChainPtr(swapChain);
}

void CNVRHIRenderLibVK::SetVSync(bool enable)
{
	m_swapChains[0]->SetVSync(enable);
}

void CNVRHIRenderLibVK::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_swapChains[0]->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CNVRHIRenderAPI::Instance.m_deviceLost = true;

	m_swapChains[0]->SetBackbufferSize(w, h);
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
