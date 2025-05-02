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

static constexpr const int StringId24Bits = 24;
static constexpr const int StringId24Mask = ((1 << StringId24Bits) - 1);

template<int idx, std::size_t N>
static constexpr int StringId24_Cexpr_Helper(const char(&str)[N], int hash)
{
	if constexpr (idx == 0)
	{
		return hash;
	}
	else
	{
		const int v1 = hash >> 19;
		const int v0 = hash << 5;
		const int chr = str[N - idx - 1];
		hash = ((v0 | v1) + chr) & StringId24Mask;
		return StringId24_Cexpr_Helper<idx - 1, N>(str, hash);
	}
}

template<std::size_t N>
static constexpr int StringId24_Cexpr(const char(&str)[N])
{
	return StringId24_Cexpr_Helper<N - 1, N>(str, N - 1);
}

template <auto V> static constexpr auto force_consteval = V;
#define StringIdConst24(x) force_consteval<StringId24_Cexpr(x)>

struct StringId24ConstHelper
{
	StringId24ConstHelper(int id) : id(id) {}
	template<std::size_t N> constexpr StringId24ConstHelper(const char(&str)[N])
		: id(StringId24_Cexpr_Helper<N - 1, N>(str, N - 1))
	{}

	operator int() const { return id; }
	uint id;
};

// generates string hash 24 bit
int			StringId24(EqStringRef str, bool caseIns = false);

// generates string hash 32 bit
uint 		StringId(EqStringRef str, bool caseIns = false);

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
EqString	fnmPathApplyExt(EqStringRef path, EqStringRef ext);
EqString	fnmPathStripExt(EqStringRef path);
EqString	fnmPathStripName(EqStringRef path);
EqString	fnmPathStripPath(EqStringRef path);

EqString	fnmPathExtractExt(EqStringRef path, bool autoLowerCase = true);
EqString	fnmPathExtractName(EqStringRef path);
EqString	fnmPathExtractPath(EqStringRef path);

// changes path separator to correct one for platform
void		fnmPathFixSeparators(EqString& str);
void		fnmPathFixSeparators(char* str);

// combines paths
EqString	fnmPathCombineF(int num, ...);

template<typename ...Args> // requires std::same_as<Args, const char*>...
EqString	fnmPathCombine(const Args&... args)
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
