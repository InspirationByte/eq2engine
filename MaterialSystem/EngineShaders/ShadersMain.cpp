//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech main entry point
//////////////////////////////////////////////////////////////////////////////////

#include "Platform.h"
#include "DebugInterface.h"
#include "ishaderapi.h"
#include "materialsystem/IMaterialSystem.h"

IShaderAPI*			g_pShaderAPI	= (IShaderAPI*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);
IMaterialSystem*	materials		= NULL;

#ifdef _WIN32

#include <crtdbg.h>

//---------------------------------------------------------------------------
// Purpose: DllMain function
//---------------------------------------------------------------------------
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

#endif // _WIN32

extern void InitShaderOverrides();

#ifdef __GNUC__
extern "C"
{
#endif // __GNUC__

ONLY_EXPORTS int InitShaderLibrary(IMaterialSystem* pMatSystem)
{
	g_pShaderAPI	= pMatSystem->GetShaderAPI();
	materials		= pMatSystem;

	// Material system already connected, continue

	// initialize override functions for matsystem shaders
	InitShaderOverrides();

	return 1;
}

#ifdef __GNUC__
}
#endif // __GNUC__
