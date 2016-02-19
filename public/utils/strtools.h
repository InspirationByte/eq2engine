//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#ifndef STRTOOLS_H
#define STRTOOLS_H

#define ASCIILINESZ         (1024)

#include "utils/DkLinkedList.h"
#include "utils/DkList.h"

#include "utils/eqstring.h"
#include "utils/eqwstring.h"

#ifdef _WIN32

#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'

#else

#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'

#endif // _WIN32

#ifdef PLAT_POSIX

#include <ctype.h>
#include <string.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define _vsnwprintf vswprintf

#endif // LINUX

#ifdef ANDROID

typedef __builtin_va_list	va_list;
#define va_start(v,l)		__builtin_va_start(v,l)
#define va_end(v)			__builtin_va_end(v)
#define va_arg(v,l)			__builtin_va_arg(v,l)

#if !defined(__STRICT_ANSI__) || __STDC_VERSION__ + 0 >= 199900L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#define va_copy(d,s)		__builtin_va_copy(d,s)
#endif

#define __va_copy(d,s)		__builtin_va_copy(d,s)
typedef __builtin_va_list	__gnuc_va_list;
typedef __gnuc_va_list		va_list;
typedef va_list				__va_list;

//wchar_t* wcsncpy(wchar_t * __restrict dst, const wchar_t * __restrict src, size_t n);

#endif // ANDROID

// fixes slashes in the directory name
void		FixSlashes( char* str );

// strips file name from path
void		StripFileName(char* path);

void		ExtractFileBase(const char* path, char* dest);

// generates string hash
int			StringToHash( const char *str );

// Do formatted arguments for string
char*		varargs(const char* fmt,...);

// Do formatted arguments for string (widechar)
wchar_t*	varargs_w(const wchar_t *fmt,...);

// Split string by multiple separators
void		xstrsplit2( const char* pString, const char* *pSeparators, int nSeparators, DkList<EqString> &outStrings );

// Split string by one separator
void		xstrsplit( const char* pString, const char* pSeparator, DkList<EqString> &outStrings );

char const* xstristr( char const* pStr, char const* pSearch );
char*		xstristr( char* pStr, char const* pSearch );

// Strips string for tabs and spaces
char*		xstreatwhite(char* str);

// Trims string from spaces, tabs, newlines
char*		xstrtrim(const char*  s);

// fast duplicate c string
char*		xstrdup(const char*  s);

// is space?
bool		xisspace(const uint32 c);

// converts string to lower case
#ifdef PLAT_POSIX
char* strupr(char* s1);
char* strlwr(char* s1);

wchar_t* wcslwr(wchar_t* str);
wchar_t* wcsupr(wchar_t* str);
#endif

//------------------------------------------------------
// wide string
//------------------------------------------------------

// Compares string
int xwcscmp ( const wchar_t *s1, const wchar_t *s2);

// compares two strings case-insensetive
int xwcsicmp( const wchar_t* s1, const wchar_t* s2 );

// finds substring in string case insensetive
wchar_t* xwcsistr( wchar_t* pStr, wchar_t const* pSearch );

// finds substring in string case insensetive
wchar_t const* xwcsistr( wchar_t const* pStr, wchar_t const* pSearch );

//------------------------------------------------------
// string conversion
//------------------------------------------------------

namespace EqStringConv
{
	class utf8_to_wchar
	{
	public:
		utf8_to_wchar(const char* val);
		operator const EqWString&() const
		{
			return m_str;
		}
	private:
		uint32 NextByte()
		{
			if (!(*m_utf8))
				return 0;

			return *m_utf8++;
		}

		int		GetLength();
		uint32	GetChar();

		ubyte* m_utf8;
		EqWString m_str;
	};

	class wchar_to_utf8
	{
	public:
		wchar_to_utf8(const wchar_t* val);
		operator const EqString&() const
		{
			return m_str;
		}
	private:
		EqString m_str;
	};
};


#endif
