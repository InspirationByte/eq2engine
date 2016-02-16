//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Provides base interface loading
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include "utils/singleton.h"

#ifndef INTERFACEMANAGER_H
#define INTERFACEMANAGER_H

//--------------------------------------------------------------
// The base core interface to be queried
//--------------------------------------------------------------
class ICoreModuleInterface
{
public:
	virtual bool		IsInitialized() const = 0;
	virtual const char*	GetInterfaceName() const = 0;
};

//--------------------------------------------------------------

#ifdef _MSC_VER
#   define ONLY_EXPORTS	            extern "C" __declspec(dllexport)
#   define ONLY_IMPORTS	            extern "C" __declspec(dllimport)

#   define IFACE_PRIORITY_EXPORT1
#   define IFACE_PRIORITY_EXPORT2

#else // __GNUC__
#   define ONLY_EXPORTS              __attribute__ ((visibility("default")))
#   define ONLY_IMPORTS

#   define IFACE_PRIORITY_EXPORT1    __attribute__ ((init_priority(110)))
#   define IFACE_PRIORITY_EXPORT2    __attribute__ ((init_priority(120)))
#endif // _MSC_VER

#ifdef CROSSLINK_LIB
#	define IEXPORTS			ONLY_IMPORTS
#else
#	ifdef DLL_EXPORT
#		define IEXPORTS		ONLY_EXPORTS
#	else
#		define IEXPORTS		ONLY_IMPORTS
#	endif
#endif // CROSSLINK_LIB

#define EXPOSE_SINGLE_INTERFACE(name,classname,interfacename)	\
	static classname g_##name##;								\
	interfacename *name = ( interfacename * )&g_##name##;

#define EXPOSE_SINGLE_INTERFACE_EX(name,classname,interfacename)	\
	static classname g_##name##;								\
	interfacename *name = ( interfacename * )&g_##name##;		\
	interfacename *Get##name##( void ) {return name;}

#define EXPOSE_SINGLE_INTERFACE_EXPORTS_EX(name,classname,interfacename)	\
	static classname g_##name##;								\
	interfacename *name = ( interfacename * )&g_##name##;		\
	IEXPORTS interfacename *Get##name##( void ) {return name;}

#define EXPOSE_SINGLE_INTERFACE_NOBASE(name,classname)		\
	static classname g_##name##;							\
	classname *name = ( classname * )&g_##name##;

#define EXPOSE_SINGLE_INTERFACE_NOBASE_EX(name,classname)		\
	static classname g_##name##;							\
	classname *name = &g_##name##;				\
	classname *Get##name##( void ) {return name;}

#define EXPOSE_SINGLE_INTERFACE_NOBASE_EXPORTS_EX(name,classname)		\
	static classname g_##name##;							\
	classname *name = &g_##name##;				\
	IEXPORTS classname *Get##name##( void ) {return name;}

//-----------------------------------------------------------------------------------------------------------------
// new interface export macros with using singletons

#ifdef DLL_EXPORT
// dll export version
#	define INTERFACE_SINGLETON(abstractclass, classname, interfacename, localname)			\
		IEXPORTS abstractclass* Get##classname();	\
		class _##classname##Singleton : public CSingletonAbstract<abstractclass>	\
		{	\
		public:\
            virtual	~_##classname##Singleton()  { Destroy(); }  \
		protected:	\
			void	Initialize()	\
			{	\
				if(!pInstance) pInstance = (abstractclass*)Get##classname();	\
			}	\
			void	Destroy()	\
			{	\
				pInstance = NULL;	\
			}	\
		};	\
		static _##classname##Singleton localname IFACE_PRIORITY_EXPORT2;

#else
// dll import version
#	define INTERFACE_SINGLETON(abstractclass, classname, interfacename, localname)			\
		IEXPORTS void*		_GetDkCoreInterface(const char* pszName);\
		class _##classname##Singleton : public CSingletonAbstract<abstractclass>	\
		{	\
		public:\
            virtual	~_##classname##Singleton()  { Destroy(); }  \
		protected:	\
			void	Initialize()	\
			{	\
				if(!pInstance) pInstance = (abstractclass*)_GetDkCoreInterface(interfacename);	\
			}	\
			void	Destroy()	\
			{	\
				pInstance = NULL;	\
			}	\
		};	\
		static _##classname##Singleton localname IFACE_PRIORITY_EXPORT2;

#endif

#define EXPORTED_INTERFACE( interfacename, classname)							\
	static classname s_##classname IFACE_PRIORITY_EXPORT1;						\
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;		\
	IEXPORTS interfacename *Get##classname( void ) {return g_p##interfacename;}

#define EXPORTED_INTERFACE_FUNCTION( interfacename, classname, funcname)		\
	static classname s_##classname IFACE_PRIORITY_EXPORT1;						\
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;		\
	IEXPORTS interfacename *funcname( void ) {return g_p##interfacename;}

#define _INTERFACE_FUNCTION( interfacename, classname, funcname)		        \
	static classname s_##classname IFACE_PRIORITY_EXPORT1;				        \
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;	    \
	interfacename *funcname( void ) {return g_p##interfacename;}


#endif //INTERFACEMANAGER_H
