//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IVirtualStream;
struct KVSection;
struct KVPairValue;

template<typename T>
struct KVPairValuesGetter;

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

// easy key iteration
struct KVKeyIterator
{
	struct Init;

	KVKeyIterator() = default;
	KVKeyIterator(const KVSection* section, const char* nameFilter = nullptr, int searchFlags = 0, int index = 0);

	operator	int() const;
	operator	const char* () const;

	operator	KVSection*() const;
	KVSection*	operator*() const;
	void		operator++();

	bool		operator==(KVKeyIterator& it) const { return it.index == index; }
	bool		operator!=(KVKeyIterator& it) const { return it.index != index; }

	bool		atEnd() const;
	void		Rewind();
private:
	bool		IsValidItem();

	const KVSection*	section{ nullptr };
	int					nameHashFilter{ 0 };
	int					searchFlags{ 0 };

	int					index{ 0 };
};

struct KVKeyIterator::Init
{
	KVKeyIterator	begin() const { return _initial; }
	KVKeyIterator	end() const;
	KVKeyIterator	_initial;
};

template<typename T = const char*>
struct KVValueIterator
{
	struct Init;

	KVValueIterator() = default;
	KVValueIterator(const KVSection* section, int index = 0)
		: section(section)
		, index(index)
	{
	}

	operator	KVPairValue*() const;
	T			operator*() const;
	void		operator++();

	bool		operator==(KVValueIterator& it) const { return it.index == index; }
	bool		operator!=(KVValueIterator& it) const { return it.index != index; }

	bool		atEnd() const;
	void		Rewind();
private:
	const KVSection*	section{ nullptr };
	int					index{ 0 };
};

template<typename T>
struct KVValueIterator<T>::Init
{
	KVValueIterator	begin() const { return _initial; }
	KVValueIterator	end() const;
	KVValueIterator	_initial;
};

inline int KV_GetValuesR(const KVSection* key, int idx, int cntIdx)
{
	return cntIdx; // end of recursion
}

template<typename T, typename ...Rest>
inline int KV_GetValuesR(const KVSection* key, int idx, int cntIdx, T& out, Rest&... outArgs)
{
	if (idx + KVPairValuesGetter<T>::vcount > key->ValueCount())
		return cntIdx;
	out = KVPairValuesGetter<T>::Get(key, idx);

	return KV_GetValuesR(key, idx + KVPairValuesGetter<T>::vcount, cntIdx + 1, outArgs...);
}

template<typename ...Args>
inline int KV_GetValuesAt(const KVSection* key, int idx, Args&... outArgs)
{
	return KV_GetValuesR(key, idx, 0, outArgs...);
}

template<typename ...Args>
inline int KV_GetValues(const KVSection* key, Args&... outArgs)
{
	const int ret = KV_GetValuesR(key, 0, 0, outArgs...);
	return ret;
}

//
// KeyValues typed value holder
//
struct KVPairValue
{
	~KVPairValue();

	KVSection*	section{ nullptr };
	char*		value{ nullptr };
	EKVPairType	type{ KVPAIR_STRING };

	union
	{
		int		nValue{ 0 };
		bool	bValue;
		float	fValue;
	};

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

	// Array values iterator
	template<typename T>
	inline typename KVValueIterator<T>::Init	Values(int startIdx = 0) const { return { KVValueIterator<T>(this, startIdx) }; }
	
	// Array keys iterator
	inline KVKeyIterator::Init					Keys(const char* nameFilter = nullptr, int searchFlags = 0) const { return { KVKeyIterator(this, nameFilter, searchFlags) }; }

	// Key values getter
	template<typename ...Args>
	inline int			GetValues(Args&... outArgs) const { return KV_GetValues(this, outArgs...); };

	// Key values getter at specific value idx
	template<typename ...Args>
	inline int			GetValuesAt(int idx, Args&... outArgs) const { return KV_GetValuesAt(this, 0, outArgs...); };

	//----------------------------------------------
	// The section functions
	//----------------------------------------------

	// searches for section and returns default empty if none found
	const KVSection&	Get(const char* pszName, int nFlags = 0) const;

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

	// TODO: private
	char				name[KV_MAX_NAME_LENGTH]{ 0 };
	int					nameHash{ 0 };
	int					line{ 0 };				// the line that the key is on

	Array<KVSection*>	keys{ PP_SL };			// the nested keys
	Array<KVPairValue*>	values{ PP_SL };
	EKVPairType			type{ KVPAIR_STRING };
	bool				unicode{ false };
};

// special wrapper class
// for better compatiblity of new class
class KeyValues
{
public:
	KeyValues() = default;
	~KeyValues() = default;

	void			Reset();

	KVKeyIterator::Init		Keys(const char* nameFilter = nullptr, int searchFlags = 0) const;
	const KVSection&		Get(const char* pszName, int nFlags = 0) const;
	KVSection*				FindSection(const char* pszName, int nFlags = 0) const;

	// loads from file
	bool				LoadFromFile(const char* pszFileName, int nSearchFlags = -1);
	bool				LoadFromStream(IVirtualStream* stream);

	bool				SaveToFile(const char* pszFileName, int nSearchFlags = -1);

	KVSection*			GetRootSection();

	KVSection*			operator[](const char* pszName);

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

// DEPRECATED for direct use. Use KVSection::GetValues / KVSection::GetValuesAt
const char*		KV_GetValueString(const KVSection* pBase, int nIndex = 0, const char* pszDefault = "" );
int				KV_GetValueInt(const KVSection* pBase, int nIndex = 0, int nDefault = 0 );
float			KV_GetValueFloat(const KVSection* pBase, int nIndex = 0, float fDefault = 0.0f );
bool			KV_GetValueBool(const KVSection* pBase, int nIndex = 0, bool bDefault = false );
Vector2D		KV_GetVector2D(const KVSection* pBase, int nIndex = 0, const Vector2D& vDefault = vec2_zero);
IVector2D		KV_GetIVector2D(const KVSection* pBase, int nIndex = 0, const IVector2D& vDefault = 0);
Vector3D		KV_GetVector3D(const KVSection* pBase, int nIndex = 0, const Vector3D& vDefault = vec3_zero);
Vector4D		KV_GetVector4D(const KVSection* pBase, int nIndex = 0, const Vector4D& vDefault = vec4_zero);

// For KV Value iterator

template<typename T>
KVValueIterator<T>::operator KVPairValue* () const
{
	return section->values[index];
}

template<typename T>
T KVValueIterator<T>::operator*() const
{
	return KVPairValuesGetter<T>::Get(section, index);
}

template<typename T>
void KVValueIterator<T>::operator++()
{
	index += KVPairValuesGetter<T>::vcount;
}

template<typename T>
bool KVValueIterator<T>::atEnd() const
{
	return section ? index >= section->values.numElem() : true;
}

template<typename T>
void KVValueIterator<T>::Rewind()
{
	index = 0;
}

template<typename T>
KVValueIterator<T> KVValueIterator<T>::Init::end() const
{
	KVValueIterator<T> endIt;
	endIt.section = _initial.section;
	endIt.index = _initial.section->ValueCount();
	return endIt;
}

template<> struct KVPairValuesGetter<const char*>
{
	static const char* Get(const KVSection* section, int index) { return KV_GetValueString(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<EqString>
{
	static EqStringRef Get(const KVSection* section, int index) { return KV_GetValueString(section, index); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<EqStringRef>
{
	static EqStringRef Get(const KVSection* section, int index) { return KV_GetValueString(section, index); }
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

