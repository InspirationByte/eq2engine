//******************* Copyright (C) Illusion Way, L.L.C 2010 *********************
//
// Description: DarkTech materialvar
//
//****************************************************************************

#ifndef IMATERIALVAR_H
#define IMATERIALVAR_H

#include "math/Vector.h"
#include "ppmem.h"

class ITexture;

class IMatVar
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual ~IMatVar() {}

	// get material var name
	virtual const char*		GetName() const	= 0;

	//set material var name
	virtual void			SetName(const char* szNewName)	= 0;

	// get material var string value
	virtual const char*		GetString()	= 0;

	// get material var integer value
	virtual int				GetInt() const	= 0;

	// get material var float value
	virtual float			GetFloat() const	= 0;

	// get vector value
	virtual const Vector2D&	GetVector2() const	= 0;
	virtual const Vector3D&	GetVector3() const	= 0;
	virtual const Vector4D&	GetVector4() const	= 0;

	// texture pointer
	virtual ITexture*		GetTexture() const = 0;

	// assign texture
	virtual void			AssignTexture(ITexture* pTexture) = 0;

	// Value setup
	virtual void			SetString(const char* szValue) = 0;
	virtual void			SetFloat(float fValue) = 0;
	virtual void			SetInt(int nValue) = 0;
	virtual void			SetVector2(const Vector2D& vector)	= 0;
	virtual void			SetVector3(const Vector3D& vector)	= 0;
	virtual void			SetVector4(const Vector4D& vector)	= 0;
};

#endif //IMATERIALVAR_H
