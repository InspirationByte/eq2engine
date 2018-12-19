//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: ShaderAPI DllMain on windows
//////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32

#include "platform/Platform.h"
#include "DebugInterface.h"

#include <crtdbg.h>

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