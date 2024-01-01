//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include <webgpu/webgpu.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "WGPUBackend.h"

#include "WGPULibrary.h"
#include "WGPURenderAPI.h"
#include "WGPUSwapChain.h"

IShaderAPI* g_renderAPI = &CWGPURenderAPI::Instance;

DECLARE_CVAR(wgpu_report_errors, "1", nullptr, 0);
DECLARE_CVAR(wgpu_break_on_error, "1", nullptr, 0);

static const char* s_wgpuErrorTypesStr[] = {
	"NoError",
	"Validation",
	"OutOfMemory",
	"Internal",
	"Unknown",
	"DeviceLost",
};

static const char* s_wgpuDeviceLostReasonStr[] = {
	"Undefined",
	"Destroyed",
};

static void OnWGPUDeviceError(WGPUErrorType type, const char* message, void*)
{
	if (wgpu_break_on_error.GetBool())
	{
		ASSERT_FAIL("WGPU device %s error:\n\n%s", s_wgpuErrorTypesStr[type], message);
	}

	if (wgpu_report_errors.GetBool())
		MsgError("[WGPU] %s - %s\n", s_wgpuErrorTypesStr[type], message);
}

static void OnWGPUDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata)
{
	ASSERT_FAIL("WGPU device lost reason %s\n\n%s", s_wgpuDeviceLostReasonStr[reason], message);
	MsgError("[WGPU] device lost reason %s, %s\n", s_wgpuDeviceLostReasonStr[reason], message);
}

static void OnWGPUAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, const char* message, void* userdata)
{
	if (status != WGPURequestAdapterStatus_Success)
	{
		// cannot find adapter?
		ErrorMsg("%s", message);
	}
	else
	{
		// use first adapter provided
		WGPUAdapter* result = static_cast<WGPUAdapter*>(userdata);
		if (*result == nullptr)
			*result = adapter;
	}
}

CWGPURenderLib::CWGPURenderLib()
{
	m_windowed = true;
}

CWGPURenderLib::~CWGPURenderLib()
{
}

bool CWGPURenderLib::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

	// optionally use WGPUInstanceDescriptor::nextInChain for WGPUDawnTogglesDescriptor
	// with various toggles enabled or disabled: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp

	m_instance = wgpuCreateInstance(nullptr);
	if (!m_instance)
		return false;

	return true;
}

IShaderAPI* CWGPURenderLib::GetRenderer() const
{
	return &CWGPURenderAPI::Instance;
}

bool CWGPURenderLib::InitAPI(const ShaderAPIParams& params)
{
	WGPUAdapter adapter = nullptr;
	WGPURequestAdapterOptions options{};
	options.powerPreference = WGPUPowerPreference_HighPerformance;
	//options.compatibleSurface = surface;
	wgpuInstanceRequestAdapter(m_instance, &options, &OnWGPUAdapterRequestEnded, &adapter);

	if (!adapter)
	{
		MsgError("No WGPU supported adapter found\n");
		return false;
	}

	{
		WGPUAdapterProperties properties = {};
		wgpuAdapterGetProperties(adapter, &properties);

		Msg("WGPU adapter: %s %s %s\n", properties.name, properties.driverDescription, properties.vendorName);
	}

	{
		WGPUSupportedLimits supLimits = {};
		wgpuAdapterGetLimits(adapter, &supLimits);

		WGPULimits requiredLimits = supLimits.limits;

		// fill ShaderAPI capabilities
		ShaderAPICapabilities& caps = CWGPURenderAPI::Instance.m_caps;
		caps.isInstancingSupported = true;
		caps.isHardwareOcclusionQuerySupported = true;
		caps.minUniformBufferOffsetAlignment = supLimits.limits.minUniformBufferOffsetAlignment;
		caps.minStorageBufferOffsetAlignment = supLimits.limits.minStorageBufferOffsetAlignment;
		caps.maxDynamicUniformBuffersPerPipelineLayout = supLimits.limits.maxDynamicUniformBuffersPerPipelineLayout;
		caps.maxDynamicStorageBuffersPerPipelineLayout = supLimits.limits.maxDynamicStorageBuffersPerPipelineLayout;
		caps.maxVertexStreams = supLimits.limits.maxVertexBuffers;
		caps.maxVertexAttributes = supLimits.limits.maxVertexAttributes;
		caps.maxTextureSize = supLimits.limits.maxTextureDimension2D;
		caps.maxTextureArrayLayers = supLimits.limits.maxTextureArrayLayers;
		caps.maxTextureUnits = supLimits.limits.maxSampledTexturesPerShaderStage;
		caps.maxVertexTextureUnits = supLimits.limits.maxSampledTexturesPerShaderStage;
		caps.maxBindGroups = supLimits.limits.maxBindGroups;
		caps.maxBindingsPerBindGroup = supLimits.limits.maxBindingsPerBindGroup;
		caps.maxTextureAnisotropicLevel = 16;
		caps.maxRenderTargets = supLimits.limits.maxColorAttachments;

		caps.maxComputeInvocationsPerWorkgroup = supLimits.limits.maxComputeInvocationsPerWorkgroup;
		caps.maxComputeWorkgroupSizeX = supLimits.limits.maxComputeWorkgroupSizeX;
		caps.maxComputeWorkgroupSizeY = supLimits.limits.maxComputeWorkgroupSizeY;
		caps.maxComputeWorkgroupSizeZ = supLimits.limits.maxComputeWorkgroupSizeZ;
		caps.maxComputeWorkgroupsPerDimension = supLimits.limits.maxComputeWorkgroupsPerDimension;

		caps.shadersSupportedFlags = SHADER_CAPS_VERTEX_SUPPORTED
									| SHADER_CAPS_PIXEL_SUPPORTED
									| SHADER_CAPS_COMPUTE_SUPPORTED;

		for (int i = FORMAT_R8; i <= FORMAT_RGBA16; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}

		for (int i = FORMAT_D16; i <= FORMAT_D24S8; i++)
		{
			caps.textureFormatsSupported[i] = true;
			caps.renderTargetFormatsSupported[i] = true;
		}

		caps.textureFormatsSupported[FORMAT_D32F] =
		caps.renderTargetFormatsSupported[FORMAT_D32F] = true;

		for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
			caps.textureFormatsSupported[i] = true;

		caps.textureFormatsSupported[FORMAT_ATI1N] = false;

		WGPUDeviceDescriptor rhiDeviceDesc{};

		const char* const enabledToggles[] = {
			"allow_unsafe_apis",
			"use_user_defined_labels_in_backend"
		};

		WGPUDawnTogglesDescriptor deviceTogglesDesc{};
		deviceTogglesDesc.enabledToggles = enabledToggles;
		deviceTogglesDesc.enabledToggleCount = elementsOf(enabledToggles);
		deviceTogglesDesc.chain.sType = WGPUSType_DawnTogglesDescriptor;

		rhiDeviceDesc.nextInChain = &deviceTogglesDesc.chain;

		WGPUFeatureName requiredFeatures[] = {
			WGPUFeatureName_TextureCompressionBC,
			WGPUFeatureName_BGRA8UnormStorage,
			// TODO: android
			//WGPUFeatureName_TextureCompressionETC2,
			//WGPUFeatureName_TextureCompressionASTC,
			//WGPUFeatureName_ShaderF16,
		};
		rhiDeviceDesc.requiredFeatures = requiredFeatures;
		rhiDeviceDesc.requiredFeatureCount = elementsOf(requiredFeatures);

		// setup required limits
		WGPURequiredLimits reqLimits{};
		reqLimits.limits = requiredLimits;

		rhiDeviceDesc.requiredLimits = &reqLimits;
		rhiDeviceDesc.deviceLostCallback = OnWGPUDeviceLost;

		m_rhiDevice = wgpuAdapterCreateDevice(adapter, &rhiDeviceDesc);

		if (!m_rhiDevice)
		{
			MsgError("Failed to create WebGPU device\n");
			g_renderWorker.Shutdown();
			return false;
		}

		m_deviceQueue = wgpuDeviceGetQueue(m_rhiDevice);
	}

	g_renderWorker.InitLoop(this, [this]() {
		// process all internal async events or error callbacks
		wgpuInstanceProcessEvents(m_instance);
		return 0;
	}, 96);

	wgpuDeviceSetUncapturedErrorCallback(m_rhiDevice, &OnWGPUDeviceError, nullptr);

	// create default swap chain
	m_currentSwapChain = static_cast<CWGPUSwapChain*>(CreateSwapChain(params.windowInfo));

	CWGPURenderAPI::Instance.m_rhiDevice = m_rhiDevice;
	CWGPURenderAPI::Instance.m_rhiQueue = m_deviceQueue;

	return true;
}

void CWGPURenderLib::ExitAPI()
{
	g_renderWorker.Shutdown();

	for (CWGPUSwapChain* swapChain : m_swapChains)
		delete swapChain;
	m_swapChains.clear();
	m_currentSwapChain = nullptr;

	m_backendType = WGPUBackendType_Null;

	if (m_deviceQueue)
		wgpuQueueRelease(m_deviceQueue);

	if(m_rhiDevice)
	{
		wgpuDeviceSetDeviceLostCallback(m_rhiDevice, nullptr, nullptr);
		wgpuDeviceRelease(m_rhiDevice);
	}

	if(m_instance)
		wgpuInstanceRelease(m_instance);

	m_instance = nullptr;
	m_rhiDevice = nullptr;
	m_deviceQueue = nullptr;
}

void CWGPURenderLib::BeginFrame(ISwapChain* swapChain)
{
	do{
		g_renderWorker.WaitForThread();
	} while (g_renderWorker.HasPendingWork());

	CWGPURenderAPI::Instance.m_deviceLost = false;

	m_currentSwapChain = swapChain ? static_cast<CWGPUSwapChain*>(swapChain) : m_swapChains[0];
	m_currentSwapChain->UpdateResize();

	// must obtain valid texture view upon Present
	m_currentSwapChain->UpdateBackbufferView();
}

void CWGPURenderLib::EndFrame()
{
	// Wait until all tasks get finished before all gets fucked by swap chain
	do {
		g_renderWorker.WaitForThread();
	} while (g_renderWorker.HasPendingWork());

	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		return 0;
	});
}

ITexturePtr	CWGPURenderLib::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChain* CWGPURenderLib::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CWGPUSwapChain* swapChain = PPNew CWGPUSwapChain(this, windowInfo, swapChainTexture);


	m_swapChains.append(swapChain);
	return swapChain;
}

void CWGPURenderLib::DestroySwapChain(ISwapChain* swapChain)
{
	if (m_swapChains.fastRemove(static_cast<CWGPUSwapChain*>(swapChain)))
		delete swapChain;
}

void CWGPURenderLib::SetVSync(bool enable)
{
	m_swapChains[0]->SetVSync(enable);
}

void CWGPURenderLib::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_swapChains[0]->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CWGPURenderAPI::Instance.m_deviceLost = true;

	m_swapChains[0]->SetBackbufferSize(w, h);
}

// changes fullscreen mode
bool CWGPURenderLib::SetWindowed(bool enabled)
{
	// FIXME: currently switching to exclusive fullscreen will guarantee device lost
	// need to handle it somehow...
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CWGPURenderLib::IsWindowed() const
{
	return m_windowed;
}

bool CWGPURenderLib::CaptureScreenshot(CImage &img)
{
	// TODO: screenshots
	return false;
}

bool CWGPURenderLib::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}