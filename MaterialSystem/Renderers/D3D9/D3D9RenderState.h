//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPIDX9 render state
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9RENDERSTATE_H
#define D3D9RENDERSTATE_H

#include "IRenderState.h"

// it's just a simple holder, no more...
class CD3D9DepthStencilState : public IRenderState
{
public:
	CD3D9DepthStencilState()
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
class CD3D9RasterizerState : public IRenderState
{
public:
	CD3D9RasterizerState()
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
class CD3D9BlendingState : public IRenderState
{
public:
	CD3D9BlendingState()
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