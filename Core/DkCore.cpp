//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// NOTENOTE: Linux does not showing russian language that was written in VC
//////////////////////////////////////////////////////////////////////////////////


#include <ctime>

#ifdef _WIN32
#include <locale.h>
#include <crtdbg.h>
#endif

#include "DkCore.h"
#include "CoreVersion.h"
#include "DebugInterface.h"
#include "ExceptionHandler.h"
#include "ConCommandFactory.h"

#include "eqCPUServices.h"

#include "utils/eqstring.h"

#ifdef PLAT_POSIX
#include <time.h>
#else

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    //Only set debug info when connecting dll
#ifdef _DEBUG
    int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
    flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
    flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
    flag |= _CRTDBG_ALLOC_MEM_DF;
    _CrtSetDbgFlag(flag); // Set flag to the new value
    _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
#endif

    return TRUE;
}

#endif // PLAT_POSIX

//------------------------------------------------------------------------------------------

bool g_bPrintLeaksOnShutdown = false;

// Экспорт интерфейса ядра
EXPORTED_INTERFACE_FUNCTION(IDkCore, CDkCore, GetCore)

IEXPORTS void*	_GetDkCoreInterface(const char* pszName)
{
	// GetCore() потому-что может быть вызвано раньше
	return GetCore()->GetInterface(pszName);
}

// Это незарегистрированные команды
extern ConCommand cmd_info;
extern ConCommand cmd_coreversion;

// callback при выходе (Разгрузке DLL)
void DkCore_onExit( void )
{
	g_pIDkCore->Shutdown();
}

EqString UTIL_GetUserName()
{
#ifdef _WIN32
    char buffer[257];

    DWORD size;
    size=sizeof(buffer);
    if (GetUserNameA(buffer,&size)==0)
        return "nouser";

    return buffer;
#else
	// TODO: linux user name
    return "";
#endif
}

ConVar *c_SupressAccessorMessages = NULL;

extern ConCommand c_developer;
extern ConCommand c_echo;

extern void Log_Init();
extern void Log_Flush();
extern void Log_Close();

extern bool g_bLoggingInitialized;

DECLARE_CONCOMMAND_FN(log_enable)
{
	Log_Init();
}

DECLARE_CONCOMMAND_FN(log_disable)
{
	Log_Close();
}

DECLARE_CONCOMMAND_FN(log_flush)
{
	Log_Flush();
}

DECLARE_CONCOMMAND_FN(addpackage)
{
	if(CMD_ARGC)
		g_fileSystem->AddPackage(CMD_ARGV(0).c_str(), SP_ROOT);
	else
		MsgWarning("Usage: fs_addpackage <package name>\n");
}

ConCommand* c_log_enable;
ConCommand* c_log_disable;
ConCommand* c_log_flush;

ConCommand* c_addpackage;

void PPMemInit();
void PPMemShutdown();
void InitMessageBoxPlatform();

extern DECLARE_CONCOMMAND_FN(developer);

CDkCore::CDkCore()
{
	m_coreConfiguration = NULL;
}

// Definition that we can't see or change throught console
bool CDkCore::Init(const char* pszApplicationName, const char* pszCommandLine)
{
#ifdef _WIN32
	setlocale(LC_ALL,"C");
	InitMessageBoxPlatform();
#endif // _WIN32

	// Командная строка
    if (pszCommandLine && strlen(pszCommandLine) > 0)
        g_cmdLine->Init(pszCommandLine);

	InitSubInterfaces();

    ASSERT(strlen(pszApplicationName) > 0);

	int nWorkdirIndex = g_cmdLine->FindArgument("-workdir");

	if(nWorkdirIndex != -1)
	{
		Msg("Setting working directory to %s\n", g_cmdLine->GetArgumentsOf(nWorkdirIndex));
#ifdef _WIN32

		SetCurrentDirectoryA( g_cmdLine->GetArgumentsOf(nWorkdirIndex) );
#else

#endif // _WIN32
	}

	m_szApplicationName = pszApplicationName;
	m_szCurrentSessionUserName = UTIL_GetUserName();

	m_coreConfiguration = new KeyValues();
	kvkeybase_t* coreConfigRoot = m_coreConfiguration->GetRootSection();

	if(!m_coreConfiguration->LoadFromFile("EQ.CONFIG", SP_ROOT))
	{
		// try create default settings
		kvkeybase_t* appDebug = coreConfigRoot->AddKeyBase("ApplicationDebug");
		appDebug->SetKey("ForceLogApplications", pszApplicationName);

		kvkeybase_t* fsSection = coreConfigRoot->AddKeyBase("FileSystem");

		fsSection->SetKey("EngineDataDir", "EqBase");
		fsSection->SetKey("DefaultGameDir", "GameData");

		kvkeybase_t* regionalConfig = coreConfigRoot->AddKeyBase("RegionalSettings");
		regionalConfig->SetKey("DefaultLanguage", "English");
	}

	bool logEnabled = false;

	kvkeybase_t* pAppDebug = coreConfigRoot->FindKeyBase("ApplicationDebug", KV_FLAG_SECTION);
	if(pAppDebug)
	{
		if(pAppDebug->FindKeyBase("ForceEnableLog", KV_FLAG_NOVALUE))
			logEnabled = true;

		if(pAppDebug->FindKeyBase("PrintLeaksOnExit", KV_FLAG_NOVALUE))
			g_bPrintLeaksOnShutdown = true;

		DkList<EqString> devModeList;

		kvkeybase_t* devModesKv = pAppDebug->FindKeyBase("DeveloperMode");
		if(devModesKv)
		{
			for(int i = 0; i < devModesKv->values.numElem();i++)
				devModeList.append(KV_GetValueString(devModesKv, i));

			if(devModeList.numElem())
				CONCOMMAND_FN(developer)( nullptr, devModeList );
		}

		kvkeybase_t* pForceLogged = pAppDebug->FindKeyBase("ForceLogApplications");
		if(pForceLogged)
		{
			for(int i = 0; i < pForceLogged->values.numElem();i++)
			{
				if(!stricmp(KV_GetValueString(pForceLogged, i), m_szApplicationName.c_str()))
				{
					logEnabled = true;
					break;
				}
			}
		}
	}

	int nNoLogIndex = g_cmdLine->FindArgument("-nolog");

	if(nNoLogIndex != -1)
		logEnabled = false;

	int nLogIndex = g_cmdLine->FindArgument("-log");

	if(nLogIndex != -1)
		logEnabled = true;

	g_bLoggingInitialized = true;

	if(logEnabled)
	{
		Log_Init();
	}

	//Remove log files
	remove("logs/Assert.log");

	char tmp_path[2048];
	sprintf(tmp_path, "logs/%s_%s.log", m_szApplicationName.GetData(), m_szCurrentSessionUserName.GetData());

	remove(tmp_path);

	// Reset counter of same commands
	g_sysConsole->ResetCounter();

	// Show core message
	CoreMessage();

	// Инициализировать CPU
	CEqCPUCaps* cpuCaps = (CEqCPUCaps*)g_cpuCaps;//(CEqCPUCaps*)g_cpuCaps.GetInstancePtr();
	cpuCaps->Init();

	// Initialize time and query perfomance
	Platform_InitTime();

	// Регистрация некоторых комманд.
	g_sysConsole->RegisterCommand(&cmd_info);
	g_sysConsole->RegisterCommand(&cmd_coreversion);
	g_sysConsole->RegisterCommand(&c_developer);
	g_sysConsole->RegisterCommand(&c_echo);

	// Регистрация некоторых комманд.
	((CConsoleCommands*)g_sysConsole)->RegisterCommands();

	c_log_enable = new ConCommand("log_enable",CONCOMMAND_FN(log_enable));
	c_log_disable = new ConCommand("log_disable",CONCOMMAND_FN(log_disable));
	c_log_flush = new ConCommand("log_flush",CONCOMMAND_FN(log_flush));

	c_addpackage = new ConCommand("fs_addpackage",CONCOMMAND_FN(addpackage),"Add packages");
	c_SupressAccessorMessages = new ConVar("c_SupressAccessorMessages","1","Supress command/variable accessing. Dispays errors only",CV_ARCHIVE);

	// Install exception handler
	if (g_cmdLine->FindArgument("-nocrashdump") == -1)
		InstallExceptionHandler();

	if(logEnabled)
		MsgAccept("\nCore: Logging console output to file is enabled.\n");
	else
		MsgError("\nCore: Logging console output to file is disabled.\n");

	// Установка статуса
	m_bInitialized = true;

	Msg("\n");

	return true;
}

bool CDkCore::Init(const char* pszApplicationName,int argc, char **argv)
{
	static EqString strCmdLine;
	strCmdLine.Empty();

	char tmp_path[2048];

    // Append arguments
    for (int i = 0; i < argc; i++)
    {
		sprintf(tmp_path, "%s ",argv[i]);

        strCmdLine.Append(tmp_path);
    }

    return Init(pszApplicationName, (char*)strCmdLine.GetData());
}

void CDkCore::InitSubInterfaces()
{
	// init memory first
	PPMemInit();

	// register core interfaces
	RegisterInterface( CMDLINE_INTERFACE_VERSION, GetCommandLineParse());
	RegisterInterface( LOCALIZER_INTERFACE_VERSION , GetCLocalize());
	RegisterInterface( CPUSERVICES_INTERFACE_VERSION , GetCEqCPUCaps());
}

KeyValues* CDkCore::GetConfig()
{
	return m_coreConfiguration;
}

void nullspew(SpewType_t type,const char* pMsg) {}

void CDkCore::Shutdown()
{
    if (!m_bInitialized)
        return;

	g_fileSystem->Shutdown();

	delete m_coreConfiguration;
	m_coreConfiguration = NULL;

	// remove interface list
	m_interfaces.clear();

    // Никакого spew'а
    SetSpewFunction(nullspew);

    ((CConsoleCommands*)g_sysConsole)->DeInit();

    g_cmdLine->DeInit();

#ifndef CORE_FINAL_RELEASE

	if(g_bPrintLeaksOnShutdown)
		PPMemInfo();

#endif

    m_bInitialized = false;

    Msg("\n*Destroying core...\n");

#ifdef _WIN32
    char date[10];
    char time[10];

    _strdate_s(date);
    _strtime_s(time);
    Msg("===================================\nLog closed: %s %s\n",date,time);
#else
    char datetime[80];

    time_t rawtime;
    struct tm* tminfo;
    char buffer[80];

    time( &rawtime );

    tminfo = localtime( &rawtime );

    strftime(datetime, 80, "%x %H:%M", tminfo);
    Msg("===================================\nLog closed: %s\n",datetime);
#endif

	// shutdown memory
	PPMemShutdown();

	Log_Close();
}

char* CDkCore::GetApplicationName()
{
    return (char*)m_szApplicationName.GetData();
}

char* CDkCore::GetCurrentUserName()
{
    return (char*)m_szCurrentSessionUserName.GetData();
}

// Interface management for engine

void CDkCore::RegisterInterface(const char* pszName, ICoreModuleInterface* ifPtr)
{
	//MsgInfo("Registering interface '%s'...\n", pszName);

	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!strcmp(m_interfaces[i].name, pszName))
			ASSERTMSG(false, varargs("Core interface module \"%s\" is already registered.", pszName));
	}

	coreInterface_t iface;
	iface.name = pszName;
	iface.ptr = ifPtr;

	m_interfaces.append(iface);
}

ICoreModuleInterface* CDkCore::GetInterface(const char* pszName)
{
	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!stricmp(m_interfaces[i].name, pszName))
			return m_interfaces[i].ptr;
	}

	return NULL;
}

void CDkCore::UnregisterInterface(const char* pszName)
{
	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!stricmp(m_interfaces[i].name, pszName))
		{
			m_interfaces.fastRemoveIndex(i);
			return;
		}
	}
}
