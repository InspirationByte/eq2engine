/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer D3D11
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"
#include "NVRHILibraryDXGIBase.h"

class CNVRHIRenderLibD3D11
	: public CNVRHIRenderLibDXGIBase
	, public RenderWorkerHandler
{
public:
	CNVRHIRenderLibD3D11();
	~CNVRHIRenderLibD3D11();

	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();

	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();
	ITexturePtr		GetCurrentBackbuffer() const;

	IShaderAPI*		GetRenderer() const;

	void			SetVSync(bool enable);
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChain*		CreateSwapChain(const RenderWindowInfo& windowInfo);
	void			DestroySwapChain(ISwapChain* swapChain);
protected:

	const char*		GetAsyncThreadName() const { return "EqRenderThread"; }
	void			BeginAsyncOperation(uintptr_t threadId) {}
	void			EndAsyncOperation() {}
	bool			IsMainThread(uintptr_t threadId) const;

	uintptr_t				m_mainThreadId{ 0 };
	WGPUInstance			m_instance{ nullptr };

	WGPUBackendType			m_rhiBackendType{ WGPUBackendType_Null };
	WGPUAdapter				m_rhiAdapter{ nullptr };
	WGPUDevice				m_rhiDevice{ nullptr };
	WGPUQueue				m_deviceQueue{ nullptr };
};

