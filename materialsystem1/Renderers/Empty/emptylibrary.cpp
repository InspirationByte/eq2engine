//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "core/DebugInterface.h"
#include "core/IDkCore.h"
#include "core/IConsoleCommands.h"

#include "emptyLibrary.h"
#include "ShaderAPIEmpty.h"

HOOK_TO_CVAR(r_screen);

// make library
CEmptyRenderLib g_library;

IShaderAPI* g_pShaderAPI = NULL;

CEmptyRenderLib::CEmptyRenderLib()
{
	GetCore()->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
}

CEmptyRenderLib::~CEmptyRenderLib()
{
	GetCore()->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

bool CEmptyRenderLib::InitCaps()
{
	return true;
}

bool CEmptyRenderLib::InitAPI( shaderAPIParams_t &params)
{
	m_Renderer = new ShaderAPIEmpty();
	m_Renderer->Init(params);

	g_pShaderAPI = m_Renderer;

	return true;
}

void CEmptyRenderLib::ExitAPI()
{
	if(!m_Renderer)
		return;

	delete m_Renderer;
	m_Renderer = NULL;
}

void CEmptyRenderLib::BeginFrame()
{

}

void CEmptyRenderLib::EndFrame(IEqSwapChain* swapChain)
{

}

void CEmptyRenderLib::SetBackbufferSize(const int w, const int h)
{

}

// changes fullscreen mode
bool CEmptyRenderLib::SetWindowed(bool enabled)
{
	return true;
}

// speaks for itself
bool CEmptyRenderLib::IsWindowed() const
{
	return ((ShaderAPIEmpty*)m_Renderer)->m_params->windowedMode;
}

bool CEmptyRenderLib::CaptureScreenshot(CImage &img)
{
	return false;
}
