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

#include "VirtualStream.h"

#ifdef LINUX
#include <ctype.h>
#endif

#include "utils/strtools.h"
#include "IVirtualStream.h"

static const char* s_szkKVValueTypes[KVPAIR_TYPES] =
{
	"string",
	"int",
	"float",
	"bool",
	"section",		// sections
};


EKVPairType KVResolvePairType( const char* string )
{
	char* typeName = (char*)string;

	// check types
	for(int i = 0; i < KVPAIR_TYPES; i++)
	{
		if(!stricmp(typeName, s_szkKVValueTypes[i]))
		{
			return (EKVPairType)i;
		}
	}

	MsgError("invalid kvpair type '%s'\n", typeName);

	return KVPAIR_STRING;
}

char* KVReadProcessString( char* pszStr )
{
	char* ptr = pszStr;
	// convert some symbols to special ones

	int sLen = strlen( pszStr );

	char* temp = (char*)malloc( sLen+10 );
	char* ptrTemp = temp;

	do
	{
		if(*ptr == '\\')
		{
			if(*(ptr+1) == '"') 
			{
				*ptrTemp++ = '"';
				ptr++;
			}
			else if(*(ptr+1) == 'n')
			{
				*ptrTemp++ = '\n';
				ptr++;
			}
			else if(*(ptr+1) == 't')
			{
				*ptrTemp++ = '\t';
				ptr++;
			}
			else
				*ptrTemp++ = *ptr;
		}
		else
			*ptrTemp++ = *ptr;

	}while(*ptr++ != NULL);

	// add NULL
	*ptrTemp++ = 0;

	// copy string
	return temp;
}

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
#define IsKVWhitespace(c)		(isspace(c) || c == '\0' || c == KV_STRING_NEWLINE)

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

#pragma todo("KV_ParseSectionV3 - MBCS isspace issues.")

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

	// allocate 129 b in stack
	char* key = (char*)stackalloc(KV_MAX_NAME_LENGTH+1);
	strcpy(key, "unnamed");

	kvkeybase_t* curpair = NULL;

	do
	{
		c = *pData;

		pLast = pData;

		if(c == '\n')
			nLine++;

		// skip non-single character white spaces, tabs, carriage returns, and newlines
		if(c == '\\' && (*(pData+1) == 'n' || *(pData+1) == 't' || *(pData+1) == '\"'))
		{
			pData++;
			continue;
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
			if((quoteMode == QM_STRING) && (c == KV_TYPE_VALUESYMBOL))
			{
				{
					int nLen = (pLast - pFirstLetter);
					char* pTempStr = (char*)malloc(nLen + 1);

					strncpy(pTempStr, pFirstLetter, nLen);
					pTempStr[nLen] = '\0';

					// set the type
					curpair->type = KVResolvePairType(pTempStr);

					free(pTempStr);
				}

				// parse value type and reset
				//pFirstLetter = pData+1;
				quoteMode = QM_NONE;
				//nValCounter--;
			}
			else if((quoteMode == QM_STRING_QUOTED && (c == KV_STRING_BEGIN_END)) || 
					(quoteMode == QM_STRING && (IsKVWhitespace(c) || (valueArray && IsKVArraySeparator(c)))))
			{
				// complete the key and add
				if(!valueArray)
					nValCounter++;
				else if(quoteMode == QM_STRING && IsKVArraySeparator(c))
					pData--; // get back by ]

				{
					int nLen = (pLast - pFirstLetter);

					if(nValCounter == 1)
					{
						// set key name
						strncpy_s(key, KV_MAX_NAME_LENGTH, pFirstLetter, nLen);
					}
					else if(nValCounter == 2)
					{
						char* pTempStr = (char*)malloc(nLen + 1);

						strncpy_s(pTempStr, nLen+1, pFirstLetter, nLen);
						pTempStr[nLen] = '\0';

						// pre-process string
						char* valueString = KVReadProcessString(pTempStr);
						free(pTempStr);

						if(valueArray)
						{
							// make it parsed if the type is different
							kvpairvalue_t* value = curpair->CreateValue();
							value->SetValueFromString(valueString);
						}
						else
						{
							// set or create a single value
							kvpairvalue_t* value = curpair->values.numElem() ? curpair->values[0] : curpair->CreateValue();
							value->SetValueFromString(valueString);
						}

						// free processed string
						free(valueString);

						curpair->SetName(key);
						pParseTo->keys.addUnique(curpair);
					}
				}

				if(nValCounter == 2 && !valueArray)
				{
					nValCounter = 0;
					//*key = 0;
					//curpair = NULL;
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

			// begin section mode
			if( c == KV_SECTION_BEGIN )
			{
				//Msg("start section\n");
				if(curpair == NULL)
				{
					MsgError("'%s' error 5 (%d): unexpected anonymous section\n", pszFileName, nModeStartLine);
					break;
				}

				quoteMode = QM_SECTION;
				nModeStartLine = nLine;
				pFirstLetter = pData+1;

				sectionDepth++;
			}
			else if( c == KV_STRING_BEGIN_END )
			{
				quoteMode = QM_STRING_QUOTED;
				nModeStartLine = nLine;
				pFirstLetter = pData+1;

				if(nValCounter == 0)
				{
					curpair = new kvkeybase_t();
					curpair->line = nModeStartLine;
				}

				//if(!valueArray) nValCounter++;
			}
			else if(c == KV_ARRAY_BEGIN && nValCounter == 1) // enable array parsing
			{
				//Msg("BEGIN ARRAY\n");
				valueArray = true;
				valueArrayStartLine = nLine;
				nValCounter++;
				continue;
			}
			else if(IsKVArraySeparator(c) && valueArray)
			{
				// add last value after separator or closing

				if(c == KV_ARRAY_END)
				{
					valueArray = false;

					// complete the key and add
					nValCounter = 0;
				}

				continue;
			}
			else
			{
				quoteMode = QM_STRING;
				nModeStartLine = nLine;
				pFirstLetter = pData;

				if(nValCounter == 0)
				{
					curpair = new kvkeybase_t();
					curpair->line = nModeStartLine;
				}

				//if(!valueArray) nValCounter++;
			}
		}
		else if(quoteMode == QM_SECTION) // section skipper and processor
		{
			// skip the next section openning but increment the depths
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

					if(valueArray)
					{
						kvkeybase_t* newsec = new kvkeybase_t();
						bool success = KV_ParseSectionV3(pFirstLetter, nLen, pszFileName, curpair, nModeStartLine) != NULL;

						if(success)
						{
							curpair->AddValue(newsec);

							curpair->SetName(key);
							pParseTo->keys.addUnique(curpair);
						}
						else
							delete newsec;
					}
					else
					{
						bool success = KV_ParseSectionV3(pFirstLetter, nLen, pszFileName, curpair, nModeStartLine) != NULL;

						if(success)
						{
							curpair->SetName(key);
							pParseTo->keys.addUnique(curpair);

							curpair = NULL; // i'ts finally done
							*key = 0;
						}
							
					}

					// disable
					nValCounter = 0;
					quoteMode = QM_NONE;
					pFirstLetter = NULL;
				} // depth
			} // KV_SECTION_END
		} // QM_SECTION
	}
	while(*pData++ && (pData - pszBuffer) < bufferSize);

	// check for errors
	bool isError = false;

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
			delete pKeyBase;
		return NULL;
	}

	return pKeyBase;
}

//------------------------------------------------------

kvpairvalue_t::~kvpairvalue_t()
{
	PPFree(value);
	delete section;
}

void kvpairvalue_t::SetValueFrom(kvpairvalue_t* from)
{
	ASSERT(from != NULL);

	type = from->type;

	SetStringValue( from->value );

	if(type == KVPAIR_INT)
	{
		nValue = from->nValue;

		fValue = nValue;
		bValue = nValue > 0;
	}
	else if(type == KVPAIR_FLOAT)
	{
		fValue = from->fValue;
		nValue = fValue;
		bValue = nValue > 0;
	}
	else if(type == KVPAIR_BOOL)
	{
		bValue = from->bValue;
		fValue = bValue;
		nValue = bValue;
	}
}

// sets string value
void kvpairvalue_t::SetStringValue( const char* pszValue )
{
	if(value)
	{
		PPFree(value);
		value = NULL;
	}

	int len = strlen(pszValue);
	value = (char*)PPAlloc(len+1);

	strcpy(value, pszValue);
}

void kvpairvalue_t::SetValueFromString( const char* pszValue )
{
	ASSERT(pszValue != NULL);

	SetStringValue( pszValue );

	delete section;
	section = NULL;

	if(type == KVPAIR_INT)
	{
		nValue = atoi( pszValue );

		fValue = nValue;
		bValue = nValue > 0;
	}
	else if(type == KVPAIR_FLOAT)
	{
		fValue = (float)atof( pszValue );
		nValue = fValue;
		bValue = nValue > 0;
	}
	else if(type == KVPAIR_BOOL)
	{
		bValue = atoi( pszValue ) > 0;
		fValue = bValue;
		nValue = bValue;
	}
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
	return KV_LoadFromFile(pszFileName, nSearchFlags, m_pKeyBase) != NULL;
}

bool KeyValues::LoadFromStream(ubyte* pData)
{
	return KV_ParseSection( (const char*)pData, NULL, m_pKeyBase, 0 ) != NULL;
}

void KV_WriteToStream_r(kvkeybase_t* pKeyBase, IVirtualStream* pStream, int nTabs, bool bOldFormat, bool pretty);

void KeyValues::SaveToFile(const char* pszFileName, int nSearchFlags)
{
	IFile* pStream = g_fileSystem->Open(pszFileName, "wt", nSearchFlags);

	if(pStream)
	{
		KV_WriteToStream_r(m_pKeyBase, pStream, 0, false, true);

		//PPMemInfo();

		g_fileSystem->Close(pStream);
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
	type = KVPAIR_STRING,
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
		delete values[i];

	values.clear();
}

// sets keybase name
void kvkeybase_t::SetName(const char* pszName)
{
	strcpy( name, pszName );
	nameHash = StringToHash(name, true);
}

const char*	kvkeybase_t::GetName() const
{
	return name;
}

//-----------------------------------------------------------------------------------------

kvpairvalue_t* kvkeybase_t::CreateValue()
{
	kvpairvalue_t* val = new kvpairvalue_t();

	val->type = type;

	values.append(val);

	return val;
}

kvkeybase_t* kvkeybase_t::CreateSectionValue()
{
	if(type != KVPAIR_SECTION)
		return NULL;

	kvpairvalue_t* val = new kvpairvalue_t();

	val->type = type;

	values.append(val);

	val->section = new kvkeybase_t();
	return val->section;
}

kvkeybase_t* kvkeybase_t::Clone() const
{
	kvkeybase_t* newKey = new kvkeybase_t();

	CopyTo(newKey);

	return newKey;
}

void kvkeybase_t::CopyTo(kvkeybase_t* dest) const
{
	for(int i = 0; i < values.numElem(); i++)
		dest->AddValue(values[i]);

	for(int i = 0; i < keys.numElem(); i++)
		dest->AddKey(keys[i]->GetName(), keys[i]->Clone());
}

void kvkeybase_t::SetValueFrom(kvkeybase_t* pOther)
{
	this->Cleanup();
	pOther->CopyTo(this);
}

// adds value to key
void kvkeybase_t::AddValue(const char* value)
{
	CreateValue();

	SetValueAt(value, values.numElem()-1);
}

void kvkeybase_t::AddValue(int nValue)
{
	CreateValue();

	SetValueAt(nValue, values.numElem()-1);
}

void kvkeybase_t::AddValue(float fValue)
{
	CreateValue();

	SetValueAt(fValue, values.numElem()-1);
}

void kvkeybase_t::AddValue(bool bValue)
{
	CreateValue();

	SetValueAt(bValue, values.numElem()-1);
}

void kvkeybase_t::AddValue(kvkeybase_t* keybase)
{
	type = KVPAIR_SECTION;

	int numVal = values.numElem();

	kvpairvalue_t* val = CreateValue();

	val->section = keybase;
	val->section->SetName(varargs("%d", numVal));
}

void kvkeybase_t::AddValue(kvpairvalue_t* value)
{
	CreateValue();

	SetValueAt(value, values.numElem()-1);
}

//-------------------------

// adds value to key
void kvkeybase_t::AddUniqueValue(const char* value)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if( !strcmp(KV_GetValueString(this, i, ""), value) )
			return;
	}

	CreateValue();

	SetValueAt(value, values.numElem()-1);
}

void kvkeybase_t::AddUniqueValue(int nValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueInt(this, i, 0) == nValue)
			return;
	}

	CreateValue();

	SetValueAt(nValue, values.numElem()-1);
}

void kvkeybase_t::AddUniqueValue(float fValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueFloat(this, i, 0.0f) == fValue)
			return;
	}

	CreateValue();

	SetValueAt(fValue, values.numElem()-1);
}

void kvkeybase_t::AddUniqueValue(bool bValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueBool(this, i, false) == bValue)
			return;
	}

	CreateValue();

	SetValueAt(bValue, values.numElem()-1);
}

//-----------------------------------------------------------------------------
// sets value
void kvkeybase_t::SetValueAt(const char* value, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	kvpairvalue_t* pairValue = values[idxAt];

	pairValue->SetStringValue(value);

	// convert'n'set
	if(type == KVPAIR_INT)
	{
		pairValue->nValue = 0;
	}
	else if(type == KVPAIR_FLOAT)
	{
		pairValue->fValue = 0;
	}
	else if(type == KVPAIR_BOOL)
	{
		pairValue->bValue = 0;
	}
}

void kvkeybase_t::SetValueAt(int nValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	kvpairvalue_t* pairValue = values[idxAt];

	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%d", nValue);

	pairValue->SetStringValue( tempbuf );

	// convert'n'set
	if(type == KVPAIR_INT)
	{
		pairValue->nValue = (int)nValue;
	}
	else if(type == KVPAIR_FLOAT)
	{
		pairValue->fValue = nValue;
	}
	else if(type == KVPAIR_BOOL)
	{
		pairValue->bValue = nValue > 0;
	}
}

void kvkeybase_t::SetValueAt(float fValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	kvpairvalue_t* pairValue = values[idxAt];

	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%g", fValue);

	pairValue->SetStringValue( tempbuf );

	// convert'n'set
	if(type == KVPAIR_INT)
	{
		pairValue->nValue = (int)fValue;
	}
	else if(type == KVPAIR_FLOAT)
	{
		pairValue->fValue = fValue;
	}
	else if(type == KVPAIR_BOOL)
	{
		pairValue->bValue = fValue > 0;
	}
}

void kvkeybase_t::SetValueAt(bool bValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	kvpairvalue_t* pairValue = values[idxAt];

	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%d", bValue ? 1 : 0);

	pairValue->SetStringValue( tempbuf );

	// convert'n'set
	if(type == KVPAIR_INT)
	{
		pairValue->nValue = bValue ? 1 : 0;
	}
	else if(type == KVPAIR_FLOAT)
	{
		pairValue->fValue = bValue ? 1.0f : 0.0f;
	}
	else if(type == KVPAIR_BOOL)
	{
		pairValue->bValue = bValue;
	}
}

void kvkeybase_t::SetValueAt(kvpairvalue_t* value, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetValueFrom(value);
}

kvkeybase_t& kvkeybase_t::SetKey(const char* name, const char* value)
{
	kvkeybase_t* pPair = (kvkeybase_t*)FindKeyBase( name );
	if(!pPair)
		return AddKey(name, value);

	pPair->ClearValues();
	pPair->type = KVPAIR_STRING;
	pPair->AddValue(value);

	return *this;
}

kvkeybase_t& kvkeybase_t::SetKey(const char* name, int nValue)
{
	kvkeybase_t* pPair = (kvkeybase_t*)FindKeyBase( name );
	if(!pPair)
		return AddKey(name, nValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_INT;
	pPair->AddValue(nValue);

	return *this;
}

kvkeybase_t& kvkeybase_t::SetKey(const char* name, float fValue)
{
	kvkeybase_t* pPair = (kvkeybase_t*)FindKeyBase( name );
	if(!pPair)
		return AddKey(name, fValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_FLOAT;
	pPair->AddValue(fValue);

	return *this;
}

kvkeybase_t& kvkeybase_t::SetKey(const char* name, bool bValue)
{
	kvkeybase_t* pPair = (kvkeybase_t*)FindKeyBase( name );
	if(!pPair)
		return AddKey(name, bValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_BOOL;
	pPair->AddValue(bValue);

	return *this;
}

kvkeybase_t& kvkeybase_t::SetKey(const char* name, kvkeybase_t* pair)
{
	if(!pair)
		return *this;

	kvkeybase_t* pPair = (kvkeybase_t*)FindKeyBase( name );
	if(!pPair)
		return AddKey(name, pair);

	pPair->Cleanup();

	pair->CopyTo(pPair);

	return *this;
}

kvkeybase_t& kvkeybase_t::AddKey(const char* name, const char* value)
{
	kvkeybase_t* pPair = AddKeyBase(name, NULL, KVPAIR_STRING);

	pPair->AddValue(value);

	return *this;
}

kvkeybase_t& kvkeybase_t::AddKey(const char* name, int nValue)
{
	kvkeybase_t* pPair = AddKeyBase(name, NULL, KVPAIR_INT);
	pPair->AddValue(nValue);

	return *this;
};

kvkeybase_t& kvkeybase_t::AddKey(const char* name, float fValue)
{
	kvkeybase_t* pPair = AddKeyBase(name, NULL, KVPAIR_FLOAT);
	pPair->AddValue(fValue);

	return *this;
}

kvkeybase_t& kvkeybase_t::AddKey(const char* name, bool bValue)
{
	kvkeybase_t* pPair = AddKeyBase(name, NULL, KVPAIR_BOOL);
	pPair->AddValue(bValue);

	return *this;
}

kvkeybase_t& kvkeybase_t::AddKey(const char* name, kvkeybase_t* pair)
{
	if(!pair)
		return *this;

	kvkeybase_t* newPair = AddKeyBase(name, NULL, KVPAIR_STRING);
	pair->CopyTo(newPair);

	return *this;
}

//-------------------------------------------------------------------------------

// searches for keybase
kvkeybase_t* kvkeybase_t::FindKeyBase(const char* pszName, int nFlags) const
{
	int hash = StringToHash(pszName);

	for(int i = 0; i < keys.numElem(); i++)
	{
		if((nFlags & KV_FLAG_SECTION) && keys[i]->keys.numElem() == 0)
			continue;

		if((nFlags & KV_FLAG_NOVALUE) && keys[i]->values.numElem() > 0)
			continue;

		if((nFlags & KV_FLAG_ARRAY) && keys[i]->values.numElem() <= 1)
			continue;

		//if(keys[i]->nameHash == hash)
		if(!stricmp(keys[i]->name, pszName))
			return keys[i];
	}

	return NULL;
}

// adds new keybase
kvkeybase_t* kvkeybase_t::AddKeyBase( const char* pszName, const char* pszValue, EKVPairType pairType)
{
	kvkeybase_t* pKeyBase = new kvkeybase_t;
	pKeyBase->SetName(pszName);
	pKeyBase->type = pairType;

	keys.append( pKeyBase );

	if(pszValue != NULL)
	{
		pKeyBase->AddValue(pszValue);
	}

	return pKeyBase;
}

// removes key base by name
void kvkeybase_t::RemoveKeyBaseByName( const char* name, bool removeAll )
{
	//int strHash = StringToHash(name, true);

	for(int i = 0; i < keys.numElem(); i++)
	{
		//if(keys[i]->nameHash == strHash)
		if(!stricmp(keys[i]->name, name))
		{
			delete keys[i];
			keys.removeIndex(i);

			if(removeAll)
				i--;
			else
				return;
		}
	}
}

void kvkeybase_t::MergeFrom(const kvkeybase_t* base, bool recursive)
{
	if(base == NULL)
		return;

	for(int i = 0; i < base->keys.numElem(); i++)
	{
		kvkeybase_t* src = base->keys[i];

		kvkeybase_t* dst = AddKeyBase(src->name);
		src->CopyTo(dst);

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
	char* pBuffer = (char*)g_fileSystem->GetFileBuffer(pszFileName, &lSize, nSearchFlags);
	char* _buffer = pBuffer;

	if(!_buffer)
	{
		DevMsg(1, "Can't open key-values file '%s'\n", pszFileName);
		return NULL;
	}

	ushort byteordermark = *((ushort*)_buffer);

	bool isUTF8 = false;

	if(byteordermark == 0xbbef)
	{
		// skip this three byte bom
		_buffer += 3;
		isUTF8 = true;
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
		pBase->SetName(pszFileName);
        pBase->unicode = isUTF8;
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

	EqString tempName;

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
					int nLen = (int)(pLast - pFirstLetter);
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
			int nLen = (int)(pLast - pFirstLetter);

			// close token
			if(nValueCounter <= 0)
			{
				tempName.Assign( pFirstLetter, nLen );

				pCurrentKeyBase->SetName(tempName.c_str());
			}
			else
			{
				char* pszString = (char*)malloc(strlen(pFirstLetter) + 1);
				strncpy( pszString, pFirstLetter, nLen);
				pszString[nLen] = '\0';

				pCurrentKeyBase->AddValue(pszString);
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

int KVCountSpecialSymbols(char* pszStr)
{
	char* ptr = pszStr;

	int specials_count = 0;

	do
	{
		if(*ptr == '"')
		{
			specials_count++;
		}
		else if(*ptr == '\n')
		{
			specials_count++;
		}
		else if(*ptr == '\t')
		{
			specials_count++;
		}
	}while(*ptr++);

	return specials_count + 1;
}

char* KVWriteProcessString( char* pszStr, char* pszTemp )
{
	char* ptr = pszStr;
	// convert some symbols to special ones

	int sLen = strlen( pszStr );

	char* temp = pszTemp; // (char*)malloc( sLen*2 );
	char* ptrTemp = temp;

	do
	{
		if(*ptr == '"')
		{
			*ptrTemp++ = '\\';
			*ptrTemp++ = *ptr;
		}
		else if(*ptr == '\n')
		{
			*ptrTemp++ = '\\';
			*ptrTemp++ = 'n';
		}
		else if(*ptr == '\t')
		{
			*ptrTemp++ = '\\';
			*ptrTemp++ = 't';
		}
		else
			*ptrTemp++ = *ptr;

	}while(*ptr++);

	// add NULL
	*ptrTemp++ = 0;

	// copy string
	return temp;
}

void KVWritePairValue(kvpairvalue_t* val, IVirtualStream* stream, int depth)
{
	// write typed data
	if(val->type == KVPAIR_STRING)
	{
		if(val->value == NULL)
		{
			stream->Print("\"%s\"", "VALUE_MISSING");
			return;
		}

		int numSpecial = KVCountSpecialSymbols(val->value);

		char* pszValueString = (char*)malloc(strlen(val->value) + numSpecial + 1);
		KVWriteProcessString( val->value, pszValueString );

		stream->Print("\"%s\"", pszValueString);

		free( pszValueString );
	}
	else if(val->type == KVPAIR_INT)
	{
		stream->Print("%d", val->nValue);
	}
	else if(val->type == KVPAIR_FLOAT)
	{
		stream->Print("%g", val->fValue);
	}
	else if(val->type == KVPAIR_BOOL)
	{
		stream->Print("%d", val->bValue);
	}
	else if(val->type == KVPAIR_SECTION)
	{
		KV_WriteToStream_r(val->section, stream, depth+1, false, false);
	}
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
			KVWritePairValue( pKey->values[j], pFile, nTabs);
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
			KVWritePairValue( pKey->values[j], pStream, nTabs);
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

// prints section to console
void KV_PrintSection(kvkeybase_t* base)
{
	CMemoryStream stream;
	stream.Open(NULL, VS_OPEN_WRITE, 2048);
	KV_WriteToStream_r(base, &stream, 0, false, true);
	char nullChar = '\0';
	stream.Write(&nullChar, 1, 1);

	Msg( "%s\n", stream.GetBasePointer() );
}

// gets value of string type
const char* KV_GetValueString( kvkeybase_t* pBase, int nIndex, const char* pszDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return pszDefault;

	return pBase->values[nIndex]->value;
}

int	KV_GetValueInt( kvkeybase_t* pBase, int nIndex, int nDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return nDefault;

	if(pBase->type == KVPAIR_INT)
		return pBase->values[nIndex]->nValue;
	else if(pBase->type == KVPAIR_FLOAT)
		return pBase->values[nIndex]->fValue;
	else if(pBase->type == KVPAIR_BOOL)
		return pBase->values[nIndex]->bValue ? 1 : 0;

	return atoi(pBase->values[nIndex]->value);
}

float KV_GetValueFloat( kvkeybase_t* pBase, int nIndex, float fDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return fDefault;

	if(pBase->type == KVPAIR_FLOAT)
		return pBase->values[nIndex]->fValue;
	else if(pBase->type == KVPAIR_INT)
		return pBase->values[nIndex]->nValue;
	else if(pBase->type == KVPAIR_BOOL)
		return pBase->values[nIndex]->bValue ? 1 : 0;

	return atof(pBase->values[nIndex]->value);
}

bool KV_GetValueBool( kvkeybase_t* pBase, int nIndex, bool bDefault)
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return bDefault;

	if(pBase->type == KVPAIR_BOOL)
		return pBase->values[nIndex]->bValue;
	else if(pBase->type == KVPAIR_FLOAT)
		return pBase->values[nIndex]->fValue > 0.0f;

	return atoi(pBase->values[nIndex]->value) > 0;
}

Vector2D KV_GetVector2D( kvkeybase_t* pBase, int nIndex, const Vector2D& vDefault)
{
	Vector2D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);

	return retVal;
}

IVector2D KV_GetIVector2D( kvkeybase_t* pBase, int nIndex, const IVector2D& vDefault)
{
	IVector2D retVal = vDefault;
	retVal.x = KV_GetValueInt(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueInt(pBase, nIndex+1, vDefault.y);

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
