//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

// Core interface helper
#include "core/core_common.h"
#include "core/IFileSystem.h"
#include "ds/MemoryStream.h"
#include "KeyValues.h"

#ifdef _MSC_VER
#pragma warning(disable: 4267)
#endif

static const char* s_szkKVValueTypes[KVPAIR_TYPES] =
{
	"string",
	"int",
	"float",
	"bool",
	"section",		// sections
};


static EKVPairType KV_ResolvePairType( const char* string )
{
	char* typeName = (char*)string;

	// check types
	for(int i = 0; i < KVPAIR_TYPES; i++)
	{
		if(!stricmp(typeName, s_szkKVValueTypes[i]))
			return (EKVPairType)i;
	}

	MsgError("invalid kvpair type '%s'\n", typeName);
	return KVPAIR_STRING;
}

// converts escape symbols to the characters
// returns new string length
static int KV_ReadProcessString( const char* pszStr, char* dest, int maxLength = INT_MAX )
{
	// convert some symbols to special ones
	size_t processLen = min(strlen( pszStr ), maxLength);

	const char* ptr = pszStr;
	char* ptrTemp = dest;
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

	}while(*ptr++ && (ptr - pszStr) < processLen);

	// add nullptr
	*ptrTemp++ = 0;

	// copy string
	return ptrTemp - dest;
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
#define KV_STRING_CARRIAGERETURN	'\r'

// commnent symbols
#define KV_COMMENT_SYMBOL			'/'
#define KV_RANGECOMMENT_BEGIN_END	'*'

#define KV_TYPE_VALUESYMBOL			':'
#define KV_BREAK					';'

#define KV_ESCAPE_SYMBOL			'\\'

#define KV_IDENT_BINARY				MCHAR4('B','K','V','S')

#define IsKVBufferEOF()				((pData - pszBuffer) > bufferSize-1)
#define IsKVArrayEndOrSeparator(c)	((c) == KV_ARRAY_SEPARATOR || (c) == KV_ARRAY_END)
#define IsKVWhitespace(c)			(isspace(c) || (c) == KV_STRING_NEWLINE || (c) == KV_STRING_CARRIAGERETURN)

//-----------------------------------------------------------------------------------------

KVPairValue::~KVPairValue()
{
	PPFree(value);
	delete section;
}

void KVPairValue::SetFrom(KVPairValue* from)
{
	ASSERT(from != nullptr);

	type = from->type;

	SetStringValue( from->value );

	if(type == KVPAIR_INT)
	{
		nValue = from->nValue;
	}
	else if(type == KVPAIR_FLOAT)
	{
		fValue = from->fValue;
	}
	else if(type == KVPAIR_BOOL)
	{
		bValue = from->bValue;
	}
	else if(type == KVPAIR_SECTION)
	{
		#pragma todo("clone the section")
	}
}

// sets string value
void KVPairValue::SetStringValue( const char* pszValue, int len /* = -1*/)
{
	if(value)
	{
		PPFree(value);
		value = nullptr;
	}

	if (len < 0)
		len = strlen(pszValue);

	value = (char*)PPAlloc(len+1);
	strncpy(value, pszValue, len);
	value[len] = 0;
}

void KVPairValue::SetFromString( const char* pszValue )
{
	ASSERT(pszValue != nullptr);

	SetStringValue( pszValue );

	delete section;
	section = nullptr;

	if(type == KVPAIR_INT)
	{
		nValue = atoi( pszValue );
	}
	else if(type == KVPAIR_FLOAT)
	{
		fValue = (float)atof( pszValue );
	}
	else if(type == KVPAIR_BOOL)
	{
		bValue = atoi( pszValue ) > 0;
	}
}

void KVPairValue::SetString(const char* value)
{
	SetStringValue(value);

	// convert'n'set
	if(type == KVPAIR_INT)
		nValue = 0;
	else if(type == KVPAIR_FLOAT)
		fValue = 0;
	else if(type == KVPAIR_BOOL)
		bValue = 0;
}

void KVPairValue::SetInt(int value)
{
	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%d", value);

	SetStringValue(tempbuf);

	// convert'n'set
	if (type == KVPAIR_INT)
		nValue = (int)value;
	else if (type == KVPAIR_FLOAT)
		fValue = value;
	else if (type == KVPAIR_BOOL)
		bValue = value > 0;
}

void KVPairValue::SetFloat(float value)
{
	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%g", value);

	SetStringValue(tempbuf);

	// convert'n'set
	if (type == KVPAIR_INT)
		nValue = (int)value;
	else if (type == KVPAIR_FLOAT)
		fValue = value;
	else if (type == KVPAIR_BOOL)
		bValue = value > 0;
}

void KVPairValue::SetBool(bool value)
{
	char tempbuf[64];

	// copy value string
	_snprintf(tempbuf, 64, "%d", value ? 1 : 0);

	SetStringValue(tempbuf);

	// convert'n'set
	if (type == KVPAIR_INT)
		nValue = value ? 1 : 0;
	else if (type == KVPAIR_FLOAT)
		fValue = value ? 1.0f : 0.0f;
	else if (type == KVPAIR_BOOL)
		bValue = value;
}

const char* KVPairValue::GetString() const
{
	return value;
}

int KVPairValue::GetInt() const
{
	if (type == KVPAIR_INT)
		return nValue;
	else if (type == KVPAIR_FLOAT)
		return fValue;
	else if (type == KVPAIR_BOOL)
		return bValue ? 1 : 0;

	return atoi(value);
}

float KVPairValue::GetFloat() const
{
	if (type == KVPAIR_FLOAT)
		return fValue;
	else if (type == KVPAIR_INT)
		return nValue;
	else if (type == KVPAIR_BOOL)
		return bValue ? 1 : 0;

	return atof(value);
}

bool KVPairValue::GetBool() const
{
	if (type == KVPAIR_BOOL)
		return bValue;
	else if (type == KVPAIR_FLOAT)
		return fValue > 0.0f;

	return atoi(value) > 0;
}

KVSection* KVSection::operator[](const char* pszName)
{
	return FindSection(pszName); 
}

KVPairValue* KVSection::operator[](int index)
{
	return values[index]; 
}

const KVSection* KVSection::operator[](const char* pszName) const
{
	return FindSection(pszName);
}

const KVPairValue* KVSection::operator[](int index) const
{
	return values[index];
}

//-----------------------------------------------------------------------------------------

KeyValues::KeyValues()
{
}

KeyValues::~KeyValues()
{
}

void KeyValues::Reset()
{
	m_root.Cleanup();
}

// searches for keybase
KVSection* KeyValues::FindSection(const char* pszName, int nFlags)
{
	return m_root.FindSection(pszName, nFlags);
}

// loads from file
bool KeyValues::LoadFromFile(const char* pszFileName, int nSearchFlags)
{
	return KV_LoadFromFile(pszFileName, nSearchFlags, &m_root) != nullptr;
}

bool KeyValues::LoadFromStream(ubyte* pData)
{
	return KV_ParseSection( (const char*)pData, 0, nullptr, &m_root, 0 ) != nullptr;
}

bool KeyValues::SaveToFile(const char* pszFileName, int nSearchFlags)
{
	IFile* pStream = g_fileSystem->Open(pszFileName, "wt", nSearchFlags);

	if(pStream)
	{
		KV_WriteToStream(pStream, &m_root, 0, true);
		g_fileSystem->Close(pStream);
	}
	else
	{
		MsgError("Cannot save keyvalues to file '%s'!\n", pszFileName);
		return false;
	}
	return true;
}

KVSection* KeyValues::GetRootSection() 
{
	return &m_root; 
}

KVSection* KeyValues::operator[](const char* pszName)
{
	return FindSection(pszName);
}

//----------------------------------------------------------------------------------------------
// KEY (PAIR) BASE
//----------------------------------------------------------------------------------------------

KVSection::KVSection()
{
	line = 0;
	unicode = false;
	type = KVPAIR_STRING,
	strcpy(name, "unnamed");
}

KVSection::~KVSection()
{
	Cleanup();
}

void KVSection::Cleanup()
{
	ClearValues();

	for(int i = 0; i < keys.numElem(); i++)
		delete keys[i];

	keys.clear();
}

void KVSection::ClearValues()
{
	for(int i = 0; i < values.numElem(); i++)
		delete values[i];

	values.clear();
}

// sets keybase name
void KVSection::SetName(const char* pszName)
{
	strncpy( name, pszName, sizeof(name));
	name[sizeof(name) - 1] = 0;

	nameHash = StringToHash(name, true);
}

const char*	KVSection::GetName() const
{
	return name;
}

//-----------------------------------------------------------------------------------------

KVPairValue* KVSection::CreateValue()
{
	// TODO: pool
	KVPairValue* val = PPNew KVPairValue();

	val->type = type;

	values.append(val);

	return val;
}

KVSection* KVSection::CreateSectionValue()
{
	if(type != KVPAIR_SECTION)
		return nullptr;

	// TODO: pool
	KVPairValue* val = PPNew KVPairValue();

	val->type = type;

	values.append(val);

	val->section = PPNew KVSection();
	return val->section;
}

KVSection* KVSection::Clone() const
{
	KVSection* newKey = PPNew KVSection();

	CopyTo(newKey);

	return newKey;
}

void KVSection::CopyTo(KVSection* dest) const
{
	CopyValuesTo(dest);

	for(int i = 0; i < keys.numElem(); i++)
		dest->AddKey(keys[i]->GetName(), keys[i]->Clone());
}

void KVSection::CopyValuesTo(KVSection* dest) const
{
	dest->ClearValues();

	for(int i = 0; i < values.numElem(); i++)
		dest->AddValue(values[i]);
}

void KVSection::SetValueFrom(KVSection* pOther)
{
	this->Cleanup();
	pOther->CopyTo(this);
}

// adds value to key
void KVSection::AddValue(const char* value)
{
	KVPairValue* val = CreateValue();
	val->SetString(value);
}

void KVSection::AddValue(int nValue)
{
	KVPairValue* val = CreateValue();
	val->SetInt(nValue);
}

void KVSection::AddValue(float fValue)
{
	KVPairValue* val = CreateValue();
	val->SetFloat(fValue);
}

void KVSection::AddValue(bool bValue)
{
	KVPairValue* val = CreateValue();
	val->SetBool(bValue);
}

void KVSection::AddValue(const Vector2D& vecValue)
{
	AddValue(vecValue.x);
	AddValue(vecValue.y);
}

void KVSection::AddValue(const Vector3D& vecValue)
{
	AddValue(vecValue.x);
	AddValue(vecValue.y);
	AddValue(vecValue.z);
}

void KVSection::AddValue(const Vector4D& vecValue)
{
	AddValue(vecValue.x);
	AddValue(vecValue.y);
	AddValue(vecValue.z);
	AddValue(vecValue.w);
}

void KVSection::AddValue(KVSection* keybase)
{
	int numVal = values.numElem();

	KVPairValue* val = CreateValue();

	val->section = keybase;
	val->section->SetName(EqString::Format("%d", numVal).ToCString());
}

void KVSection::AddValue(KVPairValue* value)
{
	KVPairValue* val = CreateValue();
	val->SetFrom(value);
}

//-------------------------

// adds value to key
void KVSection::AddUniqueValue(const char* value)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if( !strcmp(KV_GetValueString(this, i, ""), value) )
			return;
	}

	CreateValue();

	SetValue(value, values.numElem()-1);
}

void KVSection::AddUniqueValue(int nValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueInt(this, i, 0) == nValue)
			return;
	}

	CreateValue();

	SetValue(nValue, values.numElem()-1);
}

void KVSection::AddUniqueValue(float fValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueFloat(this, i, 0.0f) == fValue)
			return;
	}

	CreateValue();

	SetValue(fValue, values.numElem()-1);
}

void KVSection::AddUniqueValue(bool bValue)
{
	for(int i = 0; i < values.numElem(); i++)
	{
		if(KV_GetValueBool(this, i, false) == bValue)
			return;
	}

	CreateValue();

	SetValue(bValue, values.numElem()-1);
}

//-----------------------------------------------------------------------------
// sets value
void KVSection::SetValue(const char* value, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetString(value);
}

void KVSection::SetValue(int nValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetInt(nValue);
}

void KVSection::SetValue(float fValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetFloat(fValue);
}

void KVSection::SetValue(bool bValue, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetBool(bValue);
}

void KVSection::SetValue(const Vector2D& value, int idxAt)
{
	SetValue(value.x, idxAt++);
	SetValue(value.y, idxAt++);
}

void KVSection::SetValue(const Vector3D& value, int idxAt)
{
	SetValue(value.x, idxAt++);
	SetValue(value.y, idxAt++);
	SetValue(value.z, idxAt++);
}

void KVSection::SetValue(const Vector4D& value, int idxAt)
{
	SetValue(value.x, idxAt++);
	SetValue(value.y, idxAt++);
	SetValue(value.z, idxAt++);
	SetValue(value.w, idxAt++);
}

void KVSection::SetValue(KVPairValue* value, int idxAt)
{
	if(values.numElem() == 0)
		CreateValue();

	if(!values.inRange(idxAt))
		return;

	values[idxAt]->SetFrom(value);
}

KVSection& KVSection::SetKey(const char* name, const char* value)
{
	KVSection* pPair = (KVSection*)FindSection( name );
	if(!pPair)
		return AddKey(name, value);

	pPair->ClearValues();
	pPair->type = KVPAIR_STRING;
	pPair->AddValue(value);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, int nValue)
{
	KVSection* pPair = (KVSection*)FindSection( name );
	if(!pPair)
		return AddKey(name, nValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_INT;
	pPair->AddValue(nValue);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, float fValue)
{
	KVSection* pPair = (KVSection*)FindSection( name );
	if(!pPair)
		return AddKey(name, fValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_FLOAT;
	pPair->AddValue(fValue);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, bool bValue)
{
	KVSection* pPair = (KVSection*)FindSection( name );
	if(!pPair)
		return AddKey(name, bValue);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_BOOL;
	pPair->AddValue(bValue);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, const Vector2D& value)
{
	KVSection* pPair = (KVSection*)FindSection(name);
	if (!pPair)
		return AddKey(name, value);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_FLOAT;
	pPair->AddValue(value);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, const Vector3D& value)
{
	KVSection* pPair = (KVSection*)FindSection(name);
	if (!pPair)
		return AddKey(name, value);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_FLOAT;
	pPair->AddValue(value);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, const Vector4D& value)
{
	KVSection* pPair = (KVSection*)FindSection(name);
	if (!pPair)
		return AddKey(name, value);

	// value setup
	pPair->ClearValues();
	pPair->type = KVPAIR_FLOAT;
	pPair->AddValue(value);

	return *this;
}

KVSection& KVSection::SetKey(const char* name, KVSection* pair)
{
	if(!pair)
		return *this;

	KVSection* pPair = (KVSection*)FindSection( name );
	if(!pPair)
		return AddKey(name, pair);

	pPair->Cleanup();

	pair->CopyTo(pPair);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, const char* value)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_STRING);

	pPair->AddValue(value);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, int nValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_INT);
	pPair->AddValue(nValue);

	return *this;
};

KVSection& KVSection::AddKey(const char* name, float fValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_FLOAT);
	pPair->AddValue(fValue);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, bool bValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_BOOL);
	pPair->AddValue(bValue);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, const Vector2D& vecValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_FLOAT);
	pPair->AddValue(vecValue);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, const Vector3D& vecValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_FLOAT);
	pPair->AddValue(vecValue);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, const Vector4D& vecValue)
{
	KVSection* pPair = CreateSection(name, nullptr, KVPAIR_FLOAT);
	pPair->AddValue(vecValue);

	return *this;
}

KVSection& KVSection::AddKey(const char* name, KVSection* pair)
{
	if(!pair)
		return *this;

	KVSection* newPair = CreateSection(name, nullptr, KVPAIR_STRING);
	pair->CopyTo(newPair);

	return *this;
}

//-------------------------------------------------------------------------------

// searches for keybase
KVSection* KVSection::FindSection(const char* pszName, int nFlags) const
{
	const int hash = StringToHash(pszName, true);

	for(int i = 0; i < keys.numElem(); i++)
	{
		if((nFlags & KV_FLAG_SECTION) && keys[i]->keys.numElem() == 0)
			continue;

		if((nFlags & KV_FLAG_NOVALUE) && keys[i]->values.numElem() > 0)
			continue;

		if((nFlags & KV_FLAG_ARRAY) && keys[i]->values.numElem() <= 1)
			continue;

		if(keys[i]->nameHash == hash)
		//if(!stricmp(keys[i]->name, pszName))
			return keys[i];
	}

	return nullptr;
}

// adds new keybase
KVSection* KVSection::CreateSection( const char* pszName, const char* pszValue, EKVPairType pairType)
{
	KVSection* pKeyBase = PPNew KVSection;
	pKeyBase->SetName(pszName);
	pKeyBase->type = pairType;

	keys.append( pKeyBase );

	if(pszValue != nullptr)
	{
		pKeyBase->AddValue(pszValue);
	}

	return pKeyBase;
}


// adds existing keybase. You should set it's name manually. It should not be allocated by other keybase
void KVSection::AddSection(KVSection* keyBase)
{
	if(keyBase != nullptr)
		keys.append( keyBase );
}

// removes key base by name
void KVSection::RemoveSectionByName( const char* name, bool removeAll )
{
	const int strHash = StringToHash(name, true);

	for(int i = 0; i < keys.numElem(); i++)
	{
		if(keys[i]->nameHash == strHash)
		//if(!stricmp(keys[i]->name, name))
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

void KVSection::RemoveSection(KVSection* base)
{
	for(int i = 0; i < keys.numElem(); i++)
	{
		if(keys[i] == base)
		{
			delete keys[i];
			keys.removeIndex(i);
			return;
		}
	}
}

void KVSection::MergeFrom(const KVSection* base, bool recursive)
{
	if(base == nullptr)
		return;

	for(int i = 0; i < base->keys.numElem(); i++)
	{
		KVSection* src = base->keys[i];

		KVSection* dst = CreateSection(src->name);

		src->CopyValuesTo(dst);

		// go to next in hierarchy
		if(recursive)
			dst->MergeFrom(src, recursive);
	}
}

// checkers
bool KVSection::IsSection() const
{
	return keys.numElem() > 0;
}

bool KVSection::IsArray() const
{
	return values.numElem() > 1;
}

bool KVSection::IsDefinition() const
{
	return values.numElem() == 0;
}

// counters and accessors
int	KVSection::KeyCount() const
{
	return keys.numElem();
}

KVSection* KVSection::KeyAt(int idx) const
{
	return keys[idx];
}

int	KVSection::ValueCount() const
{
	return values.numElem();
}

KVPairValue* KVSection::ValueAt(int idx)  const
{
	return values[idx];
}

void KVSection::SetType(int newType)
{
	type = (EKVPairType)newType;
}

int	KVSection::GetType() const
{
	return type;
}

//---------------------------------------------------------------------------------------------------------
// KEYVALUES API Functions
//---------------------------------------------------------------------------------------------------------

KVSection* KV_ParseSectionV2(const char* pszBuffer, int bufferSize, const char* pszFileName, KVSection* pParseTo, int nStartLine = 0);
KVSection* KV_ParseSectionV3(const char* pszBuffer, int bufferSize, const char* pszFileName, KVSection* pParseTo, int nStartLine = 0);

KVSection* KV_ParseSection(const char* pszBuffer, int bufferSize, const char* pszFileName, KVSection* pParseTo, int nStartLine)
{
	if (bufferSize <= 0)
		bufferSize = strlen(pszBuffer);
	
	int version = 2;
	KVSection* result = nullptr;
	if(pszBuffer[0] == '$')
	{
		pszBuffer++;
		const char* lineStart = pszBuffer;
		while(!IsKVWhitespace(*pszBuffer))
		{
			pszBuffer++;
		}

		if(!strncmp("schema_kv3", lineStart, pszBuffer - lineStart))
		{
			version = 3;
		}
	}

	if(version == 3)
		result = KV_ParseSectionV3(pszBuffer, bufferSize, pszFileName, pParseTo, nStartLine);
	else
		result = KV_ParseSectionV2(pszBuffer, bufferSize, pszFileName, pParseTo, nStartLine);

	return result;
}

bool KV_Tokenizer(const char* buffer, int bufferSize, const char* fileName, const KVTokenFunc tokenFunc)
{
	enum EParserMode
	{
		MODE_DEFAULT = 0,
		MODE_SKIP_COMMENT_SINGLELINE,
		MODE_SKIP_COMMENT_MULTILINE,

		MODE_OPEN_STRING,
		MODE_QUOTED_STRING,

		MODE_PARSE_ERROR_BREAK
	};

	EParserMode mode = MODE_DEFAULT;
	int curLine = 1;
	int lastModeStartLine = 0;

	char* firstLetter = nullptr;

	int sectionDepth = 0;

	char* dataPtr = (char*)buffer;
	auto tokenFuncWrap = [&](const char* sig, ...) -> EKVTokenState {
		va_list args;
		va_start(args, sig);
		const EKVTokenState result = tokenFunc(curLine, dataPtr, sig, args);
		va_end(args);
		return result;
	};

	for (; (dataPtr - buffer) <= bufferSize; ++dataPtr)
	{
		const char c = *dataPtr;

		if(c == KV_STRING_NEWLINE)
			++curLine;

		if (c == 0)
			break;

		// check for beginning of the string
		switch (mode)
		{
			case MODE_DEFAULT:
			{
				EKVTokenState state = tokenFuncWrap("c");

				if (IsKVWhitespace(c))
					break;

				if (state == KV_PARSE_ERROR)
				{
					MsgError("'%s' (%d): unexpected '%c' while parsing\n", (fileName ? fileName : "buffer"), curLine, c);
					lastModeStartLine = curLine;
					mode = MODE_PARSE_ERROR_BREAK;
					break;
				}

				// while in skip mode we don't parse anything and let parser on top of it work
				if (state == KV_PARSE_SKIP)
					continue;

				if (c == KV_COMMENT_SYMBOL)
				{
					// we got comment symbol again
					if (*(dataPtr + 1) == KV_COMMENT_SYMBOL && mode != MODE_SKIP_COMMENT_MULTILINE)
					{
						mode = MODE_SKIP_COMMENT_SINGLELINE;
						lastModeStartLine = curLine;
						continue;
					}
					else if (*(dataPtr + 1) == KV_RANGECOMMENT_BEGIN_END && mode != MODE_SKIP_COMMENT_SINGLELINE)
					{
						mode = MODE_SKIP_COMMENT_MULTILINE;
						lastModeStartLine = curLine;
						++dataPtr; // also skip next char
						continue;
					}
					break;
				}
				else if (c == KV_SECTION_BEGIN)
				{
					++sectionDepth;
					tokenFuncWrap("s+", sectionDepth);
				}
				else if (c == KV_SECTION_END)
				{
					if (sectionDepth <= 0)
					{
						MsgError("'%s' (%d): unexpected '%c' while parsing\n", (fileName ? fileName : "buffer"), curLine, c);
						lastModeStartLine = curLine;
						mode = MODE_PARSE_ERROR_BREAK;
						break;
					}
					--sectionDepth;

					tokenFuncWrap("s-", sectionDepth);
				}
				else if (c == KV_STRING_BEGIN_END)
				{
					mode = MODE_QUOTED_STRING;
					lastModeStartLine = curLine;
					firstLetter = dataPtr + 1; // exclude quote
				}
				else if (c == KV_BREAK || c == 0)
				{
					tokenFuncWrap("b");
				}
				else if(!IsKVWhitespace(c))
				{
					mode = MODE_OPEN_STRING; // by default we always start from string
					lastModeStartLine = curLine;
					firstLetter = dataPtr;
				}
				break;
			}
			case MODE_SKIP_COMMENT_SINGLELINE:
			{
				if (c == KV_STRING_NEWLINE)
					mode = MODE_DEFAULT;
				break;
			}
			case MODE_SKIP_COMMENT_MULTILINE:
			{
				if (c == KV_RANGECOMMENT_BEGIN_END && *(dataPtr + 1) == KV_COMMENT_SYMBOL)
				{
					mode = MODE_DEFAULT;
					++dataPtr; // also skip next char
				}
				break;
			}
			case MODE_OPEN_STRING:
			{
				// trigger close on any symbol
				if (tokenFuncWrap("u") == KV_PARSE_BREAK_TOKEN || IsKVWhitespace(c) || c == KV_BREAK || c == KV_COMMENT_SYMBOL || c == KV_SECTION_BEGIN)
				{
					ASSERT(firstLetter);
					mode = MODE_DEFAULT;
				}

				if (mode != MODE_OPEN_STRING)
				{
					char tmp = *dataPtr;
					*dataPtr = 0;

					tokenFuncWrap("t", firstLetter);
					
					*dataPtr = tmp;
					--dataPtr; // open string - get back and parse character again
				}

				break;
			}
			case MODE_QUOTED_STRING:
			{
				// only trigger when on end
				if (c == KV_STRING_BEGIN_END && *(dataPtr - 1) != KV_ESCAPE_SYMBOL)
				{
					ASSERT(firstLetter);
					mode = MODE_DEFAULT;
				}

				if (mode != MODE_QUOTED_STRING)
				{
					const int stringLength = (int)(dataPtr - firstLetter);

					char* processedStringBuf = (char*)stackalloc(stringLength + 1);
					if (stringLength > 0)
						KV_ReadProcessString(firstLetter, processedStringBuf, stringLength);
					else
						*processedStringBuf = 0;

					tokenFuncWrap("t", processedStringBuf);
				}
				break;
			}
		}

		if (mode == MODE_PARSE_ERROR_BREAK)
			break;
	}

	switch (mode)
	{
		case MODE_SKIP_COMMENT_MULTILINE:
			MsgError("'%s' (%d): EOF unexpected (missing '*/')\n", (fileName ? fileName : "buffer"), lastModeStartLine);
			break;
		case MODE_QUOTED_STRING:
			MsgError("'%s' (%d): EOF unexpected (missing '\"')\n", (fileName ? fileName : "buffer"), lastModeStartLine);
	}

	return (mode == MODE_DEFAULT);
}

//
// Parses the KeyValues section string buffer to the 'pParseTo'
//
KVSection* KV_ParseSectionV2(const char* pszBuffer, int bufferSize, const char* pszFileName, KVSection* pParseTo, int nStartLine)
{
	FixedArray<KVSection*, 32> sectionStack;

	// add root section to the stack
	{
		KVSection* rootSection = pParseTo;

		if (!rootSection)
			rootSection = PPNew KVSection;

		sectionStack.append(rootSection);
	}

	KVSection* currentSection = nullptr;

	KV_Tokenizer(pszBuffer, bufferSize, pszFileName, [&](int line, const char* dataPtr, const char* sig, va_list args) {
		switch (*sig)
		{
			case 'c':
			{
				// character filtering

				// not supporting arrays on KV2
				if (*dataPtr == KV_ARRAY_SEPARATOR || *dataPtr == KV_ARRAY_BEGIN || *dataPtr == KV_ARRAY_END)
					return KV_PARSE_ERROR;

				break;
			}
			case 's':
			{
				// increase/decrease section depth
				if (*(sig + 1) == '+')
				{
					sectionStack.append(currentSection);
					currentSection = nullptr;
				}
				else if (*(sig + 1) == '-')
				{
					sectionStack.popBack();
					currentSection = nullptr;
				}

				break;
			}
			case 't':
			{
				// text token
				const char* text = va_arg(args, const char*);

				if (!currentSection)
				{
					currentSection = sectionStack.back()->CreateSection(text);
					currentSection->line = line;
				}
				else
					currentSection->AddValue(text);

				break;
			}
			case 'b':
			{
				// break
				currentSection = nullptr;
				break;
			}
		}
		return KV_PARSE_RESUME;
	});

	ASSERT(sectionStack.numElem() == 1);
	return sectionStack.front();
}

//
// Parses the V3 format of KeyValues into pParseTo
//
KVSection* KV_ParseSectionV3( const char* pszBuffer, int bufferSize, const char* pszFileName, KVSection* pParseTo, int nStartLine )
{
	FixedArray<KVSection*, 32> sectionStack;

	// add root section to the stack
	{
		KVSection* rootSection = pParseTo;

		if (!rootSection)
			rootSection = PPNew KVSection;

		sectionStack.append(rootSection);
	}

	KVSection* currentSection = nullptr;

	EqString prevToken;
	enum KV3ParserMode
	{
		MODE_DEFAULT = 0,

		MODE_MULTILINE_STRING_QUERY,
		MODE_MULTILINE_STRING,

		MODE_VALUE_TYPE,
	};

	int arrayDepth = 0;

	KV3ParserMode mode = MODE_DEFAULT;
	const char* multiLineStringStart = nullptr;

	KV_Tokenizer(pszBuffer, bufferSize, pszFileName, [&](int line, const char* dataPtr, const char* sig, va_list args) {
		switch (*sig)
		{
			case 'c':
			{
				// character filtering
				const char c = *dataPtr;

				// not supporting arrays on KV2
				if (mode == MODE_DEFAULT)
				{
					// start parsing multi-line string
					if (c == '%')
					{
						// we use query first because we have to read key name
						mode = MODE_MULTILINE_STRING_QUERY;
						return KV_PARSE_SKIP;
					}

					if (c == KV_TYPE_VALUESYMBOL)
						return KV_PARSE_SKIP;

					if (c == KV_ARRAY_BEGIN)
					{
						++arrayDepth;
						return KV_PARSE_SKIP;
					}

					if(arrayDepth == 0 && (c == KV_ARRAY_SEPARATOR || c == KV_ARRAY_END))
						return KV_PARSE_ERROR;
				}
				else if (mode == MODE_MULTILINE_STRING_QUERY)
				{
					if (c == KV_SECTION_BEGIN)
					{
						mode = MODE_MULTILINE_STRING;
						multiLineStringStart = dataPtr;
						return KV_PARSE_SKIP;
					}
				}
				else if (mode == MODE_MULTILINE_STRING)
				{
					if (c == KV_SECTION_END)
					{
						const int stringLength = dataPtr - multiLineStringStart;

						// copy the value
						KVPairValue* newValue = currentSection->CreateValue();
						newValue->SetStringValue(multiLineStringStart + 1, stringLength - 1);

						multiLineStringStart = nullptr;
						mode = MODE_DEFAULT;
					}

					return KV_PARSE_SKIP;
				}

				if (arrayDepth > 0)
				{
					if (c == KV_ARRAY_SEPARATOR)
					{
						// next item
						return KV_PARSE_SKIP;
					}

					if (c == KV_ARRAY_END)
					{
						--arrayDepth;
						return KV_PARSE_SKIP;
					}
				}

				break;
			}
			case 'u':
			{
				const char c = *dataPtr;

				if (mode == MODE_DEFAULT)
				{
					if (c == KV_TYPE_VALUESYMBOL)
					{
						mode = MODE_VALUE_TYPE;
						return KV_PARSE_BREAK_TOKEN;
					}
				}

				if (arrayDepth > 0)
				{
					if (c == KV_ARRAY_BEGIN)
						return KV_PARSE_ERROR;

					if (c == KV_SECTION_END || c == KV_ARRAY_SEPARATOR || c == KV_ARRAY_END)
						return KV_PARSE_BREAK_TOKEN;
				}

				break;
			}
			case 's':
			{
				// increase/decrease section depth
				if (*(sig + 1) == '+')
				{
					ASSERT(currentSection);

					if(arrayDepth > 0 && currentSection)
					{
						currentSection->type = KVPAIR_SECTION;
						sectionStack.append(currentSection->CreateSectionValue());
					}
					else
						sectionStack.append(currentSection);

					currentSection = nullptr;

					//ASSERT(arrayDepth == 0);
				}
				else if (*(sig + 1) == '-')
				{
					if (arrayDepth == 0)
					{
						sectionStack.popBack();
						currentSection = nullptr;
					}
					else
						currentSection = sectionStack.popBack(); 
				}

				break;
			}
			case 't':
			{
				// text token
				const char* text = va_arg(args, const char*);

				if (arrayDepth == 0 && currentSection && currentSection->ValueCount() > 0)
					currentSection = nullptr;

				if (!currentSection)
				{
					currentSection = sectionStack.back()->CreateSection(text);
					currentSection->line = line;
				}
				else
				{
					if (mode == MODE_VALUE_TYPE)
					{
						currentSection->type = KV_ResolvePairType(text);
						mode = MODE_DEFAULT;
						return KV_PARSE_SKIP;
					}
					else
					{
						KVPairValue* value = currentSection->CreateValue();
						value->SetFromString(text);
					}
				}

				break;
			}
			case 'b':
			{
				// break
				currentSection = nullptr;
				break;
			}
		}
		return KV_PARSE_RESUME;
	});

	ASSERT(sectionStack.numElem() == 1);
	return sectionStack.front();
}

//
// Loads file and parses it as KeyValues into the 'pParseTo'
//
KVSection* KV_LoadFromFile( const char* pszFileName, int nSearchFlags, KVSection* pParseTo )
{
	long lSize = 0;
	char* pBuffer = (char*)g_fileSystem->GetFileBuffer(pszFileName, &lSize, nSearchFlags);
	char* _buffer = pBuffer;

	if(!_buffer)
	{
		DevMsg(1, "Can't open key-values file '%s'\n", pszFileName);
		return nullptr;
	}

	ushort byteordermark = *((ushort*)_buffer);

	bool isUTF8 = false;
	bool isBinary = false;

	if(byteordermark == 0xbbef)
	{
		// skip this three byte bom
		_buffer += 3;
		lSize -= 3;
		isUTF8 = true;
	}
	else if(byteordermark == 0xfeff)
	{
		ASSERT(!"Only UTF-8 keyvalues supported!!!");
		PPFree( pBuffer );
		return nullptr;
	}
	else
	{
		uint ident = *((uint*)_buffer);

		// it's assuming UTF8 when using binary format
		if(ident == KV_IDENT_BINARY)
		{
			isBinary = true;
			isUTF8 = true;
		}
	}

	// load as stream
	KVSection* pBase = nullptr;
	
	if(isBinary)
		pBase = KV_ParseBinary(_buffer, lSize, pParseTo);
	else
		pBase = KV_ParseSection(_buffer, lSize, pszFileName, pParseTo, 0);

	if(pBase)
	{
		pBase->SetName(_Es(pszFileName).Path_Strip_Path().ToCString());
        pBase->unicode = isUTF8;
	}

	// required to clean memory after reading
	PPFree( pBuffer );

	return pBase;
}


//-----------------------------------------------------------------------------------------------------
// BINARY

struct kvbinvalue_s
{
	int type;	// EKVPairType

	union
	{
		int		nValue;		// treat as string length if KVPAIR_STRING
		bool	bValue;
		float	fValue;
	};

	// string is written next after the value struct
};

ALIGNED_TYPE(kvbinvalue_s,4) kvbinvalue_t;

struct kvbinbase_s
{
	int		ident;	// it must be identified by KV_IDENT_BINARY

	int		type;	// EKVPairType
	int		keyCount;

	ushort	valueCount;
	ushort	nameLen;

	// next after section header:
	// - name
	// - values
	// - nested key bases
};

KVSection* KV_ParseBinary(const char* pszBuffer, int bufferSize, KVSection* pParseTo)
{
	CMemoryStream memstr((ubyte*)pszBuffer, VS_OPEN_READ, bufferSize);
	return KV_ReadBinaryBase(&memstr, pParseTo);
}

void KV_ReadBinaryValue(IVirtualStream* stream, KVSection* addTo)
{
	kvbinvalue_t binValue;
	stream->Read(&binValue, 1, sizeof(binValue));

	if(binValue.type == KVPAIR_STRING)
	{
		 // int value as string length
		char* strVal = (char*)PPAlloc(binValue.nValue+1);

		stream->Read(strVal, 1, binValue.nValue);
		strVal[binValue.nValue] = '\0';

		addTo->AddValue(strVal);
	}
	else if(binValue.type == KVPAIR_INT)
	{
		addTo->AddValue(binValue.nValue);
	}
	else if(binValue.type == KVPAIR_FLOAT)
	{
		addTo->AddValue(binValue.fValue);
	}
	else if(binValue.type == KVPAIR_BOOL)
	{
		addTo->AddValue(binValue.bValue);
	}
	else if(binValue.type == KVPAIR_SECTION)
	{
		KVSection* parsed = KV_ReadBinaryBase(stream, nullptr);

		if(parsed)
			addTo->AddValue(parsed);
	}
}

// reads binary keybase
KVSection* KV_ReadBinaryBase(IVirtualStream* stream, KVSection* pParseTo)
{
	kvbinbase_s binBase;
	stream->Read(&binBase, 1, sizeof(binBase));

	if(binBase.ident != KV_IDENT_BINARY)
	{
		MsgError("KV_ReadBinaryBase - invalid header\n");
		return nullptr;
	}

	if(!pParseTo)
		pParseTo = PPNew KVSection();

	// read name after keybase header
	char* nameTemp = (char*)stackalloc(binBase.nameLen+1);
	stream->Read(nameTemp, 1, binBase.nameLen);
	nameTemp[binBase.nameLen] = '\0';

	pParseTo->SetName(nameTemp);
	pParseTo->type = (EKVPairType)binBase.type;

	pParseTo->unicode = true; // always as unicode UTF8
	
	// read values if we have any
	for(int i = 0; i < binBase.valueCount; i++)
	{
		KV_ReadBinaryValue(stream, pParseTo);
	}

	// read nested keybases as well
	for(int i = 0; i < binBase.keyCount; i++)
	{
		KVSection* parsed = KV_ReadBinaryBase(stream, nullptr);
		pParseTo->AddSection(parsed);
	}

	return pParseTo;
}

void KV_WriteToStreamBinary(IVirtualStream* outStream, const KVSection* base);

// writes KV value to the binary stream
void KV_WriteValueBinary(IVirtualStream* outStream, const KVPairValue* value)
{
	kvbinvalue_t binValue;
	binValue.type = value->type;

	if(binValue.type == KVPAIR_STRING)
	{
		binValue.nValue = strlen(value->value); // store string length
	}
	else if(binValue.type == KVPAIR_INT)
	{
		binValue.nValue = value->nValue;
	}
	else if(binValue.type == KVPAIR_FLOAT)
	{
		binValue.fValue = value->fValue;
	}
	else if(binValue.type == KVPAIR_BOOL)
	{
		binValue.bValue = value->bValue;
	}

	// store header
	outStream->Write(&binValue, 1, sizeof(binValue));
	
	if(binValue.type == KVPAIR_STRING)
	{
		// store the string
		outStream->Write(value->value, 1, binValue.nValue);
	}
	else if(binValue.type == KVPAIR_SECTION)
	{
		// store section after the binary
		KV_WriteToStreamBinary(outStream, value->section);
	}
}

// writes keybase to the binary stream
void KV_WriteToStreamBinary(IVirtualStream* outStream, const KVSection* base)
{
	kvbinbase_s binBase;
	memset(&binBase, 0, sizeof(binBase));

	binBase.ident = KV_IDENT_BINARY;
	binBase.keyCount = base->keys.numElem();
	binBase.valueCount = base->values.numElem();
	binBase.type = base->type;
	binBase.nameLen = strlen(base->name);

	// write header to the stream
	outStream->Write(&binBase, 1, sizeof(binBase));

	// write base name
	outStream->Write(base->name, 1, binBase.nameLen);

	// write each value of key base
	for(int i = 0; i < binBase.valueCount; i++)
	{
		KV_WriteValueBinary(outStream, base->values[i]);
	}

	// then write each subkey of this key recursively... pretty simple, huh?
	for(int i = 0; i < binBase.keyCount; i++)
	{
		KV_WriteToStreamBinary(outStream, base->keys[i]);
	}
}

//----------------------------------------------------------------------------------------------

//
// Detects the need for string quoting (very necessary for keys)
//
bool UTIL_StringNeedsQuotes( const char* pszString )
{
	char const* pLetter = pszString;

	int length = 0;

	// Check the entire string
	while (*pLetter != 0)
	{
		if(IsKVWhitespace(*pLetter))
			return true;

		switch (*pLetter)
		{
			case KV_SECTION_BEGIN:
			case KV_SECTION_END:

			case KV_ARRAY_BEGIN:
			case KV_ARRAY_END:
			case KV_ARRAY_SEPARATOR:
			case KV_TYPE_VALUESYMBOL:

			case KV_COMMENT_SYMBOL:
			//case KV_ESCAPE_SYMBOL:
			case KV_BREAK:
				return true;
			default:
				break;
		}

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

//
// If string does need quotes it will be written with them
//
static void KV_WriteSelectQuotedString(IVirtualStream* out, const char* pszString)
{
	if( UTIL_StringNeedsQuotes( pszString ) )
		out->Print("\"%s\"", pszString);
	else
		out->Print("%s", pszString);
}

//
// counts the special characters
// used for KV_PreProcessStringValue to detect extra length of buffer
//
static int KV_CountSpecialSymbols(const char* pszStr)
{
	const char* ptr = pszStr;

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

//
// converts some symbols to special ones
//
static void KV_PreProcessStringValue( char* out, const char* pszStr )
{
	const char* ptr = pszStr;
	char* temp = out;

	do
	{
		if(*ptr == '"')
		{
			*temp++ = '\\';
			*temp++ = *ptr;
		}
		else if(*ptr == '\n')
		{
			*temp++ = '\\';
			*temp++ = 'n';
		}
		else if(*ptr == '\t')
		{
			*temp++ = '\\';
			*temp++ = 't';
		}
		else
			*temp++ = *ptr;

	}while(*ptr++);

	// add null
	*temp++ = 0;
}

//-----------------------------------------------------------------------------------------------------

void KV_WriteToStreamV3(IVirtualStream* outStream, const KVSection* section, int nTabs, bool pretty);

//
// Writes the pair value
//
void KV_WritePairValue(IVirtualStream* out, const KVPairValue* val, int depth)
{
	// write typed data
	if(val->type == KVPAIR_STRING)
	{
		if(val->value == nullptr)
		{
			out->Print("\"%s\"", "VALUE_MISSING");
			return;
		}

		int numSpecial = KV_CountSpecialSymbols(val->value);

		char* outValueString = (char*)PPAlloc(strlen(val->value) + numSpecial + 1);
		KV_PreProcessStringValue( outValueString, val->value );
		KV_WriteSelectQuotedString( out, outValueString );
		//out->Print("\"%s\"", outValueString);

		PPFree( outValueString );
	}
	else if(val->type == KVPAIR_INT)
	{
		out->Print("%d", val->nValue);
	}
	else if(val->type == KVPAIR_FLOAT)
	{
		out->Print("%g", val->fValue);
	}
	else if(val->type == KVPAIR_BOOL)
	{
		out->Print("%d", val->bValue);
	}
	else if(val->type == KVPAIR_SECTION)
	{
		out->Print("%c", KV_SECTION_BEGIN);
		KV_WriteToStreamV3(out, val->section, depth+1, false);
		out->Print("%c", KV_SECTION_END);
	}
}

//
// Writes the pairbase recursively to the virtual stream
//
void KV_WriteToStream(IVirtualStream* outStream, const KVSection* section, int nTabs, bool pretty)
{
	char* tabs = (char*)stackalloc(nTabs);
	memset(tabs, 0, nTabs);

	int nTab = 0;

	if(pretty)
	{
		while(nTab < nTabs)
			tabs[nTab++] = '\t';
	}

	for(int i = 0; i < section->keys.numElem(); i++)
	{
		KVSection* pKey = section->keys[i];

		// add tabs
		if(pretty)
			outStream->Write(tabs, 1, nTabs);

		bool hasWroteValue = false;

		//
		// write key/value
		//
		{
			KV_WriteSelectQuotedString(outStream, pKey->name);

			if(pKey->type != KVPAIR_SECTION)
			{
				for(int j = 0; j < pKey->values.numElem(); j++)
				{
					outStream->Print(" ");
					KV_WritePairValue(outStream, pKey->values[j], nTabs);
				}
				hasWroteValue = true;
			}
			else
				outStream->Print("; // section arrays not supported in V2\n");
		}

		//
		// write subsection
		//
		if( pKey->keys.numElem() )
		{
			if(pretty)
			{
				outStream->Print("\n");
				outStream->Write(tabs, 1, nTabs);
			}
			outStream->Print("%c\n", KV_SECTION_BEGIN);

			KV_WriteToStream(outStream, pKey, nTabs + 1, pretty);

			if(pretty)
				outStream->Write(tabs, 1, nTabs);

			outStream->Print("%c", KV_SECTION_END);
		}

		if(hasWroteValue)
			outStream->Print("%c\n", KV_BREAK);
	}
}

//
// Writes the pairbase values
//
void KV_WriteValueV3( IVirtualStream* outStream, const KVSection* key, int nTabs)
{
	int numValues = key->values.numElem();
	bool isValueArray = numValues > 1;

	if(key->type != KVPAIR_STRING && key->type != KVPAIR_SECTION)
		outStream->Print(" %s%c ", s_szkKVValueTypes[key->type], KV_TYPE_VALUESYMBOL);
	else
		outStream->Print(" ");

	if(isValueArray)
		outStream->Print("%c ", KV_ARRAY_BEGIN);

	for(int j = 0; j < numValues; j++)
	{
		KV_WritePairValue(outStream, key->values[j], nTabs);

		// while array sepearator is not required, always add it
		if(j < numValues-1)
			outStream->Print("%c ", KV_ARRAY_SEPARATOR);
	}

	if(isValueArray)
		outStream->Print(" %c", KV_ARRAY_END);
}

//
// Writes the pairbase recursively to the virtual stream
//
void KV_WriteToStreamV3(IVirtualStream* outStream, const KVSection* section, int nTabs, bool pretty)
{
	char* tabs = (char*)stackalloc(nTabs);
	memset(tabs, 0, nTabs);

	int nTab = 0;

	if(pretty)
	{
		while(nTab < nTabs)
			tabs[nTab++] = '\t';
	}

	int numKeys = section->keys.numElem();

	for(int i = 0; i < numKeys; i++)
	{
		KVSection* pKey = section->keys[i];

		// add tabs
		if(pretty)
			outStream->Write(tabs, 1, nTabs);

		//
		// write key/value
		//
		{
			KV_WriteSelectQuotedString(outStream, pKey->name);
			KV_WriteValueV3(outStream, pKey, nTabs);
		}

		//
		// write subsection
		//
		if( pKey->IsSection() )
		{
			if(pretty)
			{
				outStream->Print("\n");
				outStream->Write(tabs, 1, nTabs);
			}

			outStream->Print("%c\n", KV_SECTION_BEGIN);

			KV_WriteToStreamV3(outStream, pKey, nTabs + 1, pretty);

			if(pretty)
				outStream->Write(tabs, 1, nTabs);

			outStream->Print("%c", KV_SECTION_END);
		}

		if(pretty)
			outStream->Print("\n");
		else if(i < numKeys-1)
			outStream->Print(" ");
	}
}

//
// Prints the pairbase to console
//
void KV_PrintSection(const KVSection* base)
{
	CMemoryStream stream(nullptr, VS_OPEN_WRITE, 2048);
	KV_WriteToStream(&stream, base, 0, true);

	char nullChar = '\0';
	stream.Write(&nullChar, 1, 1);

	Msg( "%s\n", stream.GetBasePointer() );
}

//-----------------------------------------------------------------------------------------------------
// KeyValues value helpers
//-----------------------------------------------------------------------------------------------------

static char* skip_spaces(const char* str)
{
	while (isspace(*str)) {
		++str;
	}
	return (char*)str;
}

//
// sscanf-like value getter from pairbase
//
int KV_ScanGetValue(const KVSection* pBase, int start, const char* format, ...)
{
	int ret = 0;

	va_list args;
	va_start(args, format);
	while (format[0] != '\0') 
	{
		switch (format[1])
		{
			case 'd':
			case 'i':
			case 'u':
			{
				int* intp = va_arg(args, int*);
				*intp = KV_GetValueInt(pBase, start + ret, 0);
				ret++;
				break;
			}
			case 'f':
			{
				float* flp = va_arg(args, float*);
				*flp = KV_GetValueFloat(pBase, start + ret, 0.0f);
				ret++;
				break;
			}
			case 'b':
			{
				bool* boolp = va_arg(args, bool*);
				*boolp = KV_GetValueBool(pBase, start + ret, false);
				ret++;
				break;
			}
			case 's':
			{
				char* charp = va_arg(args, char*);
				strcpy(charp, KV_GetValueString(pBase, start + ret, ""));
				ret++;
				break;
			}
		}
		++format;
	}

	va_end(args);

	return ret;
}

//
// Returns the string value of pairbase
//
const char* KV_GetValueString(const KVSection* pBase, int nIndex, const char* pszDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return pszDefault;

	return (*pBase)[nIndex]->GetString();
}


//
// Returns integer value
// Converts the best precise value if type differs
//
int	KV_GetValueInt(const KVSection* pBase, int nIndex, int nDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return nDefault;

	return (*pBase)[nIndex]->GetInt();
}

//
// Returns float value
// Converts the best precise value if type differs
//
float KV_GetValueFloat(const KVSection* pBase, int nIndex, float fDefault )
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return fDefault;

	return (*pBase)[nIndex]->GetFloat();
}

//
// Returns boolean value
// Converts the best precise value if type differs
//
bool KV_GetValueBool(const KVSection* pBase, int nIndex, bool bDefault)
{
	if(!pBase || pBase && !pBase->values.inRange(nIndex))
		return bDefault;

	return (*pBase)[nIndex]->GetBool();
}

//
// Returns Vector2D value
//
Vector2D KV_GetVector2D(const KVSection* pBase, int nIndex, const Vector2D& vDefault)
{
	Vector2D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);

	return retVal;
}

//
// Returns IVector2D value
//
IVector2D KV_GetIVector2D(const KVSection* pBase, int nIndex, const IVector2D& vDefault)
{
	IVector2D retVal = vDefault;
	retVal.x = KV_GetValueInt(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueInt(pBase, nIndex+1, vDefault.y);

	return retVal;
}

//
// Returns Vector3D value
//
Vector3D KV_GetVector3D(const KVSection* pBase, int nIndex, const Vector3D& vDefault)
{
	Vector3D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);
	retVal.z = KV_GetValueFloat(pBase, nIndex+2, vDefault.z);

	return retVal;
}

//
// Returns Vector4D value
//
Vector4D KV_GetVector4D(const KVSection* pBase, int nIndex, const Vector4D& vDefault)
{
	Vector4D retVal = vDefault;
	retVal.x = KV_GetValueFloat(pBase, nIndex, vDefault.x);
	retVal.y = KV_GetValueFloat(pBase, nIndex+1, vDefault.y);
	retVal.z = KV_GetValueFloat(pBase, nIndex+2, vDefault.z);
	retVal.w = KV_GetValueFloat(pBase, nIndex+3, vDefault.w);

	return retVal;
}
