//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Special String tools to do lesser memory errors
//////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include <stdio.h>
#include "Platform.h"
#include "strtools.h"
#include "DebugInterface.h"

#ifdef LINUX

#include <wchar.h>
#include <wctype.h>

char* strupr(char* s1)
{
   char* p = s1;
   while(*p)
   {
       toupper(*p);
       p++;
   }
   return s1;
}

char* strlwr(char* s1)
{
   char* p = s1;
   while(*p)
   {
       tolower(*p);
       p++;
   }
   return s1;
}

#endif

void stripFileName(char* path)
{
	int	length;

	length = strlen(path) - 1;
	while(length > 0 && path[length] != '/' && path[length] != '\\')
		length--;

	path[length] = 0;
}

void extractFileBase(const char* path, char* dest)
{
	const char     *src;
	src = path + strlen(path) - 1;

	// search back for first slashes
	while(src != path && *(src - 1) != '/' && *(src - 1) != '\\')
		src--;

	while(*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

//------------------------------------------
// Converts string to 32-bit integer hash
//------------------------------------------
int64 StringToHash(const char* str)
{
	ubyte* data = (ubyte*)str;
	int64 hash = 0;

	int len = strlen(str);

	for(int i = 0; i < len; i++)
	{
		hash = data[i] + (hash << 6) + (hash << 16) - hash;
	}

	return hash;
}

//------------------------------------------
// Searches string for some chars
//------------------------------------------
bool strfind(const char* str,const char* sstr, int pos, int *index)
{
	ASSERT(str != NULL);

	const char* st = strstr(str + pos, sstr);
	if (st != NULL)
	{
		if (index != NULL) *index = (unsigned int) (st - str);
		return true;
	}
	return false;
}
//------------------------------------------
// Searches string for one char
//------------------------------------------
bool strfind(const char* str,const char ch, int pos, int *index)
{
	ASSERT(str != NULL);

	int length = strlen(str) + 1;

	for (int i = pos; i < length; i++)
	{
		if (str[i] == ch)
		{
			if (index != NULL) *index = i;
			return true;
		}
	}
	return false;
}

//------------------------------------------
// Searches string for some chars with case insensetive
//------------------------------------------
bool strifind(const char* str,const char* sstr, int pos, int *index)
{
	ASSERT(str != NULL);

	const char* st = xstristr(str + pos, sstr);
	if (st != NULL)
	{
		if (index != NULL) *index = (unsigned int) (st - str);
		return true;
	}
	return false;
}

//------------------------------------------
// Removes chars from string
//------------------------------------------
bool strrem(char* str ,int pos, int len)
{
	ASSERT(str != NULL);

	int length = strlen(str) + 1;

	if (pos + len > length) return false;

	length -= len;
	int n = length - pos;
	for (int i = 0; i < n; i++)
	{
		str[pos + i] = str[pos + len + i];
	}

	return true;
}

//------------------------------------------
// Removes chars from string
//------------------------------------------
char* xstrrem_s(char* str ,const int pos, const int len)
{
	AssertValidStringPtr(str);
/*
	int length = strlen(str) - pos;

	// Sets index to pos
	str += pos;

	for (int i = 0; i <= length; i++)
	{
		str[i] = str[len + i];
	}
*/
	/*
	while(*str = c)
	{
		if (c == 0)
			return 0; // end of line;
		str++;


	}
	*/

	int length = strlen(str);
	char* remstr = new char[length+1];
	xstrcpy(remstr,str);

	if (length <= 0) return NULL;

	if (pos + len > length) return NULL; //Double check!

	int n = length - pos;
	for (int i = 0; i < n; i++)
	{
		remstr[pos + i] = str[pos + len + i];
	}

	return remstr;

	//return (char*)str;
}


char* xstreatwhite(char* str)
{
	char c = 0;

// skip whitespace
skipwhite:

	while ( (c = *str) <= ' ')
	{
		if (c == 0)
			return 0;                    // end of file;
		str++;
	}

// skip // comments
	if (c=='/' && str[1] == '/')
	{
		while (*str && *str != '\n')
			str++;
		goto skipwhite;
	}

// skip c-style comments
	if (c=='/' && str[1] == '*' )
	{
		// Skip "/*"
		str += 2;

		while ( *str  )
		{
			if ( *str == '*' &&
				 str[1] == '/' )
			{
				str += 2;
				break;
			}

			str++;
		}

		goto skipwhite;
	}
	return str;
}

//------------------------------------------
// Splits string into array
//------------------------------------------

char* xstrallocate( const char* pStr, int nMaxChars )
{
	int allocLen;
	if ( nMaxChars == -1 )
		allocLen = strlen( pStr ) + 1;
	else
		allocLen = min( (int)strlen(pStr), nMaxChars ) + 1;

	char* pOut = new char[allocLen];
	xstrncpy( pOut, pStr, allocLen );
	return pOut;
}

void xstrsplit2( const char* pString, const char* *pSeparators, int nSeparators, DkList<EqString> &outStrings )
{
	outStrings.clear();
	const char* pCurPos = pString;
	while ( 1 )
	{
		int iFirstSeparator = -1;
		const char* pFirstSeparator = 0;
		for ( int i=0; i < nSeparators; i++ )
		{
			const char* pTest = xstristr( pCurPos, pSeparators[i] );
			if ( pTest && (!pFirstSeparator || pTest < pFirstSeparator) )
			{
				iFirstSeparator = i;
				pFirstSeparator = pTest;
			}
		}

		if ( pFirstSeparator )
		{
			// Split on this separator and continue on.
			int separatorLen = strlen( pSeparators[iFirstSeparator] );
			if ( pFirstSeparator > pCurPos )
			{
				outStrings.append(_Es( pCurPos, pFirstSeparator-pCurPos ));
			}

			pCurPos = pFirstSeparator + separatorLen;
		}
		else
		{
			// Copy the rest of the string
			if ( strlen( pCurPos ) )
			{
				outStrings.append( _Es( pCurPos ) );
			}
			return;
		}
	}
}


void xstrsplit( const char* pString, const char* pSeparator, DkList<EqString> &outStrings )
{
	xstrsplit2( pString, &pSeparator, 1, outStrings );
}

//------------------------------------------
// Connects string from array
//------------------------------------------

EqString XConnectString(DkList<char*> *args, int start, int count)
{
	if(count == 0)
		count = args->numElem();

	EqString tempStr;

	if(args->numElem() > start)
	{
		for(int i = start;i < count;i++)
		{
			if(i == count-1)
				tempStr.Append(varargs("%s",args->ptr()[i]));
			else
				tempStr.Append(varargs("%s ",args->ptr()[i]));
		}
		return tempStr;
	}

	return EqString(" ");
}

//------------------------------------------
// Duplicates string
//------------------------------------------
char* xstrdup(const char*  s)
{
	AssertValidStringPtr( s );

	char*  t;
	int len = xstrlen(s)+1;

    t = new char[len];//(char*)malloc(xstrlen(s)+1);

    if (t)
	{
        xstrncpy(t,s,len);
    }
    return t;
}


//------------------------------------------
// does varargs fast. Not thread-safe
//------------------------------------------

char* varargs(const char* fmt,...)
{
	va_list		argptr;

	static int index = 0;
	static char	string[4][4096];

	char* buf = string[index];
	index = (index + 1) & 3;

	memset(buf, 0, 4096);

	va_start (argptr,fmt);
	vsnprintf(buf, 4096, fmt,argptr);
	va_end (argptr);

	return buf;
}

wchar_t* varargs_w(const wchar_t *fmt,...)
{
	va_list		argptr;

	static int index = 0;
	static wchar_t	string[4][4096];
	
	wchar_t* buf = string[index];
	index = (index + 1) & 3;

	memset(buf, 0, sizeof(wchar_t)*4096);

	va_start (argptr,fmt);
	_vsnwprintf(buf, 4096, fmt, argptr);
	va_end (argptr);

	return buf;
}

//------------------------------------------
// Strips string for spaces
//------------------------------------------
char* xstrstrip(const char*  s)
{
	AssertValidStringPtr( s );

    static char l[ASCIILINESZ+1];
	memset(l,0,sizeof(l));
	char*  last ;

    if (s==NULL) return NULL ;

	while (isspace((int)(ubyte)*s) && *s) s++;
	memset(l, 0, ASCIILINESZ+1);
	strcpy(l, s);
	last = l + strlen(l);
	while (last > l)
	{
		if (!isspace((int)(ubyte)*(last-1)))
			break ;
		last -- ;
	}
	*last = (char)0;
	return (char*)l ;
}

void xmemset ( void *dest, int fill, int count)
{
	ASSERT( count >= 0 );
	AssertValidWritePtr( dest, count );

	memset(dest,fill,count);
}

void xmemcpy ( void *dest, const void *src, int count)
{
	ASSERT( count >= 0 );
	AssertValidReadPtr( (void*)src, count );
	AssertValidWritePtr( dest, count );

	memcpy( dest, src, count );
}

void xmemmove( void *dest, const void *src, int count)
{
	ASSERT( count >= 0 );
	AssertValidReadPtr( (void*)src, count );
	AssertValidWritePtr( (void*)dest, count );

	memmove( dest, src, count );
}

int xmemcmp ( void *m1, void *m2, int count)
{
	ASSERT( count >= 0 );
	AssertValidReadPtr( (void*)m1, count );
	AssertValidReadPtr( (void*)m2, count );

	return memcmp( m1, m2, count );
}

int xstrlen (const char* str)
{
	AssertValidStringPtr(str);
	return strlen( str );
}

void xstrcpy (char* dest, const char* src)
{
	AssertValidWritePtr(dest);
	AssertValidStringPtr(src);

	strcpy( dest, src );
}

char* xstrrchr(const char* s, char c)
{
	AssertValidStringPtr( s );

    int len = xstrlen(s);
    s += len;
    while (len--)
	if (*--s == c) return (char* )s;
    return 0;
}

int xstrcmp ( const char* s1, const char* s2)
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

void xstrrem( char* str, char* substr )
{
	AssertValidStringPtr( str );
	AssertValidStringPtr( substr );

    char* found;
    char* ptr;
    int substr_len = xstrlen(substr);

    while(true)
    {
        ptr = found = NULL;
        found = xstrstr( str , substr );
        if( found != NULL )
        {
            ptr = &(found[substr_len]);
            while(true)
            {
                *found = *ptr;
                if( *ptr == 0 )
                {
                    break;
                }
                found++;
                ptr++;
            }
        }
        else
        {
            return;
        }
    }

    return;
}

int xstricmp(  const char* s1, const char* s2 )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	return stricmp( s1, s2 );
}


char* xstrstr(  const char* s1, const char* search )
{
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( search );

	return strstr( (char* )s1, search );
}

int xstrfind(char* str, char* search)
{
	int len = strlen(str);
	int len2 = strlen(search); // search len

	for(int i = 0; i < len; i++)
	{
		if(str[i] == search[0])
		{
			bool ichk = true;

			for(int z = 0; z < len2; z++)
			{
				if(str[i+z] != search[z])
					ichk = false;
			}

			if(ichk)
				return i;
		}
	}

	return -1; // failure
}

char* xstrupr ( char* start)
{
	AssertValidStringPtr( start );
	return strupr( start );
}


char* xstrlower ( char* start)
{
	AssertValidStringPtr( start );
	return strlwr(start);
}


void xstrcat (char* dest, const char* src)
{
	AssertValidStringPtr(dest);
	AssertValidStringPtr(src);

	dest += xstrlen(dest);
	xstrcpy (dest, src);
}

int xstrncmp (const char* s1, const char* s2, int count)
{
	ASSERT( count >= 0 );
	AssertValidStringPtr( s1, count );
	AssertValidStringPtr( s2, count );

	while (1)
	{
		if (!count--)
			return 0;
		if (*s1 != *s2)
			return -1;              // strings not equal
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

char* xstrnlwr(char* s, size_t count)
{
	AssertValidStringPtr( s, count );

	char* pRet = s;
	if (!s)
		return s;

	while (--count > 0)
	{
		if (!*s)
			break;

		*s = tolower(*s);
		++s;
	}

	return pRet;
}


int xstrncasecmp (const char* s1, const char* s2, int n)
{
	ASSERT( n >= 0 );
	AssertValidStringPtr( s1 );
	AssertValidStringPtr( s2 );

	int             c1, c2;

	while (1)
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;               // strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;              // strings not equal
		}
		if (!c1)
			return 0;               // strings are equal
//              s1++;
//              s2++;
	}

	return -1;
}

int xstrcasecmp (const char* s1, const char* s2)
{
	AssertValidStringPtr(s1);
	AssertValidStringPtr(s2);

	return xstrncasecmp (s1, s2, 99999);
}

int xstrnicmp (const char* s1, const char* s2, int n)
{
	ASSERT( n >= 0 );
	AssertValidStringPtr(s1);
	AssertValidStringPtr(s2);

	return xstrncasecmp( s1, s2, n );
}

// Finds a string in another string with a case insensitive test
char const* xstristr( char const* pStr, char const* pSearch )
{
	AssertValidStringPtr(pStr);
	AssertValidStringPtr(pSearch);

	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower(*pMatch) != tolower(*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}

char* xstristr( char* pStr, char const* pSearch )
{
	AssertValidStringPtr( pStr );
	AssertValidStringPtr( pSearch );

	return (char*)xstristr( (char const*)pStr, pSearch );
}

void xstrncpy( char* pDest, char const *pSrc, int maxLen )
{
	ASSERT( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pSrc );

	strncpy( pDest, pSrc, maxLen );
	if( maxLen )
		pDest[maxLen-1] = 0;
}


int xsnprintf( char* pDest, int maxLen, char const *pFormat, ... )
{
	ASSERT( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	va_list marker;

	va_start( marker, pFormat );
	int len = vsnprintf( pDest, maxLen, pFormat, marker );
	va_end( marker );

	// Len < 0 represents an overflow
	if( len < 0 )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}

int xvsnprintf( char* pDest, int maxLen, char const *pFormat, va_list params )
{
	ASSERT( maxLen >= 0 );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pFormat );

	int len = vsnprintf( pDest, maxLen, pFormat, params );

	if( len < 0 )
	{
		len = maxLen;
		pDest[maxLen-1] = 0;
	}

	return len;
}

char* xstrncat(char* pDest, const char* pSrc, size_t maxLen)
{
	AssertValidStringPtr( pDest);
	AssertValidStringPtr( pSrc );

	int len = strlen(pDest);
	maxLen = (maxLen - 1) - len;

	if ( maxLen == 0 )
		return pDest;

	char* pOut = strncat( pDest, pSrc, maxLen );
	pOut[len + maxLen] = 0;
	return pOut;
}

int UTIL_GetNumSlashes(const char* filepath)
{
    int num = 0;
    DkList<EqString> args;

    xstrsplit(filepath,"/",args);

    for (int i = 0; i < args.numElem();i++)
        num++;

    return num;
}

const char* UTIL_GetDirectoryNameEx(const char* filepath, int hierarchy_down)
{
    EqString string;
    DkList<EqString> args;

    xstrsplit(filepath,"/",args);

    for (int i = 0; i < hierarchy_down;i++)
    {
        if (i == 0)
            string = args[i];
        else
            string = string + _Es("/") + args[i];
    }

    static char data[512];
	strcpy(data, string.GetData());
    return data;
}

const char* UTIL_GetFileName(const char* filepath)
{
    DkList<EqString> args;

    xstrsplit(filepath,"/",args);

	if(!args.numElem())
		return filepath;

    static char data[512];
	strcpy(data, args[args.numElem()-1].GetData());

    return data;
}

//------------------------------------------------------
// wide string
//------------------------------------------------------

// compares two strings
int xwcscmp ( const wchar_t *s1, const wchar_t *s2)
{
	AssertValidWStringPtr( s1 );
	AssertValidWStringPtr( s2 );

	while (1)
	{
		if (*s1 != *s2)
			return -1;              // strings not equal
		if (!*s1)
			return 0;               // strings are equal
		s1++;
		s2++;
	}

	return -1;
}

// compares two strings case-insensetive
int xwcsicmp( const wchar_t* s1, const wchar_t* s2 )
{
	AssertValidWStringPtr( s1 );
	AssertValidWStringPtr( s2 );

#ifdef _WIN32
	return wcsicmp( s1, s2 );
#else
	ASSERTMSG(false, "xwcsicmp - not implemented");
	return 0;
#endif // _WIN32
}

// copies src to dest
void xwcscpy( wchar_t* dest, const wchar_t* src )
{
	AssertValidWritePtr(dest);
	AssertValidWStringPtr(src);

	wcscpy( dest, src );
}

// finds substring in string case insensetive
wchar_t* xwcsistr( wchar_t* pStr, wchar_t const* pSearch )
{
	AssertValidWStringPtr( pStr );
	AssertValidWStringPtr( pSearch );

	return (wchar_t*)xwcsistr( (wchar_t const*)pStr, pSearch );
}

// finds substring in string case insensetive
wchar_t const* xwcsistr( wchar_t const* pStr, wchar_t const* pSearch )
{
	AssertValidWStringPtr(pStr);
	AssertValidWStringPtr(pSearch);

	if (!pStr || !pSearch)
		return 0;

	wchar_t const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower(*pLetter) == tolower(*pSearch))
		{
			// Check for match
			wchar_t const* pMatch = pLetter + 1;
			wchar_t const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (towlower(*pMatch) != towlower(*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}
