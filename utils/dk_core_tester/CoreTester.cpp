//**************** Copyright (C) Parallel Prevision, L.L.C 2012 ******************
//
// Description:
//
//****************************************************************************


#include <stdio.h>

#include "Platform.h"
#include "Utils/strtools.h"
#include "IDkCore.h"
#include "Utils/Align.h"
#include "DebugInterface.h"

#include <iostream>
#include <malloc.h>

#ifdef _WIN32
#include <tchar.h>
#include <crtdbg.h>
#else
#include <unistd.h>
#endif


DECLARE_CVAR(test,cmdrate,TEST 1,0);
DECLARE_CVAR(test2,testlist,TEST 2,0);
DECLARE_CVAR(test3,HEAVYTEST,TEST 3,0);
DECLARE_CVAR(test5,SUPERTEST,TEST 5,0);

#define TEST1_VALUE "TEST_SIMPLE_VALUE"

void do_test1()
{
	Msg("TEST 1: Simple test of setting value\nIf it will not pass, be sure that is programmer error\n");
	test.SetValue(TEST1_VALUE);

	static ConCommand *cvarlist = (ConCommand*)GetCvars()->FindCommand("cvarlist");
	if(cvarlist)
	{
		Msg("See cvarlist for more info...\n");
		cvarlist->DispatchFunc(NULL);
	}
}

void do_test2()
{
	Msg("TEST 2: Command execution: 'test2 \"MUZAFAKA PISEC BLIN\"\n");

	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->SetCommandBuffer("test2 \"MUZAFAKA PISEC BLIN\""); // Tested, but with errors
	//GetCommandAccessor()->SetCommandBuffer("test2 MUZAFAKA"); // Tested, but with errors
	GetCommandAccessor()->ExecuteCommandBuffer();

/*
	Deprecated function of CORE
	GetCvars()->ExecuteCommand("test MUZAFAKA PISEC BLIN");
*/
	Msg("TEST 2 Results: PASSED at value %s\n",test2.GetString());
	static ConCommand *cvarlist = (ConCommand*)GetCvars()->FindCommand("cvarlist");
	if(cvarlist)
	{
		Msg("Executing cvarlist...\n");
		cvarlist->DispatchFunc(NULL);
	}
}

void do_test3()
{
	Msg("TEST 3: Hooking (by GetCvars()->FindConVar() ) to ConVar 'test3' \nand changing it's value to '100013453_SUKAKAKAKAKAKAKAKAKAKAKAKSKASKASKAKSAKSKASLK'\n");

	ConVar *cheats = (ConVar*)GetCvars()->FindCvar("test3");

	if(cheats)
	{
		/*
		GetCvars()->ExecuteCommand("__cheats 100");
		Msg("TEST 3 Results 1: PASSED at value %s\n",cheats->GetString());
		GetCvars()->ExecuteCommand("__cheats 512000");
		Msg("TEST 3 Results 2: PASSED at value %s\n",cheats->GetString());
		*/
		cheats->SetValue("100013453_SUKAKAKAKAKAKAKAKAKAKAKAKSKASKASKAKSAKSKASLK");
		Msg("TEST 3 Results: PASSED at value %s\n",cheats->GetString());
	}
	else
	{
		Msg("TEST 3 Results: FAILED! at NULL pointer\n");
	}

	static ConCommand *cvarlist = (ConCommand*)GetCvars()->FindCommand("cvarlist");
	if(cvarlist)
	{
		Msg("Executing cvarlist...\n");
		cvarlist->DispatchFunc(NULL);
	}
}

void do_test4()
{
	Msg("TEST 4: Append to command buffer and set all 3 test* convar values\n");

	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->SetCommandBuffer("test HACKER_TEST1;c_trackmeminfo;test2 HACKER_TEST2;test3 HACKER_TEST3"); // Tested, but with errors
	GetCommandAccessor()->ExecuteCommandBuffer();

	static ConCommand *cvarlist = (ConCommand*)GetCvars()->FindCommand("cvarlist");
	if(cvarlist)
	{
		Msg("Executing cvarlist...\n");
		cvarlist->DispatchFunc(NULL);
	}
}

void do_test5()
{
	Msg("TEST 5: Appending configuration file ( core_test.cfg )\n");

	GetCommandAccessor()->ClearCommandBuffer();
	GetCommandAccessor()->ParseFileToCommandBuffer("core_test.cfg"); // Tested, but with errors
	GetCommandAccessor()->ExecuteCommandBuffer();

	static ConCommand *cvarlist = (ConCommand*)GetCvars()->FindCommand("cvarlist");
	if(cvarlist)
	{
		Msg("Executing cvarlist...\n");
		cvarlist->DispatchFunc(NULL);
	}
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

	//Sleep(8000); //Sleep for 8 seconds

	// Core initialization test was passes successfully
	GetCore()->Init_Tools("coretester",argc,argv);

	/*
	// Filesystem is first!
	if(!GetFileSystem()->Init(false))
	{
		GetCore()->Shutdown();
		return 0;
	}
	*/

	// Command line execution test was passes not successfully
	GetCmdLine()->ExecuteCommandLine(true,true);

	GetCommandAccessor()->SetCommandBuffer("c_trackmeminfo"); // Tested, but with errors
	GetCommandAccessor()->ExecuteCommandBuffer();

	GetCommandAccessor()->SetCommandBuffer("test2 \"MUZAFAKA PISEC BLIN\""); // Tested, but with errors
	GetCommandAccessor()->ExecuteCommandBuffer();

	// Command line execution test was passes successfully
	//do_test1();

	// Passed
	//do_test2();

	// ?
	//do_test3();

	// ?
	//do_test4();

	// ?
	//do_test5();

	GetCore()->Shutdown();
	return 0;
}
