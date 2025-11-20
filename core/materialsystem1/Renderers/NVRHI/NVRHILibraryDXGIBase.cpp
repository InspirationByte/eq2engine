//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#include <nvrhi/nvrhi.h>
#include <dxgi.h>
#include <dxgi1_6.h>

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/ICommandLine.h"
#include "core/IDkCore.h"
#include "core/IFileSystem.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"

#include "imaging/ImageLoader.h"

#include "NVRHIBackend.h"
#include "NVRHILibraryDXGIBase.h"
#include "NVRHIRenderAPI.h"

#pragma comment(lib, "dxgi.lib")

DECLARE_CVAR(dxgi_adapter, "", "Graphics adapter to use", CV_ARCHIVE);

CNVRHIRenderLibDXGIBase* CNVRHIRenderLibDXGIBase::Instance = nullptr;

RefCountPtr<IDXGIAdapter> CNVRHIRenderLibDXGIBase::FindAdapter(const wchar_t* targetName)
{
	RefCountPtr<IDXGIAdapter> rhiTargetAdapter;
	RefCountPtr<IDXGIFactory1> rhiDxgiFactory;
	HRESULT result = CreateDXGIFactory1(IID_PPV_ARGS(&rhiDxgiFactory));
	if (result != S_OK)
	{
		MsgError("CreateDXGIFactory failed (%x)", result);
		return rhiTargetAdapter;
	}

	RefCountPtr<IDXGIFactory6> rhiDxgiFactory6;

	int adapterId = 0;
	while (SUCCEEDED(result))
	{
		RefCountPtr<IDXGIAdapter> rhiAdapter;

		// Try to use EnumAdapterByGpuPreference method to get the better performing GPU.
		if (rhiDxgiFactory->QueryInterface(IID_PPV_ARGS(&rhiDxgiFactory6)) == S_OK)
			result = rhiDxgiFactory6->EnumAdapterByGpuPreference(adapterId, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&rhiAdapter));
		else
			result = rhiDxgiFactory->EnumAdapters(adapterId, &rhiAdapter);

		if (SUCCEEDED(result))
		{
			DXGI_ADAPTER_DESC rhiAdapterDesc;
			rhiAdapter->GetDesc(&rhiAdapterDesc);

			// If no name is specified, return the first adapater.  
			// This is the same behaviour as the default specified for 
			// D3D11CreateDevice when no adapter is specified.
			if (!targetName || *targetName == 0)
			{
				rhiTargetAdapter = rhiAdapter;
				break;
			}

			if (EqWStringRef(rhiAdapterDesc.Description).Find(targetName) != -1)
			{
				rhiTargetAdapter = rhiAdapter;
				break;
			}
		}

		adapterId++;
	}

	return rhiTargetAdapter;
}

CNVRHIRenderLibDXGIBase::CNVRHIRenderLibDXGIBase()
{
	CNVRHIRenderLibDXGIBase::Instance = this;
}

IShaderAPI* CNVRHIRenderLibDXGIBase::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibDXGIBase::IsMainThread(uintptr_t threadId) const
{
	return g_renderWorker.GetThreadID() == threadId;
}

bool CNVRHIRenderLibDXGIBase::InitAPI(const ShaderAPIParams& params)
{
	constexpr int jobQueueSize = 1024;

	g_renderWorker.Init(this, nullptr, jobQueueSize);

	m_nvrhiFrameWaitQuery = m_nvrhiDevice->createEventQuery();
	m_nvrhiDevice->setEventQuery(m_nvrhiFrameWaitQuery, nvrhi::CommandQueue::Graphics);

	return true;
}

void CNVRHIRenderLibDXGIBase::ExitAPI()
{
	g_renderWorker.Shutdown();

	if (m_defaultSwapChain)
		m_defaultSwapChain->m_dxgiSwapChain->SetFullscreenState(false, nullptr);
	
	if (m_nvrhiDevice)
	{
		m_nvrhiDevice->waitForIdle();
		m_nvrhiDevice->runGarbageCollection();
	}

	m_defaultSwapChain = nullptr;
	m_currentSwapChain = nullptr;

	CNVRHIRenderAPI::Instance.m_rhiDevice = nullptr;
	m_nvrhiDevice = nullptr;
	m_nvrhiFrameWaitQuery = nullptr;
	m_dxgiFactory = nullptr;
	m_dxgiAdapter = nullptr;
}

void CNVRHIRenderLibDXGIBase::BeginFrame(ISwapChain* swapChain)
{
	CNVRHIRenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain.Assign(swapChain ? static_cast<CNVRHISwapChainDXGI*>(swapChain) : m_defaultSwapChain);

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

void CNVRHIRenderLibDXGIBase::EndFrame()
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

		const int bufferCount = m_currentSwapChain->m_rhiSwapChainTextures.numElem();

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

ITexturePtr	CNVRHIRenderLibDXGIBase::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChainPtr CNVRHIRenderLibDXGIBase::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CRefPtr<CNVRHISwapChainDXGI> swapChain = CRefPtr_new(CNVRHISwapChainDXGI, windowInfo, swapChainTexture);

	return ISwapChainPtr(swapChain);
}

void CNVRHIRenderLibDXGIBase::SetVSync(bool enable)
{
	m_defaultSwapChain->SetVSync(enable);
}

void CNVRHIRenderLibDXGIBase::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_defaultSwapChain->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CNVRHIRenderAPI::Instance.m_deviceLost = true;

	m_defaultSwapChain->SetBackbufferSize(w, h);
}

// changes fullscreen mode
bool CNVRHIRenderLibDXGIBase::SetWindowed(bool enabled)
{
	if (m_defaultSwapChain)
		m_defaultSwapChain->m_dxgiSwapChain->SetFullscreenState(enabled == false, nullptr);
	m_dxgiFullScreenDesc.Windowed = enabled;

	// FIXME: currently switching to exclusive fullscreen will guarantee device lost
	// need to handle it somehow...
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CNVRHIRenderLibDXGIBase::IsWindowed() const
{
	return m_windowed;
}

bool CNVRHIRenderLibDXGIBase::CaptureScreenshot(CImage &img)
{
	return nvrhiCaptureBackbufferImage(m_currentSwapChain->GetBackbuffer(), img);
}

