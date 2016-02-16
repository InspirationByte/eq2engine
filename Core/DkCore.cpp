//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2008-2014
//////////////////////////////////////////////////////////////////////////////////
// NOTENOTE: Linux does not showing russian language that was written in VC
//////////////////////////////////////////////////////////////////////////////////


#include <ctime>

#ifdef _WIN32
#include <direct.h>
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

#ifdef LINUX
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

#endif

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
    if (GetUserName(buffer,&size)==0)
        return "nouser";

    return buffer;
#else
	// TODO: linux user name
    return "linuxuser";
#endif
}

ConVar *c_SupressAccessorMessages = NULL;

void cc_exec_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgWarning("Example: exec <filename> [command lookup] - executes configuration file\n");
        return;
    }

    g_sysConsole->ClearCommandBuffer();

	if(args->numElem() > 1)
		g_sysConsole->ParseFileToCommandBuffer((char*)(args->ptr()[0]).GetData(),args->ptr()[1].GetData());
	else
		g_sysConsole->ParseFileToCommandBuffer((char*)(args->ptr()[0]).GetData());

	((CConsoleCommands*)g_sysConsole.GetInstancePtr())->EnableInitOnlyVarsChangeProtection(false);
	g_sysConsole->ExecuteCommandBuffer();
}

void cc_set_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgError("No command and arguments for set.\n");
        return;
    }

    ConVar *pConVar = (ConVar*)g_sysConsole->FindCvar(args->ptr()[0].GetData());

    if (!pConVar)
    {
        MsgError("Unknown variable '%s'\n",args->ptr()[0].GetData());
        return;
    }

    EqString pArgs;
	char tmp_path[2048];
    for (int i = 1; i < args->numElem();i++)
    {
		sprintf(tmp_path, i < args->numElem() ? (char*) "%s " : (char*) "%s" , args->ptr()[i].GetData());

        pArgs.Append(tmp_path);
    }

    if (pArgs.GetLength() > 0 && !(pConVar->GetFlags() & CV_CHEAT) && !(pConVar->GetFlags() & CV_INITONLY))
    {
        pConVar->SetValue( pArgs.GetData() );
        MsgWarning("%s set to %s\n",pConVar->GetName(), pArgs.GetData());
    }
}

void cc_toggle_f(DkList<EqString>* args)
{
    if (args->numElem() == 0)
    {
        MsgError("No command and arguments for toggle.\n");
        return;
    }

    ConVar *pConVar = (ConVar*)g_sysConsole->FindCvar(args->ptr()[0].GetData());

    if (!pConVar)
    {
        MsgError("Unknown variable '%s'\n",args->ptr()[0].GetData());
        return;
    }

	bool nValue = pConVar->GetBool();

    if (!(pConVar->GetFlags() & CV_CHEAT) && !(pConVar->GetFlags() & CV_INITONLY))
    {
        pConVar->SetBool(!nValue);
        MsgWarning("%s = %s\n",pConVar->GetName(),pConVar->GetString());
    }
}

ConCommand *c_toggle;
ConCommand *c_exec;
ConCommand *c_set;
extern ConCommand c_developer;
extern ConCommand c_echo;

bool bDoLogs = false;
bool bLoggingInitialized = false;

void cc_enable_logging(DkList<EqString>* args)
{
#ifdef LINUX
	mkdir("logs",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#else
	mkdir("logs");
#endif // LINUX
	bDoLogs = true;
}

ConCommand *c_enable_log;

void cc_disable_logging(DkList<EqString>* args)
{
	bDoLogs = false;
}

void cc_addpackage(DkList<EqString>* args)
{
	if(args->numElem())
		g_fileSystem->AddPackage(args->ptr()[0].GetData(), SP_ROOT);
	else
		MsgWarning("Usage: fs_addpackage <package name>\n");
}

ConCommand *c_disable_log;
ConCommand *c_addpackage;

void PPMemInit();
void PPMemShutdown();

extern void cc_developer_f( DkList<EqString>* args );

// Definition that we can't see or change throught console
bool CDkCore::Init(const char* pszApplicationName, const char* pszCommandLine)
{
#ifdef _WIN32
	setlocale(LC_ALL,"C");
#endif // _WIN32

	InitSubInterfaces();

    ASSERT(strlen(pszApplicationName) > 0);

	// Командная строка
    if (pszCommandLine && strlen(pszCommandLine) > 0)
	{
        g_cmdLine->Init(pszCommandLine);
	}

	int nNoLogIndex = g_cmdLine->FindArgument("-nolog");

	if(nNoLogIndex != -1)
		bDoLogs = false;

	int nLogIndex = g_cmdLine->FindArgument("-log");

	if(nLogIndex != -1)
		bDoLogs = true;

	int nWorkdirIndex = g_cmdLine->FindArgument("-workdir");

	if(nWorkdirIndex != -1)
	{
		Msg("Setting working directory to %s\n", g_cmdLine->GetArgumentsOf(nWorkdirIndex));
#ifdef _WIN32

		SetCurrentDirectory( g_cmdLine->GetArgumentsOf(nWorkdirIndex) );
#else

#endif // _WIN32
	}

    m_szApplicationName = pszApplicationName;
    m_szCurrentSessionUserName = UTIL_GetUserName();

	bLoggingInitialized = true;

	if(!m_coreConfiguration.LoadFromFile("EQ.CONFIG"))
	{
		//Msg("skip: can't open 'eq.config'\n");

		// try create default settings
		kvkeybase_t* appDebug = m_coreConfiguration.GetRootSection()->AddKeyBase("ApplicationDebug");
		appDebug->AddKeyBase("ForceLogApplications", pszApplicationName);

		kvkeybase_t* fsSection = m_coreConfiguration.GetRootSection()->AddKeyBase("FileSystem");

		fsSection->AddKeyBase("EngineDataDir", "EqBase");
		fsSection->AddKeyBase("DefaultGameDir", "GameData");

		kvkeybase_t* regionalConfig = m_coreConfiguration.GetRootSection()->AddKeyBase("RegionalSettings");
		regionalConfig->AddKeyBase("DefaultLanguage", "English");
	}

	kvkeybase_t* pAppDebug = m_coreConfiguration.FindKeyBase("ApplicationDebug", KV_FLAG_SECTION);
	if(pAppDebug)
	{
		if(pAppDebug->FindKeyBase("ForceEnableLog", KV_FLAG_NOVALUE))
			bDoLogs = true;

		if(pAppDebug->FindKeyBase("PrintLeaksOnExit", KV_FLAG_NOVALUE))
			g_bPrintLeaksOnShutdown = true;

		DkList<EqString> devModeList;

		kvkeybase_t* devModesKv = pAppDebug->FindKeyBase("DeveloperMode");
		if(devModesKv)
		{
			for(int i = 0; i < devModesKv->values.numElem();i++)
				devModeList.append(devModesKv->values[i]);

			if(devModeList.numElem())
				cc_developer_f( &devModeList );
		}

		kvkeybase_t* pForceLogged = pAppDebug->FindKeyBase("ForceLogApplications");
		if(pForceLogged)
		{
			for(int i = 0; i < pForceLogged->values.numElem();i++)
			{
				if(!stricmp(pForceLogged->values[i], pszApplicationName))
				{
					bDoLogs = true;
					break;
				}
			}
		}
	}

	if(bDoLogs)
	{
#ifdef _WIN32
		mkdir("logs");
#else
		mkdir("logs",S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
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
    CEqCPUCaps* cpuCaps = (CEqCPUCaps*)g_cpuCaps.GetInstancePtr();
    cpuCaps->Init();

    // Initialize time and query perfomance
    Platform_InitTime();

    // Регистрация некоторых комманд.
    g_sysConsole->RegisterCommand(&cmd_info);
	g_sysConsole->RegisterCommand(&cmd_coreversion);
	g_sysConsole->RegisterCommand(&c_developer);
	g_sysConsole->RegisterCommand(&c_echo);

    // Регистрация некоторых комманд.
    ((CConsoleCommands*)g_sysConsole.GetInstancePtr())->RegisterCommands();

    c_exec = new ConCommand("exec",cc_exec_f,"Execute configuration file");
    c_set = new ConCommand("set",cc_set_f,"Set ConVar value");
	c_toggle = new ConCommand("togglevar",cc_toggle_f,"Toggles ConVar value");

	c_enable_log = new ConCommand("enablelog",cc_enable_logging,"Enable logging");
    c_disable_log = new ConCommand("disablelog",cc_disable_logging,"Disable logging");
	c_addpackage = new ConCommand("fs_addpackage",cc_addpackage,"Add packages");
    c_SupressAccessorMessages = new ConVar("c_SupressAccessorMessages","1","Supress command/variable accessing. Dispays errors only",CV_ARCHIVE);

    // Install exception handler
    if (g_cmdLine->FindArgument("-nocrashdump") == -1)
        InstallExceptionHandler();

	if(bDoLogs)
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
	// always write log for tools (if -nolog is not specified)
	bDoLogs = true;

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
	RegisterInterface( FILESYSTEM_INTERFACE_VERSION, GetCFileSystem());
	RegisterInterface( LOCALIZER_INTERFACE_VERSION , GetCLocalize());
	RegisterInterface( CPUSERVICES_INTERFACE_VERSION , GetCEqCPUCaps());
}

KeyValues* CDkCore::GetConfig()
{
	return &m_coreConfiguration;
}

void nullspew(SpewType_t type,const char* pMsg) {}

void CDkCore::Shutdown()
{
    if (!m_bInitialized)
        return;

	// remove interface list
	m_interfaces.clear();

    // Никакого spew'а
    SetSpewFunction(nullspew);

    ((CConsoleCommands*)g_sysConsole.GetInstancePtr())->DeInit();

    g_cmdLine->DeInit();

#ifndef CORE_FINAL_RELEASE

	if(g_bPrintLeaksOnShutdown)
		PPMemInfo();

#endif

    m_bInitialized = false;

	// shutdown memory
	PPMemShutdown();

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
