//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#ifndef GLSHADERPROGRAM_H
#define GLSHADERPROGRAM_H

#include "ds/eqstring.h"
#include "ds/Map.h"

#include "renderers/IShaderProgram.h"
#include "renderers/ShaderAPI_defs.h"

#ifdef USE_GLES2
#include <glad_es3.h>
#else
#include <glad.h>
#endif

#define MAX_CONSTANT_NAMELEN 64

struct GLShaderConstant_t
{
	char			name[MAX_CONSTANT_NAMELEN]{ 0 };
	int				nameHash{ 0 };

	ubyte*			data{ nullptr };
	uint			size{ 0 };
	
	uint			index{ 0 };

	ER_ConstantType	type;
	int				nElements;

	bool			dirty{ false };
};

struct GLShaderSampler_t
{
	char			name[MAX_CONSTANT_NAMELEN]{ 0 };
	int				nameHash{ 0 };

	uint			index{ 0 };
	uint			uniformLoc{ 0 };
};

class CGLShaderProgram : public IShaderProgram
{
public:
	friend class			ShaderAPIGL;

							CGLShaderProgram();
							~CGLShaderProgram();

	const char*				GetName() const;
	int						GetNameHash() const { return m_nameHash; }
	void					SetName(const char* pszName);

	int						GetConstantsNum() const;
	int						GetSamplersNum() const;

protected:
	EqString				m_szName;
	int						m_nameHash;

	GLhandleARB				m_program;

	Map<int, GLShaderConstant_t>	m_constants;
	Map<int, GLShaderSampler_t>		m_samplers;
};

#endif //GLSHADERPROGRAM_H
