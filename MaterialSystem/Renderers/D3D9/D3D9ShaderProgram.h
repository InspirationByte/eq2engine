//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9SHADERPROGRAM_H
#define D3D9SHADERPROGRAM_H

#include "IShaderProgram.h"
#include "Utils/EqString.h"

#define MAX_CONSTANT_NAMELEN 64

typedef struct DX9ShaderConstant 
{
	char	name[MAX_CONSTANT_NAMELEN];
	int64	hash;

	int		vsReg;
	int		psReg;

	int		constFlags;
}DX9ShaderConstant_t;

class CD3D9ShaderProgram : public IShaderProgram
{
public:
	friend class			ShaderAPID3DX9;

							CD3D9ShaderProgram();
							~CD3D9ShaderProgram();

	const char*				GetName();
	void					SetName(const char* pszName);

	int						GetConstantsNum();
	int						GetSamplersNum();

protected:
	EqString				m_szName;

	LPDIRECT3DVERTEXSHADER9 m_pVertexShader;
	LPDIRECT3DPIXELSHADER9  m_pPixelShader;
	ID3DXConstantTable*		m_pVSConstants;
	ID3DXConstantTable*		m_pPSConstants;

	DX9ShaderConstant_t*	m_pConstants;
	Sampler_t*				m_pSamplers;

	int						m_numConstants;
	int						m_numSamplers;
};

#endif //D3D9SHADERPROGRAM_H
