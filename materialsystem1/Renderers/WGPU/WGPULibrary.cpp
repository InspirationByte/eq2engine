//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include <webgpu/webgpu.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "imaging/ImageLoader.h"

#include "WGPUBackend.h"

#include "WGPULibrary.h"
#include "WGPURenderAPI.h"
#include "WGPUSwapChain.h"

DECLARE_CVAR(wgpu_report_errors, "0", nullptr, 0);
DECLARE_CVAR(wgpu_break_on_error, "0", nullptr, 0);
DECLARE_CVAR(wgpu_backend, "", "Specifies which WebGPU backend is going to be used", CV_ARCHIVE);

static const char* s_wgpuErrorTypesStr[] = {
	"(null)"
	"NoError",
	"Validation",
	"OutOfMemory",
	"Internal",
	"Unknown",
	"DeviceLost",
};

static const char* s_wgpuDeviceLostReasonStr[] = {
	"(null)"
	"Undefined",
	"Destroyed",
};

static void OnWGPUDeviceError(WGPUErrorType type, const char* message, void*)
{
	if (wgpu_break_on_error.GetBool())
	{
		ASSERT_FAIL("WGPU device %s error (after %s):\n\n%s", s_wgpuErrorTypesStr[type], g_renderWorker.GetLastWorkName(), message);
	}

	if (wgpu_report_errors.GetBool())
		MsgError("[WGPU] after %s: %s - %s\n", s_wgpuErrorTypesStr[type], g_renderWorker.GetLastWorkName(), message);
}

static void OnWGPUDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata)
{
	ASSERT_FAIL("WGPU device lost (after %s) reason %s\n\n%s", g_renderWorker.GetLastWorkName(), s_wgpuDeviceLostReasonStr[reason], message);
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

static const char* GetWGPUBackendTypeStr(WGPUBackendType backendType)
{
	switch (backendType)
	{
		case WGPUBackendType_D3D11:
			return "D3D11";
		case WGPUBackendType_D3D12:
			return "D3D12";
		case WGPUBackendType_Metal:
			return "Metal";
		case WGPUBackendType_Vulkan:
			return "Vulkan";
		case WGPUBackendType_OpenGL:
			return "OpenGL";
		case WGPUBackendType_OpenGLES:
			return "OpenGLES";
	}
	return "Unknown";
}

static const char* GetWGPUAdapterTypeStr(WGPUAdapterType adapterType)
{
	switch (adapterType)
	{
	case WGPUAdapterType_DiscreteGPU:
		return "Discrete GPU";
	case WGPUAdapterType_IntegratedGPU:
		return "Integrated GPU";
	case WGPUAdapterType_CPU:
		return "Software";
	}
	return "Unknown";
}

static size_t wgpuLoadCacheDataFunction(void const* key, size_t keySize, void* value, size_t valueSize, void* userdata)
{
	const uint32 keyChecksum = CRC32_BlockChecksum(key, keySize);

	IFilePtr file = g_fileSystem->Open(EqString::Format("PSOCache/%u.psoc", keyChecksum), "r", SP_ROOT);

	if (!file)
		return 0;

	if (!value)
		return file->GetSize();

	return file->Read(value, 1, valueSize);
}

static void wgpuStoreCacheDataFunction(void const* key, size_t keySize, void const* value, size_t valueSize, void* userdata)
{
	const uint32 keyChecksum = CRC32_BlockChecksum(key, keySize);

	g_fileSystem->MakeDir("PSOCache", SP_ROOT);
	IFilePtr file = g_fileSystem->Open(EqString::Format("PSOCache/%u.psoc", keyChecksum), "w", SP_ROOT);
	file->Write(value, 1, valueSize);
}

bool CWGPURenderLib::InitAPI(const ShaderAPIParams& params)
{
	WGPURequestAdapterOptions options{};
	options.powerPreference = WGPUPowerPreference_HighPerformance;
	
	EqStringRef backendName = wgpu_backend.GetString();

	if (!backendName.CompareCaseIns("D3D11"))
		options.backendType = WGPUBackendType_D3D11;
	else if(!backendName.CompareCaseIns("D3D12"))
		options.backendType = WGPUBackendType_D3D12;
	else if (!backendName.CompareCaseIns("Vulkan"))
		options.backendType = WGPUBackendType_Vulkan;
	else if (!backendName.CompareCaseIns("OpenGL"))
		options.backendType = WGPUBackendType_OpenGL;
	else if (!backendName.CompareCaseIns("OpenGLES"))
		options.backendType = WGPUBackendType_OpenGLES;

	//options.compatibleSurface = surface;
	wgpuInstanceRequestAdapter(m_instance, &options, &OnWGPUAdapterRequestEnded, &m_rhiAdapter);

	if (!m_rhiAdapter)
	{
		MsgError("No WGPU supported adapter found\n");
		return false;
	}

	{
		WGPUAdapterProperties properties = {};
		wgpuAdapterGetProperties(m_rhiAdapter, &properties);

		Msg("WGPU Adapter Info: %s on %s (%s) %s\n", GetWGPUBackendTypeStr(properties.backendType), properties.name, GetWGPUAdapterTypeStr(properties.adapterType), properties.driverDescription);
	}

	{
		WGPUSupportedLimits supLimits = {};
		wgpuAdapterGetLimits(m_rhiAdapter, &supLimits);

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

		WGPUDeviceDescriptor rhiDeviceDesc{};

		FixedArray<const char*, 32> enabledToggles;
		FixedArray<const char*, 32> disabledToggles;

		enabledToggles.append("allow_unsafe_apis");
		enabledToggles.append("enable_immediate_error_handling");
		disabledToggles.append("lazy_clear_resource_on_first_use");	// this switch requires us to clear buffers and render targets

		if(g_cmdLine->FindArgument("-debugwgpu") != -1)
		{
			enabledToggles.append("use_user_defined_labels_in_backend");
			wgpu_report_errors.SetBool(true);
			wgpu_break_on_error.SetBool(true);
		}

		WGPUDawnCacheDeviceDescriptor rhiDawnCache{};
		rhiDawnCache.chain.sType = WGPUSType_DawnCacheDeviceDescriptor;
		rhiDawnCache.isolationKey = "E2Render";
		rhiDawnCache.loadDataFunction = wgpuLoadCacheDataFunction;
		rhiDawnCache.storeDataFunction = wgpuStoreCacheDataFunction;

		WGPUDawnTogglesDescriptor rhiDeviceTogglesDesc{};
		rhiDeviceTogglesDesc.chain.next = &rhiDawnCache.chain;
		rhiDeviceTogglesDesc.enabledToggles = enabledToggles.ptr();
		rhiDeviceTogglesDesc.enabledToggleCount = enabledToggles.numElem();

		rhiDeviceTogglesDesc.disabledToggles = disabledToggles.ptr();
		rhiDeviceTogglesDesc.disabledToggleCount = disabledToggles.numElem();

		rhiDeviceTogglesDesc.chain.sType = WGPUSType_DawnTogglesDescriptor;
		rhiDeviceDesc.nextInChain = &rhiDeviceTogglesDesc.chain;

		FixedArray<WGPUFeatureName, 32> requiredFeatures;
		requiredFeatures.append(WGPUFeatureName_TextureCompressionBC);
		requiredFeatures.append(WGPUFeatureName_BGRA8UnormStorage);
		requiredFeatures.append(WGPUFeatureName_SurfaceCapabilities);
		requiredFeatures.append(WGPUFeatureName_Norm16TextureFormats);
		// TODO: android
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionETC2),
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionASTC),
		//requiredFeatures.append(WGPUFeatureName_ShaderF16),

		rhiDeviceDesc.requiredFeatures = requiredFeatures.ptr();
		rhiDeviceDesc.requiredFeatureCount = requiredFeatures.numElem();

		// setup required limits
		WGPURequiredLimits reqLimits{};
		reqLimits.limits = requiredLimits;

		rhiDeviceDesc.requiredLimits = &reqLimits;
		rhiDeviceDesc.deviceLostCallback = OnWGPUDeviceLost;

		m_rhiDevice = wgpuAdapterCreateDevice(m_rhiAdapter, &rhiDeviceDesc);

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

	m_rhiBackendType = WGPUBackendType_Null;

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
	//do {
	//	g_renderWorker.WaitForThread();
	//} while (g_renderWorker.HasPendingWork());

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
	ITexturePtr currentTexture = m_currentSwapChain->GetBackbuffer();

	const int bytesPerPixel = GetBytesPerPixel(GetTexFormat(currentTexture->GetFormat()));
	const bool rbSwapped = HasTexFormatFlags(currentTexture->GetFormat(), TEXFORMAT_FLAG_SWAP_RB);

	const BufferInfo bufInfo(bytesPerPixel, currentTexture->GetWidth() * currentTexture->GetHeight());
	IGPUBufferPtr tempBuffer = g_renderAPI->CreateBuffer(bufInfo, BUFFERUSAGE_READ | BUFFERUSAGE_COPY_DST, "ScreenshotImgBuffer");
	IGPUCommandRecorderPtr cmdRecorder = g_renderAPI->CreateCommandRecorder("ScreenshotCmd");
	cmdRecorder->CopyTextureToBuffer(TextureCopyInfo{ currentTexture }, tempBuffer, TextureExtent{ currentTexture->GetWidth(), currentTexture->GetHeight(), 1 });
	Future<bool> cmdFuture = g_renderAPI->SubmitCommandBufferAwaitable(cmdRecorder->End());
	
	// wait until image is copied to the buffer
	while (!cmdFuture.HasResult()) {
		WGPU_INSTANCE_SPIN
	}

	// map buffer so we can read back pixels to our screenshot image memory
	IGPUBuffer::MapFuture future = tempBuffer->Lock(0, tempBuffer->GetSize(), 0);
	future.AddCallback([currentTexture, bytesPerPixel, rbSwapped, &img](const FutureResult<BufferMapData>& result) {
		ASSERT(result->data);

		const TextureExtent size = currentTexture->GetSize();
		ubyte* dst = img.Create(FORMAT_RGB8, size.width, size.height, 1, 1);

		for (int y = 0; y < size.height; y++)
		{
			const ubyte* src = (ubyte*)result->data + bytesPerPixel * y * size.width;
			for (int x = 0; x < size.width; ++x)
			{
				if(rbSwapped)
				{
					dst[0] = src[2];
					dst[1] = src[1];
					dst[2] = src[0];
				}
				else
				{
					dst[0] = src[0];
					dst[1] = src[1];
					dst[2] = src[2];
				}
				dst += 3;
				src += bytesPerPixel;
			}
		}
	});

	// force WebGPU to process everything it has queued
	while (!future.HasResult()) {
		WGPU_INSTANCE_SPIN
	}

	return true;
}

bool CWGPURenderLib::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}