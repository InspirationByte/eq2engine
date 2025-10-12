//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "utils/CRC32.h"

#include "imaging/ImageLoader.h"

#include "WGPUBackend.h"

#include "WGPULibrary.h"
#include "WGPURenderAPI.h"
#include "WGPUSwapChain.h"
#include "WGPUCommandRecorder.h"

DECLARE_CVAR(wgpu_reportErrors, "0", nullptr, 0);
DECLARE_CVAR(wgpu_breakOnError, "0", nullptr, 0);
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
	"(null)",
    "Unknown",
    "Destroyed",
    "InstanceDropped",
    "FailedCreation",
};

thread_local WGPUDeviceErrorContext* g_currentErrorDeviceContext = nullptr;

static void OnWGPUDeviceError(WGPUDevice const* device, WGPUErrorType type, struct WGPUStringView message, void* userdata1, void* userdata2)
{
	WGPUDeviceErrorContext* errorCtx = g_currentErrorDeviceContext;
	if (errorCtx)
	{
		errorCtx->hasError = true;

#ifndef _RETAIL
		if(errorCtx->onError)
			errorCtx->onError();
#endif
	}

	if (wgpu_breakOnError.GetBool())
	{
		ASSERT_FAIL("WGPU device %s error:\n\n%.*s", s_wgpuErrorTypesStr[type], static_cast<int>(message.length), message.data);
	}

	if (wgpu_reportErrors.GetBool())
		MsgError("[WGPU]: %s - %s\n", s_wgpuErrorTypesStr[type], message.data);
}

static void OnWGPUDeviceLost(WGPUDevice const* device, WGPUDeviceLostReason reason, struct WGPUStringView message, void* userdata1, void* userdata2)
{
	if(reason == WGPUDeviceLostReason_Destroyed)
		return;

	ASSERT_FAIL("WGPU device lost reason %s (%d)\n\n%.*s", s_wgpuDeviceLostReasonStr[reason], reason, static_cast<int>(message.length), message.data);
	MsgError("[WGPU] device lost reason %s, %.*s\n", s_wgpuDeviceLostReasonStr[reason], static_cast<int>(message.length), message.data);
}

static void OnWGPUAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata1, void* userdata2)
{
	if (status != WGPURequestAdapterStatus_Success)
	{
		// cannot find adapter?
		ErrorMsg("%s", message.data);
	}
	else
	{
		// use first adapter provided
		WGPUAdapter* result = static_cast<WGPUAdapter*>(userdata1);
		if (*result == nullptr)
			*result = adapter;
	}
}

CWGPURenderLib::CWGPURenderLib()
{
	m_windowed = true;
	m_endFrameWait.Raise();
}

CWGPURenderLib::~CWGPURenderLib()
{
}

bool CWGPURenderLib::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

	// optionally use WGPUInstanceDescriptor::nextInChain for WGPUDawnTogglesDescriptor
	// with various toggles enabled or disabled: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp

	WGPUInstanceDescriptor rhiInstanceDesc{};
	WGPUInstanceCapabilities& rhiInstanceCapabilities = rhiInstanceDesc.capabilities;
	rhiInstanceCapabilities.timedWaitAnyEnable = true;

	m_instance = wgpuCreateInstance(&rhiInstanceDesc);
	if (!m_instance)
		return false;

	return true;
}

IShaderAPI* CWGPURenderLib::GetRenderer() const
{
	return &CWGPURenderAPI::Instance;
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
	static thread_local IFileStreamPtr file;
	
	if (!value)
	{
		file = g_fileSystem->Open(EqString::Format("PSOCache/%u.psoc", keyChecksum), FS_OPEN_READ, SP_ROOT);
		if (!file)
			return 0;

		return file->GetSize();
	}

	if (!file)
		return 0;

	size_t readSize = file->Read(value, 1, valueSize);
	file = nullptr;

	return readSize;
}

static void wgpuStoreCacheDataFunction(void const* key, size_t keySize, void const* value, size_t valueSize, void* userdata)
{
	const uint32 keyChecksum = CRC32_BlockChecksum(key, keySize);

	g_fileSystem->MakeDir("PSOCache", SP_ROOT);
	IFileStreamPtr file = g_fileSystem->Open(EqString::Format("PSOCache/%u.psoc", keyChecksum), FS_OPEN_WRITE, SP_ROOT);
	file->Write(value, 1, valueSize);
}

bool CWGPURenderLib::InitAPI(const ShaderAPIParams& params)
{
	const bool isDeviceValidationEnabled = (g_cmdLine->Find("-rhivalidation") != -1);

	WGPURequestAdapterOptions options{};
	options.powerPreference = WGPUPowerPreference_HighPerformance;
	options.featureLevel = isDeviceValidationEnabled == false ? WGPUFeatureLevel_Compatibility : WGPUFeatureLevel_Core;	// NOTE: this is replaced with enum in later versions
	
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

	{
		WGPURequestAdapterCallbackInfo rhiCbInfo{};
		rhiCbInfo.callback = &OnWGPUAdapterRequestEnded;
		rhiCbInfo.userdata1 = &m_rhiAdapter;
		rhiCbInfo.mode = WGPUCallbackMode_AllowSpontaneous;
		wgpuInstanceRequestAdapter(m_instance, &options, rhiCbInfo);
	}

	if (!m_rhiAdapter)
	{
		MsgError("No WGPU supported adapter found\n");
		return false;
	}

	{
		WGPUAdapterInfo rhiAdapterInfo = {};
		wgpuAdapterGetInfo(m_rhiAdapter, &rhiAdapterInfo);

		m_rhiBackendType = rhiAdapterInfo.backendType;

		Msg("* WGPU Adapter: %s on %.*s (%s)\n", GetWGPUBackendTypeStr(rhiAdapterInfo.backendType), static_cast<int>(rhiAdapterInfo.device.length), rhiAdapterInfo.device.data, GetWGPUAdapterTypeStr(rhiAdapterInfo.adapterType));
	}

	{
		WGPULimits supLimits = {};
		wgpuAdapterGetLimits(m_rhiAdapter, &supLimits);

		WGPULimits requiredLimits = supLimits;

		// fill ShaderAPI capabilities
		ShaderAPICapabilities& caps = CWGPURenderAPI::Instance.m_caps;
		caps.minUniformBufferOffsetAlignment = supLimits.minUniformBufferOffsetAlignment;
		caps.minStorageBufferOffsetAlignment = supLimits.minStorageBufferOffsetAlignment;
		caps.maxDynamicUniformBuffersPerPipelineLayout = supLimits.maxDynamicUniformBuffersPerPipelineLayout;
		caps.maxDynamicStorageBuffersPerPipelineLayout = supLimits.maxDynamicStorageBuffersPerPipelineLayout;
		caps.maxVertexStreams = supLimits.maxVertexBuffers;
		caps.maxVertexAttributes = supLimits.maxVertexAttributes;
		caps.maxTextureSize = supLimits.maxTextureDimension2D;
		caps.maxTextureArrayLayers = supLimits.maxTextureArrayLayers;
		caps.maxTextureUnits = supLimits.maxSampledTexturesPerShaderStage;
		caps.maxVertexTextureUnits = supLimits.maxSampledTexturesPerShaderStage;
		caps.maxBindGroups = supLimits.maxBindGroups;
		caps.maxBindingsPerBindGroup = supLimits.maxBindingsPerBindGroup;
		caps.maxTextureAnisotropicLevel = 16;
		caps.maxRenderTargets = supLimits.maxColorAttachments;

		caps.maxComputeInvocationsPerWorkgroup = supLimits.maxComputeInvocationsPerWorkgroup;
		caps.maxComputeWorkgroupSizeX = supLimits.maxComputeWorkgroupSizeX;
		caps.maxComputeWorkgroupSizeY = supLimits.maxComputeWorkgroupSizeY;
		caps.maxComputeWorkgroupSizeZ = supLimits.maxComputeWorkgroupSizeZ;
		caps.maxComputeWorkgroupsPerDimension = supLimits.maxComputeWorkgroupsPerDimension;
		caps.multiDrawIndirectSupport = wgpuAdapterHasFeature(m_rhiAdapter, WGPUFeatureName_MultiDrawIndirect);

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
		disabledToggles.append("lazy_clear_resource_on_first_use");	// this switch requires us to clear buffers and render targets
		enabledToggles.append("use_user_defined_labels_in_backend");
		if(isDeviceValidationEnabled)
		{
			enabledToggles.append("enable_immediate_error_handling");
			enabledToggles.append("disable_symbol_renaming");
			wgpu_reportErrors.SetBool(true);
			wgpu_breakOnError.SetBool(true);

			CWGPURenderAPI::Instance.m_isValidationActive = true;
		}
		else
		{
			enabledToggles.append("skip_validation");
			enabledToggles.append("fxc_optimizations");
		}

		WGPUDawnCacheDeviceDescriptor rhiDawnCache{};
		rhiDawnCache.chain.sType = WGPUSType_DawnCacheDeviceDescriptor;
		rhiDawnCache.isolationKey = _WSTR("E2Render");
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

		if(wgpuAdapterHasFeature(m_rhiAdapter, WGPUFeatureName_MultiDrawIndirect))
			requiredFeatures.append(WGPUFeatureName_MultiDrawIndirect);

		//requiredFeatures.append(WGPUFeatureName_BGRA8UnormStorage);
		//requiredFeatures.append(WGPUFeatureName_SurfaceCapabilities);
		requiredFeatures.append(WGPUFeatureName_Norm16TextureFormats);
		//requiredFeatures.append(WGPUFeatureName_FlexibleTextureViews);
		
		// TODO: android
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionETC2),
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionASTC),
		//requiredFeatures.append(WGPUFeatureName_ShaderF16),

		rhiDeviceDesc.requiredFeatures = requiredFeatures.ptr();
		rhiDeviceDesc.requiredFeatureCount = requiredFeatures.numElem();
		rhiDeviceDesc.uncapturedErrorCallbackInfo.callback = OnWGPUDeviceError;

		// setup required limits
		rhiDeviceDesc.requiredLimits = &requiredLimits;
		WGPUDeviceLostCallbackInfo& rhiLostCbInfo = rhiDeviceDesc.deviceLostCallbackInfo;
		rhiLostCbInfo.callback = OnWGPUDeviceLost;
		rhiLostCbInfo.mode = WGPUCallbackMode_AllowSpontaneous;

		m_rhiDevice = wgpuAdapterCreateDevice(m_rhiAdapter, &rhiDeviceDesc);

		if (!m_rhiDevice)
		{
			MsgError("Failed to create WebGPU device\n");
			g_renderWorker.Shutdown();
			return false;
		}

		m_deviceQueue = wgpuDeviceGetQueue(m_rhiDevice);
	}

	constexpr int jobQueueSize = 1024;

	g_renderWorker.Init(this, [this]() {
		// process all internal async events or error callbacks
		wgpuInstanceProcessEvents(m_instance);
		return 0;
	}, jobQueueSize);

	m_defaultSwapChain = CRefPtr<CWGPUSwapChain>(static_cast<CWGPUSwapChain*>(CreateSwapChain(params.windowInfo).Ptr()));
	m_currentSwapChain = m_defaultSwapChain;

	CWGPURenderAPI::Instance.m_rhiInstance = m_instance;
	CWGPURenderAPI::Instance.m_rhiDevice = m_rhiDevice;
	CWGPURenderAPI::Instance.m_rhiQueue = m_deviceQueue;
	CWGPURenderAPI::Instance.m_rhiBackendType = m_rhiBackendType;

	return true;
}

void CWGPURenderLib::ExitAPI()
{
	m_endFrameWait.Wait(500);
	g_renderWorker.Shutdown();

	m_defaultSwapChain = nullptr;
	m_currentSwapChain = nullptr;

	m_rhiBackendType = WGPUBackendType_Null;

	if (m_deviceQueue)
		wgpuQueueRelease(m_deviceQueue);

	if(m_rhiDevice)
		wgpuDeviceRelease(m_rhiDevice);

	if(m_instance)
		wgpuInstanceRelease(m_instance);

	m_instance = nullptr;
	m_rhiDevice = nullptr;
	m_deviceQueue = nullptr;
}

void CWGPURenderLib::BeginFrame(ISwapChain* swapChain)
{
	m_endFrameWait.Wait();

	CWGPURenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain.Assign(swapChain ? static_cast<CWGPUSwapChain*>(swapChain) : m_defaultSwapChain);

	// must obtain valid texture view upon Present
	g_renderWorker.WaitForExecute(__func__, [this]() {
		m_currentSwapChain->UpdateResize();
		m_currentSwapChain->UpdateBackbufferView();
		return 0;
	});
}

void CWGPURenderLib::EndFrame()
{
	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		m_endFrameWait.Raise();
		return 0;
	});
}

ITexturePtr	CWGPURenderLib::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChainPtr CWGPURenderLib::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	return ISwapChainPtr(CRefPtr_new(CWGPUSwapChain, this, windowInfo, swapChainTexture));
}

void CWGPURenderLib::SetVSync(bool enable)
{
	m_defaultSwapChain->SetVSync(enable);
}

void CWGPURenderLib::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_defaultSwapChain->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CWGPURenderAPI::Instance.m_deviceLost = true;

	m_defaultSwapChain->SetBackbufferSize(w, h);
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
	static_cast<CWGPUCommandRecorder*>(cmdRecorder.Ptr())->CopyTextureToBuffer(TextureCopyInfo{ currentTexture }, tempBuffer, TextureExtent{ currentTexture->GetWidth(), currentTexture->GetHeight(), 1 });
	Future<bool> cmdFuture = g_renderAPI->SubmitCommandBufferAwaitable(cmdRecorder->End());
	
	// wait until image is copied to the buffer
	while (!cmdFuture.Wait(0)) {
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
	while (!future.Wait(0)) {
		WGPU_INSTANCE_SPIN
	}

	return true;
}

bool CWGPURenderLib::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}