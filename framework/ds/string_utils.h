//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32

#define _CORRECT_PATH_SEPARATOR_STR		"\\"
#define _INCORRECT_PATH_SEPARATOR_STR	"/"

constexpr int CORRECT_PATH_SEPARATOR	= '\\';
constexpr int INCORRECT_PATH_SEPARATOR	= '/';
#else

#define _CORRECT_PATH_SEPARATOR_STR		"/"
#define _INCORRECT_PATH_SEPARATOR_STR	"\\"

constexpr int CORRECT_PATH_SEPARATOR	= '/';
constexpr int INCORRECT_PATH_SEPARATOR	= '\\';
#endif // _WIN32

template <typename T>
decltype(auto) ToCString(const T& value)
{
	if constexpr (
		std::is_same_v<T, EqStringRef> ||
		std::is_same_v<T, EqWStringRef> ||
		std::is_same_v<T, EqString> ||
		std::is_same_v<T, EqWString>)
	{
		return value.ToCString();
	}
#if 0
	else if constexpr (
		std::is_same_v<T, std::string> ||
		std::is_same_v<T, std::wstring>)
	{
		return value.c_str();
	}
#endif
	else
	{
		return value;
	}
}

//------------------------------------------------------
// String hash
//------------------------------------------------------

static constexpr int StringId24Bits = 24;
static constexpr int StringId24Mask = ((1 << StringId24Bits) - 1);

constexpr int ASCIILower_Cexpr(int c)
{
	constexpr int d = 'a' - 'A';
	return c + (c >= 'A' && c <= 'Z' ? d : 0);
}

constexpr int StringId24_Cexpr(const char* const str, int length, bool caseIns = false)
{
	int hash = length;
	for (int i = 0; i < length; ++i)
	{
		const int v1 = hash >> 19;
		const int v0 = hash << 5;
		const int chr = caseIns ? ASCIILower_Cexpr(str[i]) : str[i];
		hash = ((v0 | v1) + chr) & StringId24Mask;
	}
	return hash;
}

template<int N>
constexpr int StringId24_Cexpr_B(const char(&str)[N], bool caseIns = false)
{
	return StringId24_Cexpr(str, N-1, caseIns);
}

template <auto V> constexpr auto force_consteval = V;

// generates string hash 24 bit
int			StringId24(EqStringRef str, bool caseIns = false);

// generates string hash 32 bit
uint 		StringId(EqStringRef str, bool caseIns = false);

#define StringIdConst24(x)	force_consteval<StringId24_Cexpr_B(x)>
#define StringIdConst(x)	force_consteval<CRC32_StringConst(x)>

//------------------------------------------------------
// String split helper
//------------------------------------------------------

// Split string by multiple separators
void		StringSplit(const char* pString, ArrayCRef<const char*> separators, Array<EqString>& outStrings);

// Split string by one separator
void		StringSplit(const char* pString, const char* separator, Array<EqString>& outStrings);

//------------------------------------------------------
// Path utils
//------------------------------------------------------

// strip operators
bool		fnmPathHasExt(EqStringRef path);
EqStringRef	fnmPathApplyExt(EqStringRef path, EqStringRef ext);
EqStringRef	fnmPathStripExt(EqStringRef path);
EqStringRef	fnmPathStripName(EqStringRef path);
EqStringRef	fnmPathStripPath(EqStringRef path);

EqStringRef	fnmPathExtractExt(EqStringRef path, bool autoLowerCase = true);
EqStringRef	fnmPathExtractName(EqStringRef path);
EqStringRef	fnmPathExtractPath(EqStringRef path);

// changes path separator to correct one for platform
void		fnmPathFixSeparators(EqString& str);
void		fnmPathFixSeparators(char* str);

// combines paths
EqStringRef	fnmPathCombineF(int num, ...);

template<typename ...Args> // requires std::same_as<Args, const char*>...
EqStringRef	fnmPathCombine(const Args&... args)
{
	return fnmPathCombineF(sizeof...(Args), ToCString(args)...);
}

//------------------------------------------------------

class AnsiUnicodeConverter
{
public:
	AnsiUnicodeConverter(EqString& outStr, EqWStringRef sourceStr);
	AnsiUnicodeConverter(EqWString& outStr, EqStringRef sourceStr);
};
