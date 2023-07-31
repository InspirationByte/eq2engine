//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2023
//////////////////////////////////////////////////////////////////////////////////
// Description: D3D11 shader impl for Eq
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ShaderProgram.h"

#define MAX_CONSTANT_NAMELEN 64

struct DX10ShaderConstant 
{
	char*	name;
	ubyte*	vsData;
	ubyte*	gsData;
	ubyte*	psData;
	int		vsBuffer;
	int		gsBuffer;
	int		psBuffer;

	int		constFlags;
};

struct DX10Sampler_t
{
	char	name[MAX_CONSTANT_NAMELEN]{ 0 };
	int		nameHash{ 0 };
	int		index{ -1 };
	int		vsIndex{ -1 };
	int		gsIndex{ -1 };
};

class CD3D10ShaderProgram : public CShaderProgram
{
	friend class			ShaderAPID3DX10;
public:
	~CD3D10ShaderProgram();

protected:
	ID3D10VertexShader*		m_vertexShader{ nullptr };
	ID3D10PixelShader*		m_pixelShader{ nullptr };
	ID3D10GeometryShader*	m_geomShader{ nullptr };
	ID3D10Blob*				m_inputSignature{ nullptr };

	ID3D10Buffer**			m_vsConstants{ nullptr };
	ID3D10Buffer**			m_gsConstants{ nullptr };
	ID3D10Buffer**			m_psConstants{ nullptr };

	ubyte**					m_vsConstMem{ nullptr };
	ubyte**					m_gsConstMem{ nullptr };
	ubyte**					m_psConstMem{ nullptr };

	bool*					m_vsDirty{ nullptr };
	bool*					m_gsDirty{ nullptr };
	bool*					m_psDirty{ nullptr };

	int						m_VSCBuffers{ 0 }; // TODO: add num prefix
	int						m_GSCBuffers{ 0 };
	int						m_PSCBuffers{ 0 };

	DX10ShaderConstant*		m_constants{ nullptr };
	DX10Sampler_t*			m_samplers{ nullptr };
	DX10Sampler_t*			m_textures{ nullptr };

	int						m_numConstants{ 0 };
	int						m_numSamplers{ 0 };
	int						m_numTextures{ 0 };
};
