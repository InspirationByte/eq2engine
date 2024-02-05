/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"

class CWGPUSwapChain;

class CWGPURenderLib : public IRenderLibrary, public RenderWorkerHandler
{
	friend class CWGPUSwapChain;
public:
	CWGPURenderLib();
	~CWGPURenderLib();

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

	Array<CWGPUSwapChain*>	m_swapChains{ PP_SL };
	int						m_swapChainCounter{ 0 };
	CWGPUSwapChain*			m_currentSwapChain{ nullptr };
	bool					m_windowed{ false };
};

