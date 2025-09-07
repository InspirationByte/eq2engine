//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// NOTENOTE: Linux does not showing russian language that was written in VC
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <locale.h>
#include <Windows.h>
#ifdef far
#	undef far
#endif
#ifdef near
#	undef near
#endif
#ifdef CRT_DEBUG_ENABLED
#include <crtdbg.h>
#endif
#else
#include <dlfcn.h>	// dlopen()
using HMODULE = void*;
#endif

#include "core/IFileSystem.h"
#include "core/ILocalize.h"
#include "CommandLine.h"
#include "ConsoleCommands.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"
#include "utils/KeyValues.h"

#include "eqCore.h"
#include "eqCPUServices.h"
#include "ExceptionHandler.h"

#ifdef HAS_LIVEPP_SUPPORT
#include "LPP_API_x64_CPP.h"
static lpp::LppDefaultAgent s_lppAgent;
#endif // HAS_LIVEPP_SUPPORT

EXPORTED_INTERFACE(IDkCore, CDkCore)

struct OSModule
{
	EqString	name;
	HMODULE		module;
};

#ifdef PLAT_WIN
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	//Only set debug info when connecting dll
#ifdef CRT_DEBUG_ENABLED
	int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
	flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
	flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
	flag |= _CRTDBG_ALLOC_MEM_DF;
	_CrtSetDbgFlag(flag); // Set flag to the new value
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

	return TRUE;
}

#endif // PLAT_POSIX

//------------------------------------------------------------------------------------------

IEXPORTS void* _GetDkCoreInterface(const char* pszName)
{
	void* iface = g_eqCore->GetInterface(pszName);
	ASSERT_MSG(iface, "Interface %s is not registered", pszName);
	return iface;
}

extern ConCommand developer;
extern ConCommand echo;

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

//-----------------------------------------------------------------------------
// Returns the directory where this .exe is running from
//-----------------------------------------------------------------------------
static char* GetBaseDir(const char* pszBuffer)
{
	const int MAX_BASE_DIR_PATH = 512;

	static char	basedir[MAX_BASE_DIR_PATH];
	char szBuffer[MAX_BASE_DIR_PATH];
	char* pBuffer = nullptr;

	strcpy(szBuffer, pszBuffer);

	pBuffer = strrchr(szBuffer, CORRECT_PATH_SEPARATOR);
	if (pBuffer)
		*(pBuffer + 1) = '\0';

	strcpy(basedir, szBuffer);

	int j = CString::Length(basedir);
	if (j > 0)
	{
		if (basedir[j - 1] == CORRECT_PATH_SEPARATOR || basedir[j - 1] == INCORRECT_PATH_SEPARATOR)
			basedir[j - 1] = 0;
	}

	return basedir;
}

static void SetupBinPath()
{
	const char* curPathEnv = getenv("PATH");

#ifdef _WIN32
	const int MAX_BIN_DIR_PATH = 2048;

	char moduleName[MAX_BIN_DIR_PATH];
	if (!GetModuleFileNameA(nullptr, moduleName, MAX_BIN_DIR_PATH))
		return;

	// Get the root directory the .exe is in
	const char* rootDir = GetBaseDir(moduleName);
	_putenv(EqString::Format("PATH=%s;%s", rootDir, curPathEnv));

#elif defined(__ANDROID__)
	// do nothing? TODO...
#else
	// TODO: POSIX implementation
#endif
}


// Definition that we can't see or change throught console
bool CDkCore::Init(const CoreAppInitParameters& initParams)
{
	ASSERT_MSG(initParams.appName != nullptr && strlen(initParams.appName) > 0, "DkCore init: application name must be not null!");
	PPMemInit();

#ifdef _WIN32
	setlocale(LC_ALL, "C");
#endif // _WIN32

	// Assume the core is always init from first thread
	Threading::SetCurrentThreadName("Main Thread");

	SetupBinPath();

	if (initParams.commandLine.ptr())
		g_cmdLine->Init(initParams.commandLine);

	const int nWorkdirIndex = g_cmdLine->Find("-workdir");
	const char* newWorkDir = g_cmdLine->GetArgumentsOf(nWorkdirIndex);
	if (newWorkDir)
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

	m_szApplicationName = initParams.appName;

	EqString appConfigName = initParams.appConfigName ? initParams.appConfigName : "E2.CONFIG";

	// try different locations of E2.CONFIG
	bool eqConfigFound = KV_LoadFromFile(appConfigName, SP_ROOT, m_coreConfiguration);
	if (!eqConfigFound)
	{
		appConfigName = "../" + appConfigName;
		eqConfigFound = KV_LoadFromFile(appConfigName, SP_ROOT, m_coreConfiguration);
		g_fileSystem->SetBasePath(".."); // little hack
	}

	if (!eqConfigFound)
	{
		// try create default settings
		KVSection& appDebug = m_coreConfiguration.CreateSection("ApplicationDebug");
		appDebug
			.SetKey("ForceLogApplications", initParams.appName);

		KVSection& fsSection = m_coreConfiguration.CreateSection("FileSystem");

		fsSection
			.SetKey("EngineDataDir", "E2Base")
			.SetKey("DefaultGameDir", "GameData");

		KVSection& regionalConfig = m_coreConfiguration.CreateSection("RegionalSettings");
		regionalConfig
			.SetKey("DefaultLanguage", "English");
	}

	bool logEnabled = false;

	const KVSection* appDebugSec = m_coreConfiguration.FindSection("ApplicationDebug", KV_FLAG_SECTION);
	if (appDebugSec)
	{
		if (appDebugSec->FindSection("ForceEnableLog", KV_FLAG_NOVALUE))
			logEnabled = true;

		if (appDebugSec->FindSection("PrintLeaksOnExit", KV_FLAG_NOVALUE))
			m_debugSettings.printMemLeaksAtExit = true;

		if (appDebugSec->FindSection("AssertPromptInDebugger", KV_FLAG_NOVALUE))
			m_debugSettings.assertPromptInDebugger = true;

		if (appDebugSec->FindSection("CrashOnAssert", KV_FLAG_NOVALUE))
			m_debugSettings.crashOnAssert = true;

		if (appDebugSec->FindSection("FullCrashDumps", KV_FLAG_NOVALUE))
			m_debugSettings.fullCrashDumps = true;

		{
			Array<EqStringRef> devModeList(PP_SL);
			const KVSection* devModesKv = appDebugSec->FindSection("DeveloperMode");
			if (devModesKv)
			{
				for (EqStringRef mode : devModesKv->Values<EqStringRef>())
					devModeList.append(mode);

				if (devModeList.numElem())
					developer.DispatchFunc(devModeList);
			}
		}

		{
			const KVSection* forceLogApps = appDebugSec->FindSection("ForceLogApplications");
			if (forceLogApps)
			{
				for (EqStringRef appName : forceLogApps->Values<EqStringRef>())
				{
					if (!m_szApplicationName.CompareCaseIns(appName))
					{
						logEnabled = true;
						break;
					}
				}
			}
		}
	}

#ifdef HAS_LIVEPP_SUPPORT
	{
		EqString livePPPath = "Tools/LivePP";
		const KVSection* livePPSec = coreConfigRoot->FindSection("LivePP", KV_FLAG_SECTION);
		if (livePPSec)
		{
			livePPSec->Get("Enable").GetValues(m_livePPEnabled);
			livePPSec->Get("Path").GetValues(livePPPath);
		}

		if (m_livePPEnabled)
		{
			livePPPath = g_fileSystem->GetAbsolutePath(SP_ROOT, livePPPath);

			EqWString livePPPathWstr;
			AnsiUnicodeConverter(livePPPathWstr, livePPPath);

			s_lppAgent = lpp::LppCreateDefaultAgent(nullptr, livePPPathWstr);
			if (lpp::LppIsValidDefaultAgent(&s_lppAgent))
			{
				const wchar_t* appModuleName = lpp::LppGetCurrentModulePath();
				s_lppAgent.EnableModule(appModuleName, lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);
				MsgInfo("LivePlusPlus Agent is initialized, live code editing is enabled");
			}
			else
			{
				MsgWarning("Unable to initialize LivePlusPlus Agent, continuing without it...");
				m_livePPEnabled = false;
			}
		}
	}
#endif

	if (g_cmdLine->Find("-nolog") != -1)
		logEnabled = false;

	if (g_cmdLine->Find("-log") != -1)
		logEnabled = true;

	{
		remove(EqString::Format("logs/%s.log", m_szApplicationName.ToCString()));
		remove("logs/Assert.log");

		g_bLoggingInitialized = true;

		if (logEnabled)
			Log_Init();
	}

	// Reset counter of same commands
	g_consoleCommands->ResetCounter();

	// Show core message
	MsgAccept("E2Core - %s %s\n", __DATE__, __TIME__);

	CEqCPUCaps* cpuCaps = (CEqCPUCaps*)g_cpuCaps.GetInstancePtr();
	cpuCaps->Init();

	ConCommandBase::Register(&developer);
	ConCommandBase::Register(&echo);

	((CConsoleCommands*)g_consoleCommands.GetInstancePtr())->Init();

	c_log_enable = PPNew ConCommand("log_enable", CONCOMMAND_FN(log_enable));
	c_log_disable = PPNew ConCommand("log_disable", CONCOMMAND_FN(log_disable));
	c_log_flush = PPNew ConCommand("log_flush", CONCOMMAND_FN(log_flush));

	// Install exception handler
	if (g_cmdLine->Find("-nocrashdump") == -1)
		InstallExceptionHandler();

	if (logEnabled)
		MsgAccept("\nCore: Logging console output to file is enabled.\n");
	else
		MsgError("\nCore: Logging console output to file is disabled.\n");

	m_isInit = true;
	return true;
}

const KVSection& CDkCore::GetConfig() const
{
	return m_coreConfiguration;
}

void CDkCore::Shutdown()
{
	if (!m_isInit)
		return;

	m_isInit = false;

	g_fileSystem->Shutdown();

	m_coreConfiguration.Clear();
	m_interfaces.clear(true);

	SAFE_DELETE(c_log_enable);
	SAFE_DELETE(c_log_disable);
	SAFE_DELETE(c_log_flush);

	SetSpewFunction(nullptr);

	((CConsoleCommands*)g_consoleCommands.GetInstancePtr())->DeInit();
	g_cmdLine->DeInit();

#if !defined(_RETAIL)
	if (m_debugSettings.printMemLeaksAtExit)
		PPMemInfo(true);
#endif

	Msg("\n*Destroying core...\n");

	// shutdown memory
	PPMemShutdown();
	Log_Close();
	m_exceptionCb.clear(true);

	for (int i = 0; i < m_modules.numElem(); i++)
	{
		CloseModule(m_modules[i]);
		i--;
	}
}

char* CDkCore::GetApplicationName() const
{
	return (char*)m_szApplicationName.GetData();
}

// loads module
OSModule* CDkCore::OpenModule(const char* mod_name, EqString* outError)
{
	EqString moduleFileName = mod_name;
	EqString modExt = fnmPathExtractExt(moduleFileName);

#ifdef _WIN32
	// make default module extension
	if (modExt.Length() == 0)
		moduleFileName = moduleFileName + ".dll";

	HMODULE mod = LoadLibraryA(moduleFileName);
	DWORD lastErr = GetLastError();

	char err[256] = { '\0' };
	if (lastErr != 0)
	{
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr,
			lastErr,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			err,
			255,
			nullptr);
	}

#else
	// make default module extension
	if (modExt.Length() == 0)
		moduleFileName = moduleFileName + ".so";

	HMODULE mod = dlopen(moduleFileName, RTLD_LAZY | RTLD_LOCAL);
	if (!mod)
	{
		moduleFileName = "lib" + moduleFileName;
		mod = dlopen(moduleFileName, RTLD_LAZY | RTLD_LOCAL);
	}

	const char* err = dlerror();
	int lastErr = 0;
#endif // _WIN32 && MEMDLL

	if (!mod)
	{
		if (outError)
			*outError = err;

		return nullptr;
	}

	g_eqCore->OnModuleLoaded(moduleFileName);

	MsgInfo("Loaded module '%s'\n", moduleFileName.ToCString());

	OSModule* pModule = PPNew OSModule;
	pModule->name = moduleFileName;
	pModule->module = (HMODULE)mod;

	return pModule;
}

// frees module
void CDkCore::CloseModule(OSModule* pModule)
{
	if (!pModule)
		return;

	g_eqCore->OnModuleUnloaded(pModule->name);

	MsgInfo("Unloaded module '%s'\n", pModule->name.ToCString());

#ifdef _WIN32
	FreeLibrary(pModule->module);
#else
	dlclose(pModule->module);
#endif // _WIN32 && MEMDLL

	delete pModule;
	m_modules.remove(pModule);
}

// returns procedure address of the loaded module
void* CDkCore::GetProcedureAddress(OSModule* pModule, const char* pszProc) const
{
#ifdef _WIN32
	return GetProcAddress(pModule->module, pszProc);
#else
	return dlsym(pModule->module, pszProc);
#endif // _WIN32 && MEMDLL
}

// Interface management for engine

void CDkCore::RegisterInterface(const char* pszName, IEqCoreModule* ifPtr)
{
	//MsgInfo("Registering interface '%s'...\n", pszName);

	for (int i = 0; i < m_interfaces.numElem(); i++)
	{
		if (!CString::Compare(m_interfaces[i].name, pszName))
			ASSERT_FAIL("Core interface module \"%s\" is already registered.", pszName);
	}

	coreInterface_t& iface = m_interfaces.append();
	iface.name = pszName;
	iface.ptr = ifPtr;
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
	for (int i = 0; i < m_interfaces.numElem(); i++)
	{
		if (!CString::Compare(m_interfaces[i].name, pszName))
			return m_interfaces[i].ptr;
	}

	return nullptr;
}

void CDkCore::UnregisterInterface(const char* pszName)
{
	for (int i = 0; i < m_interfaces.numElem(); i++)
	{
		if (!CString::Compare(m_interfaces[i].name, pszName))
		{
			m_interfaces.fastRemoveIndex(i);
			return;
		}
	}
}

bool CDkCore::IsInitialized() const
{
	return m_isInit;
}

void CDkCore::OnModuleLoaded(const char* pszName)
{
#ifdef HAS_LIVEPP_SUPPORT
	if (m_livePPEnabled)
	{
		EqWString modulePathStr;
		AnsiUnicodeConverter(modulePathStr, pszName);

		s_lppAgent.EnableModule(modulePathStr, lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);
	}
#endif
}

void CDkCore::OnModuleUnloaded(const char* pszName)
{
#ifdef HAS_LIVEPP_SUPPORT
	if (m_livePPEnabled)
	{
		EqWString modulePathStr;
		AnsiUnicodeConverter(modulePathStr, pszName);

		s_lppAgent.DisableModule(modulePathStr, lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES, nullptr, nullptr);
	}
#endif
}