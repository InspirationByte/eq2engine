// Key-Values Desc parser and writer

#pragma once

struct KVSection;

struct KVDescFieldInfo
{
	template<class T>
	struct CArrayLenGetter;

	template<class T, std::size_t N>
	struct CArrayLenGetter<T[N]> { inline static int len = N; };

	using ParseFunc = bool (*)(const KVSection& section, const char* name, void* outPtr);

	KVDescFieldInfo(int offset, const char* name, ParseFunc parseFunc)
		: name(name)
		, offset(offset)
		, parseFunc(parseFunc)
	{
	}

	EqStringRef		name;
	ParseFunc		parseFunc;
	int				offset;
};

struct KVDescFlagInfo
{
	EqStringRef		name;
	int				nameHash;
	int				value;
};

#define DEFINE_KEYVALUES_DESC_TYPE(descName) \
	struct descName { \
		static const char* GetTypeName(); \
		static ArrayCRef<KVDescFieldInfo> GetFields(); \
	};

#define DEFINE_KEYVALUES_DESC() \
	DEFINE_KEYVALUES_DESC_TYPE(Desc)

// See KeyValues.h for next defines:
// 
//		BEGIN_KEYVALUES_DESC_TYPE
//		END_KEYVALUES_DESC_TYPE
//		KV_DESC_FIELD

//-----------------------

#define DECLARE_KEYVALUES_FLAGS_DESC(enumDescName) \
	struct enumDescName { \
		static ArrayCRef<KVDescFlagInfo> GetFlags(); \
	};

// See KeyValues.h for next defines:
// 
//		BEGIN_KEYVALUES_FLAGS_DESC
//		END_KEYVALUES_FLAGS_DESC
//		KV_FLAG_DESC


//-----------------------

// TODO: Map support as Dictionary type

template<typename T>
inline bool KV_ParseDesc(T& descData, const KVSection& section)
{
	using Desc = typename T::Desc;
	for (const KVDescFieldInfo& info : Desc::GetFields())
		info.parseFunc(section, info.name, reinterpret_cast<ubyte*>(&descData) + info.offset);
	return true;
}
