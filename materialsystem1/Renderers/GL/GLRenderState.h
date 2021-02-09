//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPIDX9 render state
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLRENDERSTATE_H
#define GLRENDERSTATE_H

#include "IRenderState.h"

// it's just a simple holder, no more...
class CGLDepthStencilState : public IRenderState
{
public:
	CGLDepthStencilState()
	{
	}

	RenderStateType_e	GetType() {return RENDERSTATE_DEPTHSTENCIL;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	DepthStencilStateParams_t m_params;
};

// it's just a simple holder, no more...
class CGLRasterizerState : public IRenderState
{
public:
	CGLRasterizerState()
	{
	}

	RenderStateType_e	GetType() {return RENDERSTATE_RASTERIZER;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	RasterizerStateParams_t	m_params;
};

// it's just a simple holder, no more...
class CGLBlendingState : public IRenderState
{
public:
	CGLBlendingState()
	{
	}

	RenderStateType_e	GetType() {return RENDERSTATE_BLENDING;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	BlendStateParam_t	m_params;
};

#endif // D3D9RENDERSTATE_H