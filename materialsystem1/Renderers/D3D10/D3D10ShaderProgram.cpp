//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX10 Shader program for ShaderAPID3DX10
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include <d3dx10.h>

#include "Platform.h"
#include "ShaderAPI_defs.h"
#include "D3D10ShaderProgram.h"

CD3D10ShaderProgram::CD3D10ShaderProgram()
{
	m_pVertexShader = NULL;
	m_pPixelShader = NULL;
	m_pGeomShader = NULL;
	m_pInputSignature = NULL;

	m_pvsConstants = NULL;
	m_pgsConstants = NULL;
	m_ppsConstants = NULL;

	m_pvsConstMem = NULL;
	m_pgsConstMem = NULL;
	m_ppsConstMem = NULL;

	m_pvsDirty = NULL;
	m_pgsDirty = NULL;
	m_ppsDirty = NULL;

	m_nVSCBuffers = 0;
	m_nGSCBuffers = 0;
	m_nPSCBuffers = 0;

	m_pConstants = NULL;
	m_pSamplers = NULL;
	m_pTextures = NULL;

	m_numConstants = 0;
	m_numSamplers = 0;
	m_numTextures = 0;
}

CD3D10ShaderProgram::~CD3D10ShaderProgram()
{
	if(m_pVertexShader)
		m_pVertexShader->Release();

	if(m_pPixelShader)
		m_pPixelShader->Release();

	if(m_pGeomShader)
		m_pGeomShader->Release();

	if(m_pInputSignature)
		m_pInputSignature->Release();

	for(uint i = 0; i < m_nVSCBuffers; i++)
		m_pvsConstants[i]->Release();

	if(m_pvsConstants)
		delete [] m_pvsConstants;

	for(uint i = 0; i < m_nGSCBuffers; i++)
		m_pgsConstants[i]->Release();

	if(m_pgsConstants)
		delete [] m_pgsConstants;

	for(uint i = 0; i < m_nPSCBuffers; i++)
		m_ppsConstants[i]->Release();

	if(m_ppsConstants)
		delete [] m_ppsConstants;
		
	// delete all data carefully
	for(int i = 0; i < m_numConstants; i++)
		delete [] m_pConstants[i].name;

	if(m_pConstants)
		free(m_pConstants);

	if(m_pSamplers)
		free(m_pSamplers);

	if(m_pTextures)
		free(m_pTextures);

	if(m_ppsDirty)
		delete [] m_ppsDirty;
	if(m_pvsDirty)
		delete [] m_pvsDirty;
	if(m_pgsDirty)
		delete [] m_pgsDirty;
}

const char*	CD3D10ShaderProgram::GetName()
{
	return m_szName.GetData();
}

void CD3D10ShaderProgram::SetName(const char* pszName)
{
	m_szName = pszName;
}

int	CD3D10ShaderProgram::GetConstantsNum()
{
	return m_numConstants;
}

int	CD3D10ShaderProgram::GetSamplersNum()
{
	return m_numSamplers;
}