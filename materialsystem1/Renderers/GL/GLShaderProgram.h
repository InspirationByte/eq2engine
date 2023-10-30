//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: GL Shader program for ShaderAPIGL
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "../Shared/ShaderProgram.h"

#define MAX_CONSTANT_NAMELEN 64

// Reserved, used for shaders only
enum EConstantType : int
{
	CONSTANT_FLOAT,
	CONSTANT_VECTOR2D,
	CONSTANT_VECTOR3D,
	CONSTANT_VECTOR4D,
	CONSTANT_INT,
	CONSTANT_IVECTOR2D,
	CONSTANT_IVECTOR3D,
	CONSTANT_IVECTOR4D,
	CONSTANT_BOOL,
	CONSTANT_BVECTOR2D,
	CONSTANT_BVECTOR3D,
	CONSTANT_BVECTOR4D,
	CONSTANT_MATRIX2x2,
	CONSTANT_MATRIX3x3,
	CONSTANT_MATRIX4x4,

	CONSTANT_COUNT
};

static int s_constantTypeSizes[CONSTANT_COUNT] = {
	sizeof(float),
	sizeof(Vector2D),
	sizeof(Vector3D),
	sizeof(Vector4D),
	sizeof(int),
	sizeof(int) * 2,
	sizeof(int) * 3,
	sizeof(int) * 4,
	sizeof(int),
	sizeof(int) * 2,
	sizeof(int) * 3,
	sizeof(int) * 4,
	sizeof(Matrix2x2),
	sizeof(Matrix3x3),
	sizeof(Matrix4x4),
};

struct GLShaderConstant_t
{
	char			name[MAX_CONSTANT_NAMELEN]{ 0 };
	int				nameHash{ 0 };

	ubyte*			data{ nullptr };
	uint			size{ 0 };
	uint			uniformLoc{ 0 };

	EConstantType	type;
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

	void					Ref_DeleteObject() override;

	int						GetConstantsNum() const;
	int						GetSamplersNum() const;

protected:
	GLhandleARB				m_program;

	Map<int, GLShaderConstant_t>	m_constants{ PP_SL };
	Map<int, GLShaderSampler_t>		m_samplers{ PP_SL };
};
