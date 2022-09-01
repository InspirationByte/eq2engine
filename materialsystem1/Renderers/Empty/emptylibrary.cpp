//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
#include "ShaderAPIEmpty.h"

HOOK_TO_CVAR(r_screen);

// make library
CEmptyRenderLib g_library;
ShaderAPIEmpty s_shaderAPI;
IShaderAPI* g_pShaderAPI = &s_shaderAPI;

CEmptyRenderLib::CEmptyRenderLib()
{
	g_eqCore->RegisterInterface(RENDERER_INTERFACE_VERSION, this);
	m_windowed = true;
}

CEmptyRenderLib::~CEmptyRenderLib()
{
	g_eqCore->UnregisterInterface(RENDERER_INTERFACE_VERSION);
}

bool CEmptyRenderLib::InitCaps()
{
	return true;
}

IShaderAPI* CEmptyRenderLib::GetRenderer() const
{
	return g_pShaderAPI;
}

bool CEmptyRenderLib::InitAPI( const shaderAPIParams_t &params)
{
	s_shaderAPI.Init(params);
	return true;
}

void CEmptyRenderLib::ExitAPI()
{
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
