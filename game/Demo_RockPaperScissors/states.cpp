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

CBaseStateHandler* g_states[GAME_STATE_COUNT] = { nullptr };

namespace EqStateMgr
{

bool InitRegisterStates()
{
	g_parallelJobs->Init();
	g_audioSystem->Init();

#ifdef ENABLE_MULTIPLAYER
	Networking::InitNetworking();
#endif

	// TODO: plugin?
#ifdef IMGUI_ENABLED
	g_consoleInput->AddImGuiHandle("soundScriptEditor", [](const char* name, EqImGuiHandleTypes type) {
		static bool soundScriptMenuVisible = false;
		if (type == IMGUI_HANDLE_MENU)
		{
			if (ImGui::BeginMenu("ENGINE"))
			{
				if (ImGui::BeginMenu("SOUND"))
				{
					IMGUI_MENUITEM_CONVAR_BOOL("SHOW DEBUG INFO", snd_debug);
					IMGUI_MENUITEM_CONVAR_BOOL("SHOW SCRIPTED SOUNDS DEBUG", snd_scriptsound_debug);
					ImGui::Separator();
					ImGui::MenuItem("SCRIPT EDITOR", nullptr, &soundScriptMenuVisible);
					ImGui::EndMenu();
				}

				ImGui::EndMenu();
			}
		}
		else
			SoundScriptEditorUIDraw(soundScriptMenuVisible);
	});
#endif
	g_states[GAME_STATE_SAMPLE_GAME_DEMO] = g_State_SampleGameDemo;

	g_pHost->SetWindowTitle(GAME_WINDOW_TITLE);

	EqStateMgr::SetCurrentStateType(GAME_STATE_SAMPLE_GAME_DEMO);

	return true;
}

void PreUpdateState(float fDt)
{
}

void ShutdownStates()
{
	for (int i = 0; i < GAME_STATE_COUNT; ++i)
		g_states[i] = nullptr;

#ifdef ENABLE_MULTIPLAYER
	Networking::ShutdownNetworking();
#endif

	g_sounds->Shutdown();
	g_audioSystem->Shutdown();
	g_parallelJobs->Shutdown();
}

bool IsMultiplayerGameState()
{
	return false;
}

bool IsInGameState()
{
	return GetCurrentStateType() != GAME_STATE_SAMPLE_GAME_DEMO;
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



