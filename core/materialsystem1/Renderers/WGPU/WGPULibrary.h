/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: WebGPU renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"
#include "WGPUBackend.h"

class CWGPUSwapChain;

class CWGPURenderLib
	: public IRenderLibrary
	, public RenderWorkerHandler
{
	friend class CWGPUSwapChain;
public:
	CWGPURenderLib();
	~CWGPURenderLib();

	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();

	void			BeginFrame(ISwapChain* swapChain, bool enableVSync);
	void			EndFrame();
	ITexturePtr		GetCurrentBackbuffer() const;

	IShaderAPI*		GetRenderer() const;

	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);

protected:

	const char*		GetAsyncThreadName() const { return "EqRenderThread"; }
	void			BeginAsyncOperation(uintptr_t threadId) {}
	void			EndAsyncOperation() {}
	bool			IsMainThread(uintptr_t threadId) const;

	WGPUInstance			m_instance{ nullptr };

	WGPUBackendType			m_rhiBackendType{ WGPUBackendType_Null };
	WGPUAdapter				m_rhiAdapter{ nullptr };
	WGPUDevice				m_rhiDevice{ nullptr };
	WGPUQueue				m_deviceQueue{ nullptr };

	Threading::CEqSignal	m_endFrameWait;

	CRefPtr<CWGPUSwapChain>	m_defaultSwapChain;
	CRefPtr<CWGPUSwapChain>	m_currentSwapChain;
	int						m_swapChainCounter{ 0 };
	bool					m_windowed{ false };
};

