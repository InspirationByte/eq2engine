//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPIDX9 render state
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IRenderState.h"

// it's just a simple holder, no more...
class CGLDepthStencilState : public IRenderState
{
public:
	RenderStateType_e	GetType() const { return RENDERSTATE_DEPTHSTENCIL; }
	const void*			GetDescPtr() const { return &m_params; }

	DepthStencilStateParams m_params;
};

// it's just a simple holder, no more...
class CGLRasterizerState : public IRenderState
{
public:
	RenderStateType_e	GetType() const { return RENDERSTATE_RASTERIZER; }
	const void*			GetDescPtr() const { return &m_params; }

	RasterizerStateParams	m_params;
};

// it's just a simple holder, no more...
class CGLBlendingState : public IRenderState
{
public:
	RenderStateType_e	GetType() const { return RENDERSTATE_BLENDING; }
	const void*			GetDescPtr() const { return &m_params; }

	BlendStateParams	m_params;
};
