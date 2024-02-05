//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IConsoleCommands.h"
#include "core/IDkCore.h"
#include "core/ConVar.h"
#include "core/ConCommand.h"
#include "emptyLibrary.h"
#include "renderers/ISwapChain.h"
#include "ShaderAPIEmpty.h"

ShaderAPIEmpty s_renderApi;
IShaderAPI* g_renderAPI = &s_renderApi;

class CEmptySwapChain : public ISwapChain
{
public:
	void		SetVSync(bool enable) {}
	void*		GetWindow()  const { return nullptr; }
	
	ITexturePtr	GetBackbuffer() const 
	{
		return m_backbuffer;
	}

	void		GetBackbufferSize(int& wide, int& tall) const
	{
		wide = 800;
		tall = 600;
	}

	bool		SetBackbufferSize(int wide, int tall)
	{
		return true;
	}

	ITexturePtr	m_backbuffer;
};

CEmptyRenderLib::CEmptyRenderLib()
{
	m_windowed = true;
}

CEmptyRenderLib::~CEmptyRenderLib()
{
}

bool CEmptyRenderLib::InitCaps()
{
	return true;
}

IShaderAPI* CEmptyRenderLib::GetRenderer() const
{
	return g_renderAPI;
}

bool CEmptyRenderLib::InitAPI( const ShaderAPIParams &params)
{
	CreateSwapChain({});

	s_renderApi.Init(params);
	return true;
}

void CEmptyRenderLib::ExitAPI()
{
	for(CEmptySwapChain* swapChain : m_swapChains)
		delete swapChain;
	m_swapChains.clear();
}

void CEmptyRenderLib::BeginFrame(ISwapChain* swapChain)
{
	m_currentSwapChain = swapChain ? swapChain : m_swapChains.front();
}

void CEmptyRenderLib::EndFrame()
{
}

void CEmptyRenderLib::SetBackbufferSize(const int w, const int h)
{

}

// changes fullscreen mode
bool CEmptyRenderLib::SetWindowed(bool enabled)
{
	m_windowed = enabled;
	return true;
}

// speaks for itself
bool CEmptyRenderLib::IsWindowed() const
{
	return m_windowed;
}

bool CEmptyRenderLib::CaptureScreenshot(CImage &img)
{
	return false;
}

ITexturePtr CEmptyRenderLib::GetCurrentBackbuffer() const
{
	return m_currentSwapChain->GetBackbuffer();
}

ISwapChain*	CEmptyRenderLib::CreateSwapChain(const RenderWindowInfo& windowInfo)
{
	bool justCreated = false;

	EqString texName(EqString::Format("swapChain%d", m_swapChainCounter));
	ITexturePtr swapChainTexture = g_renderAPI->FindOrCreateTexture(texName, justCreated);
	++m_swapChainCounter;

	ASSERT_MSG(justCreated, "%s texture already has been created", texName.ToCString());

	CEmptySwapChain* swapChain = PPNew CEmptySwapChain();
	swapChain->m_backbuffer = swapChainTexture;

	m_swapChains.append(swapChain);

	return swapChain;
}

void CEmptyRenderLib::DestroySwapChain(ISwapChain* swapChain)
{
	if(m_swapChains.remove(static_cast<CEmptySwapChain*>(swapChain)))
	{
		SAFE_DELETE(swapChain);
	}	
}