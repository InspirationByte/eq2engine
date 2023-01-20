//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material Variable
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "materialvar.h"

CMatVar::~CMatVar()
{
	m_data.texture = nullptr;
}

void CMatVar::Init(const char* pszName, const char* pszValue)
{
	m_name = pszName;
	m_nameHash = StringToHash(m_name.ToCString(), true);

	if(pszValue)
		SetString( pszValue );
}

const char* CMatVar::GetName() const
{
	return m_name.ToCString();
}

void CMatVar::SetString(const char* szValue)
{
	m_data.pszValue = szValue;
	
	char* vchar = (char*)m_data.pszValue.ToCString();

	m_data.vector = 0.0f;

#define VAR_ELEM_OPEN	'['
#define VAR_ELEM_CLOSE	']'

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
					start = nullptr;
					continue; // too many elements
				}

				int len = vchar-start;
				strncpy(temp, start, min(len, (int)sizeof(temp)-1));

				m_data.vector[vec_count] = (float)atof(start);
				vec_count++;

				start = nullptr;
			}
		}
		while(vchar++);

		m_data.intValue = m_data.vector.x;
	}
	else
	{
		m_data.intValue = (int)atoi(m_data.pszValue.ToCString());
		m_data.vector.x = (float)atof(m_data.pszValue.ToCString());
	}
}

