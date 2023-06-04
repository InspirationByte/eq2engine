//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium main entry point
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "IMaterialSystem.h"

IShaderAPI*			g_pShaderAPI	= nullptr;
IMaterialSystem*	materials		= nullptr;

DECLARE_INTERNAL_SHADERS()

#ifdef _WIN32
#include <Windows.h>
#include <crtdbg.h>

//---------------------------------------------------------------------------
// Purpose: DllMain function
//---------------------------------------------------------------------------
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

#endif // _WIN32

extern void InitShaderOverrides();

#ifdef __GNUC__
extern "C"
{
#endif // __GNUC__

FUNC_EXPORTS int InitShaderLibrary(IMaterialSystem* pMatSystem)
{
	g_pShaderAPI	= pMatSystem->GetShaderAPI();
	materials		= pMatSystem;

	// register all shaders
	REGISTER_INTERNAL_SHADERS();

	// initialize override functions for matsystem shaders
	InitShaderOverrides();

	return 1;
}

#ifdef __GNUC__
}
#endif // __GNUC__
