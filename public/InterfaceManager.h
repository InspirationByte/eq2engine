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

#ifdef _MSC_VER
#   define ONLY_EXPORTS	extern "C" __declspec(dllexport)
#   define ONLY_IMPORTS	extern "C" __declspec(dllimport)
#else // __GNUC__
#   define ONLY_EXPORTS     __attribute__ ((visibility("default")))
#   define ONLY_IMPORTS
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
		IEXPORTS abstractclass*		Get##classname();	\
		class _##classname##Singleton : public CSingletonAbstract<abstractclass>	\
		{	\
		protected:	\
			void	Initialize()	\
			{	\
				if(!pInstance) pInstance = (abstractclass*)Get##classname();	\
			}	\
			void	Destroy()	\
			{	\
				if(pInstance) pInstance = NULL;	\
			}	\
		};	\
		static _##classname##Singleton localname;

#else
// dll import version
#	define INTERFACE_SINGLETON(abstractclass, classname, interfacename, localname)			\
		IEXPORTS void*		_GetDkCoreInterface(const char* pszName);\
		class _##classname##Singleton : public CSingletonAbstract<abstractclass>	\
		{	\
		protected:	\
			void	Initialize()	\
			{	\
				if(!pInstance) pInstance = (abstractclass*)_GetDkCoreInterface(interfacename);	\
			}	\
			void	Destroy()	\
			{	\
				if(pInstance) pInstance = NULL;	\
			}	\
		};	\
		static _##classname##Singleton localname;

#endif

#define EXPORTED_INTERFACE( interfacename, classname)							\
	static classname s_##classname;												\
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;		\
	IEXPORTS interfacename *Get##classname( void ) {return g_p##interfacename;}	\

#define EXPORTED_INTERFACE_FUNCTION( interfacename, classname, funcname)		\
	static classname s_##classname;												\
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;		\
	IEXPORTS interfacename *funcname( void ) {return g_p##interfacename;}

#define _INTERFACE_FUNCTION( interfacename, classname, funcname)		\
	static classname s_##classname;												\
	interfacename* g_p##interfacename = ( interfacename * )&s_##classname;		\
	interfacename *funcname( void ) {return g_p##interfacename;}


#endif //INTERFACEMANAGER_H
