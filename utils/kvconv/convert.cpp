//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Key-Values file converter
//////////////////////////////////////////////////////////////////////////////////

#include "KeyValuesA.h"
#include "DebugInterface.h"

void ConvertOldToNewKeyValues(const char* pszInputFileName, const char* pszOutputFileName)
{
	KeyValuesA kv;
	if(kv.LoadFromFile( pszInputFileName ))
	{
		kv.SaveToFile(pszOutputFileName, -1);
	}
	else
	{
		MsgError("Failed to convert keyvalues file '%s'!\n", pszInputFileName);
	}
}