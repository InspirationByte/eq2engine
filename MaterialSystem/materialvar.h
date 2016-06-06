
//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech materialvar
//////////////////////////////////////////////////////////////////////////////////

#ifndef CMATVAR_H
#define CMATVAR_H

#include "materialsystem/IMaterialVar.h"
#include "utils/eqstring.h"

class CMatVar : public IMatVar
{
public:
				CMatVar();

	// initializes the material var
	void		Init(const char* pszName,const char* pszValue);

	// returns name
	const char* GetName() const;

	// sets new name
	void		SetName(const char* szNewName);

	// Value returners
	const char* GetString();
	int			GetInt() const;
	float		GetFloat() const;

	Vector2D	GetVector2() const;
	Vector3D	GetVector3() const;
	Vector4D	GetVector4() const;

	// Value setup
	void		SetString(const char* szValue);
	void		SetFloat(float fValue);
	void		SetInt(int nValue);

	void		SetVector2(Vector2D &vector);
	void		SetVector3(Vector3D &vector);
	void		SetVector4(Vector4D &vector);

	// texture pointer
	ITexture*	GetTexture() const;

	// assigns texture
	void		AssignTexture(ITexture* pTexture);

private:
	EqString	m_pszVarName;
	EqString	m_pszValue;

	Vector4D	m_vector;

	int			m_nValue;
	int			m_isDirtyString;

	ITexture*	m_pAssignedTexture;
};

#endif //CMATVAR_H
