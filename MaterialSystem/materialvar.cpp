//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech materialvar
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "materialvar.h"
#include "IMaterialSystem.h"

CMatVar::CMatVar()
{
	m_flValue = 0.0f;
	m_nValue = 0;
	m_vVector = Vector4D(0);
	m_pAssignedTexture = NULL;
}

// initializes the material var
void CMatVar::Init(const char* pszName,const char* pszValue)
{
	m_pszVarName = pszName;
	m_pszValue	 = pszValue;

	m_flValue = (float)atof(m_pszValue.GetData());
	m_nValue  = (int)m_flValue;

	sscanf(pszValue,"[%f %f]",&m_vVector.x, &m_vVector.y);
	sscanf(pszValue,"[%f %f %f]",&m_vVector.x, &m_vVector.y, &m_vVector.z);
	sscanf(pszValue,"[%f %f %f %f]",&m_vVector.x, &m_vVector.y, &m_vVector.z, &m_vVector.w);
}

int CMatVar::GetInt()
{
	return m_nValue;
}

float CMatVar::GetFloat()
{
	return m_flValue;
}

const char* CMatVar::GetName()
{
	return m_pszVarName.GetData();
}
// sets new name
void CMatVar::SetName(const char* szNewName)
{
	m_pszVarName = szNewName;
}

// gives string
const char* CMatVar::GetString()
{
	return m_pszValue.GetData();
}

// Value setup
void CMatVar::SetString(const char* szValue)
{
	m_pszValue = szValue;

	m_flValue = (float)atof(m_pszValue.GetData());
	m_nValue  = (int)m_flValue;
	sscanf(szValue,"[%f %f]",&m_vVector.x, &m_vVector.y);
	sscanf(szValue,"[%f %f %f]",&m_vVector.x, &m_vVector.y, &m_vVector.z);
	sscanf(szValue,"[%f %f %f %f]",&m_vVector.x, &m_vVector.y, &m_vVector.z, &m_vVector.w);
}

void CMatVar::SetFloat(float fValue)
{
	m_flValue = fValue;
	m_nValue  = (int)m_flValue;
	m_pszValue = varargs("%f",m_flValue);
	m_vVector = Vector4D(fValue,0,0,0);
}

void CMatVar::SetInt(int nValue)
{
	m_nValue = nValue;
	m_flValue = m_nValue;
	m_pszValue = varargs("%i",m_nValue);
	m_vVector = Vector4D(nValue,0,0,1);
}

void CMatVar::SetVector2(Vector2D &vector)
{
	m_vVector.x = vector.x;
	m_vVector.y = vector.y;
	m_vVector.z = 0.0f;
	m_vVector.w = 1.0f;
}

void CMatVar::SetVector3(Vector3D &vector)
{
	m_vVector.x = vector.x;
	m_vVector.y = vector.y;
	m_vVector.z = vector.z;
	m_vVector.w = 1.0f;
}

void CMatVar::SetVector4(Vector4D &vector)
{
	m_vVector = vector;
}

Vector2D CMatVar::GetVector2()
{
	return m_vVector.xy();
}

Vector3D CMatVar::GetVector3()
{
	return m_vVector.xyz();
}

Vector4D CMatVar::GetVector4()
{
	return m_vVector;
}

// texture pointer
ITexture* CMatVar::GetTexture()
{
	return m_pAssignedTexture;
}

// assigns texture
void CMatVar::AssignTexture(ITexture* pTexture)
{
	m_pAssignedTexture = pTexture;
}
