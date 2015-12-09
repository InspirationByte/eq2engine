//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics model generator main src
//////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>
#include <iostream>
#include <malloc.h>

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"
#include "model.h"
#include "cmdlib.h"
#include "physgen_process.h"

const char* GetFormatName(int id)
{
	switch(id)
	{
		case EQUILIBRIUM_MODEL_SIGNATURE:
			return "Equilibrium Geometry Format (EGF)";
#ifdef EQUILIBRIUMFX_MODEL_FORMAT
		case EQUILIBRIUMFX_MODEL_SIGNATURE:
			return "Equilibrium FX Geometry Format (EGX)";
#endif
#ifdef SUNSHINE_MODEL_FORMAT
		case SUNSHINE_MODEL_SIGNATURE:
			return "Sunshine Geometry Format (SGF)";
#endif
#ifdef SUNSHINEFX_MODEL_FORMAT
		case SUNSHINEFX_MODEL_SIGNATURE:
			return "Sunshine FX Geometry Format (SGX)";
#endif
	}
	return "invalid format";
}

bool IsValidModelIdentifier(int id)
{
	switch(id)
	{
		case EQUILIBRIUM_MODEL_SIGNATURE:
			return true;
#ifdef EQUILIBRIUMFX_MODEL_FORMAT
		case EQUILIBRIUMFX_MODEL_SIGNATURE:
			return true;
#endif
#ifdef SUNSHINE_MODEL_FORMAT
		case SUNSHINE_MODEL_SIGNATURE:
			return true;
#endif
#ifdef SUNSHINEFX_MODEL_FORMAT
		case SUNSHINEFX_MODEL_SIGNATURE:
			return true;
#endif
	}
	return false;
}

modelheader_t* g_pHdr = NULL;
KeyValues *g_pScript = NULL;
bool g_forcegroupdivision = false;
DkStr outputname("invalid_ragdoll_01.pod");

void cc_load_egf(DkList<DkStr> *args)
{
	if(args)
	{
		if(args->numElem() <= 0)
		{
			MsgWarning("Example: physgen +egf <filename>.egf");
			return;
		}

		ubyte* buffer = (ubyte *)GetFileSystem()->GetFileBuffer(args->ptr()[0].getData());

		if(buffer == NULL)
		{
			MsgError("Cannot open '%s' model\n",args->ptr()[0].getData());
			return;
		}

		basemodelheader_t* pBaseHdr = (basemodelheader_t*)buffer;
		if(!IsValidModelIdentifier(pBaseHdr->m_nIdentifier))
		{
			delete [] buffer;
			MsgError("Invalid model file '%s'\n",args->ptr()[0].getData());
			return;
		}

		if(pBaseHdr->m_nVersion != EQUILIBRIUM_MODEL_VERSION)
		{
			MsgError("Wrong model model version, should be %i, excepted %i\n", EQUILIBRIUM_MODEL_VERSION, pBaseHdr->m_nVersion);
			delete [] buffer;
			return;
		}

		DkStr mod_name = args->ptr()[0];
		mod_name = mod_name.stripFileExtension();

		DkStr script_name(mod_name);
		script_name.append(".rbi");

		if(args->numElem() > 1)
		{
			mod_name = args->ptr()[1];
		}

		outputname = mod_name;
		outputname.append(".pod");

		Msg("Output set to %s\n", outputname.getData());

		if(GetFileSystem()->FileExist(script_name.getData()))
		{
			g_pScript = new KeyValues();
			if(!g_pScript->LoadFromFile(script_name.getData())) // load rigid body info
			{
				MsgWarning("Warning! No RBI script loaded for physics model generator!\n");
				delete g_pScript;
				g_pScript = NULL;
			}
		}
		else
		{
			MsgWarning("Warning! No RBI script for physics model generator!\n");
		}

		// model loaded, take it's header
		g_pHdr = (modelheader_t*)pBaseHdr;
	}
}

ConCommand cc_load_egf_cmd("egf",cc_load_egf,"Input model");

DECLARE_CMD(help, Display help message, 0)
{
	Msg("PhysHen is a physics model generator written\nto reduce physics model generation time on engine load\n");
	Msg("Example:\n");
	Msg("'physgen +egf test.egf' - generates physics model for test.egf with default mass parameters and saves it to test.pod\n");
	Msg("'physgen +egf test.egf +forcegroupdivision' - generates physics model, and forced division on geometry groups if specified\n");

	Msg("'+cvarlist' and '+cmdlist' for information about all commands and variables\n\n");
}

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

	// Core initialization test was passes successfully
	GetCore()->Init_Tools("physgen",argc,argv);

	// Filesystem is first!
	if(!GetFileSystem()->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}
	Msg("----------------------------------------------\n");
	Msg("PhysGen - Equilibrium physics generator program v1.0\n");
	Msg("Input model format is 'EQGF', version %d\n", EQUILIBRIUM_MODEL_VERSION);
	Msg("----------------------------------------------\n\n");

	Msg("Add '+help' to command line to show arguments and usage\n\n");

	// Command line execution test was passes not successfully
	GetCmdLine()->ExecuteCommandLine(true,true);

	physgenmakeparams_t params;
	params.pModel = g_pHdr;
	params.script = g_pScript;
	params.forcegroupdivision = false;

	params.outputname[0] = 0;
	strcpy(params.outputname, outputname.getData());

	// process loaded model
	GeneratePhysicsModel(&params);

	if(g_pScript)
	{
		delete g_pScript;
		g_pScript = NULL;
	}

	GetCore()->Shutdown();
	return 0;
}
