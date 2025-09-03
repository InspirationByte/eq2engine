/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer (base for D3D11 & D3D12)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include "../IRenderLibrary.h"

class CNVRHISwapChainDXGI;

class CNVRHIRenderLibDXGIBase
	: public IRenderLibrary
{
	friend class CNVRHISwapChainDXGI;
public:
	CNVRHIRenderLibDXGIBase();
	~CNVRHIRenderLibDXGIBase();

	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();

	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();
	ITexturePtr		GetCurrentBackbuffer() const;

	void			SetVSync(bool enable);
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);
protected:

	Threading::CEqSignal		m_endFrameWait;

	Array<CNVRHISwapChainDXGI*>	m_swapChains{ PP_SL };
	int							m_swapChainCounter{ 0 };
	CNVRHISwapChainDXGI*		m_currentSwapChain{ nullptr };
	bool						m_windowed{ false };
};

