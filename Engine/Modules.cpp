//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine system module loader
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "platform/Platform.h"

ConVar r_renderer("r_renderer","EqD3D9RHI"/*"EqShaderAPIGL"*/,"Renderer API to use",CV_ARCHIVE);

DKMODULE* LoadRendererModule()
{
	// loads rendering library.
	return g_fileSystem->LoadModule(r_renderer.GetString());
}

DKMODULE* LoadGameModule()
{
#ifdef PLAT_POSIX
	EqString moduleName(varargs("%s/GameLib.so", g_fileSystem->GetCurrentGameDirectory()));
#else if PLAT_WIN
	EqString moduleName(varargs("%s/GameLib.dll", g_fileSystem->GetCurrentGameDirectory()));
#endif // PLAT_WIN

	// first extract it from package if it's not exist
	g_fileSystem->ExtractFile( moduleName.GetData(), true );

	// loads game library from current game directory
	return g_fileSystem->LoadModule( moduleName.GetData() );
}