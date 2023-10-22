#pragma once
#include <webgpu/webgpu.h>
#include <dawn/dawn_wsi.h>
#include "renderers/IEqSwapChain.h"

class CWGPURenderLib;

class CWGPUSwapChain : public IEqSwapChain
{
public:
	friend class CWGPURenderLib;

	~CWGPUSwapChain();
	CWGPUSwapChain(CWGPURenderLib* host, void* window);

	void*			GetWindow() const { return m_window; }
	int				GetMSAASamples() const { return m_msaaSamples; }

	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();
	
protected:
	CWGPURenderLib*				m_host{ nullptr };

	void*						m_window{ nullptr };
	DawnSwapChainImplementation m_swapImpl;
	WGPUTextureFormat			m_swapFmt{ WGPUTextureFormat_Undefined };
	WGPUSwapChain				m_swapChain{ nullptr };
	IVector2D					m_backbufferSize{ 0 };
	int							m_msaaSamples{ 1 };
};