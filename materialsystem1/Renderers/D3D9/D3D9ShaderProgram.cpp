//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#include "D3D9ShaderProgram.h"
#include "renderers/ShaderAPI_defs.h"

#include "utils/strtools.h"

#include <d3d9.h>
#include <d3dx9.h>

CD3D9ShaderProgram::CD3D9ShaderProgram()
{
	m_pVertexShader = nullptr;
	m_pPixelShader = nullptr;
	m_pVSConstants = nullptr;
	m_pPSConstants = nullptr;
}

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

const char*	CD3D9ShaderProgram::GetName() const
{
	return m_szName.GetData();
}

void CD3D9ShaderProgram::SetName(const char* pszName)
{
	m_szName = pszName;
	m_nameHash = StringToHash(pszName);
}

int	CD3D9ShaderProgram::GetConstantsNum() const
{
	return m_constants.size();
}

int	CD3D9ShaderProgram::GetSamplersNum() const
{
	return m_samplers.size();
}