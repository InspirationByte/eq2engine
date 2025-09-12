#include "core/core_common.h"
#include "string_utils.h"
#include "utils/CRC32.h"

//------------------------------------------
// Converts string to 24-bit integer hash
//------------------------------------------
int StringId24(EqStringRef str, bool caseIns )
{
	ASSERT(str);
	return StringId24_Cexpr(str.ToCString(), str.Length(), caseIns);
}

// hashes string. Returns value in 32 bits range
uint StringId(EqStringRef str, bool caseIns)
{
	uint strCrc = 0;
	CRC32_InitChecksum(strCrc);

	const char* data = str.GetData();
	int len = str.Length();
	while (len--)
	{
		const uint chr = caseIns ? CType::LowerChar(*data++) : *data++;
		CRC32_Update(strCrc, chr);
	}

	return strCrc;
}

void StringSplit(const char* pString, ArrayCRef<const char*> separators, Array<EqString>& outStrings)
{
	if (!pString || *pString == 0)
		return;

	outStrings.clear();
	const char* pCurPos = pString;
	while (1)
	{
		int iFirstSeparator = -1;
		const char* pFirstSeparator = nullptr;
		for (int i = 0; i < separators.numElem(); i++)
		{
			const char* pTest = CString::SubStringCaseIns(pCurPos, separators[i]);
			if (pTest && (!pFirstSeparator || pTest < pFirstSeparator))
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if (pFirstSeparator)
		{
			// Split on this separator and continue on.
			const int separatorLen = CString::Length(separators[iFirstSeparator]);
			if (pFirstSeparator > pCurPos)
			{
				outStrings.append(_Es(pCurPos, pFirstSeparator - pCurPos));
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if (strlen(pCurPos))
			{
				outStrings.append(_Es(pCurPos));
			}
			return;
		}
	}
}

void StringSplit(const char* pString, const char* separator, Array<EqString>& outStrings)
{
	StringSplit(pString, ArrayCRef(&separator, 1), outStrings);
}


//------------------------------------------------------
// Path utils
//------------------------------------------------------

bool fnmPathHasExt(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return true;
	}
	return false;
}

EqStringRef fnmPathApplyExt(EqStringRef path, EqStringRef ext)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return path.Left(i + 1) + ext;
	}

	if (path.Length() > 0 && path[path.Length() - 1] == '.')
		return path + ext;
	return path + "." + ext;
}

EqStringRef fnmPathStripExt(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
			return path.Left(i);
	}
	return path;
}

EqStringRef fnmPathStripName(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == CORRECT_PATH_SEPARATOR || path[i] == INCORRECT_PATH_SEPARATOR)
			return path.Left(i);
	}
	return EqString::EmptyStr;
}

EqStringRef fnmPathStripPath(EqStringRef path)
{
	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == CORRECT_PATH_SEPARATOR || path[i] == INCORRECT_PATH_SEPARATOR)
			return path.Right(path.Length() - 1 - i);
	}
	return path;
}

		 
EqStringRef fnmPathExtractExt(EqStringRef path, bool autoLowerCase)
{
	static thread_local EqString outPath;
	outPath.Empty();

	for (int i = path.Length() - 1; i >= 0; i--)
	{
		if (path[i] == '.')
		{
			outPath = path.Right(path.Length() - 1 - i);
			break;
		}
	}

	if (autoLowerCase)
	{
		char* data = outPath.GetData();
		for (int i = 0; i < outPath.Length(); ++i)
			data[i] = CType::LowerChar(data[i]);
	}
	return outPath;
}

EqStringRef fnmPathExtractName(EqStringRef path)
{
	return fnmPathStripPath(path);
}

EqStringRef fnmPathExtractPath(EqStringRef path)
{
	return fnmPathStripName(path);
}

EqStringRef fnmPathCombineF(int num, ...)
{
	static thread_local EqString outPath;
	outPath.Empty();

	va_list	argptr;
	va_start(argptr, num);

	int maxLength = 0;
	FixedArray<EqStringRef, 32> paths;
	for (int i = 0; i < num; ++i)
	{
		EqStringRef pathPart = va_arg(argptr, const char*);
		if (!pathPart.Length())
			continue;
		paths.append(pathPart);
		maxLength += pathPart.Length() + 1;
	}
	va_end(argptr);

	outPath.Resize(maxLength);
	for (int i = 0; i < paths.numElem(); ++i)
	{
		outPath.Append(paths[i].TrimChar(_CORRECT_PATH_SEPARATOR_STR _INCORRECT_PATH_SEPARATOR_STR, true, true));
		if (i < paths.numElem() - 1)
			outPath.Append(CORRECT_PATH_SEPARATOR);
	}

	fnmPathFixSeparators(outPath);
	return outPath;
}

void fnmPathFixSeparators(EqString& str)
{
	const int length = str.Length();
	char* data = str.GetData();
	for (int i = 0; i < length; ++i)
	{
		if (data[i] == INCORRECT_PATH_SEPARATOR)
			data[i] = CORRECT_PATH_SEPARATOR;
	}
}

void fnmPathFixSeparators(char* str)
{
	if (!str)
		return;

	while (*str)
	{
		if (*str == INCORRECT_PATH_SEPARATOR)
			*str = CORRECT_PATH_SEPARATOR;
		str++;
	}
}


//------------------------------------------------------
// string conversion
//------------------------------------------------------
namespace EqStringConv
{
static uint32 GetUTF8NextByte(ubyte** utf8)
{
	if (!*(*utf8))
		return 0;

	return *(*utf8)++;
}

static uint32 udec(uint32 val)
{
	return (val & 0x3f);
}

static uint32 GetWideChar(ubyte** utf8)
{
	const uint32 b1 = GetUTF8NextByte(utf8);
	if (!b1)
		return 0;

	// Determine whether we are dealing
	// with a one-, two-, three-, or four-
	// byte sequence.
	if ((b1 & 0x80) == 0)
	{
		// 1-byte sequence: 000000000xxxxxxx = 0xxxxxxx
		return b1;
	}
	else if ((b1 & 0xe0) == 0xc0)
	{
		// 2-byte sequence: 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
		uint32 r = (b1 & 0x1f) << 6;
		r |= udec(GetUTF8NextByte(utf8));
		return r;
	}
	else if ((b1 & 0xf0) == 0xe0)
	{
		// 3-byte sequence: zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
		uint32 r = (b1 & 0x0f) << 12;
		r |= udec(GetUTF8NextByte(utf8)) << 6;
		r |= udec(GetUTF8NextByte(utf8));
		return r;
	}
	else if ((b1 & 0xf8) == 0xf0)
	{
		// 4-byte sequence: 11101110wwwwzzzzyy + 110111yyyyxxxxxx
		//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
		// (uuuuu = wwww + 1)
		int b2 = udec(GetUTF8NextByte(utf8));
		int b3 = udec(GetUTF8NextByte(utf8));
		int b4 = udec(GetUTF8NextByte(utf8));
		return ((b1 & 7) << 18) | ((b2 & 0x3f) << 12) |
			((b3 & 0x3f) << 6) | (b4 & 0x3f);
	}

	//bad start for UTF-8 multi-byte sequence
	return '?';
}

static int GetUTF8Length(ubyte* utf8)
{
	int utfStringLength = 0;
	ubyte* tmp = utf8;
	while (GetWideChar(&tmp))
		++utfStringLength;

	return utfStringLength;
}

}

//--------------------------------------------------------------

AnsiUnicodeConverter::AnsiUnicodeConverter(EqWString& outStr, EqStringRef sourceStr)
{
	ASSERT(sourceStr.IsValid());

	ubyte* utf8 = (ubyte*)sourceStr.GetData();
	int length = EqStringConv::GetUTF8Length(utf8);
	outStr.Empty();
	outStr.ExtendAlloc(length);
	do {
		const uint32 wch = EqStringConv::GetWideChar(&utf8);
		if (!wch)
			break;
		outStr.Append(wch);
	} while (length--);
}

AnsiUnicodeConverter::AnsiUnicodeConverter(EqString& outStr, EqWStringRef sourceStr)
{
	ASSERT(sourceStr.IsValid());

	int len = sourceStr.Length() * 4;
	outStr.Empty();
	outStr.ExtendAlloc(len);

	const wchar_t* val = sourceStr.GetData();
	uint32 code;
	do
	{
		code = *val++;

		if(code == 0)
			break;

		if (code <= 0x7F)
		{
			outStr.Append((char)code);
		}
		else if (code <= 0x7FF)
		{
			outStr.Append((code >> 6) + 192);
			outStr.Append((code & 63) + 128);
		}
		else if (code <= 0xFFFF)
		{
			outStr.Append((code >> 12) + 224);
			outStr.Append(((code >> 6) & 63) + 128);
			outStr.Append((code & 63) + 128);
		}
		else if (code <= 0x10FFFF)
		{
			outStr.Append((code >> 18) + 240);
			outStr.Append(((code >> 12) & 63) + 128);
			outStr.Append(((code >> 6) & 63) + 128);
			outStr.Append((code & 63) + 128);
		}
		else if (0xd800 <= code && code <= 0xdfff)
		{
			//invalid block of utf8
		}
	}
	while(len--);
}
