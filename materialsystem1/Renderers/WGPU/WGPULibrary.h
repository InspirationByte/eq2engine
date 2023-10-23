//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <webgpu/webgpu.h>
#include "../IRenderLibrary.h"

class CWGPUSwapChain;

class CWGPURenderLib : public IRenderLibrary
{
	friend class CWGPUSwapChain;
public:
	CWGPURenderLib();
	~CWGPURenderLib();

	bool			InitCaps();

	bool			InitAPI(const ShaderAPIParams& params);
	void			ExitAPI();

	void			BeginFrame(IEqSwapChain* swapChain = nullptr);
	void			EndFrame();

	IShaderAPI*		GetRenderer() const;

	void			SetBackbufferSize(int w, int h);
	void			SetFocused(bool inFocus) {}

	bool			SetWindowed(bool enabled);
	bool			IsWindowed() const;

	bool			CaptureScreenshot(CImage &img);

	IEqSwapChain*	CreateSwapChain(const RenderWindowInfo& windowInfo);
	void			DestroySwapChain(IEqSwapChain* swapChain);

protected:

	WGPUInstance			m_instance{ nullptr };

	WGPUBackendType			m_backendType{ WGPUBackendType_Null };
	WGPUDevice				m_rhiDevice{ nullptr };
	WGPUQueue				m_deviceQueue{ nullptr };

	Array<CWGPUSwapChain*>	m_swapChains{ PP_SL };
	CWGPUSwapChain*			m_currentSwapChain{ nullptr };
	bool					m_windowed{ false };
};

