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
	for (CoreExceptionCallback cb : static_cast<CDkCore*>(g_eqCore)->GetExceptionHandlers())
		cb();
}

#ifdef PLAT_WIN

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#ifdef far
#	undef far
#endif
#ifdef near
#	undef near
#endif

#pragma warning(push)
#pragma warning(disable:4477)
#pragma warning(disable:4313)

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

static bool CreateMiniDump( EXCEPTION_POINTERS* pep, char* dumpPath, int dumpPathMaxLen)
{
	const bool fullCrashDumps = g_eqCore->GetDebugSettings().fullCrashDumps;

	SYSTEMTIME t;
	GetSystemTime(&t);

	CString::PrintF(dumpPath, dumpPathMaxLen, "logs/%s_%4d%02d%02d_%02d%02d%02d.dmp", g_eqCore->GetApplicationName(), t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);

	MINIDUMP_TYPE dumpType = MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory);
	if (fullCrashDumps)
	{
		// Configure to save full application dump
		dumpType = MINIDUMP_TYPE(MiniDumpWithFullMemory | MiniDumpWithDataSegs | MiniDumpWithHandleData |
			MiniDumpWithUnloadedModules | MiniDumpWithIndirectlyReferencedMemory |
			MiniDumpWithFullMemoryInfo | MiniDumpWithThreadInfo | MiniDumpWithTokenInformation);
	}

	HANDLE dumpFileFd = CreateFileA(dumpPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (!dumpFileFd || dumpFileFd == INVALID_HANDLE_VALUE)
	{
		MsgError("Unable to create crash dump\n");
		return false;
	}

	MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo;
	dumpExceptionInfo.ThreadId = GetCurrentThreadId();
	dumpExceptionInfo.ExceptionPointers = pep;
	dumpExceptionInfo.ClientPointers = FALSE;

	const BOOL result = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFileFd, dumpType, (pep != nullptr) ? &dumpExceptionInfo : nullptr, nullptr, nullptr);
	CloseHandle(dumpFileFd);

	if (!result)
		MsgError("%s write error\n", fullCrashDumps ? "Crash dump" : "Mini dump");
	else
		MsgInfo("%s saved to: %s\n", fullCrashDumps ? "Crash dump" : "Mini dump", dumpPath);

	return result;
}

static void PrintStackTrace()
{
	MsgInfo("\nCrash call stack:\n");

	CONTEXT context;
	RtlCaptureContext(&context);

	STACKFRAME64 stack;
	DWORD machine_type;
#ifdef _M_IX86
	machine_type = IMAGE_FILE_MACHINE_I386;
	stack.AddrPC.Offset = context.Eip;
	stack.AddrFrame.Offset = context.Ebp;
	stack.AddrStack.Offset = context.Esp;
#elif _M_X64
	machine_type = IMAGE_FILE_MACHINE_AMD64;
	stack.AddrPC.Offset = context.Rip;
	stack.AddrFrame.Offset = context.Rsp;
	stack.AddrStack.Offset = context.Rsp;
#elif _M_ARM64
	machine_type = IMAGE_FILE_MACHINE_ARM64;
	stack.AddrPC.Offset = context.Pc;
	stack.AddrFrame.Offset = context.Fp;
	stack.AddrStack.Offset = context.Sp;
#else
#error "Unsupported platform"
#endif

	stack.AddrPC.Mode = AddrModeFlat;
	stack.AddrFrame.Mode = AddrModeFlat;
	stack.AddrStack.Mode = AddrModeFlat;

	HANDLE process = GetCurrentProcess();
	HANDLE thread = GetCurrentThread();

	SymInitialize(process, NULL, TRUE);
	SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);

	DWORD frameNum = 0;
    while (StackWalk64(
        machine_type,
        process,
        thread,
        &stack,
        &context,
        NULL,
        SymFunctionTableAccess64,
        SymGetModuleBase64,
        NULL)) 
	{
        if (stack.AddrPC.Offset == 0)
            break;

        DWORD64 symAddr  = stack.AddrPC.Offset;
        char symBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)] = {0};
        SYMBOL_INFO* symbol  = (SYMBOL_INFO *)symBuffer;
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen   = MAX_SYM_NAME;

        // Get line information
        IMAGEHLP_LINE64 line = {0};
        line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
        DWORD lineDisp = 0;
        BOOL hasLine = SymGetLineFromAddr64(process, symAddr, &lineDisp, &line);

        char funcName[MAX_SYM_NAME] = "<unknown>";
		DWORD64 displacement = 0;
        if (SymFromAddr(process, symAddr, &displacement, symbol))
            strncpy(funcName, symbol->Name, MAX_SYM_NAME - 1);
		funcName[MAX_SYM_NAME - 1] = '\0';

        // Format line information
        char lineInfo[256] = "<unknown>";
        if (hasLine)
			CString::PrintF(lineInfo, sizeof(lineInfo), "%s:%lu", line.FileName, line.LineNumber);

        // Print with better alignment using format specifiers
        Msg("0x%016llX %-60.60s %s\n", symAddr, funcName, lineInfo);

        frameNum++;
    }

    SymCleanup(process);
}

static LONG WINAPI _exceptionCB(EXCEPTION_POINTERS *ExceptionInfo)
{
    const EXCEPTION_RECORD* pRecord = ExceptionInfo->ExceptionRecord;

	//if (pRecord->ExceptionCode == EXCEPTION_BREAKPOINT ||
	//	pRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	//{
	//	return EXCEPTION_EXECUTE_HANDLER;
	//}

	MsgError("\n==========================================================\n");

	const char* pName;
	const char* pDescription;
	GetExceptionStrings(pRecord->ExceptionCode, &pName, &pDescription);
	MsgError("Exception: %s\n%s\n", pName, pDescription);

	if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if (pRecord->ExceptionInformation[0])
			MsgError("- the thread attempted to write to an inaccessible address %p\n", pRecord->ExceptionInformation[1]);
		else
			MsgError("- the thread attempted to read the inaccessible data at %p\n", pRecord->ExceptionInformation[1]);
	}

	MsgError("\n");

	char dumpPath[2048];
	const bool miniDumpCreated = CreateMiniDump(ExceptionInfo, dumpPath, sizeof(dumpPath));

	DoCoreExceptionCallbacks();

	PrintStackTrace();

	if (!g_eqCore->GetDebugSettings().crashOnAssert)
	{
		if (miniDumpCreated)
		{
			CrashMsg("Exception code: %s (0x%x)\n"
				"At address: %p\n\n"
				"See application log for details.\nCrash dump path: %s",
				pName, pRecord->ExceptionCode,
				pRecord->ExceptionAddress,
				dumpPath);
		}
		else
		{
			CrashMsg("Exception code: %s (0x%x)\n"
				"At address: %p\n\n"
				"See application log for details.\nCrash dump was not created",
				pName, pRecord->ExceptionCode,
				pRecord->ExceptionAddress);
		}
	}

	MsgError("==========================================================\n\n");

	// dump memory allocator
	PPMemInfo(true);

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

#pragma warning(pop)

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

	MsgError("\nCaught Segfault at %p\n", info->si_addr);

	DoCoreExceptionCallbacks();

	PrintStackTrace();

	Msg("---------------------\n");

	PPMemInfo(true);

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

	MsgError("\nUnexpected Exception: %s\n", exceptionText);

	DoCoreExceptionCallbacks();

	PrintStackTrace();

	Msg("---------------------\n");

	PPMemInfo(true);

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
