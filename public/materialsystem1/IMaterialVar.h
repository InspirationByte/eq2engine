//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Material Variable
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "IMaterial.h"

using MatBufferProxy = MatVarProxy<GPUBufferView>;
using MatTextureProxy = MatVarProxy<ITexturePtr>;
using MatStringProxy = MatVarProxy<EqString>;
using MatIntProxy = MatVarProxy<int>;
using MatFloatProxy = MatVarProxy<float>;
using MatVec2Proxy = MatVarProxy<Vector2D>;
using MatVec3Proxy = MatVarProxy<Vector3D>;
using MatVec4Proxy = MatVarProxy<Vector4D>;
using MatM3x3Proxy = MatVarProxy<Matrix3x3>;
using MatM4x4Proxy = MatVarProxy<Matrix4x4>;

template<typename T>
inline MatVarProxy<T>::MatVarProxy(int varIdx, MaterialVarBlock& owner)
{
	ASSERT(varIdx != -1);

	m_matVarIdx = varIdx;
	m_vars = CWeakPtr(&owner);
}

template<typename T>
inline void MatVarProxy<T>::Purge()
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	var.texture = nullptr;
	var.strValue.Clear();
}

template<>
inline void MatVarProxy<float>::Set(const float& fValue)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	var.vector[0] = fValue;
}

template<>
inline void MatVarProxy<int>::Set(const int& nValue)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	var.intValue = nValue;
}

template<>
inline void MatVarProxy<Vector2D>::Set(const Vector2D& vector)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	*(Vector2D*)var.vector = vector;
}

template<>
inline void MatVarProxy<Vector3D>::Set(const Vector3D& vector)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	*(Vector3D*)var.vector = vector;
}

template<>
inline void MatVarProxy<Vector4D>::Set(const Vector4D& vector)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	*(Vector4D*)var.vector = vector;
}

template<>
inline void MatVarProxy<Matrix3x3>::Set(const Matrix3x3& matrix)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	*(Matrix3x3*)var.vector = matrix;
}

template<>
inline void MatVarProxy<Matrix4x4>::Set(const Matrix4x4& matrix)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	*(Matrix4x4*)var.vector = matrix;
}

template<>
inline const int& MatVarProxy<int>::Get() const
{
	if (!m_vars)
	{
		static const int _zero = 0;
		return _zero;
	}

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.intValue;
}

template<>
inline const float& MatVarProxy<float>::Get() const
{
	if (!m_vars)
	{
		static const float _zero = 0.0f;
		return _zero;
	}

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.vector[0];
}

template<>
inline float* MatVarProxy<float>::GetArray() const
{
	if (!m_vars)
		return nullptr;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.vector;
}

template<>
inline const EqString& MatVarProxy<EqString>::Get() const
{
	if (!m_vars)
		return EqString::EmptyStr;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.strValue;
}

template<>
inline const Vector2D& MatVarProxy<Vector2D>::Get() const
{
	if (!m_vars)
		return vec2_zero;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return *(Vector2D*)var.vector;
}

template<>
inline const Vector3D& MatVarProxy<Vector3D>::Get() const
{
	if (!m_vars)
		return vec3_zero;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return *(Vector3D*)var.vector;
}

template<>
inline const Vector4D& MatVarProxy<Vector4D>::Get() const
{
	if (!m_vars)
		return vec4_zero;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return *(Vector4D*)var.vector;
}

template<>
inline const Matrix3x3& MatVarProxy<Matrix3x3>::Get() const
{
	if (!m_vars)
		return identity3;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return *(Matrix3x3*)var.vector;
}

template<>
inline const Matrix4x4& MatVarProxy<Matrix4x4>::Get() const
{
	if (!m_vars)
		return identity4;

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return *(Matrix4x4*)var.vector;
}

template<>
inline const ITexturePtr& MatVarProxy<ITexturePtr>::Get() const
{
	if (!m_vars)
		return ITexturePtr::Null();

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.texture;
}

template<>
inline void MatVarProxy<ITexturePtr>::Set(const ITexturePtr& pTexture)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	if (pTexture == var.texture)
		return;

	var.texture = pTexture;
}

template<>
inline const GPUBufferView& MatVarProxy<GPUBufferView>::Get() const
{
	if (!m_vars)
	{
		static GPUBufferView _empty;
		return _empty;
	}

	const MatVarData& var = m_vars->variables[m_matVarIdx];
	return var.buffer;
}

template<>
inline void MatVarProxy<GPUBufferView>::Set(const GPUBufferView& buffer)
{
	if (!m_vars)
		return;

	MatVarData& var = m_vars->variables[m_matVarIdx];
	if (buffer == var.buffer)
		return;

	var.buffer = buffer;
}
