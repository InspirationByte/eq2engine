//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#include <d3d9.h>
#include <d3dx9.h>

#include "ShaderAPI_defs.h"
#include "D3D9ShaderProgram.h"

CD3D9ShaderProgram::CD3D9ShaderProgram()
{
	m_pVertexShader = NULL;
	m_pPixelShader = NULL;
	m_pVSConstants = NULL;
	m_pPSConstants = NULL;

	m_pConstants = NULL;
	m_pSamplers = NULL;

	m_numConstants = 0;
	m_numSamplers = 0;
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

	/*
	// delete all data carefully
	for(int i = 0; i < m_numConstants; i++)
	{
		delete [] m_pConstants[i].name;
	}
	*/

	if(m_pConstants)
		free(m_pConstants);

	/*
	// delete all data carefully
	for(int i = 0; i < m_numSamplers; i++)
	{
		delete [] m_pSamplers[i].name;
	}
	*/

	if(m_pSamplers)
		free(m_pSamplers);
}

const char*	CD3D9ShaderProgram::GetName()
{
	return m_szName.GetData();
}

void CD3D9ShaderProgram::SetName(const char* pszName)
{
	m_szName = pszName;
}

int	CD3D9ShaderProgram::GetConstantsNum()
{
	return m_numConstants;
}

int	CD3D9ShaderProgram::GetSamplersNum()
{
	return m_numSamplers;
}