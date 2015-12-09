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

#ifdef _WIN32

#define CORRECT_PATH_SEPARATOR '\\'
#define INCORRECT_PATH_SEPARATOR '/'

#else

#define CORRECT_PATH_SEPARATOR '/'
#define INCORRECT_PATH_SEPARATOR '\\'

#endif // _WIN32

#ifdef LINUX

#include <ctype.h>
#include <string.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

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

#endif // ANDROID

#endif // LINUX

// Fixes slashes in the directory name
static void FixSlashes( char* str )
{
    while ( *str )
    {
        if ( *str == INCORRECT_PATH_SEPARATOR )
        {
            *str = CORRECT_PATH_SEPARATOR;
        }
        str++;
    }
}

void stripFileName(char* path);

void extractFileBase(const char* path, char* dest);

int64 StringToHash(const char* str);

#define HASHSTRING(str) \
	static int64 s_##str##Hash = StringToHash(#str)

#define STRINGHASH(str) s_##str##Hash

// Splits string to queue
//void SplitString( const char* pString, const char* pSeparator, DkLinkedList<String> &outStrings );

// Finds string in another string and index of it
bool strfind(const char* str,const char* sstr, int pos, int *index);

bool strfind(const char* str,const char ch,int pos,int *index);

bool strifind(const char* str,const char* sstr, int pos, int *index);

// Removes some chars from string in range
bool strrem(char* str ,const int pos, const int len);

// Removes some chars from string in range
char* xstrrem_s(char* str ,const int pos, const int len);

// Eat white spaces and remarks loop
char* xstreatwhite(char* str);

// Do formatted arguments for string
char* varargs(const char* fmt,...);

// Do formatted arguments for string
wchar_t* varargs_w(const wchar_t *fmt,...);

// Split string by multiple separators
void xstrsplit2( const char* pString, const char* *pSeparators, int nSeparators, DkList<EqString> &outStrings );

// Split string by one separator
void xstrsplit( const char* pString, const char* pSeparator, DkList<EqString> &outStrings );

// Restrores splitted string
//char* ConnectString(Array<char*> *args, int start = 0, int count = 0);

// Restrores splitted string
//char* ConnectString(int argc, char** argv, int start = 0, int count = 0);

// Restrores splitted string (smart)
//String XConnectString(Array<char*> *args, int start = 0, int count = 0);

// Duplicates string
char* xstrdup(const char*  s);

// Strips string for tabs and spaces
char* xstrstrip(const char*  s);

// allocates memory for new string
char* xstrallocate( const char* pStr, int nMaxChars );

// NOTE: More of these tools has NULL-termination.

// Sets memory of specified pointer
void xmemset ( void *dest, int fill, int count);

// Copies memory to specified pointer
void xmemcpy ( void *dest, const void *src, int count);

// moves memory to specified pointer
void xmemmove( void *dest, const void *src, int count);

//Compares memory
int xmemcmp ( void *m1, void *m2, int count);

//Lenght of string
int xstrlen (const char* str);

// Makes copy of string
void xstrcpy (char* dest, const char* src);

// Finds a string in another string with a case insensitive test
char* xstrrchr(const char* s, char c);

// Compares string
int xstrcmp ( const char* s1, const char* s2);

// Removes match substr in str
void xstrrem( char* str, char* substr );

// Compares string (ignores case)
int xstricmp(  const char* s1, const char* s2 );

//Finds substring in another string
char* xstrstr(  const char* s1, const char* search );

// To upper conversion
char* xstrupr ( char* start);

// To lower conversion
char* xstrlower ( char* start);

// Concat strings
void xstrcat (char* dest, const char* src);

// Compares string with specified length
int xstrncmp (const char* s1, const char* s2, int count);

// Makes Lower case string with specified length
char* xstrnlwr(char* s, size_t count);

// String non-case compare
int xstrncasecmp (const char* s1, const char* s2, int n);

// String case compare
int xstrcasecmp (const char* s1, const char* s2);

// String ignore-case compare with spec. length
int xstrnicmp (const char* s1, const char* s2, int n);

// Finds a string in another string with a case insensitive test
char const* xstristr( char const* pStr, char const* pSearch );

// Finds a string in another string with a case insensitive test
char* xstristr( char* pStr, char const* pSearch );

// Copies string to another string with max length
void xstrncpy( char* pDest, char const *pSrc, int maxLen );

// Prints string to another
int xsnprintf( char* pDest, int maxLen, char const *pFormat, ... );

// Prints string from format
int xvsnprintf( char* pDest, int maxLen, char const *pFormat, va_list params );

// Concat string
char* xstrncat(char* pDest, const char* pSrc, size_t maxLen);

// searches string for another string
int	xstrfind(char* str,char* search);

// returns slash count from file path
int UTIL_GetNumSlashes(const char* filepath);

// returns directory name down to slashes
const char* UTIL_GetDirectoryNameEx(const char* filepath, int hierarchy_down);

// extracts file name from path
const char* UTIL_GetFileName(const char* filepath);

// converts string to lower case
#ifdef LINUX
char* strupr(char* s1);
char* strlwr(char* s1);

#endif

//------------------------------------------------------
// wide string
//------------------------------------------------------

// Compares string
int xwcscmp ( const wchar_t *s1, const wchar_t *s2);

// compares two strings case-insensetive
int xwcsicmp( const wchar_t* s1, const wchar_t* s2 );

// copies src to dest
void xwcscpy( wchar_t* dest, const wchar_t* src );

// finds substring in string case insensetive
wchar_t* xwcsistr( wchar_t* pStr, wchar_t const* pSearch );

// finds substring in string case insensetive
wchar_t const* xwcsistr( wchar_t const* pStr, wchar_t const* pSearch );

#endif
