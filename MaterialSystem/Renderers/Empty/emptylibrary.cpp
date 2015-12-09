//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D Rendering library interface
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
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

bool CEmptyRenderLib::InitAPI(const shaderapiinitparams_t &params)
{
	savedParams = params;

	m_Renderer = new ShaderAPIEmpty();
	m_Renderer->Init(savedParams);

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

bool CEmptyRenderLib::CaptureScreenshot(CImage &img)
{
	return false;
}
