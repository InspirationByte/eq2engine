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
#include "ShaderAPIEmpty.h"

HOOK_TO_CVAR(r_screen);

ShaderAPIEmpty s_renderApi;
IShaderAPI* g_renderAPI = &s_renderApi;

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
	s_renderApi.Init(params);
	return true;
}

void CEmptyRenderLib::ExitAPI()
{
}

void CEmptyRenderLib::BeginFrame(IEqSwapChain* swapChain)
{

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
