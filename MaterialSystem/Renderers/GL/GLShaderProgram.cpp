//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#include "GLShaderProgram.h"

CGLShaderProgram::CGLShaderProgram()
{
	m_samplers = NULL;
	m_constants = NULL;

	m_vertexShader = 0;
	m_fragmentShader = 0;
	m_program = 0;
	m_numSamplers = 0;
	m_numConstants = 0;
}

CGLShaderProgram::~CGLShaderProgram()
{
	for (int j = 0; j < m_numConstants; j++)
		delete m_constants[j].data;

	free(m_samplers);
	free(m_constants);

	glDeleteShader(m_vertexShader);
	glDeleteShader(m_fragmentShader);
	glDeleteProgram(m_program);
}
