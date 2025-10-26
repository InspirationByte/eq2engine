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
	/*
	m_d3d12Lib = LoadLibraryA("d3d12.dll");
	if (!m_d3d12Lib) {
		MsgError("Failed to load d3d12.dll");
		return false;
	}

#define INIT_FN(fn, name) fn = reinterpret_cast<decltype(fn)>(GetProcAddress(m_d3d12Lib, name))
	INIT_FN(s_d3d12CreateDeviceFnPtr, "D3D12CreateDevice");
	INIT_FN(s_d3d12GetDebugInterfaceFnPtr, "D3D12GetDebugInterface");
	INIT_FN(s_d3d12SerializeVersionedRootSignatureFnPtr, "D3D12SerializeVersionedRootSignature");
#undef INIT_FN

	g_consoleCommands->RegisterCommand(&d3d12_adapter);

#ifdef NVRHI_WITH_VALIDATION
	g_consoleCommands->RegisterCommand(&d3d12_validation);
#endif
*/

	return true;
}

bool CNVRHIRenderLibVK::InitAPI(const ShaderAPIParams& params)
{
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

	// create default swap chain
	m_defaultSwapChain = CRefPtr<CNVRHISwapChainVK>(static_cast<CNVRHISwapChainVK*>(CreateSwapChain(params.windowInfo).Ptr()));

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
