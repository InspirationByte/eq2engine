/////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2024
//////////////////////////////////////////////////////////////////////////////////
// Description: NVRHI renderer D3D11
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <nvrhi/nvrhi.h>
#include <nvrhi/d3d11.h>
#include "../IRenderLibrary.h"
#include "../RenderWorker.h"
#include "NVRHILibraryDXGIBase.h"

class CNVRHIRenderLibD3D11
	: public CNVRHIRenderLibDXGIBase
	, public RenderWorkerHandler
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

	RefCountPtr<ID3D11Device>		m_rhiDevice12;
	RefCountPtr<ID3D11CommandQueue>	m_rhiGraphicsQueue;
	RefCountPtr<ID3D11CommandQueue>	m_rhiComputeQueue;
	RefCountPtr<ID3D11CommandQueue>	m_rhiCopyQueue;
};

