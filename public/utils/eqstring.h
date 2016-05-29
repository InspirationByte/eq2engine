//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine string base
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQSTRING_H
#define EQSTRING_H

#include <stdlib.h>
#include <string.h>
#include "dktypes.h"

#ifdef __GNUG__
class EqWString;
#include "eqwstring.h"
#endif // __GNUG__

#define _Es EqString

// TODO: implement safe copy-on-write

class EqString
{
	friend class EqWString;
public:

	EqString();
	~EqString();

	EqString(const char c);

	EqString(const char* pszString, int len = -1);
	EqString(const EqString &str, int nStart = 0, int len = -1);

	// conversion from wide char string
	EqString(const wchar_t* pszString, int len = -1);
	EqString(const EqWString &str, int nStart = 0, int len = -1);

	// checks the data is non overflowing
	bool		IsValid() const;

	// data for printing
	const char* GetData() const;

	// nice std thing :)
	const char*	c_str() const {return GetData();}

	// length of it
	uint		Length() const;

	// string allocated size in bytes
	uint		GetSize() const;

	// erases and deallocates data
	void		Clear();

	// empty the string and restore base buffer
	void		Empty();

	// an internal operation of allocation/extend
	bool		ExtendAlloc(uint nSize);

	// just a resize
	bool		Resize(uint nSize, bool bCopy = true);

	// string assignment (or setvalue)
	void		Assign(const char* pszStr, int len = -1);
	void		Assign(const EqString &str, int nStart = 0, int len = -1);

	// wide char string assignment (or setvalue)
	void		Assign(const wchar_t* pszStr, int len = -1);
	void		Assign(const EqWString &str, int nStart = 0, int len = -1);

	// appends another string
	void		Append(const char c);
	void		Append(const char* pszStr, int nCount = -1);
	void		Append(const EqString &str);

	// inserts another string at position
	void		Insert(const char* pszStr, int nInsertPos);
	void		Insert(const EqString &str, int nInsertPos);

	// removes characters
	void		Remove(uint nStart, uint nCount);

	// replaces characters
	void		Replace( char whichChar, char to );

	// converters
	EqString	LowerCase() const;
	EqString	UpperCase() const;

	// comparators
	int			Compare(const char* pszStr) const;
	int			Compare(const EqString &str) const;

	int			CompareCaseIns(const char* pszStr) const;
	int			CompareCaseIns(const EqString &str) const;

	int			GetMathingChars(const char* pszStr) const;
	int			GetMathingChars(const EqString &str) const;

	// rightmost\leftmost string extractors
	EqString	Left(int nCount) const;
	EqString	Right(int nCount) const;
	EqString	Mid(int nStart, int nCount) const;

	// strip operators
	EqString	Path_Strip_Ext() const;
	EqString	Path_Strip_Name() const;
	EqString	Path_Strip_Path() const;

	EqString	Path_Extract_Ext() const;
	EqString	Path_Extract_Name() const;
	EqString	Path_Extract_Path() const;

	EqString	EatWhiteSpaces() const;

	void		Path_FixSlashes() const;

	// search, returns char index
	int			Find(const char* pszSub, bool bCaseSensetive = false, int nStart = 0) const;

	//------------------------------------------------------------------------------------------------

	EqString& operator = (const EqString& other)
	{
		this->Assign( other );
		return *this;
	}

	EqString& operator = (const char* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	EqString& operator = (char* pszStr)
	{
		this->Assign( pszStr );
		return *this;
	}

	friend EqString operator+( const EqString &a, const EqString &b )
	{
		EqString result(a);
		result.Append(b);

		return result;
	}

	friend EqString operator+( const EqString &a, const char *b )
	{
		EqString result(a);
		result.Append(b);

		return result;
	}

	friend EqString operator+( const char *a, const EqString &b )
	{
		EqString result(a);
		result.Append(b);

		return result;
	}

	// case sensitive comparators
	friend bool	operator==( const EqString &a, const EqString &b )
	{
		return !strcmp(a.GetData(), b.GetData());
	}

	friend bool	operator==( const EqString &a, const char *b )
	{
		return !strcmp(a.GetData(), b);
	}

	friend bool	operator==( const char *a, const EqString &b )
	{
		return !strcmp(a, b.GetData());
	}

	friend bool	operator!=( const EqString &a, const EqString &b )
	{
		return !(a == b);
	}

	friend bool	operator!=( const EqString &a, const char *b )
	{
		return !(a == b);
	}

	friend bool	operator!=( const char *a, const EqString &b )
	{
		return !(a == b);
	}


protected:
	char*		m_pszString;

	uint16		m_nLength;			// length of string
	uint16		m_nAllocated;		// allocation size
};

#endif // EQSTRING_H
