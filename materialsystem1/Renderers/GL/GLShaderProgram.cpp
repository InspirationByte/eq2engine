//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "shaderapigl_def.h"
#include "GLShaderProgram.h"
#include "ShaderAPIGL.h"

bool GLCheckError(const char* op, ...);

CGLShaderProgram::CGLShaderProgram()
{
	m_program = 0;
}

CGLShaderProgram::~CGLShaderProgram()
{
	for (auto it = m_constants.begin(); !it.atEnd(); ++it)
		delete [] it.value().data;
}

void CGLShaderProgram::Ref_DeleteObject()
{
	s_renderApi.FreeShaderProgram(this);
	RefCountedObject::Ref_DeleteObject();
}

int	CGLShaderProgram::GetConstantsNum() const
{
	return m_constants.size();
}

int	CGLShaderProgram::GetSamplersNum() const
{
	return m_samplers.size();
}