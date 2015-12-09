//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPIDX10 render state
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D10RENDERSTATE_H
#define D3D10RENDERSTATE_H

#include "IRenderState.h"

#include <d3d10.h>
#include <d3dx10.h>

// it's just a simple holder, no more...
class CD3D10DepthStencilState : public IRenderState
{
public:
	CD3D10DepthStencilState()
	{
		m_dsState = NULL;
	}

	RenderStateType_e	GetType() {return RENDERSTATE_DEPTHSTENCIL;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	DepthStencilStateParams_t	m_params;

	ID3D10DepthStencilState*	m_dsState;
};

// it's just a simple holder, no more...
class CD3D10RasterizerState : public IRenderState
{
public:
	CD3D10RasterizerState()
	{
		m_rsState = NULL;
	}

	RenderStateType_e	GetType() {return RENDERSTATE_RASTERIZER;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	RasterizerStateParams_t		m_params;
	ID3D10RasterizerState*		m_rsState;
};

// it's just a simple holder, no more...
class CD3D10BlendingState : public IRenderState
{
public:
	CD3D10BlendingState()
	{
	}

	RenderStateType_e	GetType() {return RENDERSTATE_BLENDING;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	BlendStateParam_t	m_params;

	ID3D10BlendState*	m_blendState;
};

// it's just a simple holder, no more...
class CD3D10SamplerState : public IRenderState
{
public:
	CD3D10SamplerState()
	{
	}

	RenderStateType_e	GetType() {return RENDERSTATE_SAMPLER;}

	void* GetDescPtr()
	{
		return &m_params;
	}

	SamplerStateParam_t	m_params;

	ID3D10SamplerState*	m_samplerState;
};


#endif // D3D10RENDERSTATE_H