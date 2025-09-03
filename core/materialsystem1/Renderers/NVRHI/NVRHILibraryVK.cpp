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
#include "NVRHILibraryVK.h"
#include "NVRHISwapChainVK.h"
#include "NVRHIRenderAPI.h"

DECLARE_CVAR(vulkan_validation, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR(vulkan_break_on_error, "0", nullptr, CV_UNREGISTERED);


CNVRHIRenderLibVK::CNVRHIRenderLibVK()
{
	m_windowed = true;
	m_endFrameWait.Raise();
}

CNVRHIRenderLibVK::~CNVRHIRenderLibVK()
{
}

bool CNVRHIRenderLibVK::InitCaps()
{
	m_mainThreadId = Threading::GetCurrentThreadID();

	// optionally use WGPUInstanceDescriptor::nextInChain for WGPUDawnTogglesDescriptor
	// with various toggles enabled or disabled: https://dawn.googlesource.com/dawn/+/refs/heads/main/src/dawn/native/Toggles.cpp

	m_instance = wgpuCreateInstance(nullptr);
	if (!m_instance)
		return false;

	return true;
}

IShaderAPI* CNVRHIRenderLibVK::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibVK::InitAPI(const ShaderAPIParams& params)
{

	{
		// fill ShaderAPI capabilities
		ShaderAPICapabilities& caps = CNVRHIRenderAPI::Instance.m_caps;
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

	// create default swap chain
	m_currentSwapChain = static_cast<CNVRHISwapChainVK*>(CreateSwapChain(params.windowInfo));

	CNVRHIRenderAPI::Instance.m_rhiDevice = m_rhiDevice;
	CNVRHIRenderAPI::Instance.m_rhiQueue = m_deviceQueue;

	return true;
}

void CNVRHIRenderLibVK::ExitAPI()
{
	m_endFrameWait.Wait(500);
	g_renderWorker.Shutdown();

	m_swapChains.clear();
	m_currentSwapChain = nullptr;


}

void CNVRHIRenderLibVK::BeginFrame(ISwapChain* swapChain)
{
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

void CNVRHIRenderLibVK::EndFrame()
{
	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		m_endFrameWait.Raise();
		return 0;
	});
}

ITexturePtr	CNVRHIRenderLibVK::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChain* CNVRHIRenderLibVK::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CNVRHISwapChain* swapChain = PPNew CNVRHISwapChain(this, windowInfo, swapChainTexture);

	m_swapChains.append(swapChain);
	return swapChain;
}

void CNVRHIRenderLibVK::DestroySwapChain(ISwapChain* swapChain)
{
	if (m_swapChains.fastRemove(static_cast<CNVRHISwapChain*>(swapChain)))
		delete swapChain;
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

bool CNVRHIRenderLibVK::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}