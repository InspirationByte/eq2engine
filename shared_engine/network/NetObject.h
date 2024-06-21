//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network property variable
//////////////////////////////////////////////////////////////////////////////////

#pragma once

namespace Networking { class Buffer; }

template <class _TYPE, class CHANGER>
class CNetworkVarBase
{
public:
	using TYPE = _TYPE;

	CNetworkVarBase() = default;
	CNetworkVarBase(const TYPE& val) 
		: m_value(val)
	{
	}

	const TYPE&	Set(const TYPE& val)
	{
		if(memcmp( &m_value, &val, sizeof(TYPE) ))
		{
			OnNetworkStateChanged();
			m_value = val;
		}

		return m_value;
	}

	const TYPE&	Change(const TYPE& val)
	{
		m_value = val;

		return m_value;
	}

	const TYPE&	Get() const	{ return m_value; }

	TYPE& GetForModify()
	{
		OnNetworkStateChanged();
		return m_value;
	}

	const TYPE& operator=(TYPE const& v) { return Set(v); }

	operator const TYPE& () const { return m_value; }
	operator TYPE& () {return m_value;}

	const TYPE* operator->() const { return &m_value; }

	const TYPE& operator++()
	{
		return (*this += 1);
	}

	TYPE operator--()
	{
		return (*this -= 1);
	}

	TYPE operator++( int ) // postfix version..
	{
		TYPE val = m_value;
		(*this += 1);
		return val;
	}

	TYPE operator--( int ) // postfix version..
	{
		TYPE val = m_value;
		(*this -= 1);
		return val;
	}

	template< class C >
	const TYPE& operator+=( const C &val )
	{
		return Set( m_value + ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator-=( const C &val )
	{
		return Set( m_value - ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator/=( const C &val )
	{
		return Set( m_value / ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator*=( const C &val )
	{
		return Set( m_value * ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator^=( const C &val )
	{
		return Set( m_value ^ ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator|=( const C &val )
	{
		return Set( m_value | ( const TYPE )val );
	}

	template< class C >
	const TYPE& operator&=( const C &val )
	{
		return Set( m_value & ( const TYPE )val );
	}

	TYPE m_value;

protected:
	inline void OnNetworkStateChanged()
	{
		CHANGER::OnNetworkStateChanged( this );
	}
};

//--------------------------------------------------------------------------------------------------------

// Use this macro when you want to embed a structure inside your entity and have CNetworkVars in it.

template< class T >
static inline void DispatchNetworkStateChanged(T* pObj, void *pVar)
{
	pObj->OnNetworkStateChanged(pVar);
}

// Internal macros used in definitions of network vars.
#define NETWORK_VAR_DECL( type, name, base ) \
	friend class NetworkVar_##name; \
	using NetworkVar_##name##Cntr = ThisClass ; \
	struct NetworkVar_##name { \
		static inline void OnNetworkStateChanged( void* ptr ) { \
			DispatchNetworkStateChanged((NetworkVar_##name##Cntr*)(((uintptr_t)ptr) - offsetOf(NetworkVar_##name##Cntr,name)), (void*)(uintptr_t)offsetOf(NetworkVar_##name##Cntr,name)); \
		} \
	}; \
	base< type, NetworkVar_##name > name

#define CNetworkVar( type, name )	\
	NETWORK_VAR_DECL( type, name, CNetworkVarBase )

#define DECLARE_EMBEDDED_NETWORKVAR() \
	void OnNetworkStateChanged( void* ptr ) {}

#define CNetworkVarEmbedded( type, name ) \
	friend class NetworkVar_##name; \
	using NetworkVar_##name##Cntr = ThisClass ; \
	struct NetworkVar_##name : public type { \
		template< class T > NetworkVar_##name& operator=( const T &val ) { *((type*)this) = val; return *this; } \
		void CopyFrom( const type &src ) { *((type *)this) = src; OnNetworkStateChanged(this); } \
		void OnNetworkStateChanged( void* ptr ) { \
			DispatchNetworkStateChanged(((NetworkVar_##name##Cntr*)(((char*)this) - offsetOf(NetworkVar_##name##Cntr,name))), (void*)(uintptr_t)offsetOf(NetworkVar_##name##Cntr,name)); \
		} \
	}; \
	NetworkVar_##name name; 

//--------------------------------------------------------------------------------------------------------
// Data map declaration

enum ENetPropFlags
{
	NETPROP_FLAG_SEND = (1 << 0),
	NETPROP_FLAG_ARRAY = (1 << 1),
};

enum ENetPropTypes
{
	NETPROP_BYTE = 0,
	NETPROP_SHORT,
	NETPROP_INT,

	NETPROP_FLOAT,
	NETPROP_VECTOR2D,
	NETPROP_VECTOR3D,
	NETPROP_VECTOR4D,

	NETPROP_FREAL,
	NETPROP_FVECTOR2D,
	NETPROP_FVECTOR3D,
	NETPROP_FVECTOR4D,

	NETPROP_NETPROP,		// nesting
};

struct NetPropertyMap;

// network property address holder
struct NetProperty
{
	const char*		name{ nullptr };
	int				nameHash{ 0 };

	int				flags{ 0 };		// ENetPropFlags
	int				type{ 0 };		// ENetPropTypes

	uint			offset{ 0 };
	uint			size{ 0 };

	NetPropertyMap* nestedMap{ nullptr };
};

// variable map basics
struct NetPropertyMap
{
	const char*				mapNam{ nullptr };
	NetPropertyMap*			baseMap{ nullptr };
	ArrayCRef<NetProperty>	props{ nullptr };
};

#define BEGIN_NETWORK_TABLE_GUTS( className ) \
	template <typename T> NetPropertyMap* NetTableInit(T *); \
	template<> NetPropertyMap* NetTableInit<className>( className * ); \
	namespace className##_NetTableInit { \
		NetPropertyMap* g_NetTableHolder = NetTableInit( (className *)nullptr );  \
	} \
	template<> NetPropertyMap* NetTableInit<className>( className * ) { \
		using NetPropertyHostType = className; \
		static const NetProperty netTableDesc[] = { \
			{},

#define END_NETWORK_TABLE() \
		}; \
		if (sizeof(netTableDesc) > sizeof(netTableDesc[0])) \
			NetPropertyHostType::netPropertyMap.props = ArrayCRef(netTableDesc, elementsOf(netTableDesc)); \
		return &NetPropertyHostType::netPropertyMap; \
	}

#define BEGIN_NETWORK_TABLE( className ) \
	NetPropertyMap className::netPropertyMap = { #className, &BaseClass::netPropertyMap, nullptr }; \
	NetPropertyMap* className::GetNetPropertyMap() { return &netPropertyMap; } \
	BEGIN_NETWORK_TABLE_GUTS( className )

#define BEGIN_NETWORK_TABLE_NO_BASE( className ) \
	NetPropertyMap className::netPropertyMap = { #className, nullptr, nullptr }; \
	NetPropertyMap* className::GetNetPropertyMap() { return &netPropertyMap; } \
	BEGIN_NETWORK_TABLE_GUTS( className )

// creates data description map for structures
#define DECLARE_SIMPLE_NETWORK_TABLE() \
	static NetPropertyMap netPropertyMap; \
	template <typename T> friend NetPropertyMap* NetTableInit(T *);

// creates data description map for classes
#define	DECLARE_NETWORK_TABLE() \
	DECLARE_SIMPLE_NETWORK_TABLE() \
	virtual NetPropertyMap* GetNetPropertyMap();

//------------------------------------------------------------------------------------

#define memb(structure,member)			(((structure *)0)->member)

template<typename T, bool _enum = std::is_enum_v<T>>
struct NetPropType;

template<> struct NetPropType<char, false>		{ static const ENetPropTypes type = NETPROP_BYTE; };
template<> struct NetPropType<short, false>		{ static const ENetPropTypes type = NETPROP_SHORT; };
template<> struct NetPropType<int, false>		{ static const ENetPropTypes type = NETPROP_INT; };
template<> struct NetPropType<float, false>		{ static const ENetPropTypes type = NETPROP_FLOAT; };
template<> struct NetPropType<FReal, false>		{ static const ENetPropTypes type = NETPROP_FREAL; };
template<> struct NetPropType<Vector2D, false>	{ static const ENetPropTypes type = NETPROP_VECTOR2D; };
template<> struct NetPropType<Vector3D, false>	{ static const ENetPropTypes type = NETPROP_VECTOR3D; };
template<> struct NetPropType<Vector4D, false>	{ static const ENetPropTypes type = NETPROP_VECTOR4D; };
template<> struct NetPropType<FVector2D, false>	{ static const ENetPropTypes type = NETPROP_FVECTOR2D; };
template<> struct NetPropType<FVector3D, false>	{ static const ENetPropTypes type = NETPROP_FVECTOR3D; };
template<> struct NetPropType<FVector4D, false>	{ static const ENetPropTypes type = NETPROP_FVECTOR4D; };

template<> struct NetPropType<bool, false> : NetPropType<char> {};
template<> struct NetPropType<uint8, false> : NetPropType<char> {};
template<> struct NetPropType<int8, false> : NetPropType<char> {};
template<> struct NetPropType<uint16, false> : NetPropType<short> {};
template<> struct NetPropType<uint32, false> : NetPropType<int> {};

template<typename T>
struct NetPropType<T, true> : NetPropType<std::underlying_type_t<T>, false> {};

// network variable declarations

#define DEFINE_SENDPROP(name) \
	{#name, StringToHashConst(#name), NETPROP_FLAG_SEND, NetPropType<typename decltype(NetPropertyHostType::name)::TYPE>::type, offsetOf(NetPropertyHostType, name), sizeof(NetPropertyHostType::name), nullptr},

#define DEFINE_SENDPROP_EMBEDDED(name) \
	{#name, StringToHashConst(#name), NETPROP_FLAG_SEND, NETPROP_NETPROP, offsetOf(NetPropertyHostType, name), 0, &memb(NetPropertyHostType, name).netPropertyMap },

#define NETWORK_CHANGELIST(name)			m_changeList_##name
#define DECLARE_NETWORK_CHANGELIST(name)	Array<uint>	NETWORK_CHANGELIST(name){ PP_SL }

void PackNetworkVariables(void* objectPtr, const NetPropertyMap* map, Networking::Buffer* buffer, Array<uint>& changeList);
void UnpackNetworkVariables(void* objectPtr, const NetPropertyMap* map, Networking::Buffer* buffer);
