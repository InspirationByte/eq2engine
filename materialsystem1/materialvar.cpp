//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Material Variable
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "materialsystem1/renderers/IShaderAPI.h"
#include "materialvar.h"

#define VAR_ELEM_OPEN	'['
#define VAR_ELEM_CLOSE	']'

void MatVarHelper::Init(MatVarData& data, const char* pszValue)
{
	if (!pszValue)
		return;

	SetString(data, pszValue);
}

void MatVarHelper::SetString(MatVarData& data, const char* szValue)
{
	data.strValue = szValue;
	data.vector[0] = 0.0f;

	const char* vchar = data.strValue;

	if(*vchar == VAR_ELEM_OPEN)
	{
		int vec_count = 0;
		char temp[32]{ 0 };

		const char* start = ++vchar;

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

				data.vector[vec_count] = (float)atof(start);
				vec_count++;

				start = nullptr;
			}
		}
		while(vchar++);

		data.intValue = data.vector[0];
	}
	else
	{
		data.intValue = (int)atoi(data.strValue);
		data.vector[0] = (float)atof(data.strValue);
	}
}

