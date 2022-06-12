//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX9 Shader program for ShaderAPID3DX9
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "renderers/IShaderProgram.h"

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
struct IDirect3DVertexShader9;
struct IDirect3DPixelShader9;

class CD3D9ShaderProgram : public IShaderProgram
{
public:
	friend class ShaderAPID3DX9;

	CD3D9ShaderProgram() = default;
	~CD3D9ShaderProgram();

	const char*						GetName() const { return m_szName.ToCString(); }
	void							SetName(const char* pszName);

protected:
	EqString						m_szName;
	int								m_nameHash{ 0 };

	IDirect3DVertexShader9*			m_pVertexShader{ nullptr };
	IDirect3DPixelShader9*			m_pPixelShader{ nullptr };
	ID3DXConstantTable*				m_pVSConstants{ nullptr };
	ID3DXConstantTable*				m_pPSConstants{ nullptr };

	Map<int, DX9ShaderConstant_t>	m_constants{ PP_SL };
	Map<int, DX9Sampler_t>			m_samplers{ PP_SL };
};
