//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// NOTENOTE: Linux does not showing russian language that was written in VC
//////////////////////////////////////////////////////////////////////////////////


#include <ctime>

#ifdef _WIN32
#include <locale.h>
#ifdef CRT_DEBUG_ENABLED
#include <crtdbg.h>
#endif
#endif

#include "eqCore.h"

#include "core/IFileSystem.h"
#include "core/InterfaceManager.h"
#include "core/ILocalize.h"
#include "core/DebugInterface.h"
#include "eqCPUServices.h"

#include "ExceptionHandler.h"
#include "ConsoleCommands.h"

#include "ds/eqstring.h"

#ifdef PLAT_POSIX
#include <time.h>
#else

BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
    //Only set debug info when connecting dll
#ifdef CRT_DEBUG_ENABLED
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
	IDkCore* core = GetCore();

	// Filesystem is required by mobile port
	if(!strcmp(pszName, FILESYSTEM_INTERFACE_VERSION))
	{
		if (!core->GetInterface(FILESYSTEM_INTERFACE_VERSION))
			core->RegisterInterface(FILESYSTEM_INTERFACE_VERSION, g_fileSystem);
	}

	return core->GetInterface(pszName);
}

// callback при выходе (Разгрузке DLL)
void DkCore_onExit( void )
{
	GetCore()->Shutdown();
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

ConCommand* c_log_enable;
ConCommand* c_log_disable;
ConCommand* c_log_flush;

void PPMemInit();
void PPMemShutdown();
void InitMessageBoxPlatform();

extern DECLARE_CONCOMMAND_FN(developer);

CDkCore::CDkCore()
{
	m_coreConfiguration = NULL;
}

//-----------------------------------------------------------------------------
// Returns the directory where this .exe is running from
//-----------------------------------------------------------------------------
static char* GetBaseDir(const char* pszBuffer)
{
	static char	basedir[MAX_PATH];
	char szBuffer[MAX_PATH];
	int j;
	char* pBuffer = NULL;

	strcpy(szBuffer, pszBuffer);

	pBuffer = strrchr(szBuffer, '\\');
	if (pBuffer)
	{
		*(pBuffer + 1) = '\0';
	}

	strcpy(basedir, szBuffer);

	j = strlen(basedir);
	if (j > 0)
	{
		if ((basedir[j - 1] == '\\') ||
			(basedir[j - 1] == '/'))
		{
			basedir[j - 1] = 0;
		}
	}

	return basedir;
}

static void SetupBinPath()
{
	const char* pPath = getenv("PATH");

	char szBuffer[4096];
	memset(szBuffer, 0, sizeof(szBuffer));

	char moduleName[MAX_PATH];
#ifdef _WIN32
	if (!GetModuleFileNameA(NULL, moduleName, MAX_PATH))
		return;

	// Get the root directory the .exe is in
	char* pRootDir = GetBaseDir(moduleName);
	sprintf(szBuffer, "PATH=%s;%s", pRootDir, pPath);
	_putenv(szBuffer);
#elif defined(__ANDROID__)
	// do nothing? TODO...
#else
	// TODO: POSIX implementation
	static_assert(false);
#endif

}

// Definition that we can't see or change throught console
bool CDkCore::Init(const char* pszApplicationName, const char* pszCommandLine)
{
#ifdef _WIN32
	setlocale(LC_ALL,"C");
	InitMessageBoxPlatform();
#endif // _WIN32

	SetupBinPath();

	// Командная строка
    if (pszCommandLine && strlen(pszCommandLine) > 0)
        g_cmdLine->Init(pszCommandLine);

	InitSubInterfaces();

    ASSERT(strlen(pszApplicationName) > 0);

	const int nWorkdirIndex = g_cmdLine->FindArgument("-workdir");
	const char* newWorkDir = g_cmdLine->GetArgumentsOf(nWorkdirIndex);
	if(newWorkDir)
	{
		Msg("Setting working directory to %s\n", newWorkDir);
#ifdef _WIN32
		SetCurrentDirectoryA(newWorkDir);
#elif defined(__ANDROID__)
		// do nothing? TODO...
#else
		chdir(newWorkDir);
#endif // _WIN32
	}

	m_szApplicationName = pszApplicationName;

	m_coreConfiguration = PPNew KeyValues();
	kvkeybase_t* coreConfigRoot = m_coreConfiguration->GetRootSection();

	// try different locations of EQ.CONFIG
	bool found = m_coreConfiguration->LoadFromFile("EQ.CONFIG", SP_ROOT);
	if (!found)
		found = m_coreConfiguration->LoadFromFile("../EQ.CONFIG", SP_ROOT);

	if(!found)
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

		Array<EqString> devModeList{ PP_SL };

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
				if(!stricmp(KV_GetValueString(pForceLogged, i), m_szApplicationName.ToCString()))
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
	sprintf(tmp_path, "logs/%s.log", m_szApplicationName.GetData());

	remove(tmp_path);

	// Reset counter of same commands
	g_consoleCommands->ResetCounter();

	// Show core message
	MsgAccept("Equilibrium 2 - %s %s\n", __DATE__, __TIME__);

	// Инициализировать CPU
	CEqCPUCaps* cpuCaps = (CEqCPUCaps*)g_cpuCaps;
	cpuCaps->Init();

	// Регистрация некоторых комманд.
	g_consoleCommands->RegisterCommand(&c_developer);
	g_consoleCommands->RegisterCommand(&c_echo);

	// Регистрация некоторых комманд.
	((CConsoleCommands*)g_consoleCommands)->RegisterCommands();

	c_log_enable = PPNew ConCommand("log_enable",CONCOMMAND_FN(log_enable));
	c_log_disable = PPNew ConCommand("log_disable",CONCOMMAND_FN(log_disable));
	c_log_flush = PPNew ConCommand("log_flush",CONCOMMAND_FN(log_flush));

	c_SupressAccessorMessages = PPNew ConVar("c_SupressAccessorMessages","1","Supress command/variable accessing. Dispays errors only",CV_ARCHIVE);

	// Install exception handler
	if (g_cmdLine->FindArgument("-nocrashdump") == -1)
	{
		InstallExceptionHandler();
	}

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

	char tmp_str[2048];

    // Append arguments
    for (int i = 0; i < argc; i++)
    {
		bool hasSpaces = strchr(argv[i], ' ') || strchr(argv[i], '\t');
		if (hasSpaces)
			sprintf(tmp_str, "\"%s\" ", argv[i]);
		else
			sprintf(tmp_str, "%s ", argv[i]);

        strCmdLine.Append(tmp_str);
    }

    return Init(pszApplicationName, (char*)strCmdLine.GetData());
}

void CDkCore::InitSubInterfaces()
{
	// init memory first
	PPMemInit();

	// register core interfaces
	RegisterInterface( CMDLINE_INTERFACE_VERSION, GetCCommandLine());
	RegisterInterface( LOCALIZER_INTERFACE_VERSION , GetCLocalize());
	RegisterInterface( CPUSERVICES_INTERFACE_VERSION , GetCEqCPUCaps());
}

KeyValues* CDkCore::GetConfig() const
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

	// drop all modules
	//for (int i = 0; i < m_interfaces.numElem(); i++)
	//	delete m_interfaces[i].ptr;

	m_interfaces.clear();

    // Никакого spew'а
    SetSpewFunction(nullspew);

    ((CConsoleCommands*)g_consoleCommands)->DeInit();

    g_cmdLine->DeInit();

#ifndef CORE_FINAL_RELEASE

	if(g_bPrintLeaksOnShutdown)
		PPMemInfo(false);

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

char* CDkCore::GetApplicationName() const
{
    return (char*)m_szApplicationName.GetData();
}

// Interface management for engine

void CDkCore::RegisterInterface(const char* pszName, IEqCoreModule* ifPtr)
{
	//MsgInfo("Registering interface '%s'...\n", pszName);

	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!strcmp(m_interfaces[i].name, pszName))
			ASSERT_FAIL("Core interface module \"%s\" is already registered.", pszName);
	}

	coreInterface_t iface;
	iface.name = pszName;
	iface.ptr = ifPtr;

	m_interfaces.append(iface);
}

void CDkCore::AddExceptionCallback(CoreExceptionCallback callback)
{
	m_exceptionCb.append(callback);
}

void CDkCore::RemoveExceptionCallback(CoreExceptionCallback callback)
{
	m_exceptionCb.fastRemove(callback);
}

IEqCoreModule* CDkCore::GetInterface(const char* pszName) const
{
	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!strcmp(m_interfaces[i].name, pszName))
			return m_interfaces[i].ptr;
	}

	return NULL;
}

void CDkCore::UnregisterInterface(const char* pszName)
{
	for(int i = 0; i < m_interfaces.numElem(); i++)
	{
		if(!strcmp(m_interfaces[i].name, pszName))
		{
			m_interfaces.fastRemoveIndex(i);
			return;
		}
	}
}

bool CDkCore::IsInitialized() const
{
	return m_bInitialized;
}