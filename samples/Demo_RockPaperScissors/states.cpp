//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: States of Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IEqParallelJobs.h"
#include "core/IFileSystem.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "sys/sys_host.h"
#include "sys/sys_in_console.h"
#include "audio/eqSoundEmitterSystem.h"

#ifdef IMGUI_ENABLED
#include <imgui.h>
#include "audio/SoundScriptEditorUI.h"
#endif

#include "state_sample_game_demo.h"

#define GAME_WINDOW_TITLE	"Rock Paper Scissors"

namespace eqAppStateMng
{

static CAppStateBase* s_appStates[APP_STATE_COUNT] = { nullptr };

const char* GetAppNameTitle()
{
	return GAME_WINDOW_TITLE;
}

CAppStateBase* GetAppStateByType(int stateType)
{
	return s_appStates[stateType];
}

bool InitAppStates()
{
	s_appStates[APP_STATE_SAMPLE_GAME_DEMO] = g_State_SampleGameDemo;

	g_audioSystem->Init();

#ifdef ENABLE_MULTIPLAYER
	Networking::InitNetworking();
#endif
#ifdef IMGUI_ENABLED
	g_consoleInput->AddDebugMenu("ENGINE/Sound/Script Editor UI", SoundScriptEditorUIDraw);
#endif

	eqAppStateMng::SetCurrentStateType(APP_STATE_SAMPLE_GAME_DEMO);

	return true;
}

void ShutdownAppStates()
{
	for (int i = 0; i < APP_STATE_COUNT; ++i)
		s_appStates[i] = nullptr;

#ifdef ENABLE_MULTIPLAYER
	Networking::ShutdownNetworking();
#endif

	g_sounds->Shutdown();
	g_audioSystem->Shutdown();
	g_parallelJobs->Shutdown();
}

bool IsPauseAllowed()
{
	return false;
}

void SignalPause()
{
}

}

#ifdef PLAT_ANDROID

#include "SDL_main.h"
#undef main

extern int main(int argc, char* argv[]);

extern "C"
{
	// GCC is a piece of shit when linking static with export
	DECLSPEC int SDL_main(int argc, char* argv[])
	{
		return main(argc, argv);
	}
}



#endif



