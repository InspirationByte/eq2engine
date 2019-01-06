//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Crash report library connection
//////////////////////////////////////////////////////////////////////////////////

#include "platform/Platform.h"
#include "platform/MessageBox.h"
#include "ppmem.h"
#include "stdio.h"
#include "IDkCore.h"
#include "DebugInterface.h"
#include "ExceptionHandler.h"

#ifdef _WIN32

#include "dbgHelp.h"
#include "stdlib.h"

typedef struct _MODULEINFO {  LPVOID lpBaseOfDll;  DWORD SizeOfImage;  LPVOID EntryPoint;
} MODULEINFO, *LPMODULEINFO;

typedef BOOL (APIENTRY *ENUMPROCESSMODULESFUNC)(HANDLE hProcess, HMODULE* lphModule, DWORD cb, LPDWORD lpcbNeeded);
typedef BOOL (APIENTRY *GETMODULEINFORMATIONPROC)(HANDLE hProcess, HMODULE hModule, LPMODULEINFO lpmodinfo, DWORD cb);



typedef struct exception_codes_s {
	DWORD		exCode;
	const char*	exName;
	const char*	exDescription;
} exception_codes;

exception_codes except_info[] = {
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


void GetExceptionStrings( DWORD code, const char* *pName, const char* *pDescription )
{
	int i;
	int count = sizeof(except_info) / sizeof(exception_codes);
	for (i = 0; i < count; i++)
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

void CreateMiniDump( EXCEPTION_POINTERS* pep )
{
	// Open the file

	char tmp_path[2048];
	sprintf(tmp_path, "logs/CrashDump_%s_%s.dmp", GetCore()->GetApplicationName(), GetCore()->GetCurrentUserName());

	HANDLE hFile = CreateFile(tmp_path, GENERIC_READ | GENERIC_WRITE,
	0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );

	if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
	{
		// Create the minidump

		MINIDUMP_EXCEPTION_INFORMATION mdei;

		mdei.ThreadId           = GetCurrentThreadId();
		mdei.ExceptionPointers  = pep;
		mdei.ClientPointers     = FALSE;

		MINIDUMP_TYPE mdt       = MiniDumpNormal;

		BOOL rv = MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(),
			hFile, mdt, (pep != 0) ? &mdei : 0, 0, 0 );

		if( !rv )
			ErrorMsg("Minidump write error\n");
		else
			WarningMsg("Minidump saved to:\n%s\n", tmp_path);

		// Close the file
		CloseHandle( hFile );
	}
	else
	{
		ErrorMsg("Minidump file creation error\n");
	}

}

static LONG WINAPI _exceptionCB(EXCEPTION_POINTERS *ExceptionInfo)
{
    EXCEPTION_RECORD* pRecord = ExceptionInfo->ExceptionRecord;

	const char *pName, *pDescription;
	GetExceptionStrings( pRecord->ExceptionCode, &pName, &pDescription );

	char tmp_path[2048];
	sprintf(tmp_path, "\nUnhandled Exception !!!\nException code: %s (%p)\nAddress: %p\n\n\nSee application log for details.",pName, (void*)pRecord->ExceptionCode, pRecord->ExceptionAddress);

	_InternalAssert(NULL, NULL, tmp_path);

	CrashMsg(tmp_path);

	MsgError("\nUnhandled Exception !!!\nException code: %s (%p)\nAddress: %p\n\n",pName, pRecord->ExceptionCode,pRecord->ExceptionAddress);

	if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
	{
		if (pRecord->ExceptionInformation[0])
		{
			MsgError("Info: the thread attempted to write to an inaccessible address %p\n",
				pRecord->ExceptionInformation[1]);
		}
		else
		{
			MsgError("Info: the thread attempted to read the inaccessible data at %p\n",
				pRecord->ExceptionInformation[1]);
		}
	}

	MsgError("Description: %s\n", pDescription);
	MsgError("\nModules listing:\n");

	// show modules list
	HMODULE PsapiLib = LoadLibrary("psapi.dll");
	if (PsapiLib)
	{
		ENUMPROCESSMODULESFUNC		EnumProcessModules;
		GETMODULEINFORMATIONPROC	GetModuleInformation;
		EnumProcessModules = (ENUMPROCESSMODULESFUNC)GetProcAddress (PsapiLib, "EnumProcessModules");
		GetModuleInformation = (GETMODULEINFORMATIONPROC)GetProcAddress (PsapiLib, "GetModuleInformation");
		if (EnumProcessModules && GetModuleInformation)
		{
			DWORD needBytes;
			HMODULE hModules[1024];
			if (EnumProcessModules( GetCurrentProcess(), hModules, sizeof(hModules), &needBytes ))
			{
				if (needBytes <= sizeof(hModules))
				{
					DWORD i;
					DWORD numModules = needBytes / sizeof(HMODULE);

					for(i = 0; i < numModules; i++)
					{
						char modname[MAX_PATH];
						if (GetModuleFileName(hModules[i], modname, MAX_PATH))
						{
							modname[MAX_PATH-1] = 0;
							MsgError("%s : ", modname);
						}
						else
							MsgError("<error> : ");

						MODULEINFO modInfo;
						if (GetModuleInformation(GetCurrentProcess(), hModules[i], &modInfo, sizeof(modInfo)))
						{
							MsgError("Base address: %p, Image size: %p\n", modInfo.lpBaseOfDll, modInfo.SizeOfImage);
						}
						else
							MsgError("<error>\n");
					}
				}
				else
					MsgError("Too many modules loaded!\n");
			}
			else
				MsgError("EnumProcessModules failed!\n");
		}
		else
			MsgError("Can't import functions from psapi.dll!\n");

		FreeLibrary(PsapiLib);
	}
	else
		MsgError("Unable to load psapi.dll!\n");

	MsgError("==========================================================\n\n");

	PPMemInfo();

	CreateMiniDump(ExceptionInfo);

    return EXCEPTION_EXECUTE_HANDLER;
}

typedef LONG (WINAPI *EXCEPTHANDLER)(EXCEPTION_POINTERS *ExceptionInfo);
static EXCEPTHANDLER oldHandler = NULL;
static int handler_installed = 0;

static _purecall_handler oldPureCall = NULL;

void eqPureCallhandler(void)
{
	ASSERTMSG(false, "Pure virtual function call");
}

void InstallExceptionHandler()
{
	oldHandler = SetUnhandledExceptionFilter(_exceptionCB);

//#ifdef _DEBUG
	oldPureCall = _get_purecall_handler();
	_set_purecall_handler( eqPureCallhandler );
//#endif // _DEBUG

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

#else

#pragma todo("crash handler for Linux/Mac")

void InstallExceptionHandler()
{
}

#endif //_WIN32
