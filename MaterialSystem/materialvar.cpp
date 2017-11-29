//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech materialvar
//////////////////////////////////////////////////////////////////////////////////

#include "DebugInterface.h"
#include "materialvar.h"
#include "utils/strtools.h"
#include "renderers/IShaderAPI.h"

CMatVar::CMatVar() : m_nameHash(0), m_nValue(0), m_vector(0.0f), m_pAssignedTexture(NULL), m_isDirtyString(0)
{
}

CMatVar::~CMatVar()
{
	AssignTexture(NULL);
}

// initializes the material var
void CMatVar::Init(const char* pszName, const char* pszValue)
{
	SetName(pszName);

	if(pszValue != NULL)
		SetString( pszValue );
}

int CMatVar::GetInt() const
{
	return m_nValue;
}

float CMatVar::GetFloat() const
{
	return m_vector.x;
}

const char* CMatVar::GetName() const
{
	return m_name.c_str();
}

// sets new name
void CMatVar::SetName(const char* szNewName)
{
	m_name = szNewName;
	m_nameHash = StringToHash(m_name.c_str(), true);
}

// gives string
const char* CMatVar::GetString()
{
	if(m_isDirtyString > 0)
	{
		switch(m_isDirtyString)
		{
			case 1:	// float
				m_pszValue = varargs("%f",m_vector.x);
				break;
			case 2:	// int
				m_pszValue = varargs("%i",m_nValue);
				break;
			case 3:	// TODO: vector2D
				break;
			case 4:	// TODO: vector3D
				break;
			case 5:	// TODO: vector4D
				break;
		}
		m_isDirtyString = 0;
	}

	return m_pszValue.GetData();
}

#define VAR_ELEM_OPEN	'['
#define VAR_ELEM_CLOSE	']'

// Value setup
void CMatVar::SetString(const char* szValue)
{
	m_pszValue = szValue;
	
	char* vchar = (char*)m_pszValue.c_str();

	m_vector = 0.0f;

	if(*vchar == VAR_ELEM_OPEN)
	{
		int vec_count = 0;
		char temp[32];

		char* start = ++vchar;

		do
		{
			if(*vchar == 0)
				break;

			if(!start && !isspace(*vchar) && *vchar != VAR_ELEM_CLOSE)
				start = vchar;
			else if(start && (isspace(*vchar) || *vchar == VAR_ELEM_CLOSE))
			{
				if(vec_count == 4)
				{
					start = NULL;
					continue; // too many elements
				}

				int len = vchar-start;
				strncpy(temp, start, min(len, (int)sizeof(temp)-1));

				m_vector[vec_count] = (float)atof(start);
				vec_count++;

				start = NULL;
			}
		}
		while(vchar++);

		m_nValue = m_vector.x;
	}
	else
	{
		m_nValue  = (int)atoi(m_pszValue.c_str());
		m_vector.x = (float)atof(m_pszValue.c_str());
	}

	m_isDirtyString = 0;
}

void CMatVar::SetFloat(float fValue)
{
	m_vector = Vector4D(fValue,0,0,0);
	m_nValue  = (int)m_vector.x;
	m_isDirtyString = 1;
}

void CMatVar::SetInt(int nValue)
{
	m_nValue = nValue;
	m_vector = Vector4D(nValue,0,0,0);
	m_isDirtyString = 2;
}

void CMatVar::SetVector2(const Vector2D& vector)
{
	m_vector.x = vector.x;
	m_vector.y = vector.y;
	m_vector.z = 0.0f;
	m_vector.w = 1.0f;

	m_isDirtyString = 3;
}

void CMatVar::SetVector3(const Vector3D& vector)
{
	m_vector.x = vector.x;
	m_vector.y = vector.y;
	m_vector.z = vector.z;
	m_vector.w = 1.0f;

	m_isDirtyString = 4;
}

void CMatVar::SetVector4(const Vector4D& vector)
{
	m_vector = vector;
	m_isDirtyString = 5;
}

const Vector2D& CMatVar::GetVector2() const
{
	return m_vector.xy();
}

const Vector3D& CMatVar::GetVector3() const
{
	return m_vector.xyz();
}

const Vector4D& CMatVar::GetVector4() const
{
	return m_vector;
}

// texture pointer
ITexture* CMatVar::GetTexture() const
{
	return m_pAssignedTexture;
}

// assigns texture
void CMatVar::AssignTexture(ITexture* pTexture)
{
	if(m_pAssignedTexture)
		g_pShaderAPI->FreeTexture(m_pAssignedTexture);

	m_pAssignedTexture = pTexture;

	if(m_pAssignedTexture)
		m_pAssignedTexture->Ref_Grab();
}
