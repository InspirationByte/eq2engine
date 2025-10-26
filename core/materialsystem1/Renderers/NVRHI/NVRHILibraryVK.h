/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/vulkan.h>
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"

class CNVRHISwapChainVK;

class CNVRHIRenderLibVK
	: public IRenderLibrary
	, public RenderWorkerHandler
{
	friend class CNVRHISwapChainVK;
public:
	CNVRHIRenderLibVK();

	bool			InitCaps();
	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();
	
	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();

	IShaderAPI*		GetRenderer() const;
	ITexturePtr		GetCurrentBackbuffer() const;

	void			SetVSync(bool enable);
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage& img);

	virtual ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);
protected:

	const char*		GetAsyncThreadName() const { return "EqRenderThread"; }
	void			BeginAsyncOperation(uintptr_t threadId) {}
	void			EndAsyncOperation() {}
	bool			IsMainThread(uintptr_t threadId) const;

	Array<CNVRHISwapChainVK*>	m_swapChains{ PP_SL };
	int							m_swapChainCounter{ 0 };

	CRefPtr<CNVRHISwapChainVK>	m_currentSwapChain;
	CRefPtr<CNVRHISwapChainVK>	m_defaultSwapChain;

	nvrhi::DeviceHandle			m_nvrhiDevice;
	nvrhi::EventQueryHandle		m_nvrhiFrameWaitQuery;

	bool						m_windowed{ false };
};

