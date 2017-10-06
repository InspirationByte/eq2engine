//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: World compiler
//////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"
#include "cmdlib.h"
#include "eqwc.h"

#include <iostream>
#include <malloc.h>

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif

world_globals_t worldGlobals;

DKMODULE *g_matsystemmod = NULL;

IMaterialSystem* materials = NULL;

DECLARE_CVAR_NONSTATIC(__cheats,1,"Wireframe",0);

void InitStubMatSystem()
{
	// init matsystem
	g_matsystemmod = g_fileSystem->LoadModule("EqMatSystem.dll");

	if(!g_matsystemmod)
	{
		ErrorMsg("Cannot open EqMatSystem module!\n");
		exit(0);
	}

	materials = (IMaterialSystem*)GetCore()->GetInterface(MATSYSTEM_INTERFACE_VERSION);

	if(!materials)
	{
		ErrorMsg("ERROR! Couldn't get interface of EqMatSystem!");
		exit(0);
	}

	matsystem_render_config_t materials_config;

	materials_config.enableBumpmapping = false;
	materials_config.enableSpecular = false;
	materials_config.enableShadows = false;
	materials_config.wireframeMode = false;
	materials_config.threadedloader = false;
	materials_config.lowShaderQuality = false;
	materials_config.editormode = false;

	materials_config.ffp_mode = false;
	materials_config.lighting_model = MATERIAL_LIGHT_UNLIT;
	materials_config.stubMode = true;

	DefaultShaderAPIParameters(&materials_config.shaderapi_params);
	materials_config.shaderapi_params.windowedMode = true;
	materials_config.shaderapi_params.windowHandle = NULL;
	materials_config.shaderapi_params.screenFormat = FORMAT_RGB8;

	bool materialSystemStatus = materials->Init("materials/", "EqShaderAPIEmpty", materials_config);

	if(!materialSystemStatus)
	{
		ErrorMsg("ERROR! Cannot initialize material system!");
		exit(0);
	}

	if(!materials->LoadShaderLibrary("eqBaseShaders"))
	{
		MsgError("Cannot load eqBaseShaders.dll!\n");
		exit(0);
	}
}

void Usage()
{
	Msg("Equilibrium world compiler usage:\n");
	Msg(" eqwc -world <world name> [arguments]\n\n");
	Msg(" Arguments:\n");
	Msg("  -waitafterbuild : wait for a key after building\n");
	Msg("  -onlyents : rebuild entities and re-save entity lump only\n");
	Msg("  -nocsg : disables brush model CSG\n");
	Msg("  -nosectors : disables sector subdivision (debug)\n");
	Msg("  -2dsectors : process sectors in X-Z coordinates (big levels solution)\n");
	Msg("  -sectorsize <n> : sets sector size to <n> value\n");
	Msg("  -luxelspermeter <f> : sets luxel per meter to <f> value, not depends on lightmap size\n");
	Msg("  -pickthresh <f> : sets sets surface pick threshold to <f> value for lightmap UV parametrization\n");
	Msg("  -lightmapsize <n> : sets lightmap texture size");
	Msg("  -nolightmap : disables lightmap and it's UV calculation\n");
};

int main(int argc, char **argv)
{
	//Only set debug info when connecting dll
	#ifdef _DEBUG
		#define _CRTDBG_MAP_ALLOC
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		flag |= _CRTDBG_CHECK_ALWAYS_DF; // Turn on CrtCheckMemory
		flag |= _CRTDBG_ALLOC_MEM_DF;
		_CrtSetDbgFlag(flag); // Set flag to the new value
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	#endif

#ifdef _WIN32
	Install_SpewFunction();
#endif

	//Sleep(8000); //Sleep for 8 seconds

	// Core initialization
	GetCore()->Init("eqwc",argc,argv);

	// Filesystem is first!
	if(!g_fileSystem->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}

	// Command line execution
	g_cmdLine->ExecuteCommandLine(true,true);

	Msg("--------------------------------------------------------------------\n");
	Msg("Equilibrium World Compiler\n");
	Msg(" Version 0.8c\n");
	Msg("--------------------------------------------------------------------\n");

	InitStubMatSystem();

	// setup compiler defaults
	memset(&worldGlobals, 0, sizeof(worldGlobals));

	worldGlobals.worldName = "no-world";
	worldGlobals.worldBox.Reset();
	
	worldGlobals.bOnlyEnts			= false;						// only entities build. New engine is good that there is no worldspawn, no brush and model ents
	worldGlobals.bBrushCSG			= true;							// brush CSG
	worldGlobals.bSectorDivision	= true;							// sector subdivisions
	worldGlobals.bSectorsInXZ		= false;						// sector subdivisions in 2D?
	worldGlobals.bNoLightmap		= false;						// no lightmap UV?
	worldGlobals.bOnlyPhysics		= false;
	worldGlobals.fSectorSize		= EQW_SECTOR_DEFAULT_SIZE;		// sector size to be compiled

	worldGlobals.numLightmaps		= 0;
	worldGlobals.lightmapSize		= DEFAULT_LIGHTMAP_SIZE;
	worldGlobals.nPackThreshold		= DEFAULT_PACK_THRESH;			
	worldGlobals.fPickThreshold		= DEFAULT_PICK_THRESH;			// pick angle thresh
	worldGlobals.fLuxelsPerMeter	= DEFAULT_LUXEL;				// luxel scale, also depends on final lightmap size

	bool bWorldSpecified = false;

	bool bWait = false;

	for(int i = 0; i < g_sysConsole->GetArgumentCount(); i++)
	{
		if(!stricmp("-world", g_cmdLine->GetArgumentString(i)))
		{
			worldGlobals.worldName = g_cmdLine->GetArgumentString(i+1);
			bWorldSpecified = true;
		}	
		if(!stricmp("-waitafterbuild", g_cmdLine->GetArgumentString(i)))
		{
			bWait = true;
		}	
		else if(!stricmp("-onlyents", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("Only entities mode\n");
			worldGlobals.bOnlyEnts = true;
		}
		else if(!stricmp("-nocsg", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("Brush CSG disabled\n");
			worldGlobals.bBrushCSG = false;
		}
		else if(!stricmp("-nosectors", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("Sector division disabled\n");
			worldGlobals.bSectorDivision = false;
		}
		else if(!stricmp("-2dsectors", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("2D X-Z Sector division enabled\n");
			worldGlobals.bSectorsInXZ = true;
		}
		else if(!stricmp("-sectorsize", g_cmdLine->GetArgumentString(i)))
		{
			worldGlobals.fSectorSize = atof(g_cmdLine->GetArgumentString(i+1));
			MsgInfo("Sector size = %g\n", worldGlobals.fSectorSize);
		}
		else if(!stricmp("-luxelspermeter", g_cmdLine->GetArgumentString(i)))
		{
			worldGlobals.fLuxelsPerMeter = atof(g_cmdLine->GetArgumentString(i+1));
			MsgInfo("Luxels per meter = %g\n", worldGlobals.fLuxelsPerMeter);
		}
		else if(!stricmp("-pickthresh", g_cmdLine->GetArgumentString(i)))
		{
			worldGlobals.fPickThreshold = atof(g_cmdLine->GetArgumentString(i+1));
			MsgInfo("LMUV Pick threshold = %g\n", worldGlobals.fPickThreshold);
		}
		else if(!stricmp("-lightmapsize", g_cmdLine->GetArgumentString(i)))
		{
			worldGlobals.lightmapSize = atoi(g_cmdLine->GetArgumentString(i+1));
			MsgInfo("Lightmap size = %d\n", worldGlobals.lightmapSize);
		}
		else if(!stricmp("-nolightmap", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("Lightmap UV disabled\n");
			worldGlobals.bNoLightmap = true;
		}
		else if(!stricmp("-onlyphysics", g_cmdLine->GetArgumentString(i)))
		{
			MsgInfo("Rebuild physics geometry only\n");
			worldGlobals.bOnlyPhysics = true;
		}
		
	}

	if(bWorldSpecified)
	{
		EqString worldFullPath(varargs("worlds/%s/level.edlvl", worldGlobals.worldName.GetData()));

		Msg("Loading world '%s'\n", worldGlobals.worldName.GetData());

		KeyValues level_kv;

		// Open keyvalues
		if(level_kv.LoadFromFile(worldFullPath.GetData()))
		{
			// find required 'EditorLevel' section
			kvkeybase_t* pSection = level_kv.GetRootSection()->FindKeyBase("EditorLevel", KV_FLAG_SECTION);
			if(pSection)
			{
				ParseWorldData( pSection );

				BuildWorld();

				WriteWorld();
			}
			else
				MsgError("ERROR: World source file '%s' is invalid\n", worldFullPath.GetData());
		}
		else
			MsgError("ERROR: Failed to open '%s' for compilation\n", worldFullPath.GetData());
	}
	else
	{
		Usage();
		bWait = true;
	}

	// beep waiter
	if(bWait)
	{
		MsgWarning("\nPress any key to continue...");
		Beep(1200,100);
		Beep(1500,100);
		Beep(1800,100);
		Beep(2000,100);
		getchar();
	}

	materials->Shutdown();

	g_fileSystem->FreeModule( g_matsystemmod );
	g_matsystemmod = NULL;

	GetCore()->Shutdown();
	return 0;
}
