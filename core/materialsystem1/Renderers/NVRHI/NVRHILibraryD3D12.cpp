//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include <nvrhi/nvrhi.h>
#include <dxgi.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>

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

#pragma comment(lib, "d3d12.lib")

DECLARE_CVAR(d3d12_adapter, "", "Adapter to use", CV_UNREGISTERED);
DECLARE_CVAR(d3d12_validation, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR(d3d12_break_on_error, "0", nullptr, CV_UNREGISTERED);

#define HR_RETURN(hr, fmt, ...) if(FAILED(hr)) { MsgError(fmt, __VA_ARGS__); return false; }

bool CNVRHIRenderLibD3D12::InitCaps()
{
	g_consoleCommands->RegisterCommand(&d3d12_adapter);
	g_consoleCommands->RegisterCommand(&d3d12_validation);
	g_consoleCommands->RegisterCommand(&d3d12_break_on_error);

	m_mainThreadId = Threading::GetCurrentThreadID();

	return true;
}

IShaderAPI* CNVRHIRenderLibD3D12::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibD3D12::InitAPI(const ShaderAPIParams& params)
{
	EqWString adapterName;
	AnsiUnicodeConverter(adapterName, d3d12_adapter.GetString());

	RefCountPtr<IDXGIAdapter> rhiAdapter = FindAdapter(adapterName);

	DXGI_ADAPTER_DESC rhiAdapterDesc;
	rhiAdapter->GetDesc(&rhiAdapterDesc);

	{
		EqString descAdapterName;
		AnsiUnicodeConverter(descAdapterName, rhiAdapterDesc.Description);
		Msg("* NVRHI Adapter: %s\n", descAdapterName.ToCString());

		//rhiAdapterDesc.DedicatedVideoMemory
	}

	const bool debugRuntimeLayer = d3d12_validation.GetBool();

	HRESULT hr;
	if (debugRuntimeLayer)
	{
		RefCountPtr<ID3D12Debug> pDebug;
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebug));
		HR_RETURN(hr, "Can't get ID3D12Debug interface");
		pDebug->EnableDebugLayer();
	}

	RefCountPtr<IDXGIFactory2> pDxgiFactory;
	UINT dxgiFactoryFlags = debugRuntimeLayer ? DXGI_CREATE_FACTORY_DEBUG : 0;
	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&pDxgiFactory));
	HR_RETURN(hr, "Cannot create IDXGIFactory2 interface");

	hr = D3D12CreateDevice(
		rhiAdapter,
		D3D_FEATURE_LEVEL_11_1,
		IID_PPV_ARGS(&m_rhiDevice12));
	HR_RETURN(hr, "Failed to create D3D12 device");

	if (debugRuntimeLayer)
	{
		RefCountPtr<ID3D12InfoQueue> pInfoQueue;
		m_rhiDevice12->QueryInterface(&pInfoQueue);

		if (pInfoQueue)
		{
#ifdef _DEBUG
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif

			D3D12_MESSAGE_ID disableMessageIDs[] =
			{
				D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
				D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH, // descriptor validation doesn't understand acceleration structures
				D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET, // disable warning when there is no color attachment (e.g. shadow atlas)
				D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH // barrier validation error caused by cinematics - not sure how to fix, suppress for now
			};

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.pIDList = disableMessageIDs;
			filter.DenyList.NumIDs = sizeof(disableMessageIDs) / sizeof(disableMessageIDs[0]);
			pInfoQueue->AddStorageFilterEntries(&filter);
		}
	}

	rhiAdapter->QueryInterface(IID_PPV_ARGS(&m_rhiDxgiAdapter));

	D3D12_COMMAND_QUEUE_DESC queueDesc;
	ZeroMemory(&queueDesc, sizeof(queueDesc));
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.NodeMask = 1;
	hr = m_rhiDevice12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_rhiGraphicsQueue));
	HR_RETURN(hr, "Can't create D3D12 graphics queue");
	m_rhiGraphicsQueue->SetName(L"Graphics Queue");

	// TODO: ShaderAPIParams
	const bool enableComputeQueue = false;
	const bool enableCopyQueue = false;
	if (enableComputeQueue)
	{
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		hr = m_rhiDevice12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_rhiComputeQueue));
		HR_RETURN(hr, "Can't create D3D12 compute queue");
		m_rhiComputeQueue->SetName(L"Compute Queue");
	}

	if (enableCopyQueue)
	{
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		hr = m_rhiDevice12->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_rhiCopyQueue));
		HR_RETURN(hr, "Can't create D3D12 copy queue");
		m_rhiCopyQueue->SetName(L"Copy Queue");
	}

	{
		// fill ShaderAPI capabilities
		ShaderAPICapabilities& caps = CNVRHIRenderAPI::Instance.m_caps;
		caps.minUniformBufferOffsetAlignment = 256;
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


	// create default swap chain
	m_currentSwapChain = CRefPtr<CNVRHISwapChainDXGI>(static_cast<CNVRHISwapChainDXGI*>(CreateSwapChain(params.windowInfo).Ptr()));

	//CNVRHIRenderAPI::Instance.m_rhiDevice = m_rhiDevice;

	return true;
}

void CNVRHIRenderLibD3D12::ExitAPI()
{
	g_consoleCommands->UnregisterCommand(&d3d12_adapter);
	g_consoleCommands->UnregisterCommand(&d3d12_validation);
	g_consoleCommands->UnregisterCommand(&d3d12_break_on_error);

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