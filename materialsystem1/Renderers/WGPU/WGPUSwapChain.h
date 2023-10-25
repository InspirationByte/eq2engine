#pragma once
#include <webgpu/webgpu.h>
#include "renderers/ISwapChain.h"
#include "renderers/ShaderAPI_defs.h"

class CWGPURenderLib;

class CWGPUSwapChain : public ISwapChain
{
public:
	friend class CWGPURenderLib;

	~CWGPUSwapChain();
	CWGPUSwapChain(CWGPURenderLib* host, const RenderWindowInfo& windowInfo);

	void*			GetWindow() const;
	int				GetMSAASamples() const { return m_msaaSamples; }

	ITexturePtr		GetBackbuffer() const;

	void			GetBackbufferSize(int& wide, int& tall) const;
	bool			SetBackbufferSize(int wide, int tall);

	bool			SwapBuffers();
	
protected:
	CWGPURenderLib*		m_host{ nullptr };

	RenderWindowInfo	m_winInfo;

	WGPUTextureFormat	m_swapFmt{ WGPUTextureFormat_Undefined };
	WGPUSurface			m_surface{ nullptr };
	WGPUSwapChain		m_swapChain{ nullptr };
	IVector2D			m_backbufferSize{ 0 };
	int					m_msaaSamples{ 1 };
};