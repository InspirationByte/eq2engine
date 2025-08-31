//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include <nvrhi/nvrhi.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "imaging/ImageLoader.h"

#include "NVRHIBackend.h"
#include "NVRHILibraryD3D12.h"
#include "NVRHISwapChainDXGI.h"
#include "NVRHIRenderAPI.h"

DECLARE_CVAR(d3d12_validation, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR(d3d12_break_on_error, "0", nullptr, CV_UNREGISTERED);

CNVRHIRenderLibD3D12::CNVRHIRenderLibD3D12()
{
	m_windowed = true;
	m_endFrameWait.Raise();
}

CNVRHIRenderLibD3D12::~CNVRHIRenderLibD3D12()
{
}

bool CNVRHIRenderLibD3D12::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

	CNVRHIRenderLibDXGIBase::InitCaps();

	return true;
}

IShaderAPI* CNVRHIRenderLibD3D12::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibD3D12::InitAPI(const ShaderAPIParams& params)
{
	CNVRHIRenderLibDXGIBase::InitAPI(params);

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

	wgpuInstanceRequestAdapter(m_instance, &options, &OnWGPUAdapterRequestEnded, &m_rhiAdapter);

	if (!m_rhiAdapter)
	{
		MsgError("No WGPU supported adapter found\n");
		return false;
	}

	{
		WGPUAdapterInfo rhiAdapterInfo = {};
		wgpuAdapterGetInfo(m_rhiAdapter, &rhiAdapterInfo);

		Msg("* WGPU Adapter: %s on %s (%s)\n", GetWGPUBackendTypeStr(rhiAdapterInfo.backendType), EqString(rhiAdapterInfo.device.data, rhiAdapterInfo.device.length).ToCString(), GetWGPUAdapterTypeStr(rhiAdapterInfo.adapterType));
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
		disabledToggles.append("lazy_clear_resource_on_first_use");	// this switch requires us to clear buffers and render targets
		enabledToggles.append("use_user_defined_labels_in_backend");
		if(g_cmdLine->FindArgument("-debugwgpu") != -1)
		{
			enabledToggles.append("enable_immediate_error_handling");
			enabledToggles.append("disable_symbol_renaming");
			wgpu_report_errors.SetBool(true);
			wgpu_break_on_error.SetBool(true);
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
		requiredFeatures.append(WGPUFeatureName_BGRA8UnormStorage);
		//requiredFeatures.append(WGPUFeatureName_SurfaceCapabilities);
		requiredFeatures.append(WGPUFeatureName_Norm16TextureFormats);
		// TODO: android
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionETC2),
		//requiredFeatures.append(WGPUFeatureName_TextureCompressionASTC),
		//requiredFeatures.append(WGPUFeatureName_ShaderF16),

		rhiDeviceDesc.requiredFeatures = requiredFeatures.ptr();
		rhiDeviceDesc.requiredFeatureCount = requiredFeatures.numElem();
		rhiDeviceDesc.uncapturedErrorCallbackInfo.callback = OnWGPUDeviceError;

		// setup required limits
		WGPURequiredLimits reqLimits{};
		reqLimits.limits = requiredLimits;

		rhiDeviceDesc.requiredLimits = &reqLimits;
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

	// create default swap chain
	m_currentSwapChain = static_cast<CWGPUSwapChain*>(CreateSwapChain(params.windowInfo));

	CWGPURenderAPI::Instance.m_rhiDevice = m_rhiDevice;
	CWGPURenderAPI::Instance.m_rhiQueue = m_deviceQueue;

	return true;
}

void CNVRHIRenderLibD3D12::ExitAPI()
{
	CNVRHIRenderLibDXGIBase::ExitAPI();

	m_endFrameWait.Wait(500);
	g_renderWorker.Shutdown();

	for (CNVRHISwapChainDXGI* swapChain : m_swapChains)
		delete swapChain;

	m_swapChains.clear();
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

void CNVRHIRenderLibD3D12::BeginFrame(ISwapChain* swapChain)
{
	CNVRHIRenderLibDXGIBase::BeginFrame(swapChain);

	m_endFrameWait.Wait();

	CNVRHIRenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain = swapChain ? static_cast<CNVRHISwapChain*>(swapChain) : m_swapChains[0];

	// must obtain valid texture view upon Present
	g_renderWorker.WaitForExecute(__func__, [this]() {
		m_currentSwapChain->UpdateResize();
		m_currentSwapChain->UpdateBackbufferView();
		return 0;
	});
}

void CNVRHIRenderLibD3D12::EndFrame()
{
	CNVRHIRenderLibDXGIBase::EndFrame();

	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		m_endFrameWait.Raise();
		return 0;
	});
}

ITexturePtr	CNVRHIRenderLibD3D12::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChain* CNVRHIRenderLibD3D12::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CNVRHISwapChainDXGI* swapChain = PPNew CNVRHISwapChainDXGI(this, windowInfo, swapChainTexture);

	m_swapChains.append(swapChain);
	return swapChain;
}

void CNVRHIRenderLibD3D12::DestroySwapChain(ISwapChain* swapChain)
{
	if (m_swapChains.fastRemove(static_cast<CNVRHISwapChainDXGI*>(swapChain)))
		delete swapChain;
}

void CNVRHIRenderLibD3D12::SetVSync(bool enable)
{
	m_swapChains[0]->SetVSync(enable);
}

void CNVRHIRenderLibD3D12::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_swapChains[0]->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CNVRHIRenderAPI::Instance.m_deviceLost = true;

	m_swapChains[0]->SetBackbufferSize(w, h);
}

// changes fullscreen mode
bool CNVRHIRenderLibD3D12::SetWindowed(bool enabled)
{
	// FIXME: currently switching to exclusive fullscreen will guarantee device lost
	// need to handle it somehow...
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CNVRHIRenderLibD3D12::IsWindowed() const
{
	return m_windowed;
}

bool CNVRHIRenderLibD3D12::CaptureScreenshot(CImage &img)
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

bool CNVRHIRenderLibD3D12::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}