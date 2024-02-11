//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Crash report library connection
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "ExceptionHandler.h"
#include "eqCore.h"

static void DoCoreExceptionCallbacks()
{
	const Array<CoreExceptionCallback>& handlerCallbacks = ((CDkCore*)g_eqCore)->GetExceptionHandlers();
	for (int i = 0; i < handlerCallbacks.numElem(); i++)
	{
		handlerCallbacks[i]();
	}
}

#ifdef PLAT_WIN

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>

typedef struct _MODULEINFO {
	LPVOID lpBaseOfDll;  
	DWORD SizeOfImage; 
	LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

using ENUMPROCESSMODULESFUNC = BOOL (APIENTRY *)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
using GETMODULEINFORMATIONPROC = BOOL (APIENTRY *)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);

struct exception_codes {
	DWORD		exCode;
	const char*	exName;
	const char*	exDescription;
};

static exception_codes except_info[] = {
	{EXCEPTION_ACCESS_VIOLATION,		"ACCESS VIOLATION",
	"The thread tried to read from or write to a virtual address for which it does not have the appropriate access."},

	{EXCEPTION_ARRAY_BOUNDS_EXCEEDED,	"ARRAY BOUNDS EXCEEDED",
	"The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."},

	{EXCEPTION_BREAKPOINT,				"BREAKPOINT",
	"A breakpoint was encountered."},

	{EXCEPTION_DATATYPE_MISALIGNMENT,	"DATATYPE MISALIGNMENT",
	"The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."},

	{EXCEPTION_FLT_DENORMAL_OPERAND,	"FLT DENORMAL OPERAND",
	"One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value. "},

	{EXCEPTION_FLT_DIVIDE_BY_ZERO,		"FLT DIVIDE BY ZERO",
	"The thread tried to divide a floating-point value by a floating-point divisor of zero. "},

	{EXCEPTION_FLT_INEXACT_RESULT,		"FLT INEXACT RESULT",
	"The result of a floating-point operation cannot be represented exactly as a decimal fraction. "},

	{EXCEPTION_FLT_INVALID_OPERATION,	"FLT INVALID OPERATION",
	"This exception represents any floating-point exception not included in this list. "},

	{EXCEPTION_FLT_OVERFLOW,			"FLT OVERFLOW",
	"The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type. "},

	{EXCEPTION_FLT_STACK_CHECK,			"FLT STACK CHECK",
	"The stack overflowed or underflowed as the result of a floating-point operation. "},

	{EXCEPTION_FLT_UNDERFLOW,			"FLT UNDERFLOW",
	"The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type. "},

	{EXCEPTION_ILLEGAL_INSTRUCTION,		"ILLEGAL INSTRUCTION",
	"The thread tried to execute an invalid instruction. "},

	{EXCEPTION_IN_PAGE_ERROR,			"IN PAGE ERROR",
	"The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network. "},

	{EXCEPTION_INT_DIVIDE_BY_ZERO,		"INT DIVIDE BY ZERO",
	"The thread tried to divide an integer value by an integer divisor of zero. "},

	{EXCEPTION_INT_OVERFLOW,			"INT OVERFLOW",
	"The result of an integer operation caused a carry out of the most significant bit of the result. "},

	{EXCEPTION_INVALID_DISPOSITION,		"INVALID DISPOSITION",
	"An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception. "},

	{EXCEPTION_NONCONTINUABLE_EXCEPTION,"NONCONTINUABLE EXCEPTION",
	"The thread tried to continue execution after a noncontinuable exception occurred. "},

	{EXCEPTION_PRIV_INSTRUCTION,		"PRIV INSTRUCTION",
	"The thread tried to execute an instruction whose operation is not allowed in the current machine mode. "},

	{EXCEPTION_SINGLE_STEP,				"SINGLE STEP",
	"A trace trap or other single-instruction mechanism signaled that one instruction has been executed. "},

	{EXCEPTION_STACK_OVERFLOW,			"STACK OVERFLOW",
	"The thread used up its stack. "}
};

static void GetExceptionStrings( DWORD code, const char* *pName, const char* *pDescription )
{
	for (int i = 0; i < elementsOf(except_info); i++)
	{
		if (code == except_info[i].exCode)
		{
			*pName = except_info[i].exName;
			*pDescription = except_info[i].exDescription;
			return;
		}
	}

	*pName = "Unknown exception";
	*pDescription = "n/a";
}

static void CreateMiniDump( EXCEPTION_POINTERS* pep )
{
	SYSTEMTIME t;
	GetSystemTime(&t);

	char dumpPath[2048];
	sprintf(dumpPath, "logs/%s_%4d%02d%02d_%02d%02d%02d.mdmp", g_eqCore->GetApplicationName(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

	HANDLE dumpFileFd = CreateFileA(dumpPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (!dumpFileFd || dumpFileFd == INVALID_HANDLE_VALUE)
	{
		MsgError("Unable to create crash dump");
		return;
	}

	const MINIDUMP_TYPE dumpType = MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);

	MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo;
	dumpExceptionInfo.ThreadId = GetCurrentThreadId();
	dumpExceptionInfo.ExceptionPointers = pep;
	dumpExceptionInfo.ClientPointers = FALSE;

	const bool result = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), dumpFileFd, dumpType, (pep != 0) ? &dumpExceptionInfo : 0, 0, 0 );
	if( !result )
		ErrorMsg("Minidump write error\n");
	else
		WarningMsg("Minidump saved to:\n%s\n", dumpPath);

	// Close the file
	CloseHandle( dumpFileFd );
}

static void PrintCurrentProcessModules()
{
	HMODULE PsapiLib = LoadLibraryA("psapi.dll");
	if (!PsapiLib)
	{
		MsgError("Can't load psapi.dll, modules listing unavailable\n");
		return;
	}

	defer{
		FreeLibrary(PsapiLib);
	};

	ENUMPROCESSMODULESFUNC EnumProcessModules = (ENUMPROCESSMODULESFUNC)GetProcAddress(PsapiLib, "EnumProcessModules");
	GETMODULEINFORMATIONPROC GetModuleInformation = (GETMODULEINFORMATIONPROC)GetProcAddress(PsapiLib, "GetModuleInformation");
	if (!EnumProcessModules || !GetModuleInformation)
	{
		MsgError("Can't import functions from psapi.dll!\n");
		return;
	}

	DWORD needBytes;
	HMODULE hModules[1024];
	if (!EnumProcessModules(GetCurrentProcess(), hModules, sizeof(hModules), &needBytes))
	{
		MsgError("EnumProcessModules failed!\n");
		return;
	}

	if (needBytes > sizeof(hModules))
	{
		MsgError("Module limit exceeded (1024), can't print\n");
		return;
	}

	MsgError("\nModules listing:\n");

	const int numModules = needBytes / sizeof(HMODULE);
	for (int i = 0; i < numModules; i++)
	{
		char modName[MAX_PATH];
		if (GetModuleFileNameA(hModules[i], modName, MAX_PATH))
		{
			modName[MAX_PATH - 1] = 0;
			MsgError("%s : ", modName);
		}
		else
			MsgError("<error> : ");

		MODULEINFO modInfo;
		if (GetModuleInformation(GetCurrentProcess(), hModules[i], &modInfo, sizeof(modInfo)))
			MsgError("Base address: %p, Image size: %p\n", modInfo.lpBaseOfDll, modInfo.SizeOfImage);
		else
			MsgError("<error>\n");
	}
}

static LONG WINAPI _exceptionCB(EXCEPTION_POINTERS *ExceptionInfo)
{
    const EXCEPTION_RECORD* pRecord = ExceptionInfo->ExceptionRecord;

	//if (pRecord->ExceptionCode == EXCEPTION_BREAKPOINT ||
	//	pRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	//{
	//	return EXCEPTION_EXECUTE_HANDLER;
	//}

	const char* pName;
	const char* pDescription;
	GetExceptionStrings(pRecord->ExceptionCode, &pName, &pDescription);

	CrashMsg("We've got an fatal error\nMinidump will be saved in logs folder.\n\n"
		"Exception code: %s (0x%x)\n"
		"Address: %p\n\n\n"
		"See application log for details.", 
		pName, pRecord->ExceptionCode,
		pRecord->ExceptionAddress);

	CreateMiniDump(ExceptionInfo);

	DoCoreExceptionCallbacks();

	if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if (pRecord->ExceptionInformation[0])
			MsgError("Info: the thread attempted to write to an inaccessible address %p\n", pRecord->ExceptionInformation[1]);
		else
			MsgError("Info: the thread attempted to read the inaccessible data at %p\n", pRecord->ExceptionInformation[1]);
	}

	MsgError("\nDescription: %s\n", pDescription);

	// show modules list
	PrintCurrentProcessModules();

	MsgError("==========================================================\n\n");

	// dump memory allocator
	PPMemInfo(false);

    return EXCEPTION_EXECUTE_HANDLER;
}

typedef LONG (WINAPI *EXCEPTHANDLER)(EXCEPTION_POINTERS *ExceptionInfo);
static EXCEPTHANDLER oldHandler = nullptr;
static int handler_installed = 0;

static _purecall_handler oldPureCall = nullptr;

static void eqPureCallhandler(void)
{
	ASSERT_FAIL("Pure virtual function call");
}

void InstallExceptionHandler()
{
	oldHandler = SetUnhandledExceptionFilter(_exceptionCB);

	oldPureCall = _get_purecall_handler();
	_set_purecall_handler( eqPureCallhandler );

	handler_installed = 1;
}


void UnInstallExceptionHandler()
{
	if (handler_installed)
	{
		MsgError("*EXH: Removing exception handler...");
		SetUnhandledExceptionFilter( oldHandler );
		_set_purecall_handler( oldPureCall );
		MsgError("*EXH: OK\n");
	}
}

#elif defined(PLAT_POSIX)

#if defined(PLAT_ANDROID)

// we need to fake it sadly.
int backtrace(void **array, int size) { return 0; }
char **backtrace_symbols(void *const *array, int size) { return 0; }
void backtrace_symbols_fd (void *const *array, int size, int fd) {}

#else
#include <execinfo.h>
#endif

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

static void SignalBasic(int sig) 
{
	void* btarray[64];
	size_t btsize = backtrace(btarray, 64);
	fprintf(stderr, "Error: caught signal %d:\n", sig);
	backtrace_symbols_fd(btarray, btsize, 2);
	abort();
}

static void PrintStackTrace()
{
	void* btarray[64];
	size_t btsize = backtrace(btarray, 64);
	MsgError("\nStack trace:\n");

	char** symbols = backtrace_symbols(btarray, btsize);
	for(unsigned int i = 0; i < btsize; ++i)
		MsgError(" %s\n", symbols[i]);
}

static void SignalExtended(int signum, siginfo_t* info, void* arg)
{
	signal(SIGSEGV, SignalBasic);

	// Trace
	MsgError("\nCaught Segfault at %p\n", info->si_addr);
	DoCoreExceptionCallbacks();

	PrintStackTrace();

	Msg("---------------------\n");

	PPMemInfo(false);

	abort();
}

static void OnSTDExceptionThrown()
{
	static bool firstThrow = true;
	const char* exceptionText = nullptr;

	//Find the exception to try
	try 
	{
		if(!firstThrow) 
		{
			exceptionText = "empty exception";
		}
		else 
		{
			firstThrow = false;
			throw; //Haaax
		}
	}
	catch(const char* text) 
	{
		exceptionText = text;
	}
	catch(std::exception const& exc)
	{
		exceptionText = exc.what();
	}
	catch(...)
	{
		exceptionText = "unknown exception";
	}

	// Print exception text
	MsgError("\nUnexpected Exception: %s\n", exceptionText);

	// Trace
	DoCoreExceptionCallbacks();

	PrintStackTrace();

	Msg("---------------------\n");

	PPMemInfo(false);

	abort();
}

void InstallExceptionHandler()
{
	struct sigaction act;
	memset(&act, 0, sizeof(struct sigaction));

	sigemptyset(&act.sa_mask);
	act.sa_sigaction = &SignalExtended;
	act.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &act, nullptr);

	std::set_terminate(OnSTDExceptionThrown);
}

#endif //_WIN32
