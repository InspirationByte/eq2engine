//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: DX10 Shader program for ShaderAPID3DX10
//////////////////////////////////////////////////////////////////////////////////

#ifndef D3D9SHADERPROGRAM_H
#define D3D9SHADERPROGRAM_H

#include "IShaderProgram.h"
#include "ds/eqstring.h"

typedef struct DX10ShaderConstant 
{
	char*	name;
	ubyte*	vsData;
	ubyte*	gsData;
	ubyte*	psData;
	int		vsBuffer;
	int		gsBuffer;
	int		psBuffer;

	int		constFlags;
}DX10ShaderConstant_t;


class CD3D10ShaderProgram : public IShaderProgram
{
public:
	friend class			ShaderAPID3DX10;

							CD3D10ShaderProgram();
							~CD3D10ShaderProgram();

	const char*				GetName();
	void					SetName(const char* pszName);

	int						GetConstantsNum();
	int						GetSamplersNum();

protected:
	EqString					m_szName;

	ID3D10VertexShader*		m_pVertexShader;
	ID3D10PixelShader*		m_pPixelShader;
	ID3D10GeometryShader*	m_pGeomShader;
	ID3D10Blob*				m_pInputSignature;

	ID3D10Buffer **			m_pvsConstants;
	ID3D10Buffer **			m_pgsConstants;
	ID3D10Buffer **			m_ppsConstants;

	ubyte**					m_pvsConstMem;
	ubyte**					m_pgsConstMem;
	ubyte**					m_ppsConstMem;

	bool*					m_pvsDirty;
	bool*					m_pgsDirty;
	bool*					m_ppsDirty;

	uint					m_nVSCBuffers;
	uint					m_nGSCBuffers;
	uint					m_nPSCBuffers;

	DX10ShaderConstant*		m_pConstants;
	Sampler_t*				m_pSamplers;
	Sampler_t*				m_pTextures;

	int						m_numConstants;
	int						m_numSamplers;
	int						m_numTextures;
};

#endif //D3D9SHADERPROGRAM_H
