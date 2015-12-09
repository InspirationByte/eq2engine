//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine system module loader
//////////////////////////////////////////////////////////////////////////////////

#include "core_base_header.h"
#include "Platform.h"

ConVar r_renderer("r_renderer","EqD3D9RHI"/*"EqShaderAPIGL"*/,"Renderer API to use",CV_ARCHIVE);

DKMODULE* LoadRendererModule()
{
	// loads rendering library.
	return GetFileSystem()->LoadModule(r_renderer.GetString());
}

DKMODULE* LoadGameModule()
{
#ifdef PLAT_POSIX
	EqString moduleName(varargs("%s/GameLib.so", GetFileSystem()->GetCurrentGameDirectory()));
#else if PLAT_WIN
	EqString moduleName(varargs("%s/GameLib.dll", GetFileSystem()->GetCurrentGameDirectory()));
#endif // PLAT_WIN

	// first extract it from package if it's not exist
	GetFileSystem()->ExtractFile( moduleName.GetData(), true );

	// loads game library from current game directory
	return GetFileSystem()->LoadModule( moduleName.GetData() );
}