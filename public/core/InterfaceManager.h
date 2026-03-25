//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base interface loading
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#define CORE_INTERFACE(name) 	\
	static const char* 	CoreInterfaceName() { return name; } \
	const char*			GetInterfaceName() const { return CoreInterfaceName(); }

//--------------------------------------------------------------
// The base core interface to be queried
//--------------------------------------------------------------
class IEqCoreModule
{
public:
	virtual ~IEqCoreModule() = default;
	virtual bool		IsInitialized() const = 0;
	virtual const char*	GetInterfaceName() const = 0;
};

//--------------------------------------------------------------

#ifdef _MSC_VER
#   define DLL_EXPORT	             __declspec(dllexport)
#   define DLL_IMPORT	             __declspec(dllimport)
#   define FUNC_EXPORTS	             extern "C" DLL_EXPORT
#   define FUNC_IMPORTS	             extern "C" DLL_IMPORT
#	define CLASS_EXPORTS             DLL_EXPORT
#	define CLASS_IMPORTS             DLL_IMPORT

#else // __GNUC__
#   define DLL_EXPORT                __attribute__ ((visibility("default")))
#   define DLL_IMPORT
#   define FUNC_EXPORTS              DLL_EXPORT
#   define FUNC_IMPORTS              DLL_IMPORT
#	define CLASS_EXPORTS
#	define CLASS_IMPORTS
#endif // _MSC_VER

#ifdef CROSSLINK_LIB
#	define IEXPORTS			FUNC_IMPORTS
#else
#	ifdef COREDLL_EXPORT
#		define IEXPORTS		FUNC_EXPORTS
#	else
#		define IEXPORTS		FUNC_IMPORTS
#	endif
#endif // CROSSLINK_LIB

//-----------------------------------------------------------------------------------------------------------------


#define ENTRYPOINT_INTERFACE_SINGLETON( AbstractClass, localname )  \
	IEXPORTS AbstractClass* Get##AbstractClass##Impl();				\
	static AbstractClass* localname = Get##AbstractClass##Impl();		// this thing is designed to fool the LLVM/GCC because it's fucking mystery

#ifdef CORE_INTERFACE_EXPORT

// dll export version
#define INTERFACE_SINGLETON(AbstractClass, localname)	\
	IEXPORTS AbstractClass* Get##AbstractClass##Impl(); \
	class CDkCoreIface_##AbstractClass { \
	public: \
		AbstractClass*	GetInstancePtr()	{ return Get##AbstractClass##Impl(); } \
		AbstractClass*	operator->()		{ return GetInstancePtr(); } \
		operator		AbstractClass*()	{ return GetInstancePtr(); } \
	}; \
	static CDkCoreIface_##AbstractClass localname;

#define EXPORTED_INTERFACE( AbstractClass, ImplClass )	\
	IEXPORTS AbstractClass* Get##AbstractClass##Impl( void ) {    \
		static ImplClass s_##ImplClass;					\
		return ( AbstractClass * )&s_##ImplClass;		\
	}													\
	AbstractClass* _iface_##AbstractClass = Get##AbstractClass##Impl();

#else

// dll import version
#define INTERFACE_SINGLETON(AbstractClass, localname) \
	IEXPORTS void* _GetDkCoreInterface(const char* pszName); \
	class _##AbstractClass##SingletonInstantiator {	\
	public: \
		_##AbstractClass##SingletonInstantiator() { \
			instance = (AbstractClass*)_GetDkCoreInterface(AbstractClass::CoreInterfaceName()); \
		} \
		AbstractClass* instance; \
	}; \
	class CDkCoreIface_##AbstractClass { \
	public: \
		AbstractClass*	GetInstancePtr()	{ static _##AbstractClass##SingletonInstantiator i; return i.instance; } \
		AbstractClass*	operator->()		{ return GetInstancePtr(); } \
		operator		AbstractClass*()	{ return GetInstancePtr(); } \
	}; \
	static CDkCoreIface_##AbstractClass localname;

#endif // CORE_INTERFACE_EXPORT
