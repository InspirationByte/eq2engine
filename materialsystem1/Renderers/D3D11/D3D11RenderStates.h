//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 render states impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <d3d10.h>

#include "renderers/IRenderState.h"
#include "renderers/ShaderAPI_defs.h"


class CD3D10DepthStencilState : public IRenderState
{
public:
	RenderStateType_e	GetType() const { return RENDERSTATE_DEPTHSTENCIL; }
	const void*			GetDescPtr() const { return &m_params;}

	DepthStencilStateParams_t	m_params;
	ID3D10DepthStencilState*	m_dsState{ nullptr };
};


class CD3D10RasterizerState : public IRenderState
{
public:
	RenderStateType_e	GetType() const { return RENDERSTATE_RASTERIZER; }
	const void*			GetDescPtr() const { return &m_params; }

	RasterizerStateParams_t		m_params;
	ID3D10RasterizerState*		m_rsState{ nullptr };
};


class CD3D10BlendingState : public IRenderState
{
public:
	RenderStateType_e	GetType() const {return RENDERSTATE_BLENDING;}
	const void*			GetDescPtr() const { return &m_params;}

	BlendStateParam_t	m_params;
	ID3D10BlendState*	m_blendState{ nullptr };
};


class CD3D10SamplerState : public IRenderState
{
public:
	RenderStateType_e	GetType() const {return RENDERSTATE_SAMPLER;}
	const void*			GetDescPtr() const { return &m_params; }

	SamplerStateParam_t	m_params;
	ID3D10SamplerState*	m_samplerState{ nullptr };
};
