//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/ShaderProgram.h"

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

class CGLShaderProgram : public CShaderProgram
{
public:
	friend class			ShaderAPIGL;

							CGLShaderProgram();
							~CGLShaderProgram();

	int						GetConstantsNum() const;
	int						GetSamplersNum() const;

protected:
	GLhandleARB				m_program;

	Map<int, GLShaderConstant_t>	m_constants{ PP_SL };
	Map<int, GLShaderSampler_t>		m_samplers{ PP_SL };
};
