//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine wide string base
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQWSTRING_H
#define EQWSTRING_H

#include <stdlib.h>
#include <string.h>

#include "core/dktypes.h"

#ifdef __GNUG__
class EqString;
#include "eqstring.h"
#endif // __GNUG__

// TODO: implement safe copy-on-write

#define _EWs EqWString

int xwcscmp ( const wchar_t *s1, const wchar_t *s2);

class EqWString
{
	friend class EqString;

public:
	EqWString();
	~EqWString();

	EqWString(const wchar_t c);

	// convert from UTF8 string
	EqWString(const char* pszString, int len = -1);
	EqWString(const EqString& str, int nStart = 0, int len = -1);

	EqWString(const wchar_t* pszString, int len = -1);
	EqWString(const EqWString &str, int nStart = 0, int len = -1);

	// data for printing
	const wchar_t*	GetData() const;

	// nice std thing :)
	const wchar_t*	c_str() const {return GetData();}

	// length of it
	uint			Length() const;

	// string allocated size in bytes
	uint			GetSize() const;

	// erases and deallocates data
	void			Clear();

	// empty the string and restore base buffer
	void			Empty();

	// an internal operation of allocation/extend
	bool			ExtendAlloc(int nSize);

	// just a resize
	bool			Resize(uint nSize, bool bCopy = true);

	// string assignment with conversion (or setvalue)
	void			Assign(const char* pszStr, int len = -1);
	void			Assign(const EqString &str, int nStart = 0, int len = -1);

	// string assignment (or setvalue)
	void			Assign(const wchar_t* pszStr, int len = -1);
	void			Assign(const EqWString &str, int nStart = 0, int len = -1);

	// appends another string
	void			Append(const wchar_t c);
	void			Append(const wchar_t* pszStr, int nCount = -1);
	void			Append(const EqWString &str);

	// inserts another string at position
	void			Insert(const wchar_t* pszStr, int nInsertPos);
	void			Insert(const EqWString &str, int nInsertPos);

	// removes characters
	void			Remove(uint nStart, uint nCount);

	// replaces characters
	void			Replace( wchar_t whichChar, wchar_t to );

	// rightmost\leftmost string extractors
	EqWString		Left(int nCount);
	EqWString		Right(int nCount);
	EqWString		Mid(int nStart, int nCount);

	// converters
	EqWString		LowerCase();
	EqWString		UpperCase();

	// comparators
	int				Compare(const wchar_t* pszStr);
	int				Compare(const EqWString &str);

	int				CompareCaseIns(const wchar_t* pszStr);
	int				CompareCaseIns(const EqWString &str);

	// search, returns char index
	int				Find(const wchar_t* pszSub, bool bCaseSensetive = false, int nStart = 0);

	// searches for substring and replaces it
	int				ReplaceSubstr(const wchar_t* find, const wchar_t* replaceTo, bool bCaseSensetive = false, int nStart = 0);

	EqWString& operator = (const EqWString& other)
	{
		this->Assign( other );
		return *this;
	}

	EqWString& operator = (const wchar_t* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	friend EqWString operator+( const EqWString &a, const EqWString &b )
	{
		EqWString result(a);
		result.Append(b);

		return result;
	}

	friend EqWString operator+( const EqWString &a, const wchar_t *b )
	{
		EqWString result(a);
		result.Append(b);

		return result;
	}

	friend EqWString operator+( const wchar_t *a, const EqWString &b )
	{
		EqWString result(a);
		result.Append(b);

		return result;
	}

	// case sensitive comparators
	friend bool	operator==( const EqWString &a, const EqWString &b )
	{
		return !xwcscmp(a.GetData(), b.GetData());
	}

	friend bool	operator==( const EqWString &a, const wchar_t* b )
	{
		return !xwcscmp(a.GetData(), b);
	}

	friend bool	operator==( const wchar_t *a, const EqWString &b )
	{
		return !xwcscmp(a, b.GetData());
	}

	friend bool	operator!=( const EqWString &a, const EqWString &b )
	{
		return !(a == b);
	}

	friend bool	operator!=( const EqWString &a, const wchar_t *b )
	{
		return !(a == b);
	}

	friend bool	operator!=( const wchar_t *a, const EqWString &b )
	{
		return !(a == b);
	}


protected:
	wchar_t*	m_pszString;

	uint16		m_nLength;			// length of string
	uint16		m_nAllocated;		// allocation size
};

#endif // EQWSTRING_H
