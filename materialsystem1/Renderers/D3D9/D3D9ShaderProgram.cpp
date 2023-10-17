//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapid3d9_def.h"
#include "D3D9ShaderProgram.h"
#include "ShaderAPID3D9.h"

extern ShaderAPID3D9 s_renderApi;

CD3D9ShaderProgram::~CD3D9ShaderProgram()
{
	if(m_pVertexShader)
		m_pVertexShader->Release();

	if(m_pPixelShader)
		m_pPixelShader->Release();

	if(m_pPSConstants)
		m_pPSConstants->Release();

	if(m_pVSConstants)
		m_pVSConstants->Release();
}

void CD3D9ShaderProgram::Ref_DeleteObject()
{
	s_renderApi.FreeShaderProgram(this);
	RefCountedObject::Ref_DeleteObject();
}