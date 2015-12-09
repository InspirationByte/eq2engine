//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game monkey script for Equilibrium game
//////////////////////////////////////////////////////////////////////////////////

#include "EqGMS.h"
#include "DebugInterface.h"
#include "IDebugOverlay.h"

#include "EntityBind.h"
#include "AI/AIScriptTask.h"

// create
CEqScriptSystem* g_pScriptSys = NULL;

CEqScriptSystem* GetScriptSys()
{
	if(!g_pScriptSys)
	{
		g_pScriptSys = new CEqScriptSystem();
	}

	return g_pScriptSys;
}

// convars and commands

DECLARE_CMD(gms_exec, "Executes GM script", CV_CHEAT)
{
	if(CMD_ARGC > 0)
	{
		EqString cmdStr;
		CMD_ARG_CONCAT(cmdStr);

		g_pScriptSys->ExecuteString( (cmdStr + ';').GetData(), NULL, true );
	}
}

ConVar gms_disable("gms_disable", "0", "Disables script execution", CV_CHEAT);

//----------------------------------------------------------------------------------------

EQGMS_DEFINE_GLOBALFUNC(module)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_STRING_PARAM(a_string, 0);

	GetScriptSys()->ExecuteFile(a_string, NULL, true);

	return GM_OK;
}

void GM_CDECL EqPrint(gmMachine * a_machine, const char * a_string)
{
	Msg(a_string);
}

//----------------------------------------------------------------------------------------

CEqScriptSystem::CEqScriptSystem()
{
	m_pMachine = new gmMachine();
	m_pMachine->SetDebugMode(true);

	m_bInit = false;
}

void CEqScriptSystem::Init()
{
	if(m_bInit)
		return;

	m_bInit = true;

	gmMachine::s_printCallback = EqPrint;

	gmBindMathLib(m_pMachine);
	gmBindStringLib(m_pMachine);
	gmBindVector3Lib(m_pMachine);

	// more game-specific
	gmBindEntityLib(m_pMachine);
	gmBindAITaskLib(m_pMachine);

	// make some things
	ExecuteFile("scripts/init.gms", NULL, true);
}

void CEqScriptSystem::Shutdown()
{
	delete m_pMachine;
}

void CEqScriptSystem::RegisterLibrary(const char* a_asTable)
{
	// not so needed
}

void CEqScriptSystem::RegisterLibraryFunction(const char * a_name, gmCFunction a_function, const char * a_asTable)
{
	CScopedMutex m(m_Mutex);

	m_pMachine->RegisterLibraryFunction(a_name, a_function, a_asTable);
}


void CEqScriptSystem::ExecuteString(const char* pszStr, gmUserObject* pObject, bool bInstantExecute)
{
	CScopedMutex m(m_Mutex);

	gmVariable gmThis;

	if(pObject)
		gmThis.SetUser(pObject);
	else
		gmThis.Nullify();

	int nThread = GM_INVALID_THREAD;
	int errors = m_pMachine->ExecuteString( pszStr, &nThread, bInstantExecute, NULL, &gmThis );

	if(errors > 0)
	{
		// log all execution errors
		bool first = true;
		const char * message;

		while((message = m_pMachine->GetLog().GetEntry(first))) 
		{
			MsgError("%s", message);
		}

		m_pMachine->GetLog().Reset();
	}
}

void CEqScriptSystem::ExecuteFile(const char* pszFileName, gmUserObject* pObject, bool bInstantExecute)
{
	CScopedMutex m(m_Mutex);
	//Msg("Loading and executing script '%s'...\n", pszFileName);

	char* pszFileData = GetFileSystem()->GetFileBuffer(pszFileName, NULL, SP_MOD);

	if(!pszFileData)
	{
		MsgError("Script file '%s' not found!\n", pszFileName);
		return;
	}

	gmVariable gmThis;

	if(pObject)
		gmThis.SetUser(pObject);
	else
		gmThis.Nullify();

	int nThread = GM_INVALID_THREAD;
	int errors = m_pMachine->ExecuteString( pszFileData, &nThread, bInstantExecute, pszFileName, &gmThis);

	if(errors > 0)
	{
		// log all execution errors
		bool first = true;
		const char * message;

		while((message = m_pMachine->GetLog().GetEntry(first))) 
		{
			MsgError("%s: %s", pszFileName, message);
		}

		m_pMachine->GetLog().Reset();
	}

	PPFree(pszFileData);
}

void CEqScriptSystem::Update(float fDt)
{
	if(gms_disable.GetBool())
		return;

	CScopedMutex m(m_Mutex);

	int nExecutionThreads = m_pMachine->Execute((uint32)fDt * 1000);

	// log all execution errors
	bool first = true;
	const char * message;
    
	while((message = m_pMachine->GetLog().GetEntry(first))) 
	{
		MsgError("%s", message);
	}

	m_pMachine->GetLog().Reset();

	debugoverlay->Text(ColorRGBA(1,0.5,1,1), "Eq script system threads: %d", nExecutionThreads);
}

gmTableObject* CEqScriptSystem::AllocTableObject()
{
	gmTableObject* pObject = m_pMachine->AllocTableObject();

	m_pMachine->AddCPPOwnedGMObject(pObject);

	return pObject;
}

void CEqScriptSystem::FreeTableObject(gmTableObject* pObject)
{
	if(!pObject)
		return;

	m_pMachine->RemoveCPPOwnedGMObject(pObject);
}

gmUserObject* CEqScriptSystem::AllocUserObject(void* pUserObj, gmType type)
{
	gmUserObject* pObject = m_pMachine->AllocUserObject(pUserObj, type);

	m_pMachine->AddCPPOwnedGMObject(pObject);

	return pObject;
}

void CEqScriptSystem::FreeUserObject(gmUserObject* pObject)
{
	if(!pObject)
		return;

	m_pMachine->RemoveCPPOwnedGMObject(pObject);
}

gmMachine* CEqScriptSystem::GetMachine()
{
	return m_pMachine;
}