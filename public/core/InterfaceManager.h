//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base interface loading
//////////////////////////////////////////////////////////////////////////////////

#ifndef INTERFACEMANAGER_H
#define INTERFACEMANAGER_H

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


#define ENTRYPOINT_INTERFACE_SINGLETON( abstractclass, classname, localname ) \
	IEXPORTS abstractclass* Get##classname();								  \
	static abstractclass* localname = Get##classname();		// this thing is designed to fool the LLVM/GCC because it's fucking mystery

#ifdef CORE_INTERFACE_EXPORT

// dll export version
#define INTERFACE_SINGLETON(abstractclass, classname, interfacename, localname)	\
	IEXPORTS abstractclass* Get##classname();									\
	class CDkCoreInterface_##classname {		\
	public:										\
		abstractclass*	GetInstancePtr()	{ return Get##classname(); } \
		abstractclass*	operator->()		{ return GetInstancePtr(); }			\
		operator		abstractclass*()	{ return GetInstancePtr(); }			\
	};																				\
	static CDkCoreInterface_##classname localname;

#define EXPORTED_INTERFACE( interfacename, classname )	\
	IEXPORTS interfacename *Get##classname( void ) {    \
		static classname s_##classname;					\
		return ( interfacename * )&s_##classname;		\
	}													\
	interfacename* _inteface##classname = Get##classname();

#else

// dll import version
#define INTERFACE_SINGLETON(abstractclass, classname, interfacename, localname)	\
	IEXPORTS void* _GetDkCoreInterface(const char* pszName);						\
	class _##classname##SingletonInstantiator {	\
	public:										\
		_##classname##SingletonInstantiator()	{ instance = (abstractclass*)_GetDkCoreInterface(interfacename); }	\
		abstractclass* instance;				\
	};											\
	class CDkCoreInterface_##classname {		\
	public:										\
		abstractclass*	GetInstancePtr()	{ static _##classname##SingletonInstantiator i; return i.instance; } \
		abstractclass*	operator->()		{ return GetInstancePtr(); }			\
		operator		abstractclass*()	{ return GetInstancePtr(); }			\
	};																				\
	static CDkCoreInterface_##classname localname;

#endif // _DKLAUNCHER_

#endif //INTERFACEMANAGER_H
