//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network property variable
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETPROPVAR_H
#define NETPROPVAR_H

#include "utils/DkList.h"
#include "utils/strtools.h"

template <class TYPE, class CHANGER>
class CNetworkVarBase
{
public:
	CNetworkVarBase() {}
	CNetworkVarBase(const TYPE& val)			{m_value = val;}

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

	const TYPE&				Get() const					{return m_value;}

	TYPE& GetForModify()
	{
		OnNetworkStateChanged();
		return m_value;
	}

	const TYPE& operator	=(TYPE const& v)			{return Set(v);}

	operator const			TYPE&() const				{return m_value;}
	operator				TYPE&()						{return m_value;}

	const TYPE*				operator->() const			{return &m_value; }

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

	TYPE			m_value;

protected:
	inline void OnNetworkStateChanged()
	{
		CHANGER::OnNetworkStateChanged( this );
	}
};

//--------------------------------------------------------------------------------------------------------

// Use this macro when you want to embed a structure inside your entity and have CNetworkVars in it.
template< class T >
static inline void DispatchNetworkStateChanged(T* pObj)
{
	pObj->OnNetworkStateChanged();
}
template< class T >
static inline void DispatchNetworkStateChanged(T* pObj, void *pVar)
{
	pObj->OnNetworkStateChanged(pVar);
}

// Internal macros used in definitions of network vars.
#define NETWORK_VAR_DECL( type, name, base ) \
	friend class NetworkVar_##name; \
	typedef ThisClass NetworkVar_##name##Cntr; \
	class NetworkVar_##name { \
	public: \
		static inline void OnNetworkStateChanged( void* ptr ) { \
			DispatchNetworkStateChanged((NetworkVar_##name##Cntr*)(((uintptr_t)ptr) - offsetOf(NetworkVar_##name##Cntr,name)), (void*)(uintptr_t)offsetOf(NetworkVar_##name##Cntr,name)); \
		} \
	}; \
	base< type, NetworkVar_##name > name;

#define CNetworkVar( type, name )	\
	NETWORK_VAR_DECL( type, name, CNetworkVarBase )

#define DECLARE_EMBEDDED_NETWORKVAR() \
	void OnNetworkStateChanged() {} \
	void OnNetworkStateChanged( void* ptr ) {}

#define CNetworkVarEmbedded( type, name ) \
	friend class NetworkVar_##name; \
	typedef ThisClass NetworkVar_##name##Cntr; \
	class NetworkVar_##name : public type { \
		template< class T > NetworkVar_##name& operator=( const T &val ) { *((type*)this) = val; return *this; } \
	public: \
		void CopyFrom( const type &src ) { *((type *)this) = src; OnNetworkStateChanged(this); } \
		void OnNetworkStateChanged() { \
			DispatchNetworkStateChanged(((NetworkVar_##name##Cntr*)(((char*)this) - offsetOf(NetworkVar_##name##Cntr,name)))); \
		} \
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

// network property address holder
struct netprop_t
{
	const char*					name;
	int							nameHash;

	int							flags;		// ENetPropFlags
	int							type;		// ENetPropTypes

	uint						offset;
	uint						size;

	struct netvariablemap_t*	nestedMap;
};

static inline void HashNetPropNames(netprop_t* props, int numProps)
{
	for (int i = 0; i < numProps; i++)
		props[i].nameHash = props[i].name ? StringToHash(props[i].name) : 0;
}

// variable map basics
struct netvariablemap_t
{
	const char*				m_mapName;
	netvariablemap_t*		m_baseMap;

	int16					m_numProps;

	netprop_t*				m_props;
};

#define BEGIN_NETWORK_TABLE_GUTS( className ) \
	template <typename T> netvariablemap_t *NetTableInit(T *); \
	template <> netvariablemap_t *NetTableInit<className>( className * ); \
	namespace className##_NetTableInit \
	{ \
		netvariablemap_t *g_NetTableHolder = NetTableInit( (className *)NULL );  \
	} \
	\
	template <> netvariablemap_t *NetTableInit<className>( className * ) \
	{ \
		typedef className classNameTypedef; \
		static netprop_t netTableDesc[] = \
		{ \
			{ NULL, 0, 0, 0,  0, 0 },

#define END_NETWORK_TABLE() \
		}; \
		\
		if ( sizeof( netTableDesc ) > sizeof( netTableDesc[0] ) ) \
		{ \
			classNameTypedef::m_NetworkVariableMap.m_numProps = elementsOf( netTableDesc ) - 1; \
			classNameTypedef::m_NetworkVariableMap.m_props = &netTableDesc[1]; \
		} \
		else \
		{ \
			classNameTypedef::m_NetworkVariableMap.m_numProps = 1; \
			classNameTypedef::m_NetworkVariableMap.m_props = netTableDesc; \
		} \
		HashNetPropNames(netTableDesc, elementsOf( netTableDesc ));\
		return &classNameTypedef::m_NetworkVariableMap; \
	}

#define BEGIN_NETWORK_TABLE( className ) \
	netvariablemap_t className::m_NetworkVariableMap = { #className, &BaseClass::m_NetworkVariableMap, 0, NULL }; \
	netvariablemap_t *className::GetNetworkTableMap( void ) { return &m_NetworkVariableMap; } \
	BEGIN_NETWORK_TABLE_GUTS( className )

//
#define BEGIN_NETWORK_TABLE_NO_BASE( className ) \
	netvariablemap_t className::m_NetworkVariableMap = { #className, NULL, 0, NULL }; \
	netvariablemap_t* className::GetNetworkTableMap( void ) { return &m_NetworkVariableMap; } \
	BEGIN_NETWORK_TABLE_GUTS( className )

// creates data description map for structures
#define DECLARE_SIMPLE_NETWORK_TABLE() \
	static netvariablemap_t m_NetworkVariableMap; \
	template <typename T> friend netvariablemap_t* NetTableInit(T *);

// creates data description map for classes
#define	DECLARE_NETWORK_TABLE() \
	DECLARE_SIMPLE_NETWORK_TABLE() \
	virtual netvariablemap_t* GetNetworkTableMap( void );

#define	DECLARE_NETWORK_TABLE_PUREVIRTUAL() \
	DECLARE_SIMPLE_NETWORK_TABLE() \
	virtual netvariablemap_t* GetNetworkTableMap( void ) = 0;

#define	DECLARE_NETWORK_TABLE_NOVIRTUAL() \
	DECLARE_SIMPLE_NETWORK_TABLE() \
	netvariablemap_t* GetNetworkTableMap( void );

#define NETWORK_CHANGELIST(name) m_changeList_##name

#define DECLARE_NETWORK_CHANGELIST(name)	DkList<uint>	NETWORK_CHANGELIST(name);

//------------------------------------------------------------------------------------

#define memb(structure,member)			(((structure *)0)->member)

// network variable declarations
#define DEFINE_SENDPROP_BYTE(name)		{#name, 0, NETPROP_FLAG_SEND, NETPROP_BYTE, offsetOf(classNameTypedef, name), sizeof(ubyte), nullptr}
#define DEFINE_SENDPROP_INT(name)		{#name, 0, NETPROP_FLAG_SEND, NETPROP_INT, offsetOf(classNameTypedef, name), sizeof(int), nullptr}
#define DEFINE_SENDPROP_FLOAT(name)		{#name, 0, NETPROP_FLAG_SEND, NETPROP_FLOAT, offsetOf(classNameTypedef, name), sizeof(float), nullptr}
#define DEFINE_SENDPROP_FREAL(name)		{#name, 0, NETPROP_FLAG_SEND, NETPROP_FREAL, offsetOf(classNameTypedef, name), sizeof(FReal), nullptr}

#define DEFINE_SENDPROP_VECTOR2D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_VECTOR2D, offsetOf(classNameTypedef, name), sizeof(Vector2D), nullptr}
#define DEFINE_SENDPROP_VECTOR3D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_VECTOR3D, offsetOf(classNameTypedef, name), sizeof(Vector3D), nullptr}
#define DEFINE_SENDPROP_VECTOR4D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_VECTOR4D, offsetOf(classNameTypedef, name), sizeof(Vector4D), nullptr}

#define DEFINE_SENDPROP_FVECTOR2D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_FVECTOR2D, offsetOf(classNameTypedef, name), sizeof(FVector2D), nullptr}
#define DEFINE_SENDPROP_FVECTOR3D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_FVECTOR3D, offsetOf(classNameTypedef, name), sizeof(FVector3D), nullptr}
#define DEFINE_SENDPROP_FVECTOR4D(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_FVECTOR4D, offsetOf(classNameTypedef, name), sizeof(FVector4D), nullptr}

#define DEFINE_SENDPROP_EMBEDDED(name)	{#name, 0, NETPROP_FLAG_SEND, NETPROP_NETPROP, offsetOf(classNameTypedef, name), 0, &memb(classNameTypedef, name).m_NetworkVariableMap}


/*
namespace Networking
{

};*/

#endif // NETPROPVAR_H
