//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include <nvrhi/nvrhi.h>
#include <nvrhi/validation.h>
#include <dxgi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <d3d12sdklayers.h>

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

//extern "C" {
//__declspec(dllexport) const UINT D3D12SDKVersion = 608;
//__declspec(dllexport) const char* D3D12SDKPath = u8".\\D3D12\\";
//}

DECLARE_CVAR(d3d12_adapter, "", "Adapter to use", CV_UNREGISTERED);
DECLARE_CVAR(d3d12_validation, "0", nullptr, CV_UNREGISTERED);
DECLARE_CVAR_F(nvrhi_validation);
DECLARE_CVAR_F(nvrhi_breakOnError);

#define HR_RETURN(hr, fmt, ...) if(FAILED(hr)) { MsgError("ERROR: D3D12 failure - " fmt "\n", __VA_ARGS__); return false; }
#define HR_ASSERT(hr, fmt, ...) if(FAILED(hr)) { ASSERT_FAIL("ERROR: D3D12 failure - " fmt "\n", __VA_ARGS__); return false; }

PFN_D3D12_CREATE_DEVICE	CNVRHIRenderLibD3D12::s_d3d12CreateDeviceFnPtr = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE CNVRHIRenderLibD3D12::s_d3d12GetDebugInterfaceFnPtr = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE CNVRHIRenderLibD3D12::s_d3d12SerializeVersionedRootSignatureFnPtr = nullptr;

// Wrap
HRESULT WINAPI D3D12SerializeVersionedRootSignature(
	_In_ const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pRootSignature,
	_Out_ ID3DBlob** ppBlob,
	_Always_(_Outptr_opt_result_maybenull_) ID3DBlob** ppErrorBlob)
{
	return CNVRHIRenderLibD3D12::s_d3d12SerializeVersionedRootSignatureFnPtr(pRootSignature, ppBlob, ppErrorBlob);
}

bool CNVRHIRenderLibD3D12::InitCaps()
{
	CNVRHIRenderLibDXGIBase::Instance = this;
	CNVRHIRenderAPI::Instance.m_rhiBackendType = NVRHI_BACKEND_D3D12;

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

	return true;
}

IShaderAPI* CNVRHIRenderLibD3D12::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibD3D12::InitAPI(const ShaderAPIParams& params)
{
#ifdef NVRHI_WITH_VALIDATION
	const bool isDeviceValidationEnabled = (g_cmdLine->Find("-rhivalidation") != -1);
	if (isDeviceValidationEnabled)
	{
		d3d12_validation.SetBool(true);
		nvrhi_validation.SetBool(true);
		nvrhi_breakOnError.SetBool(true);
	}
#endif

	EqWString adapterName;
	AnsiUnicodeConverter(adapterName, d3d12_adapter.GetString());

	RefCountPtr<IDXGIAdapter> rhiAdapter = FindAdapter(adapterName);

	DXGI_ADAPTER_DESC rhiAdapterDesc;
	rhiAdapter->GetDesc(&rhiAdapterDesc);

	{
		EqString descAdapterName;
		AnsiUnicodeConverter(descAdapterName, rhiAdapterDesc.Description);
		Msg("* NVRHI D3D12 Adapter: %s\n", descAdapterName.ToCString());
	}

	HRESULT hr;
	UINT dxgiFactoryFlags = 0;

#ifdef NVRHI_WITH_VALIDATION
	const bool debugRuntimeLayer = d3d12_validation.GetBool();

	if (debugRuntimeLayer)
	{
		RefCountPtr<ID3D12Debug> pDebug;
		hr = s_d3d12GetDebugInterfaceFnPtr(IID_PPV_ARGS(&pDebug));
		if (hr == S_OK && pDebug)
			pDebug->EnableDebugLayer();

		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
	HR_ASSERT(hr, "Cannot create IDXGIFactory2 interface");

	hr = s_d3d12CreateDeviceFnPtr(
		rhiAdapter,
		D3D_FEATURE_LEVEL_11_1,
		IID_PPV_ARGS(&m_rhiDevice12));
	HR_RETURN(hr, "Failed to create D3D12 device");

#ifdef NVRHI_WITH_VALIDATION
	if (debugRuntimeLayer)
	{
		RefCountPtr<ID3D12InfoQueue> rhiInfoQueue;
		m_rhiDevice12->QueryInterface(&rhiInfoQueue);

		if (rhiInfoQueue)
		{
#ifdef _DEBUG
			rhiInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			rhiInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif

			FixedArray<D3D12_MESSAGE_ID, 64> disableMessageIDs;
			disableMessageIDs.append(D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE);
			disableMessageIDs.append(D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE);
			disableMessageIDs.append(D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH);
			disableMessageIDs.append(D3D12_MESSAGE_ID_COMMAND_LIST_STATIC_DESCRIPTOR_RESOURCE_DIMENSION_MISMATCH);
			disableMessageIDs.append(D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_RENDERTARGETVIEW_NOT_SET);
			disableMessageIDs.append(D3D12_MESSAGE_ID_INVALID_SUBRESOURCE_STATE); // currently suppressed as depth targets need supporting it

			D3D12_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.pIDList = disableMessageIDs.ptr();
			filter.DenyList.NumIDs = disableMessageIDs.numElem();
			rhiInfoQueue->AddStorageFilterEntries(&filter);
		}
	}
#endif // NVRHI_WITH_VALIDATION
	{
		RefCountPtr<IDXGIFactory5> pDxgiFactory5;
		if (SUCCEEDED(m_dxgiFactory->QueryInterface(IID_PPV_ARGS(&pDxgiFactory5))))
		{
			BOOL supported = 0;
			if (SUCCEEDED(pDxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &supported, sizeof(supported))))
			{
				m_dxgiTearingSupported = (supported != 0);
			}
		}
	}


	rhiAdapter->QueryInterface(IID_PPV_ARGS(&m_dxgiAdapter));

	D3D12_COMMAND_QUEUE_DESC rhiQueueDesc;
	ZeroMemory(&rhiQueueDesc, sizeof(rhiQueueDesc));
	rhiQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	rhiQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	rhiQueueDesc.NodeMask = 1;
	hr = m_rhiDevice12->CreateCommandQueue(&rhiQueueDesc, IID_PPV_ARGS(&m_rhiGraphicsQueue));
	HR_RETURN(hr, "Can't create D3D12 graphics queue");
	m_rhiGraphicsQueue->SetName(L"Graphics Queue");

	// TODO: ShaderAPIParams
	const bool enableComputeQueue = false;
	const bool enableCopyQueue = false;
	if (enableComputeQueue)
	{
		rhiQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
		hr = m_rhiDevice12->CreateCommandQueue(&rhiQueueDesc, IID_PPV_ARGS(&m_rhiComputeQueue));
		HR_RETURN(hr, "Can't create D3D12 compute queue");
		m_rhiComputeQueue->SetName(L"Compute Queue");
	}

	if (enableCopyQueue)
	{
		rhiQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
		hr = m_rhiDevice12->CreateCommandQueue(&rhiQueueDesc, IID_PPV_ARGS(&m_rhiCopyQueue));
		HR_RETURN(hr, "Can't create D3D12 copy queue");
		m_rhiCopyQueue->SetName(L"Copy Queue");
	}

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

		for (int i = FORMAT_DXT1; i <= FORMAT_ATI2N; i++)
			caps.textureFormatsSupported[i] = true;

		caps.textureFormatsSupported[FORMAT_ATI1N] = false;
	}

	nvrhi::d3d12::DeviceDesc deviceDesc;
	deviceDesc.errorCB = &CNVRHIMessageCallback::Instance;
	deviceDesc.pDevice = m_rhiDevice12;
	deviceDesc.pAdapter = m_dxgiAdapter;
	deviceDesc.pGraphicsCommandQueue = m_rhiGraphicsQueue;
	deviceDesc.pComputeCommandQueue = m_rhiComputeQueue;
	deviceDesc.pCopyCommandQueue = m_rhiCopyQueue;

	m_nvrhiDevice = nvrhi::d3d12::createDevice(deviceDesc);
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
	if (params.windowInfo.windowType != RHI_WINDOW_HANDLE_UNKNOWN)
	{
		HWND window = (HWND)params.windowInfo.get(RenderWindowInfo::WINDOW);

		m_defaultSwapChain = CRefPtr<CNVRHISwapChainDXGI>(static_cast<CNVRHISwapChainDXGI*>(CNVRHIRenderLibDXGIBase::CreateSwapChain(params.windowInfo).Ptr()));

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiFullscreenDesc{};
		dxgiFullscreenDesc.Windowed = true;

		RefCountPtr<IDXGISwapChain1> pSwapChain1;
		hr = m_dxgiFactory->CreateSwapChainForHwnd(m_rhiGraphicsQueue,
			window,
			&m_defaultSwapChain->m_dxgiSwapChainDesc, &dxgiFullscreenDesc, nullptr,
			&pSwapChain1);
		HR_ASSERT(hr, "Failed to create main swap chain");

		hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_defaultSwapChain->m_dxgiSwapChain));
		HR_ASSERT(hr, "Failed to create main swap chain");

		// Required.
		hr = m_dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);
		HR_ASSERT(hr, "MakeWindowAssociation failed");
	}

	if (m_defaultSwapChain && !CreateSwapchainTargets(static_cast<CNVRHISwapChainDXGI*>(m_defaultSwapChain)))
	{
		CrashMsg("Failed to initialize main swap chain");
		return false;
	}

	return CNVRHIRenderLibDXGIBase::InitAPI(params);
}

ISwapChainPtr CNVRHIRenderLibD3D12::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	ISwapChainPtr swapChain = CNVRHIRenderLibDXGIBase::CreateSwapChain(windowInfo);

	CNVRHISwapChainDXGI* swapChainImpl = static_cast<CNVRHISwapChainDXGI*>(swapChain.Ptr());

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiFullscreenDesc{};
	dxgiFullscreenDesc.Windowed = true;

	const HWND window = (HWND)windowInfo.get(RenderWindowInfo::WINDOW);

	RefCountPtr<IDXGISwapChain1> pSwapChain1;
	HRESULT hr = m_dxgiFactory->CreateSwapChainForHwnd(m_rhiGraphicsQueue,
		window,
		&swapChainImpl->m_dxgiSwapChainDesc, &dxgiFullscreenDesc, nullptr,
		&pSwapChain1);

	HR_ASSERT(hr, "Failed to create main swap chain");

	hr = pSwapChain1->QueryInterface(IID_PPV_ARGS(&swapChainImpl->m_dxgiSwapChain));
	HR_ASSERT(hr, "Failed to create main swap chain");

	// Required.
	hr = m_dxgiFactory->MakeWindowAssociation(window, DXGI_MWA_NO_WINDOW_CHANGES);
	HR_ASSERT(hr, "MakeWindowAssociation failed");

	if (!CreateSwapchainTargets(static_cast<CNVRHISwapChainDXGI*>(swapChain.Ptr())))
		return nullptr;

	return swapChain;
}

bool CNVRHIRenderLibD3D12::CreateSwapchainTargets(CNVRHISwapChainDXGI* swapChain) const
{
	if (!swapChain)
		return false;

	IDXGISwapChain3* dxgiSwapChain = swapChain->m_dxgiSwapChain;
	const DXGI_SWAP_CHAIN_DESC1& dxgiSwapChainDesc = swapChain->m_dxgiSwapChainDesc;
	swapChain->m_d3d12SwapChainBuffers.setNum(dxgiSwapChainDesc.BufferCount);
	swapChain->m_rhiSwapChainTextures.setNum(dxgiSwapChainDesc.BufferCount);

	for (UINT i = 0; i < dxgiSwapChainDesc.BufferCount; i++)
	{
		const HRESULT hr = dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&swapChain->m_d3d12SwapChainBuffers[i]));
		HR_ASSERT(hr, "Cant create buffer for swap chain");

		nvrhi::TextureDesc textureDesc;
		textureDesc.width = dxgiSwapChainDesc.Width;
		textureDesc.height = dxgiSwapChainDesc.Height;
		textureDesc.sampleCount = dxgiSwapChainDesc.SampleDesc.Count;
		textureDesc.sampleQuality = dxgiSwapChainDesc.SampleDesc.Quality;
		textureDesc.format = swapChain->m_swapChainFormat;
		textureDesc.debugName = swapChain->GetBackbuffer()->GetName();
		textureDesc.isRenderTarget = true;
		textureDesc.isUAV = false;
		textureDesc.initialState = nvrhi::ResourceStates::Present;
		textureDesc.keepInitialState = true;

		swapChain->m_rhiSwapChainTextures[i] = m_nvrhiDevice->createHandleForNativeTexture(nvrhi::ObjectTypes::D3D12_Resource, nvrhi::Object(swapChain->m_d3d12SwapChainBuffers[i]), textureDesc);
	}

	return true;
}

void CNVRHIRenderLibD3D12::ExitAPI()
{
	g_consoleCommands->UnregisterCommand(&d3d12_adapter);
	g_consoleCommands->UnregisterCommand(&d3d12_validation);

	CNVRHIRenderLibDXGIBase::ExitAPI();

	m_rhiDevice12 = nullptr;
	m_rhiGraphicsQueue = nullptr;
	m_rhiComputeQueue = nullptr;
	m_rhiCopyQueue = nullptr;

	FreeModule(m_d3d12Lib);
	m_d3d12Lib = nullptr;
}