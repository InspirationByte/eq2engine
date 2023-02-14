//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX10 Shader program for ShaderAPID3DX10
//////////////////////////////////////////////////////////////////////////////////

#include <d3d10.h>
#include "core/core_common.h"
#include "D3D11ShaderProgram.h"

CD3D10ShaderProgram::~CD3D10ShaderProgram()
{
	if(m_vertexShader)
		m_vertexShader->Release();

	if(m_pixelShader)
		m_pixelShader->Release();

	if(m_geomShader)
		m_geomShader->Release();

	if(m_inputSignature)
		m_inputSignature->Release();

	for(int i = 0; i < m_VSCBuffers; i++)
		m_vsConstants[i]->Release();

	if(m_vsConstants)
		delete [] m_vsConstants;

	for(int i = 0; i < m_GSCBuffers; i++)
		m_gsConstants[i]->Release();

	if(m_gsConstants)
		delete [] m_gsConstants;

	for(int i = 0; i < m_PSCBuffers; i++)
		m_psConstants[i]->Release();

	if(m_psConstants)
		delete [] m_psConstants;
		
	// delete all data carefully
	for(int i = 0; i < m_numConstants; i++)
		delete [] m_constants[i].name;

	if(m_constants)
		free(m_constants);

	if(m_samplers)
		free(m_samplers);

	if(m_textures)
		free(m_textures);

	if(m_psDirty)
		delete [] m_psDirty;
	if(m_vsDirty)
		delete [] m_vsDirty;
	if(m_gsDirty)
		delete [] m_gsDirty;
}