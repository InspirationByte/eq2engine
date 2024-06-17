//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Recursive Key-Values system for configuration and other files
//				Implements JSON-like, but lightweight notation
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "KeyValuesDesc.h"

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
	KVKeyIterator(const KVSection* section);
	KVKeyIterator(const KVSection* section, const char* nameFilter, int searchFlags = 0, int index = 0);

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
	KVValueIterator(const KVSection* key)
		: key(key)
	{
	}

	KVValueIterator(const KVSection* key, int index);

	operator	KVPairValue*() const;
	T			operator*() const;
	void		operator++();

	bool		operator==(KVValueIterator& it) const { return it.index == index; }
	bool		operator!=(KVValueIterator& it) const { return it.index != index; }

	bool		atEnd() const;
	void		Rewind();
private:
	const KVSection*	key{ nullptr };
	int					index{ 0 };
};

template<typename T>
struct KVValueIterator<T>::Init
{
	KVValueIterator	begin() const { return _initial; }
	KVValueIterator	end() const;
	KVValueIterator	_initial;
};

template<typename ...Args>
struct KVValues
{
	using TupleRef = std::tuple<Args&...>;
	using TupleVal = std::tuple<Args...>;

	KVValues(Args&... outArgs) 
		: outArgs(std::tie(outArgs...))
		, newValues(outArgs...)
		, count(0)
	{
	}

	KVValues(KVValues&& other) noexcept
		: outArgs(other.outArgs)
		, newValues(std::move(other.newValues))
		, count(other.count)
		, cancel(other.cancel)
	{
		other.cancel = true;
	}

	~KVValues()
	{
		if (cancel)
			return;
		ApplyImpl(std::index_sequence_for<Args...>{});
	}

	KVValues& operator=(KVValues&& other) noexcept
	{
		if (this != &other)
		{
			outArgs = other.outArgs;
			newValues = std::move(other.newValues);
			count = other.count;
			cancel = other.cancel;
			other.cancel = true; // Prevent the moved-from object from applying its changes
		}
		return *this;
	}

	void		Cancel() { cancel = true; }
	operator	int() const { return count; }

	template<std::size_t... Is>
	void ApplyImpl(std::index_sequence<Is...>)
	{
		((std::get<Is>(outArgs) = std::get<Is>(newValues)), ...);
	}

	TupleVal	newValues;
	TupleRef	outArgs;
	int			count{ 0 };
	bool		cancel{ false };
};

template<typename ...Args>
int KV_GetValuesAt(const KVSection* key, int idx, Args&... outArgs);

template<typename ...Args>
int KV_GetValues(const KVSection* key, Args&... outArgs);

template<typename ...Args>
KVValues<Args...> KV_TryGetValuesAt(const KVSection* key, int idx, Args&... outArgs);

template<typename ...Args>
KVValues<Args...> KV_TryGetValues(const KVSection* key, Args&... outArgs);

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
	inline typename KVValueIterator<T>::Init	Values(int startIdx) const { return { KVValueIterator<T>(this, startIdx) }; }
	
	template<typename T>
	inline typename KVValueIterator<T>::Init	Values() const { return { KVValueIterator<T>(this) }; }

	// Array keys iterator
	inline KVKeyIterator::Init	Keys(const char* nameFilter = nullptr, int searchFlags = 0) const { return { KVKeyIterator(this, nameFilter, searchFlags) }; }

	// Key values getter
	template<typename ...Args>
	inline int					GetValues(Args&... outArgs) const { return KV_GetValues(this, outArgs...); };

	// Key values getter at specific value idx
	template<typename ...Args>
	inline int					GetValuesAt(int idx, Args&... outArgs) const { return KV_GetValuesAt(this, idx, outArgs...); };

	// Key values getter
	template<typename ...Args>
	inline KVValues<Args...>	TryGetValues(Args&... outArgs) const { return KV_TryGetValues(this, outArgs...); };

	// Key values getter at specific value idx
	template<typename ...Args>
	inline KVValues<Args...>	TryGetValuesAt(int idx, Args&... outArgs) const { return KV_TryGetValuesAt(this, idx, outArgs...); };

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
	KVPairValue&		operator[](int index);

	const KVSection*	operator[](const char* pszName) const;
	const KVPairValue&	operator[](int index) const;


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
	EqString			name;
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
KVValueIterator<T>::KVValueIterator(const KVSection* key, int index)
	: key(key)
	, index(min(index, key->ValueCount())) // limit to end()
{
}

template<typename T>
KVValueIterator<T>::operator KVPairValue* () const
{
	return key->values[index];
}

template<typename T>
T KVValueIterator<T>::operator*() const
{
	return KVPairValuesGetter<T>::Get(key, index);
}

template<typename T>
void KVValueIterator<T>::operator++()
{
	index += KVPairValuesGetter<T>::vcount;
}

template<typename T>
bool KVValueIterator<T>::atEnd() const
{
	return key ? index >= key->ValueCount() : true;
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
	endIt.key = _initial.key;
	endIt.index = _initial.key->ValueCount();
	return endIt;
}

template<> struct KVPairValuesGetter<const char*>
{
	static const char* Get(const KVSection* section, int index) { return (*section)[index].GetString(); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<float>
{
	static float Get(const KVSection* section, int index) { return (*section)[index].GetFloat(); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<int>
{
	static int Get(const KVSection* section, int index) { return (*section)[index].GetInt(); }
	static const int vcount = 1;
};

template<> struct KVPairValuesGetter<bool>
{
	static bool Get(const KVSection* section, int index) { return (*section)[index].GetBool(); }
	static const int vcount = 1;
};

// define aliases that use same code
template<> struct KVPairValuesGetter<EqStringRef> : KVPairValuesGetter<const char*> {};
template<> struct KVPairValuesGetter<EqString> : KVPairValuesGetter<const char*> {};
template<> struct KVPairValuesGetter<FReal> : KVPairValuesGetter<float> {};
template<> struct KVPairValuesGetter<uint> : KVPairValuesGetter<int> {};
template<> struct KVPairValuesGetter<int16> : KVPairValuesGetter<int> {};
template<> struct KVPairValuesGetter<uint16> : KVPairValuesGetter<int> {};
template<> struct KVPairValuesGetter<int8> : KVPairValuesGetter<int> {};
template<> struct KVPairValuesGetter<uint8> : KVPairValuesGetter<int> {};

template <typename T>
struct KVPairValuesGetter<TVec2D<T>>
{
	using CompGetter = KVPairValuesGetter<T>;
	static TVec2D<T> Get(const KVSection* section, int index)
	{
		return TVec2D<T>(CompGetter::Get(section, index), CompGetter::Get(section, index+1));
	}
	static const int vcount = 2;
};

template <typename T>
struct KVPairValuesGetter<TVec3D<T>>
{
	using CompGetter = KVPairValuesGetter<T>;
	static TVec3D<T> Get(const KVSection* section, int index)
	{
		return TVec3D<T>(CompGetter::Get(section, index), CompGetter::Get(section, index + 1), CompGetter::Get(section, index + 2));
	}
	static const int vcount = 3;
};

template <typename T>
struct KVPairValuesGetter<TVec4D<T>>
{
	using CompGetter = KVPairValuesGetter<T>;
	static TVec4D<T> Get(const KVSection* section, int index)
	{
		return TVec4D<T>(CompGetter::Get(section, index), CompGetter::Get(section, index + 1), CompGetter::Get(section, index + 2), CompGetter::Get(section, index + 3));
	}
	static const int vcount = 4;
};

namespace kvdetail
{
inline int GetValuesR(const KVSection* key, int idx, int cntIdx)
{
	return cntIdx; // end of recursion
}

template<typename T, typename ...Rest>
inline int GetValuesR(const KVSection* key, int idx, int cntIdx, T& out, Rest&... outArgs)
{
	if (idx + KVPairValuesGetter<T>::vcount > key->ValueCount())
		return cntIdx;
	out = KVPairValuesGetter<T>::Get(key, idx);

	return GetValuesR(key, idx + KVPairValuesGetter<T>::vcount, cntIdx + 1, outArgs...);
}

template<typename ...Args, std::size_t... Is>
int GetValuesImpl(const KVSection* key, int idx, std::index_sequence<Is...>, std::tuple<Args...>& newValues)
{
	return GetValuesR(key, idx, 0, std::get<Is>(newValues)...);
}
}

template<typename ...Args>
int KV_GetValuesAt(const KVSection* key, int idx, Args&... outArgs)
{
	return kvdetail::GetValuesR(key, idx, 0, outArgs...);
}

template<typename ...Args>
int KV_GetValues(const KVSection* key, Args&... outArgs)
{
	return kvdetail::GetValuesR(key, 0, 0, outArgs...);
}

template<typename ...Args>
KVValues<Args...> KV_TryGetValuesAt(const KVSection* key, int idx, Args&... outArgs)
{
	KVValues<Args...> values(outArgs...);
	values.count = kvdetail::GetValuesImpl(key, idx, std::index_sequence_for<Args...>{}, values.newValues);
	return values;
}

template<typename ...Args>
KVValues<Args...> KV_TryGetValues(const KVSection* key, Args&... outArgs)
{
	KVValues<Args...> values(outArgs...);
	values.count = kvdetail::GetValuesImpl(key, 0, std::index_sequence_for<Args...>{}, values.newValues);
	return values;
}

// KeyValues parser desc detail impl

namespace kvdetail
{
template<typename T>
struct DescField : public KVDescFieldInfo
{
	DescField(int offset, const char* name) : KVDescFieldInfo(offset, name, &Parse) {}
	static bool Parse(const KVSection& section, const char* name, void* outPtr)
	{
		return section.Get(name).GetValues(*reinterpret_cast<T*>(outPtr));
	}
};

template<typename T>
struct DescFieldEmbedded : public KVDescFieldInfo
{
	DescFieldEmbedded(int offset, const char* name) : KVDescFieldInfo(offset, name, &Parse) {}
	static bool Parse(const KVSection& section, const char* name, void* outPtr)
	{
		return KV_ParseDesc<T>(*reinterpret_cast<T*>(outPtr), section.Get(name));
	}
};

template<typename T>
struct DescFieldArray : public KVDescFieldInfo
{
	DescFieldArray(int offset, const char* name) : KVDescFieldInfo(offset, name, &Parse) {}
	static bool Parse(const KVSection& section, const char* name, void* outPtr)
	{
		const KVSection& sec = section.Get(name);
		const int valueCount = sec.ValueCount();

		if constexpr (std::is_array_v<T>)
		{
			T& arrayRef = *reinterpret_cast<T*>(outPtr);

			for (int i = 0; i < min(static_cast<int>(CArrayLenGetter<T>::len), valueCount); ++i)
				KV_GetValuesAt(&sec, i, arrayRef[i]);
		}
		else
		{
			using ITEM = typename T::ITEM;
			Array<ITEM>& arrayRef = *reinterpret_cast<T*>(outPtr);

			arrayRef.reserve(arrayRef.numElem() + valueCount);
			for (int i = 0; i < valueCount; ++i)
				KV_GetValuesAt(&sec, i, arrayRef.append());
		}
		return true;
	}
};

template<typename T>
struct DescFieldEmbeddedArray : public KVDescFieldInfo
{
	DescFieldEmbeddedArray(int offset, const char* name) : KVDescFieldInfo(offset, name, &Parse) {}
	static bool Parse(const KVSection& section, const char* name, void* outPtr)
	{
		const KVSection& sec = section.Get(name);
		if constexpr (std::is_array_v<T>)
		{
			T& arrayRef = *reinterpret_cast<T*>(outPtr);
			for (int i = 0; i < min(static_cast<int>(CArrayLenGetter<T>::len), sec.KeyCount()); ++i)
				KV_ParseDesc(arrayRef[i], *sec.KeyAt(i));
		}
		else
		{
			using ITEM = typename T::ITEM;
			Array<ITEM>& arrayRef = *reinterpret_cast<T*>(outPtr);

			arrayRef.reserve(arrayRef.numElem() + sec.KeyCount());
			for (const KVSection* embSec : sec.Keys())
				KV_ParseDesc(arrayRef.append(), *embSec);
		}
		return true;
	}
};

template<typename T, typename FLAGS_DESC>
struct DescFieldFlags : public KVDescFieldInfo
{
	DescFieldFlags(int offset, const char* name) : KVDescFieldInfo(offset, name, &Parse) {}
	static bool Parse(const KVSection& section, const char* name, void* outPtr)
	{
		T& flagsValue = *reinterpret_cast<T*>(outPtr);
		flagsValue = 0;

		for (EqStringRef flagName : section.Get(name).Values<EqStringRef>())
		{
			const int flagNameHash = StringToHash(flagName, true);
			for (const KVDescFlagInfo& flagInfo : FLAGS_DESC::GetFlags())
			{
				if (flagInfo.nameHash == flagNameHash)
				{
					flagsValue |= flagInfo.value;
					break;
				}
			}
		}

		return true;
	}
};
} // namespace kvdetail

#define BEGIN_KEYVALUES_DESC_TYPE(classname, descName) \
	const char* descName::GetTypeName() { return #classname; } \
	ArrayCRef<KVDescFieldInfo> descName::GetFields() { \
		using DescType = classname; \
		static KVDescFieldInfo descFields[] = {

#define BEGIN_KEYVALUES_DESC(classname) \
	BEGIN_KEYVALUES_DESC_TYPE(classname, classname ## ::Desc)

#define END_KEYVALUES_DESC \
		}; \
		return descFields; \
	};

#define KV_DESC_FIELD(name) \
	kvdetail::DescField<decltype(DescType::name)>{offsetOf(DescType, name), #name},

#define KV_DESC_EMBEDDED(name) \
	kvdetail::DescFieldEmbedded<decltype(DescType::name)>{offsetOf(DescType, name), #name},

#define KV_DESC_ARRAY_FIELD(name) \
	kvdetail::DescFieldArray<decltype(DescType::name)>{offsetOf(DescType, name), #name},

#define KV_DESC_ARRAY_EMBEDDED(name) \
	kvdetail::DescFieldEmbeddedArray<decltype(DescType::name)>{offsetOf(DescType, name), #name},

#define KV_DESC_FLAGS(name, flagsType) \
	kvdetail::DescFieldFlags<decltype(DescType::name), flagsType>{offsetOf(DescType, name), #name},

//------------------

#define BEGIN_KEYVALUES_FLAGS_DESC(enumDescName) \
	ArrayCRef<KVDescFlagInfo> enumDescName::GetFlags() { \
		static KVDescFlagInfo descFlags[] = {

#define END_KEYVALUES_FLAGS_DESC \
		}; \
		return descFlags; \
	};

#define KV_FLAG_DESC(value, name) {name, StringToHashConst(name), static_cast<int>(value)},