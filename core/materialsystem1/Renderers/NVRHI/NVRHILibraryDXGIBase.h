/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer (base for D3D11 & D3D12)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include <dxgi1_6.h>
#include "NVRHISwapChainDXGI.h"
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"

class CNVRHISwapChainDXGI;
struct IDXGIAdapter;
using nvrhi::RefCountPtr;

class CNVRHIRenderLibDXGIBase
	: public IRenderLibrary
	, public RenderWorkerHandler
{
	friend class CNVRHISwapChainDXGI;
public:
	static CNVRHIRenderLibDXGIBase* Instance;

	CNVRHIRenderLibDXGIBase() = default;

	virtual bool	InitAPI(const ShaderAPIParams& params);
	virtual void	ExitAPI();

	virtual void	BeginFrame(ISwapChain* swapChain = nullptr);
	virtual void	EndFrame();

	IShaderAPI*		GetRenderer() const;
	ITexturePtr		GetCurrentBackbuffer() const;

	void			SetVSync(bool enable);
	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	virtual ISwapChainPtr	CreateSwapChain(const RenderWindowInfo& windowInfo);
	virtual bool			CreateSwapchainTargets(CNVRHISwapChainDXGI* swapChain) const = 0;
protected:

	// Find an adapter whose name contains the given string.
	static RefCountPtr<IDXGIAdapter> FindAdapter(const wchar_t* targetName);
	
	const char*						GetAsyncThreadName() const { return "EqRenderThread"; }
	void							BeginAsyncOperation(uintptr_t threadId) {}
	void							EndAsyncOperation() {}
	bool							IsMainThread(uintptr_t threadId) const;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC	m_dxgiFullScreenDesc{};
	RefCountPtr<IDXGIFactory2>		m_dxgiFactory;
	RefCountPtr<IDXGIAdapter3>		m_dxgiAdapter;
	nvrhi::DeviceHandle				m_nvrhiDevice;
	nvrhi::EventQueryHandle			m_nvrhiFrameWaitQuery;

	CRefPtr<CNVRHISwapChainDXGI>	m_currentSwapChain;
	CRefPtr<CNVRHISwapChainDXGI>	m_defaultSwapChain;
	int								m_swapChainCounter{ 0 };
	bool							m_windowed{ true };
	bool							m_dxgiTearingSupported{ false };
};

