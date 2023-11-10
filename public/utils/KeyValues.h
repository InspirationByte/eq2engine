//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IVirtualStream;

/*

Example of key-values file contents:

key;
key2 value;
key3 value1 value2;			// value array
key4 "string with spaces";	// string including special characters like \n \r \"

key_section
{
	nested value value2
	{
		key value;
		// ...
	};
};

*/

//
enum EKVPairType
{
	KVPAIR_STRING = 0,	// default
	KVPAIR_INT,
	KVPAIR_FLOAT,
	KVPAIR_BOOL,

	KVPAIR_SECTION,		// sections

	KVPAIR_TYPES,
};

// tune this (depends on size of used memory)
#define KV_MAX_NAME_LENGTH		128

enum EKVSearchFlags
{
	KV_FLAG_SECTION = (1 << 0),
	KV_FLAG_NOVALUE = (1 << 1),
	KV_FLAG_ARRAY	= (1 << 2)
};

//
// KeyValues typed value holder
//
struct KVPairValue
{
	KVPairValue()
	{
		value = nullptr;
		section = nullptr;
		type = KVPAIR_STRING;
	}

	~KVPairValue();

	EKVPairType	type;

	union
	{
		int		nValue;
		bool	bValue;
		float	fValue;
	};

	struct KVSection*	section;

	char*				value;

	// sets string value
	void				SetStringValue(const char* pszValue, int len = -1);
	void				SetFromString(const char* pszValue);
	void				SetFrom(KVPairValue* from);

	// get/set
	const char*			GetString() const;
	int					GetInt() const;
	float				GetFloat() const;
	bool				GetBool() const;

	void				SetString(const char* value);
	void				SetInt(int nValue);
	void				SetFloat(float fValue);
	void				SetBool(bool bValue);
};

//
// key values base
//
struct KVSection
{
	KVSection();
	~KVSection();

	void				Cleanup();
	void				ClearValues();

	// sets section name
	void				SetName(const char* pszName);
	const char*			GetName() const;

	//----------------------------------------------
	// The section functions
	//----------------------------------------------

	// searches for section
	KVSection*			FindSection(const char* pszName, int nFlags = 0) const;

	// adds new section
	KVSection*			CreateSection(const char* pszName, const char* pszValue = nullptr, EKVPairType pairType = KVPAIR_STRING);

	// adds existing section. You should set it's name manually. It should not be allocated by other section
	void				AddSection(KVSection* keyBase);

	// removes key base by name
	void				RemoveSectionByName( const char* name, bool removeAll = false );

	// removes key base
	void				RemoveSection(KVSection* base);

	//-----------------------------------------------------


	KVSection&			SetKey(const char* name, const char* value);
	KVSection&			SetKey(const char* name, int nValue);
	KVSection&			SetKey(const char* name, float fValue);
	KVSection&			SetKey(const char* name, bool bValue);
	KVSection&			SetKey(const char* name, const Vector2D& value);
	KVSection&			SetKey(const char* name, const Vector3D& value);
	KVSection&			SetKey(const char* name, const Vector4D& value);
	KVSection&			SetKey(const char* name, KVSection* value);

	KVSection&			AddKey(const char* name, const char* value);
	KVSection&			AddKey(const char* name, int nValue);
	KVSection&			AddKey(const char* name, float fValue);
	KVSection&			AddKey(const char* name, bool bValue);
	KVSection&			AddKey(const char* name, const Vector2D& value);
	KVSection&			AddKey(const char* name, const Vector3D& value);
	KVSection&			AddKey(const char* name, const Vector4D& value);
	KVSection&			AddKey(const char* name, KVSection* value);

	//----------------------------------------------
	// The self-key functions
	//----------------------------------------------

	void				SetValueFrom( KVSection* pOther );

	KVPairValue*		CreateValue();
	KVSection*			CreateSectionValue();

	KVSection*			Clone() const;
	void				CopyTo(KVSection* dest) const;
	void				CopyValuesTo(KVSection* dest) const;

	// adds value to key
	void				AddValue(const char* value);
	void				AddValue(int nValue);
	void				AddValue(float fValue);
	void				AddValue(bool bValue);
	void				AddValue(const Vector2D& vecValue);
	void				AddValue(const Vector3D& vecValue);
	void				AddValue(const Vector4D& vecValue);
	void				AddValue(KVSection* keybase);
	void				AddValue(KVPairValue* value);

	// adds unique value to key
	void				AddUniqueValue(const char* value);
	void				AddUniqueValue(int nValue);
	void				AddUniqueValue(float fValue);
	void				AddUniqueValue(bool bValue);

	// sets value
	void				SetValue(const char* value, int idxAt = 0);
	void				SetValue(int nValue, int idxAt = 0);
	void				SetValue(float fValue, int idxAt = 0);
	void				SetValue(bool bValue, int idxAt = 0);
	void				SetValue(const Vector2D& value, int idxAt = 0);
	void				SetValue(const Vector3D& value, int idxAt = 0);
	void				SetValue(const Vector4D& value, int idxAt = 0);
	void				SetValue(KVPairValue* value, int idxAt = 0);

	KVSection*			operator[](const char* pszName);
	KVPairValue*		operator[](int index);

	const KVSection*	operator[](const char* pszName) const;
	const KVPairValue*	operator[](int index) const;


	//----------------------------------------------

	// copy all values recursively
	void				MergeFrom(const KVSection* base, bool recursive);

	// checkers
	bool				IsSection() const;
	bool				IsArray() const;
	bool				IsDefinition() const;

	int					KeyCount() const;
	KVSection*			KeyAt(int idx) const;

	int					ValueCount() const;
	KVPairValue*		ValueAt(int idx) const;

	void				SetType(int newType);
	int					GetType() const;

	// the line that the key is on
	int					line;

	char				name[KV_MAX_NAME_LENGTH];
	int					nameHash;

	Array<KVPairValue*>	values{ PP_SL };
	EKVPairType			type;		// default type of values

	// the nested keys
	Array<KVSection*>	keys{ PP_SL };
	bool				unicode;
};

// easy key iteration
struct KVKeyIterator
{
	KVKeyIterator(const KVSection* section, const char* nameFilter = nullptr, int searchFlags = 0);

	operator	int() const;
	operator	const char* () const;

	KVSection*	operator*() const;
	void		operator++();

	bool		atEnd() const;

	void		Rewind();

private:
	bool		IsValidItem();

	const KVSection*	section{ nullptr };
	int					nameHashFilter;
	int					index{ 0 };
	int					searchFlags{ 0 };
};

template<typename T>
struct KVPairValuesGetter;

template<typename T = const char*>
struct KVValueIterator
{
	KVValueIterator(const KVSection* section)
		: section(section)
	{
	}

	operator		int() const { return index; }
	operator		T() const { return KVPairValuesGetter<T>::Get(section, index);  }

	KVPairValue*	operator*() const { return section ? section->values[index] : nullptr; }
	void			operator++() { index += KVPairValuesGetter<T>::vcount; }

	bool			atEnd() const { return section ? index >= section->values.numElem() : true; }

	void			Rewind() { index = 0; }
private:
	const KVSection*	section{ nullptr };
	int					index{ 0 };
};

// special wrapper class
// for better compatiblity of new class
class KeyValues
{
public:
	KeyValues();
	~KeyValues();

	void			Reset();

	// searches for keybase
	KVSection*		FindSection(const char* pszName, int nFlags = 0) const;

	// loads from file
	bool			LoadFromFile(const char* pszFileName, int nSearchFlags = -1);
	bool			LoadFromStream(IVirtualStream* stream);

	bool			SaveToFile(const char* pszFileName, int nSearchFlags = -1);

	KVSection*		GetRootSection();

	KVSection*		operator[](const char* pszName);

private:
	KVSection	m_root;
};

//---------------------------------------------------------------------------------------------------------
// KEYVALUES API Functions
//---------------------------------------------------------------------------------------------------------

/*
	KV parse token function callback
		signature might be:
			c  <string>	- default mode character parsed
			s+ <int>	- section depth increase
			s- <int>	- section depth decrease
			b			- break
			t  <string>	- text token
			u			- unquoted text token character
*/

enum EKVTokenState
{
	KV_PARSE_ERROR = -1,
	KV_PARSE_RESUME = 0,
	KV_PARSE_SKIP,

	KV_PARSE_BREAK_TOKEN,	// for unquoted strings
};

using KVTokenFunc = EqFunction<EKVTokenState(int line, const char* curPtr, const char* sig, va_list& arg)>;

bool			KV_Tokenizer(const char* buffer, int bufferSize, const char* fileName, const KVTokenFunc tokenFunc);
KVSection*		KV_LoadFromStream(IVirtualStream* stream, KVSection* pParseTo = nullptr);
KVSection*		KV_LoadFromFile( const char* pszFileName, int nSearchFlags = -1, KVSection* pParseTo = nullptr);

KVSection*		KV_ParseSection(const char* pszBuffer, int bufferSize, const char* pszFileName = nullptr, KVSection* pParseTo = nullptr, int nStartLine = 0);
KVSection*		KV_ReadBinaryBase(IVirtualStream* stream, KVSection* pParseTo = nullptr);
KVSection*		KV_ParseBinary(const char* pszBuffer, int bufferSize, KVSection* pParseTo = nullptr);

void			KV_PrintSection(const KVSection* base);

void			KV_WriteToStream(IVirtualStream* outStream, const KVSection* section, int nTabs = 0, bool pretty = true);
void			KV_WriteToStreamV3(IVirtualStream* outStream, const KVSection* section, int nTabs = 0, bool pretty = true);

void			KV_WriteToStreamBinary(IVirtualStream* outStream, const KVSection* base);

//-----------------------------------------------------------------------------------------------------
// KeyValues value helpers
//-----------------------------------------------------------------------------------------------------

int					KV_ScanGetValue(const KVSection* pBase, int start, const char* format, ...);

const char*			KV_GetValueString(const KVSection* pBase, int nIndex = 0, const char* pszDefault = "" );
int					KV_GetValueInt(const KVSection* pBase, int nIndex = 0, int nDefault = 0 );
float				KV_GetValueFloat(const KVSection* pBase, int nIndex = 0, float fDefault = 0.0f );
bool				KV_GetValueBool(const KVSection* pBase, int nIndex = 0, bool bDefault = false );
Vector2D			KV_GetVector2D(const KVSection* pBase, int nIndex = 0, const Vector2D& vDefault = vec2_zero);
IVector2D			KV_GetIVector2D(const KVSection* pBase, int nIndex = 0, const IVector2D& vDefault = 0);
Vector3D			KV_GetVector3D(const KVSection* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero);
Vector4D			KV_GetVector4D(const KVSection* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero);

// For KV Value iterator

template<> struct KVPairValuesGetter<const char*>
{
	static const char* Get(const KVSection* section, int index) { return KV_GetValueString(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<EqString>
{
	static const char* Get(const KVSection* section, int index) { return KV_GetValueString(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<float>
{
	static float Get(const KVSection* section, int index) { return KV_GetValueFloat(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<int>
{
	static int Get(const KVSection* section, int index) { return KV_GetValueInt(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<bool>
{
	static bool Get(const KVSection* section, int index) { return KV_GetValueBool(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<Vector2D>
{
	static Vector2D Get(const KVSection* section, int index) { return KV_GetVector2D(section, index); }
	static const int vcount = 2;
};

template<> struct KVPairValuesGetter<IVector2D>
{
	static IVector2D Get(const KVSection* section, int index) { return KV_GetIVector2D(section, index); }
	static const int vcount = 2;
};

template<> struct KVPairValuesGetter<Vector3D>
{
	static Vector3D Get(const KVSection* section, int index) { return KV_GetVector3D(section, index); }
	static const int vcount = 3;
};

template<> struct KVPairValuesGetter<Vector4D>
{
	static Vector4D Get(const KVSection* section, int index) { return KV_GetVector4D(section, index); }
	static const int vcount = 4;
};

