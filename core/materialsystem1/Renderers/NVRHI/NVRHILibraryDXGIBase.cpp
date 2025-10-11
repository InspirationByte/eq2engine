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
	m_endFrameWait.Raise();
}

IShaderAPI* CNVRHIRenderLibDXGIBase::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

void CNVRHIRenderLibDXGIBase::ExitAPI()
{
	m_endFrameWait.Wait(500);

	m_defaultSwapChain = nullptr;
	m_currentSwapChain = nullptr;
}

void CNVRHIRenderLibDXGIBase::BeginFrame(ISwapChain* swapChain)
{
	m_nvrhiDevice->runGarbageCollection();
	m_endFrameWait.Wait();

	CNVRHIRenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain.Assign(swapChain ? static_cast<CNVRHISwapChainDXGI*>(swapChain) : m_defaultSwapChain);

	// must obtain valid texture view upon Present
	//g_renderWorker.WaitForExecute(__func__, [this]() {
		m_currentSwapChain->UpdateResize();
		m_currentSwapChain->UpdateBackbufferView();
	//	return 0;
	//});
}

void CNVRHIRenderLibDXGIBase::EndFrame()
{
	//g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		m_endFrameWait.Raise();
	//	return 0;
	//});
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
	/*
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

	*/
	return false;
}

