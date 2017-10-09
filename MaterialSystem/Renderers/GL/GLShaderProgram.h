//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLSHADERPROGRAM_H
#define GLSHADERPROGRAM_H

#include "IShaderProgram.h"
#include "utils/eqstring.h"
#include "ShaderAPI_defs.h"

#ifdef USE_GLES2
#include "glad_es3.h"
#else
#include "glad.h"
#endif

#define MAX_CONSTANT_NAMELEN 64

struct GLShaderConstant_t
{
	char name[MAX_CONSTANT_NAMELEN];
	ubyte*			data;
	uint			index;
	ER_ConstantType	type;
	int				nElements;
	bool			dirty;
};

struct GLShaderSampler_t
{
	char name[MAX_CONSTANT_NAMELEN];
	uint index;
};

class CGLShaderProgram : public IShaderProgram
{
public:
	friend class			ShaderAPIGL;

							CGLShaderProgram();
							~CGLShaderProgram();

	const char*				GetName() {return m_szName.c_str();}
	void					SetName(const char* pszName) {m_szName = pszName;}

	int						GetConstantsNum() {return m_numConstants;}
	int						GetSamplersNum() {return m_numSamplers;}

protected:
	EqString				m_szName;

	GLhandleARB				m_program;
	GLhandleARB				m_vertexShader;
	GLhandleARB				m_fragmentShader;

	// GLhandleARB			m_geomShader;
	// GLhandleARB			m_hullShader;

	GLShaderConstant_t*		m_constants; // or uniforms
	GLShaderSampler_t*		m_samplers;

	int						m_numConstants;
	int						m_numSamplers;
};

#endif //GLSHADERPROGRAM_H
