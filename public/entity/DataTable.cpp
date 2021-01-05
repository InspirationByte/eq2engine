//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Entity data table for use in savegames and restoring from
//				savegames / parsing entity KVs
//////////////////////////////////////////////////////////////////////////////////

#include "DataTable.h"
#include "utils/strtools.h"

#include "core/DebugInterface.h"
#include "core/platform/Platform.h"

bool CDataVariable::Convert( DataVarType_e newType )
{
	if ( newType == varType )
		return true;

	// there VOID is NULL
	if ( newType == DTVAR_TYPE_VOID )
	{
		iVal[0] = 0;
		newType = DTVAR_TYPE_VOID;
		return true;
	}

	switch ( varType )
	{
		case DTVAR_TYPE_INTEGER:
		{
			switch ( newType )
			{
				case DTVAR_TYPE_FLOAT:
				{
					SetFloat( (float)iVal[0] );
					return true;
				}
				case DTVAR_TYPE_BOOLEAN:
				{
					SetBool( iVal > 0 );
					return true;
				}
			}
			break;
		}
		case DTVAR_TYPE_FLOAT:
		{
			switch ( newType )
			{
				case DTVAR_TYPE_INTEGER:
				{
					SetInt((int)flVal);
					return true;
				}
				case DTVAR_TYPE_BOOLEAN:
				{
					SetBool( flVal > 0 );
					return true;
				}
			}
			break;
		}
		// the most types are converted from string
		case DTVAR_TYPE_STRING:
		{
			switch ( newType )
			{
				case DTVAR_TYPE_INTEGER:
				{
					if (pszVal[0])
						SetInt(atoi(pszVal));
					else
						SetInt(0);

					return true;
				}
				case DTVAR_TYPE_FLOAT:
				{
					if (pszVal[0])
						SetFloat( atof(pszVal) );
					else
						SetFloat(0);

					return true;
				}
				case DTVAR_TYPE_BOOLEAN:
				{
					if (pszVal[0])
						SetBool( atoi(pszVal) > 0 );
					else
						SetBool(false);

					return true;
				}
				case DTVAR_TYPE_VECTOR3D:
				{
					Vector3D tmpVec = vec3_zero;

					if (sscanf(pszVal, "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
						sscanf(pszVal, "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);

					SetVector3D( tmpVec );
					return true;
				}

			}
		
			break;
		}
	}

	// invalid conversion
	return false;
}

void DataMap_PrintFlagsStr(int flags)
{
	if(flags & FIELD_KEY)
		Msg("FIELD_KEY ");

	if(flags & FIELD_FUNCTIONPTR)
		Msg("FIELD_FUNCTION ");

	if(flags & FIELD_ARRAY)
		Msg("FIELD_ARRAY ");

	if(flags & FIELD_LIST)
		Msg("FIELD_LIST ");

	if(flags & FIELD_LINKEDLIST)
		Msg("FIELD_LINKEDLIST ");
}

void DataMap_Print(dataDescMap_t* dataMap, int spaces)
{
	if(!dataMap)
		return;

	char* spaceStr = (char*)stackalloc(spaces+1);

	for(int i = 0; i < spaces; i++)
		spaceStr[i] = ' ';

	spaceStr[spaces] = 0;

	while(dataMap != nullptr)
	{
		Msg(spaceStr);
		Msg("'%s' fields: %d, size: %d\n", dataMap->dataClassName, dataMap->numFields, dataMap->dataSize);

		for(int i = 0; i < dataMap->numFields; i++)
		{
			dataDescField_t& mapVar = dataMap->fields[i];
			Msg(spaceStr);
			Msg("field '%s' (%s) offset=%d ", mapVar.name, s_dataVarTypeNames[mapVar.type], mapVar.offset);

			DataMap_PrintFlagsStr(mapVar.nFlags);
			Msg("\n");

			if(mapVar.type == DTVAR_TYPE_NESTED)
			{
				Msg(spaceStr);

				Msg("{\n");
				DataMap_Print(mapVar.dataMap, spaces+4);

				Msg(spaceStr);
				Msg("}\n");
			}
		}

		dataMap = dataMap->baseMap;
	}
}