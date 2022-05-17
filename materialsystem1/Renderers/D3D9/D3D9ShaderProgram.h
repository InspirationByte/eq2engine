//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9SHADERPROGRAM_H
#define D3D9SHADERPROGRAM_H

#include "ds/eqstring.h"
#include "ds/Map.h"

#include "renderers/IShaderProgram.h"
#include "renderers/ShaderAPI_defs.h"

#include <d3d9.h>


#define MAX_CONSTANT_NAMELEN 64

struct DX9ShaderConstant_t
{
	char	name[MAX_CONSTANT_NAMELEN]{ 0 };
	int		nameHash{ 0 };
	int		vsReg{ -1 };
	int		psReg{ -1 };
	int		constFlags{ 0 };
};

struct DX9Sampler_t
{
	char	name[MAX_CONSTANT_NAMELEN]{ 0 };
	int		nameHash{ 0 };
	int		index{ -1 };
	int		vsIndex{ -1 };
};

struct ID3DXConstantTable;

class CD3D9ShaderProgram : public IShaderProgram
{
public:
	friend class			ShaderAPID3DX9;

							CD3D9ShaderProgram();
							~CD3D9ShaderProgram();

	const char*				GetName() const;
	int						GetNameHash() const { return m_nameHash; }
	void					SetName(const char* pszName);

	int						GetConstantsNum() const;
	int						GetSamplersNum() const;

protected:
	EqString					m_szName;
	int							m_nameHash;

	LPDIRECT3DVERTEXSHADER9		m_pVertexShader;
	LPDIRECT3DPIXELSHADER9		m_pPixelShader;
	ID3DXConstantTable*			m_pVSConstants;
	ID3DXConstantTable*			m_pPSConstants;

	Map<int, DX9ShaderConstant_t>	m_constants{ PP_SL };
	Map<int, DX9Sampler_t>			m_samplers{ PP_SL };
};

#endif //D3D9SHADERPROGRAM_H
