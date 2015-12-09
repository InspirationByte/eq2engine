
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
	const char* GetName();

	// sets new name
	void		SetName(const char* szNewName);

	// Value returners
	const char* GetString();
	int			GetInt();
	float		GetFloat();

	Vector2D	GetVector2();
	Vector3D	GetVector3();
	Vector4D	GetVector4();

	// Value setup
	void		SetString(const char* szValue);
	void		SetFloat(float fValue);
	void		SetInt(int nValue);

	void		SetVector2(Vector2D &vector);
	void		SetVector3(Vector3D &vector);
	void		SetVector4(Vector4D &vector);

	// texture pointer
	ITexture*	GetTexture();

	// assigns texture
	void		AssignTexture(ITexture* pTexture);

private:
	EqString	m_pszVarName;
	EqString	m_pszValue;

	Vector4D	m_vVector;
	float		m_flValue;
	int			m_nValue;

	ITexture*	m_pAssignedTexture;
};

#endif //CMATVAR_H
