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
#include "NVRHILibraryD3D11.h"
#include "NVRHISwapChainDXGI.h"
#include "NVRHIRenderAPI.h"

CNVRHIRenderLibDXGIBase::CNVRHIRenderLibDXGIBase()
{
	m_windowed = true;
	m_endFrameWait.Raise();
}

CNVRHIRenderLibDXGIBase::~CNVRHIRenderLibDXGIBase()
{
}

bool CNVRHIRenderLibDXGIBase::InitCaps()
{


	return true;
}

IShaderAPI* CNVRHIRenderLibDXGIBase::GetRenderer() const
{
	return &CNVRHIRenderAPI::Instance;
}

bool CNVRHIRenderLibDXGIBase::InitAPI(const ShaderAPIParams& params)
{


	return true;
}

void CNVRHIRenderLibDXGIBase::ExitAPI()
{
	m_endFrameWait.Wait(500);
	g_renderWorker.Shutdown();

	for (CNVRHISwapChainDXGI* swapChain : m_swapChains)
		delete swapChain;

	m_swapChains.clear();
	m_currentSwapChain = nullptr;
}

void CNVRHIRenderLibDXGIBase::BeginFrame(ISwapChain* swapChain)
{
	m_endFrameWait.Wait();

	CNVRHIRenderAPI::Instance.m_deviceLost = false;
	m_currentSwapChain = swapChain ? static_cast<CNVRHISwapChainDXGI*>(swapChain) : m_swapChains[0];

	// must obtain valid texture view upon Present
	g_renderWorker.WaitForExecute(__func__, [this]() {
		m_currentSwapChain->UpdateResize();
		m_currentSwapChain->UpdateBackbufferView();
		return 0;
	});
}

void CNVRHIRenderLibDXGIBase::EndFrame()
{
	g_renderWorker.Execute(__func__, [this]() {
		m_currentSwapChain->SwapBuffers();
		m_endFrameWait.Raise();
		return 0;
	});
}

ITexturePtr	CNVRHIRenderLibDXGIBase::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChain* CNVRHIRenderLibDXGIBase::CreateSwapChain(const RenderWindowInfo& windowInfo)
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

void CNVRHIRenderLibDXGIBase::DestroySwapChain(ISwapChain* swapChain)
{
	if (m_swapChains.fastRemove(static_cast<CNVRHISwapChainDXGI*>(swapChain)))
		delete swapChain;
}

void CNVRHIRenderLibDXGIBase::SetVSync(bool enable)
{
	m_swapChains[0]->SetVSync(enable);
}

void CNVRHIRenderLibDXGIBase::SetBackbufferSize(const int w, const int h)
{
	int oldW, oldH;
	m_swapChains[0]->GetBackbufferSize(oldW, oldH);

	if(w != oldW || h != oldH)
		CNVRHIRenderAPI::Instance.m_deviceLost = true;

	m_swapChains[0]->SetBackbufferSize(w, h);
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

	return false;
}

