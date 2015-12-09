//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game monkey script for Equilibrium game
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQGMS_H
#define EQGMS_H

#include "gmthread.h"
#include "gmCall.h"
#include "gmScript/binds/gmMathLib.h"
#include "gmScript/binds/gmStringLib.h"
#include "gmScript/binds/gmVector3Lib.h"

#include "Utils/eqthread.h"

#include "EntityBind.h"

#ifdef GetObject
#undef GetObject
#endif // GetObject

using namespace Threading;

class BaseEntity;

class CEqScriptSystem
{
public:
	CEqScriptSystem();

	void			Init();
	void			Shutdown();

	// executes string, Use CEqScriptObject as "THIS"
	void			ExecuteString(const char* pszStr, gmUserObject* pObject = NULL, bool bInstantExecute = false);

	// executes script file, Use CEqScriptObject as "THIS"
	void			ExecuteFile(const char* pszFileName, gmUserObject* pObject = NULL, bool bInstantExecute = false);

	void			Update(float fDt);

	void			RegisterLibrary(const char* a_asTable = NULL);
	void			RegisterLibraryFunction(const char * a_name, gmCFunction a_function, const char * a_asTable);

	gmTableObject*	AllocTableObject();
	void			FreeTableObject(gmTableObject* pObject);

	gmUserObject*	AllocUserObject(void* pUserObj, gmType type);
	void			FreeUserObject(gmUserObject* pObject);

	gmMachine*		GetMachine();

protected:
	gmMachine*		m_pMachine;

	bool			m_bInit;

	CEqMutex		m_Mutex;
};

CEqScriptSystem* GetScriptSys();

#define EQGMS_DEFINE_LIBFUNC(libname, name)										\
	int GM_CDECL EQGMS_##libname##_##name##(gmThread* a_thread);				\
	class CEqGMS##libname##name##_Registrator									\
	{																			\
	public:																		\
		CEqGMS##libname##name##_Registrator()									\
		{																		\
			GetScriptSys()->RegisterLibraryFunction(#name, EQGMS_##libname##_##name##, #libname); \
		}																		\
	};																			\
	static CEqGMS##libname##name##_Registrator s_##libname##_##name##_registrator;	\
	int GM_CDECL EQGMS_##libname##_##name##(gmThread* a_thread)

#define EQGMS_DEFINE_GLOBALFUNC(name)										\
	int GM_CDECL EQGMSGlobal##name##(gmThread* a_thread);				\
	class CEqGMSGlobal##name##_Registrator									\
	{																			\
	public:																		\
		CEqGMSGlobal##name##_Registrator()									\
		{																		\
			GetScriptSys()->RegisterLibraryFunction(#name, EQGMSGlobal##name##, NULL); \
		}																		\
	};																			\
	static CEqGMSGlobal##name##_Registrator s_global_##name##_registrator;	\
	int GM_CDECL EQGMSGlobal##name##(gmThread* a_thread)

#define EQGMS_REGISTER_FUNCTION(table, name, func)												\
	{																							\
		gmFunctionObject* funcObj = GetScriptSys()->GetMachine()->AllocFunctionObject(func);	\
		table->Set(GetScriptSys()->GetMachine(), name, gmVariable(GM_FUNCTION, (gmptr)funcObj));\
	}

#define EQGMS_DEFINE_SCRIPTOBJECT(type)		\
	public:									\
	gmTableObject* GetScriptTableObject() { return m_pTableObject; } \
	gmUserObject* GetScriptUserObject() { if(m_pUserObject == NULL){m_pUserObject = GetScriptSys()->AllocUserObject(this, type);} return m_pUserObject;} \
	protected:								\
	gmTableObject*		m_pTableObject;		\
	gmUserObject*		m_pUserObject;

#define EQGMS_INIT_OBJECT() m_pTableObject = GetScriptSys()->AllocTableObject(); m_pUserObject = NULL;

#define OBJ_BEGINCALL(name)  {gmCall call;	\
	if(call.BeginTableFunction(GetScriptSys()->GetMachine(), name, GetScriptTableObject(), gmVariable(GetScriptUserObject()), false))

#define OBJ_CALLDONE }

#define OBJ_BINDSCRIPT(name, obj) GetScriptSys()->ExecuteFile(name, obj->GetScriptUserObject(), true)

#endif // EQGMS_H