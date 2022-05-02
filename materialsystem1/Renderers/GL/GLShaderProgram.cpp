//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#include "GLShaderProgram.h"
#include "utils/strtools.h"

extern bool GLCheckError(const char* op);

CGLShaderProgram::CGLShaderProgram()
{
	m_program = 0;
}

CGLShaderProgram::~CGLShaderProgram()
{
	for (auto it = m_constants.begin(); it != m_constants.end(); ++it)
		delete [] it.value().data;

	if (m_program)
	{
		glDeleteProgram(m_program);
		GLCheckError("delete shader program");
	}
}

const char* CGLShaderProgram::GetName() const
{
	return m_szName.GetData();
}

void CGLShaderProgram::SetName(const char* pszName)
{
	m_szName = pszName;
	m_nameHash = StringToHash(pszName);
}

int	CGLShaderProgram::GetConstantsNum() const
{
	return m_constants.size();
}

int	CGLShaderProgram::GetSamplersNum() const
{
	return m_samplers.size();
}