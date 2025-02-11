///////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Data package file (dpk)
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "DPKUtils.h"
#include "dpk_defs.h"

static int DPK_FixCharSeparator(const int chr)
{
	constexpr int DPK_PATH_SEPARATOR = '/';

	if(chr == CORRECT_PATH_SEPARATOR || chr == INCORRECT_PATH_SEPARATOR)
		return DPK_PATH_SEPARATOR;
	return chr;
}

static int DPK_HashDJB2(const char* str, int hash = 5381)
{
	// http://www.cse.yorku.ca/~oz/hash.html
	while (int c = *str++)
	{
		// hash * 33 + c
		hash = ((hash << 5) + hash) + DPK_FixCharSeparator((int)CType::LowerChar<char>(c));
	}

	return hash;
}

int DPK_FilenameHash(const char* filename, int version)
{
	// TODO: hash function that could be used with root path concatenation

	if (version == 7)
	{
		int len = CString::Length(filename);
		const char* ptr = filename;

		int hash = len;
		for (; len > 0; --len)
		{
			const int v1 = hash >> 19;
			const int v0 = hash << 5;

			const int chr = CType::LowerChar(*ptr);

			hash = ((v0 | v1) + DPK_FixCharSeparator(chr)) & StringId24Mask;
			++ptr;
		}
		return hash;
	}

	return DPK_HashDJB2(filename);
}

int	DPK_FilenameHashAppend(const char* filename, int startHash)
{
	if(!startHash)
		return DPK_HashDJB2(filename);
	return DPK_HashDJB2(filename, startHash);
}