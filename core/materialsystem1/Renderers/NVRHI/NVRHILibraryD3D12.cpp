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
	m_currentSwapChain = CRefPtr<CNVRHISwapChainDXGI>(static_cast<CNVRHISwapChainDXGI*>(CreateSwapChain(params.windowInfo).Ptr()));

	CNVRHIRenderAPI::Instance.m_rhiDevice = m_rhiDevice;
	CNVRHIRenderAPI::Instance.m_rhiQueue = m_deviceQueue;

	return true;
}

void CNVRHIRenderLibD3D12::ExitAPI()
{
	CNVRHIRenderLibDXGIBase::ExitAPI();

	g_renderWorker.Shutdown();
}

void CNVRHIRenderLibD3D12::BeginFrame(ISwapChain* swapChain)
{
	CNVRHIRenderLibDXGIBase::BeginFrame(swapChain);
}

void CNVRHIRenderLibD3D12::EndFrame()
{
	CNVRHIRenderLibDXGIBase::EndFrame();
}

bool CNVRHIRenderLibD3D12::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId; // always run in separate thread
}