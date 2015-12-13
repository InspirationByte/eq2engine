//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

// Core interface helper
#include <stdarg.h>

#include "core_base_header.h"
#include "DebugInterface.h"

#include "KeyValues.h"

#ifdef LINUX
#include <ctype.h>
#endif

#include "utils/strtools.h"
#include "IVirtualStream.h"

// section beginning / ending
#define KV_SECTION_BEGIN			'{'
#define KV_SECTION_END				'}'

#define KV_ARRAY_BEGIN				'['
#define KV_ARRAY_END				']'
#define KV_ARRAY_SEPARATOR			','

// string symbols
#define KV_STRING_BEGIN_END			'\"'
#define KV_STRING_NEWLINE			'\n'

// commnent symbols
#define KV_COMMENT_SYMBOL			'/'
#define KV_RANGECOMMENT_BEGIN_END	'*'

#define KV_TYPE_SYMBOL				'&'
#define KV_TYPE_VALUESYMBOL			':'

#define KV_BREAK					';'

#define IsKVArraySeparator(c)	(c == KV_ARRAY_SEPARATOR || c == KV_ARRAY_END)
#define IsKVWhitespace(c)		(xisspace(c) || c == '\0' || c == KV_STRING_NEWLINE)

//-----------------------------------------------------------------------------------------

enum EQuoteMode
{
	QM_NONE = 0,

	QM_COMMENT_LINE,
	QM_COMMENT_RANGE,

	QM_STRING,
	QM_STRING_QUOTED,

	QM_SECTION,			// section or array
};

#pragma todo("KV_ParseSectionV3 - MBCS isspace and carriage return check...")

// V3 is backward compatible with V1
kvkeybase_t* KV_ParseSectionV3( const char* pszBuffer, int bufferSize, const char* pszFileName, kvkeybase_t* pParseTo, int nStartLine )
{
	kvkeybase_t* pKeyBase = pParseTo;

	if(!pKeyBase)
		pKeyBase = new kvkeybase_t;

	const char* pData = (char*)pszBuffer;
	const char* pLast = pData;
	char c = *pData;

	const char* pFirstLetter = NULL;

	EQuoteMode quoteMode = QM_NONE;
	bool valueArray = false;
	int valueArrayStartLine = -1;

	int sectionDepth = 0;

	int nValCounter = 0;

	int nLine = nStartLine;
	int nModeStartLine = -1;

	kvkeybase_t* currentKey = NULL;

	// allocate 129 b in stack
	//char* key = (char*)stackalloc(KV_MAX_NAME_LENGTH+1);
	//strcpy(key, "unnamed");

	bool isError = false;

	do
	{
		c = *pData;

		pLast = pData;

		if(c == '\n')
			nLine++;

		// skip non-single character white spaces, tabs, and newlines
		if(c == '\\')
		{
			if(*(pData+1) == 'n' || *(pData+1) == 't' || *(pData+1) == '\"')
			{
				pData++;
				continue;
			}
		}

		// check for comment mode enabling
		if ( c == KV_COMMENT_SYMBOL && quoteMode == QM_NONE)
		{
			char commentend = *(pData+1);

			if(commentend == KV_COMMENT_SYMBOL)
			{
				quoteMode = QM_COMMENT_LINE;
				nModeStartLine = nLine;
				continue;
			}
			else if(commentend == KV_RANGECOMMENT_BEGIN_END)
			{
				quoteMode = QM_COMMENT_RANGE;
				nModeStartLine = nLine;
				continue;
			}
		}
		else if ( c == KV_STRING_NEWLINE && quoteMode == QM_COMMENT_LINE ) // check comment mode disabling
		{
			// Stop commentary mode after new line
			quoteMode = QM_NONE;
			continue;
		}
		else if ( c == KV_RANGECOMMENT_BEGIN_END && quoteMode == QM_COMMENT_RANGE )
		{
			char commentend = *(pData+1);
			if(commentend == KV_COMMENT_SYMBOL)
			{
				quoteMode = QM_NONE;
				pData++;
				continue;
			}
		}

		// skip chars when in comment mode
		if(quoteMode == QM_COMMENT_RANGE || quoteMode == QM_COMMENT_LINE)
			continue;

		// reading a key/value characters
		if(quoteMode == QM_STRING || quoteMode == QM_STRING_QUOTED)
		{
			// check for reading array

			// read type name
			if(quoteMode == QM_STRING && c == KV_TYPE_VALUESYMBOL)
			{
				{
					int nLen = (pLast - pFirstLetter);
					char* pTempStr = (char*)malloc(nLen + 1);

					strncpy(pTempStr, pFirstLetter, nLen);
					pTempStr[nLen] = '\0';

					//Msg("VALUE TYPE: '%s'\n", pTempStr);

					free(pTempStr);
				}

				// parse value type and reset
				quoteMode = QM_NONE;
			}
			else if((	quoteMode == QM_STRING_QUOTED	&& c == KV_STRING_BEGIN_END) ||
						(quoteMode == QM_STRING			&& ( c == KV_BREAK || IsKVWhitespace(c) || (valueArray && IsKVArraySeparator(c)))))

			{
				if(quoteMode == QM_STRING)
				{
					if(valueArray && IsKVArraySeparator(c))
					{
						pData--; // get back by ]
					}
				}

				// complete the key and add
				if(!valueArray)
				{
					nValCounter++;
				}

				{
					int nLen = (pLast - pFirstLetter);
					char* pTempStr = (char*)malloc(nLen + 1);

					strncpy(pTempStr, pFirstLetter, nLen);
					pTempStr[nLen] = '\0';

					if(nValCounter == 1)
					{
						currentKey->SetName(pTempStr);
					}
					else if(nValCounter == 2)
					{
						// append value if we parsing an array
						if(valueArray)
							currentKey->AppendValue(pTempStr);
						else
							currentKey->SetValue(pTempStr);
					}

					//Msg("Read quoted string: '%s' (nValCounter = %d)\n", pTempStr, nValCounter);

					free(pTempStr);
				}

				if(nValCounter == 2 && !valueArray)
				{
					nValCounter = 0;
					//currentKey = NULL;
				}

				// force break
				if(quoteMode == QM_STRING && c == KV_BREAK && !valueArray)
				{
					nValCounter = 0;
					currentKey = NULL;
				}

				quoteMode = QM_NONE;
				pFirstLetter = NULL;
			}
		}
		else if(quoteMode == QM_NONE) // test for begin read keys/sections
		{
			// skip whitespaces
			if(isspace(c))
				continue;

			if( c == KV_BREAK && !valueArray )
			{
				nValCounter = 0;
				currentKey = NULL;
				continue;
			}

			// begin section mode
			if( c == KV_SECTION_BEGIN )
			{
				quoteMode = QM_SECTION;
				nModeStartLine = nLine;
				pFirstLetter = pData+1;

				sectionDepth++;

				nValCounter = 0;
			}
			else if( c == KV_STRING_BEGIN_END ) // start parsing quoted string
			{
				quoteMode = QM_STRING_QUOTED;
				nModeStartLine = nLine;
				pFirstLetter = pData+1;

				if(nValCounter == 0)
				{
					currentKey = new kvkeybase_t();
					pKeyBase->keys.append(currentKey);
				}
			}
			else if(c == KV_ARRAY_BEGIN && nValCounter == 1) // enable array parsing
			{
				valueArray = true;
				valueArrayStartLine = nLine;
				nValCounter++;
				continue;
			}
			else if(IsKVArraySeparator(c) && valueArray) // check array separator or ending
			{
				// add last value after separator or closing
				if(c == KV_ARRAY_END)
				{
					// unlock value counter
					valueArray = false;
					nValCounter = 0;

					//currentKey = NULL;
				}

				continue;
			}
			else
			{
				// start parsing simple string
				quoteMode = QM_STRING;
				nModeStartLine = nLine;
				pFirstLetter = pData;

				if(nValCounter == 0)
				{
					currentKey = new kvkeybase_t();
					pKeyBase->keys.append(currentKey);
				}
			}
		}
		else if(quoteMode == QM_SECTION) // section skipper and processor
		{
			// begin section mode
			if( c == KV_SECTION_BEGIN )
			{
				if(sectionDepth == 0)
					pFirstLetter = pData+1;

				sectionDepth++;
			}
			else if( c == KV_SECTION_END )
			{
				if(sectionDepth > 0)
					sectionDepth--;

				if(sectionDepth == 0)
				{
					// read buffer
					int nLen = (pLast - pFirstLetter);

					if(!currentKey)
					{
						ASSERT(!"Parse error: anonymous sections are not allowed\n");
					}

					// alloc section and parse
					kvkeybase_t* section = KV_ParseSectionV3(pFirstLetter, nLen, pszFileName, currentKey, nModeStartLine);
					if(!section)
					{
						pKeyBase->keys.fastRemove(currentKey);
						return NULL;
					}

					// disable
					quoteMode = QM_NONE;
					pFirstLetter = NULL;
				}
			}
		}
	}
	while(*pData++ && (pData-pszBuffer) < bufferSize);

	// check for errors

	// if mode is not none, then is error
	if(quoteMode != QM_NONE)
	{
		if(quoteMode == QM_COMMENT_RANGE)
		{
			MsgError("'%s' error 1 (%d): unexpected EOF, did you forgot '*/'?\n", pszFileName, nModeStartLine);
			isError = true;
		}
		else if(quoteMode == QM_SECTION)
		{
			MsgError("'%s' error 2 (%d): missing '}'\n", pszFileName, nModeStartLine);
			isError = true;
		}
		else if(quoteMode == QM_STRING_QUOTED)
		{
			MsgError("'%s' error 3 (%d): missing '\"'\n", pszFileName, nModeStartLine);
			isError = true;
		}
	}

	if(valueArray)
	{
		MsgError("'%s' error 4 (%d): array - missing ']'\n", pszFileName, valueArrayStartLine);
		isError = true;
	}

	if(isError)
	{
		if(pParseTo != pKeyBase)
			delete pKeyBase;;
		return NULL;
	}

	return pKeyBase;
}

//-----------------------------------------------------------------------------------------

// searches for keybase
kvkeybase_t* KeyValues::FindKeyBase(const char* pszName, int nFlags)
{
	if(!m_pKeyBase)
		return NULL;

	return m_pKeyBase->FindKeyBase(pszName, nFlags);
}

// loads from file
bool KeyValues::LoadFromFile(const char* pszFileName, int nSearchFlags)
{
	kvkeybase_t* default_base = m_pKeyBase;

	m_pKeyBase = KV_LoadFromFile(pszFileName, nSearchFlags);

	if(m_pKeyBase)
	{
		delete default_base;
		return true;
	}

	m_pKeyBase = default_base;

	return false;
}

bool KeyValues::LoadFromStream(ubyte* pData)
{
	kvkeybase_t* default_base = m_pKeyBase;

	m_pKeyBase = KV_ParseSection( (const char*)pData, NULL, NULL, 0 );

	if(m_pKeyBase)
	{
		delete default_base;
		return true;
	}

	m_pKeyBase = default_base;

	return false;
}

void KV_WriteToStream_r(kvkeybase_t* pKeyBase, IVirtualStream* pStream, int nTabs, bool bOldFormat, bool pretty);

void KeyValues::SaveToFile(const char* pszFileName, int nSearchFlags)
{
	IFile* pStream = GetFileSystem()->Open(pszFileName, "wt", nSearchFlags);

	if(pStream)
	{
		KV_WriteToStream_r(m_pKeyBase, pStream, 0, false, true);

		//PPMemInfo();

		GetFileSystem()->Close(pStream);
	}
	else
	{
		MsgError("Cannot save keyvalues to file '%s'!\n", pszFileName);
	}
}


kvkeybase_t::kvkeybase_t()
{
	line = 0;
	unicode = false;
	strcpy(name, "unnamed");
}

kvkeybase_t::~kvkeybase_t()
{
	Cleanup();
}

void kvkeybase_t::Cleanup()
{
	ClearValues();

	for(int i = 0; i < keys.numElem(); i++)
		delete keys[i];

	keys.clear();
}

void kvkeybase_t::ClearValues()
{
	for(int i = 0; i < values.numElem(); i++)
		free(values[i]);

	values.clear();
}

// sets keybase name
void kvkeybase_t::SetName(const char* pszName)
{
	strcpy( name, pszName );
}

// searches for keybase
kvkeybase_t* kvkeybase_t::FindKeyBase(const char* pszName, int nFlags) const
{
	for(int i = 0; i < keys.numElem(); i++)
	{
		if((nFlags & KV_FLAG_SECTION) && keys[i]->keys.numElem() == 0)
			continue;

		if((nFlags & KV_FLAG_NOVALUE) && keys[i]->values.numElem() > 0)
			continue;

		if((nFlags & KV_FLAG_ARRAY) && keys[i]->values.numElem() <= 1)
			continue;

		if(!stricmp(keys[i]->name, pszName))
			return keys[i];
	}

	return NULL;
}

// adds new keybase
kvkeybase_t* kvkeybase_t::AddKeyBase( const char* pszName, const char* pszValue)
{
	kvkeybase_t* pKeyBase = new kvkeybase_t;
	pKeyBase->SetName(pszName);
	keys.append( pKeyBase );

	if(pszValue != NULL)
	{
		char* alloc_str = (char*)malloc(strlen(pszValue) + 1);
		strcpy(alloc_str, pszValue);

		pKeyBase->values.append( alloc_str );
	}

	return pKeyBase;
}


// sets the key
void kvkeybase_t::SetKey(const char* pszName, const char* pszValue)
{
	kvkeybase_t* pKeyBase = FindKeyBase(pszName);

	if(!pKeyBase)
	{
		pKeyBase = new kvkeybase_t;
		pKeyBase->SetName(pszName);
		keys.append( pKeyBase );
	}

	if(pszValue != NULL)
	{
		pKeyBase->SetValueByIndex(pszValue);
	}
}

// removes key base
void kvkeybase_t::RemoveKeyBase( const char* pszName )
{
	ASSERTMSG(false, "kvkeybase_t::RemoveKeyBase not implemented");
}

// sets string value array
void kvkeybase_t::SetValue( const char* pszString )
{
	SetValueByIndex(pszString);
}

// sets string value to array index
void kvkeybase_t::SetValueByIndex( const char* pszValue, int nIdx )
{
	if(nIdx < 0)
		return;

	if( nIdx > values.numElem()-1)
	{
		values.setNum( values.numElem() + 1 );
		values[nIdx] = NULL;
	}

	// delete old value
	if(values[nIdx])
		free(values[nIdx]);

	values[nIdx] = (char*)malloc(strlen(pszValue) + 1);
	strcpy(values[nIdx], pszValue);
}

// sets array of values
void kvkeybase_t::SetValues(const char** pszStrings, int count)
{
	// clear previous values
	for(int i = 0; i < values.numElem(); i++)
	{
		if(values[i])
			free(values[i]);

		values[i] = NULL;
	}

	values.setNum( count );

	// set new ones
	for(int i = 0; i < count; i++)
	{
		values[i] = (char*)malloc(strlen(pszStrings[i]) + 1);

		strcpy(values[i], pszStrings[i]);
	}
}

// adds value
int	kvkeybase_t::AppendValue( const char* pszValue )
{
	char* pszAlloc = (char*)malloc(strlen(pszValue) + 1);
	strcpy(pszAlloc, pszValue);

	return values.append(pszAlloc);
}

void kvkeybase_t::MergeFrom(const kvkeybase_t* base, bool recursive)
{
	if(base == NULL)
		return;

	for(int i = 0; i < base->keys.numElem(); i++)
	{
		kvkeybase_t* src = base->keys[i];

		kvkeybase_t* dst = AddKeyBase(src->name);

		dst->SetValues((const char**)src->values.ptr(), src->values.numElem());

		// go to next in hierarchy
		if(recursive)
			dst->MergeFrom(src, recursive);
	}
}

// checkers
bool kvkeybase_t::IsSection() const
{
	return keys.numElem() > 0;
}

bool kvkeybase_t::IsArray() const
{
	return values.numElem() > 1;
}

bool kvkeybase_t::IsDefinition() const
{
	return values.numElem() == 0;
}


// API functions

kvkeybase_t* KV_LoadFromFile( const char* pszFileName, int nSearchFlags, kvkeybase_t* pParseTo )
{
	long lSize = 0;
	char* pBuffer = (char*)GetFileSystem()->GetFileBuffer(pszFileName, &lSize, nSearchFlags);
	char* _buffer = pBuffer;

	if(!_buffer)
	{
		DevMsg(1, "Can't open key-values file '%s'\n", pszFileName);
		return NULL;
	}

	wchar_t byteordermark = *((wchar_t*)_buffer);

	if(byteordermark == 0xbbef)
	{
		// skip this three byte bom
		_buffer += 3;
	}
	else if(byteordermark == 0xfeff)
	{
		ASSERT(!"Only UTF-8 keyvalues supported!!!");
		PPFree( pBuffer );
		return NULL;
	}

	// load as stream
	// KV_ParseSectionV3(_buffer, lSize, pszFileName, pParseTo, 1);
	kvkeybase_t* pBase = KV_ParseSection(_buffer, pszFileName, pParseTo, 0);

	if(pBase)
	{
		strcpy(pBase->name, pszFileName);

		if(byteordermark == 0xbbef)
			pBase->unicode = true;
	}

	// required to clean memory after reading
	PPFree( pBuffer );

	return pBase;
}

#define NOCOMMENT		0
#define LINECOMMENT		1
#define RANGECOMMENT	2

// version V2
kvkeybase_t* KV_ParseSection( const char* pszBuffer, const char* pszFileName, kvkeybase_t* pParseTo, int nStartLine )
{
	const char* pData = (char*)pszBuffer;
	char c;

	// set the first character of data
	c = *pData;

	const char *pFirstLetter = NULL;
	const char*	pLast = pData;

	int			nSectionLetterLine = 0;
	int			nQuoteLetterLine = 0;

	bool		bInQuotes = false;
	bool		bInSection = false;

	// Skip for sections
	int nSectionRecursionSkip = 0;

	kvkeybase_t* pKeyBase = pParseTo;

	if(!pKeyBase)
		pKeyBase = new kvkeybase_t;

	kvkeybase_t* pCurrentKeyBase = NULL;
	kvkeybase_t* pPrevKeyBase = NULL;

	int bCommentaryMode = NOCOMMENT;

	int nValueCounter = 0;

	int nLine = nStartLine;

	for ( ; ; ++pData )
	{
		c = *pData;

		if(c == '\0')
			break;

		pLast = pData;

		if(c == KV_STRING_NEWLINE)
			nLine++;

		// check commentary mode
		if ( c == KV_COMMENT_SYMBOL && !bInQuotes )
		{
			char commentend = *(pData+1);

			// we got comment symbol again
			if( commentend == KV_COMMENT_SYMBOL && bCommentaryMode != RANGECOMMENT )
			{
				bCommentaryMode = LINECOMMENT;
				continue;
			}
			else if( commentend == KV_RANGECOMMENT_BEGIN_END && bCommentaryMode != LINECOMMENT )
			{
				bCommentaryMode = RANGECOMMENT;
				continue;
			}
		}

		// Stop cpp commentary mode after newline
		if ( c == KV_STRING_NEWLINE && bCommentaryMode == 1 )
		{
			bCommentaryMode = NOCOMMENT;
			continue;
		}

		if ( c == KV_RANGECOMMENT_BEGIN_END && bCommentaryMode == 2 )
		{
			char commentend = *(pData+1);
			if(commentend == KV_COMMENT_SYMBOL)
			{
				bCommentaryMode = NOCOMMENT;
				pData++; // little hack
				continue;
			}
		}

		// skip commented text
		if(bCommentaryMode)
			continue;

		// TODO: replace/skip special characters in here

		// if we found section opening character and there is a key base
		if( c == KV_SECTION_BEGIN && !bInQuotes)
		{
			// keybase must be created
			if(!pCurrentKeyBase)
			{
				MsgError("'%s' (%d): section has no keybase\n", pszFileName ? pszFileName : "buffer", nLine+1);

				if(pParseTo != pKeyBase)
					delete pKeyBase;
				return NULL;
			}

			// Do skip only if we have in another section
			if(!pFirstLetter && (nSectionRecursionSkip == 0))
			{
				bInSection = true;
				pFirstLetter = pData + 1;
				nSectionLetterLine = nLine;
			}

			// Up recursion by one
			nSectionRecursionSkip++;
			continue;
		}

		if( pFirstLetter && bInSection ) // if we have parsing section
		{
			if( c == KV_SECTION_END )
			{
				if(nSectionRecursionSkip > 0)
					nSectionRecursionSkip--;

				// if we have reached this section ending, start parsing it's contents
				if(nSectionRecursionSkip == 0)
				{
					int nLen = (int)pLast - (int)pFirstLetter;
					char* pszTemp = (char*)malloc(nLen+1);
					memcpy(pszTemp, pFirstLetter, nLen);
					pszTemp[nLen] = 0;

					// recurse
					kvkeybase_t* pBase = KV_ParseSection( pszTemp, pszFileName, pCurrentKeyBase, nSectionLetterLine );

					// if it got all killed
					if(!pBase)
					{
						free( pszTemp );
						//delete pKeyBase;

						return NULL;
					}

					free( pszTemp );

					bInSection = false;
					pFirstLetter = NULL;

					// NOTE: we could emit code below to not use KV_BREAK character strictly after section
					pCurrentKeyBase = NULL;
					nValueCounter = 0;
				}
			}

			continue; // don't parse the entire section until we got a section ending
		}

		// if not in quotes and found whitespace
		// or if in quotes and there is closing character
		// TODO: check \" inside quotes
		if( pCurrentKeyBase && pFirstLetter &&
			((!bInQuotes && (isspace(c) || (c == KV_BREAK))) ||
			(bInQuotes && (c == KV_STRING_BEGIN_END))))
		{
			int nLen = (int)pLast - (int)pFirstLetter;

			// close token
			if(nValueCounter <= 0)
			{
				strncpy( pCurrentKeyBase->name, pFirstLetter, nLen);
				pCurrentKeyBase->name[nLen] = '\0';
			}
			else
			{
				char* pszString = (char*)malloc(strlen(pFirstLetter) + 1);
				strncpy( pszString, pFirstLetter, nLen);
				pszString[nLen] = '\0';

				pCurrentKeyBase->values.append(pszString);
			}

			pFirstLetter = NULL;
			bInQuotes = false;

			if(c == KV_BREAK)
			{
				pPrevKeyBase = pCurrentKeyBase;
				pCurrentKeyBase = NULL;
				nValueCounter = 0;
			}

			continue;
		}

		// end keybase if we got semicolon
		if( !bInQuotes && (c == KV_BREAK) )
		{
			pCurrentKeyBase = NULL;
			nValueCounter = 0;
			continue;
		}

		// start token
		if ( !pFirstLetter && (c != KV_BREAK) &&
			(c != KV_SECTION_BEGIN) && (c != KV_SECTION_END))
		{
			// if we got quote character or this is not a space character
			// begin new token

			if((c == KV_STRING_BEGIN_END) || !isspace(c))
			{
				// create keybase or increment value counter
				if( pCurrentKeyBase )
				{
					nValueCounter++;
				}
				else
				{
					pCurrentKeyBase = new kvkeybase_t;
					pCurrentKeyBase->line = nLine;
					pKeyBase->keys.append( pCurrentKeyBase );
				}

				if( c == KV_STRING_BEGIN_END )
					bInQuotes = true;

				nQuoteLetterLine = nLine;

				pFirstLetter = pData + (bInQuotes ? 1 : 0);

				continue;
			}
		}
	}

	if( bInQuotes )
	{
		MsgError("'%s' (%d): unexcepted end of file, you forgot to close quotes\n", pszFileName ? pszFileName : "buffer", nQuoteLetterLine+1);


		if(pParseTo != pKeyBase)
			delete pKeyBase;
		pKeyBase = NULL;
	}

	if( pCurrentKeyBase )
	{
		MsgError("'%s' (%d): EOF passed, excepted ';'\n", pszFileName ? pszFileName : "buffer", pCurrentKeyBase->line+1);
		if(pParseTo != pKeyBase)
			delete pKeyBase;
		pKeyBase = NULL;
	}

	if( bInSection )
	{
		MsgError("'%s' (%d): EOF passed, excepted '}'\n", pszFileName ? pszFileName : "buffer", nSectionLetterLine+1);
		if(pParseTo != pKeyBase)
			delete pKeyBase;
		pKeyBase = NULL;
	}

	if( bCommentaryMode == 2 )
	{
		MsgError("'%s' (%d): EOF passed, excepted '*/', check whole text please\n", pszFileName ? pszFileName : "buffer", nLine+1);
		if(pParseTo != pKeyBase)
			delete pKeyBase;
		pKeyBase = NULL;
	}

	return pKeyBase;
}

bool UTIL_StringNeedsQuotes( const char* pszString )
{
	char const* pLetter = pszString;

	int length = 0;

	// Check the entire string
	while (*pLetter != 0)
	{
		// break on spaces, but allow newline characters
		// in key/values
		if(isspace(*pLetter) && *pLetter != '\n')
			return true;

		length++;

		// commentary also must be quoted
		if(*pLetter == KV_COMMENT_SYMBOL)
		{
			if(*(pLetter+1) == KV_RANGECOMMENT_BEGIN_END || *(pLetter+1) == KV_COMMENT_SYMBOL)
				return true;
		}
		else if(*pLetter == KV_RANGECOMMENT_BEGIN_END)
		{
			if(*(pLetter+1) == KV_COMMENT_SYMBOL)
				return true;
		}

		pLetter++;
	}

	// always indicate empty strings
	if(length == 0)
		return true;

	return false;
}

void WriteSelectQuotedString(const char* pszString, IVirtualStream* pStream)
{
	if( UTIL_StringNeedsQuotes( pszString ) )
		pStream->Print("\"%s\"", pszString);
	else
		pStream->Print("%s", pszString);
}

void WriteFileSelectQuotedString(const char* pszString, DKFILE* pFile)
{
	if( UTIL_StringNeedsQuotes( pszString ) )
		pFile->Print("\"%s\"", pszString);
	else
		pFile->Print("%s", pszString);
}

// writes key-values section.
void KV_WriteToFile_r(kvkeybase_t* pKeyBase, DKFILE* pFile, int nTabs, bool bOldFormat)
{
	char* tabs = (char*)stackalloc(nTabs);
	memset(tabs, 0, nTabs);

	int nTab = 0;

	while(nTab < nTabs)
		tabs[nTab++] = '\t';

	for(int i = 0; i < pKeyBase->keys.numElem(); i++)
	{
		kvkeybase_t* pKey = pKeyBase->keys[i];

		// add tabs
		pFile->Write(tabs, 1, nTabs);

		WriteFileSelectQuotedString( pKey->name, pFile);

		for(int j = 0; j < pKey->values.numElem(); j++)
		{
			pFile->Print(" ");
			WriteFileSelectQuotedString( pKey->values[j], pFile);
		}

		// write subsection

		if( pKey->keys.numElem() )
		{
			pFile->Print("\n");
			pFile->Write(tabs, 1, nTabs);
			pFile->Print("{\n");

			KV_WriteToFile_r(pKey, pFile, nTabs + 1, bOldFormat);

			pFile->Write(tabs, 1, nTabs);
			pFile->Print("}");
		}

		if(bOldFormat)
			pFile->Print("\n");
		else
			pFile->Print(";\n");
	}
}

void KV_WriteToStream_r(kvkeybase_t* pKeyBase, IVirtualStream* pStream, int nTabs, bool bOldFormat, bool pretty)
{
	char* tabs = (char*)stackalloc(nTabs);
	memset(tabs, 0, nTabs);

	int nTab = 0;

	if(pretty)
	{
		while(nTab < nTabs)
			tabs[nTab++] = '\t';
	}

	for(int i = 0; i < pKeyBase->keys.numElem(); i++)
	{
		kvkeybase_t* pKey = pKeyBase->keys[i];

		// add tabs
		if(pretty)
		{
			pStream->Write(tabs, 1, nTabs);
		}

		WriteSelectQuotedString( pKey->name, pStream);

		for(int j = 0; j < pKey->values.numElem(); j++)
		{
			pStream->Print(" ");
			WriteSelectQuotedString( pKey->values[j], pStream);
		}

		// write subsection

		if( pKey->keys.numElem() )
		{
			if(pretty)
			{
				pStream->Print("\n");
				pStream->Write(tabs, 1, nTabs);
			}
			pStream->Print("{\n");

			KV_WriteToStream_r(pKey, pStream, nTabs + 1, bOldFormat, pretty);

			if(pretty)
			{
				pStream->Write(tabs, 1, nTabs);
			}

			pStream->Print("}");
		}

		if(bOldFormat)
			pStream->Print("\n");
		else
			pStream->Print(";\n");
	}
}

// gets value of string type
const char* KV_GetValueString( kvkeybase_t* pBase, int nIndex, const char* pszDefault )
{
	if(pBase && nIndex >= 0 && nIndex < pBase->values.numElem())
		return pBase->values[nIndex];
	else
		return pszDefault;
}

int	KV_GetValueInt( kvkeybase_t* pBase, int nIndex, int nDefault )
{
	// make sure that index is valid
	if(pBase && nIndex >= 0 && nIndex < pBase->values.numElem())
		return atoi( pBase->values[nIndex] );
	else
		return nDefault;
}

float KV_GetValueFloat( kvkeybase_t* pBase, int nIndex, float fDefault )
{
	// make sure that index is valid
	if(pBase && nIndex >= 0 && nIndex < pBase->values.numElem())
	{
		return atof( pBase->values[nIndex] );
	}
	else
		return fDefault;
}

bool KV_GetValueBool( kvkeybase_t* pBase, int nIndex, bool bDefault)
{
	// make sure that index is valid
	if(pBase && nIndex >= 0 && nIndex < pBase->values.numElem())
		return atoi( pBase->values[nIndex] ) > 0;
	else
		return bDefault;
}

Vector2D KV_GetVector2D( kvkeybase_t* pBase, int nIndex, const Vector2D& vDefault)
{
	Vector2D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);

	return retVal;
}

Vector3D KV_GetVector3D( kvkeybase_t* pBase, int nIndex, const Vector3D& vDefault)
{
	Vector3D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);
	retVal.z = KV_GetValueFloat(pBase, nIndex+2, vDefault.z);

	return retVal;
}

Vector4D KV_GetVector4D( kvkeybase_t* pBase, int nIndex, const Vector4D& vDefault)
{
	Vector4D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);
	retVal.z = KV_GetValueFloat(pBase, nIndex+2, vDefault.z);
	retVal.w = KV_GetValueFloat(pBase, nIndex+3, vDefault.w);

	return retVal;
}
