/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer D3D12
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/d3d12.h>
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"
#include "NVRHILibraryDXGIBase.h"

using nvrhi::RefCountPtr;

class CNVRHIRenderLibD3D12
	: public CNVRHIRenderLibDXGIBase
	, public RenderWorkerHandler		// might be not needed
{
public:
	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();

	void			BeginFrame(ISwapChain* swapChain = nullptr);
	void			EndFrame();

	IShaderAPI*		GetRenderer() const;
protected:

	const char*		GetAsyncThreadName() const { return "EqRenderThread"; }
	void			BeginAsyncOperation(uintptr_t threadId) {}
	void			EndAsyncOperation() {}
	bool			IsMainThread(uintptr_t threadId) const;

	uintptr_t				m_mainThreadId{ 0 };
	RefCountPtr<ID3D12Device>		m_rhiDevice12;
	RefCountPtr<ID3D12CommandQueue>	m_rhiGraphicsQueue;
	RefCountPtr<ID3D12CommandQueue>	m_rhiComputeQueue;
	RefCountPtr<ID3D12CommandQueue>	m_rhiCopyQueue; 
	nvrhi::DeviceHandle				m_nvrhiDevice;
};

