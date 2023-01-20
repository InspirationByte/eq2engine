//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Material Variable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class CMatVar;
class ITexture;
using ITexturePtr = CRefPtr<ITexture>;

class IMaterial;
using IMaterialPtr = CRefPtr<IMaterial>;

class MatVarProxy
{
	friend class CMaterial;
public:
	MatVarProxy() = default;

	bool				IsValid() const { return m_material; }

	void				SetInt(int nValue);
	void				SetFloat(float fValue);
	void				SetVector2(const Vector2D& vector);
	void				SetVector3(const Vector3D& vector);
	void				SetVector4(const Vector4D& vector);
	void				SetTexture(const ITexturePtr& pTexture);

	const char*			GetString() const;
	int					GetInt() const;
	float				GetFloat() const;
	const Vector2D&		GetVector2() const;
	const Vector3D&		GetVector3() const;
	const Vector4D&		GetVector4() const;
	ITexturePtr			GetTexture() const;

	void				operator=(std::nullptr_t) { m_matVarIdx = -1; m_material = nullptr; }

private:
	MatVarProxy(int varIdx, IMaterial* owner); // only CMatVar can initialize us

	CWeakPtr<IMaterial> m_material;	// used only to track material existance
	int					m_matVarIdx{ -1 };
};

// TODO: move somewhere else
#include "IMaterial.h"
#include "renderers/ITexture.h"
#include "renderers/IShaderAPI.h"

inline MatVarProxy::MatVarProxy(int varIdx, IMaterial* owner)
{
	ASSERT(varIdx != -1);
	ASSERT(owner);

	m_matVarIdx = varIdx;
	m_material = CWeakPtr(owner);
}

inline int MatVarProxy::GetInt() const
{
	if (!m_material)
		return 0;

	return m_material->VarAt(m_matVarIdx).intValue;
}

inline float MatVarProxy::GetFloat() const
{
	if (!m_material)
		return 0.0f;

	return m_material->VarAt(m_matVarIdx).vector.x;
}

// gives string
inline const char* MatVarProxy::GetString() const
{
	if (!m_material)
		return nullptr;

	return m_material->VarAt(m_matVarIdx).pszValue;
}

inline void MatVarProxy::SetFloat(float fValue)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	var.vector = Vector4D(fValue,0,0,0);
	var.intValue  = (int)var.vector.x;
}

inline void MatVarProxy::SetInt(int nValue)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	var.intValue = nValue;
	var.vector = Vector4D(nValue,0,0,0);
}

inline void MatVarProxy::SetVector2(const Vector2D& vector)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	var.vector = Vector4D(vector, 0.0f, 1.0f);
}

inline void MatVarProxy::SetVector3(const Vector3D& vector)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	var.vector = Vector4D(vector, 1.0f);
}

inline void MatVarProxy::SetVector4(const Vector4D& vector)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	var.vector = vector;
}

inline const Vector2D& MatVarProxy::GetVector2() const
{
	if (!m_material)
		return vec2_zero;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	return var.vector.xy();
}

inline const Vector3D& MatVarProxy::GetVector3() const
{
	if (!m_material)
		return vec3_zero;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	return var.vector.xyz();
}

inline const Vector4D& MatVarProxy::GetVector4() const
{
	if (!m_material)
		return vec4_zero;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	return var.vector;
}

// texture pointer
inline ITexturePtr MatVarProxy::GetTexture() const
{
	if (!m_material)
		return nullptr;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	return var.texture;
}

// assigns texture
inline void MatVarProxy::SetTexture(const ITexturePtr& pTexture)
{
	if (!m_material)
		return;

	MatVarData& var = m_material->VarAt(m_matVarIdx);
	if (pTexture == var.texture)
		return;

	var.texture = pTexture;
}