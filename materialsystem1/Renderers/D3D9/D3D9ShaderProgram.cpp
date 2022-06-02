//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#include "D3D9ShaderProgram.h"
#include "utils/strtools.h"
#include <d3dx9.h>

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

void CD3D9ShaderProgram::SetName(const char* pszName)
{
	m_szName = pszName;
	m_nameHash = StringToHash(pszName);
}