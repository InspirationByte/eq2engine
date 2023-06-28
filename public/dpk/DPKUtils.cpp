///////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "DPKUtils.h"

// Fixes slashes in the directory name
void DPK_RebuildFilePath(const char* str, char* newstr)
{
	char* pnewstr = newstr;
	char cprev = 0;

	while (*str)
	{
		while (cprev == *str && (cprev == CORRECT_PATH_SEPARATOR || cprev == INCORRECT_PATH_SEPARATOR))
			str++;

		*pnewstr = *str;
		pnewstr++;

		cprev = *str;
		str++;
	}

	*pnewstr = 0;
}

void DPK_FixSlashes(EqString& str)
{
	char* tempStr = (char*)stackalloc(str.Length() + 1);
	memset(tempStr, 0, str.Length());

	DPK_RebuildFilePath(str.ToCString(), tempStr);

	char* ptr = tempStr;
	while (*ptr)
	{
		if (*ptr == '\\')
			*ptr = '/';

		ptr++;
	}

	str.Assign(tempStr);
}

int DPK_FilenameHash(const char* filename)
{
	// TODO: hash function that could be used with root path concatenation
	return StringToHash(filename, true);
}