//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: States of Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "core/IDkCore.h"
#include "core/IEqParallelJobs.h"
#include "core/IFileSystem.h"
#include "core/ConCommand.h"
#include "core/ConVar.h"
#include "core/IConsoleCommands.h"

#include "sys/sys_host.h"
#include "sys/sys_in_console.h"
#include "audio/eqSoundEmitterSystem.h"
#include "studio/StudioCache.h"
#include "materialsystem1/IMaterialSystem.h"

#ifdef IMGUI_ENABLED
#include <imgui.h>
#include "audio/SoundScriptEditorUI.h"
#endif

#include "gpudriven_main.h"
#include "physics/IStudioShapeCache.h"

#define GAME_WINDOW_TITLE	"GPU Driven Rendering"

static CEmptyStudioShapeCache s_shapeCache;

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
	s_appStates[APP_STATE_MAIN_GAMELOOP] = g_State_Demo;

#ifdef ENABLE_MULTIPLAYER
	Networking::InitNetworking();
#endif

	// TODO: plugin?
#ifdef IMGUI_ENABLED
	g_consoleInput->AddDebugMenu("ENGINE/Sound/Script Editor UI", SoundScriptEditorUIDraw);
	//g_consoleInput->AddDebugMenu("ENGINE/Render/Instance Manager Debug UI", DemoInstManagerDebugDrawUI);
#endif

	g_pHost->SetWindowTitle(GAME_WINDOW_TITLE);
	g_eqCore->RegisterInterface(&s_shapeCache);
	g_studioCache->Init(g_parallelJobs->GetJobMng());

	g_matSystem->RegisterShaderOverride("BaseUnlit", [](int instanceFormatId) -> const char* {
		if (instanceFormatId == SHADER_VERTEX_ID(EGFVertexGameObj))
			return "BaseUnlitGame";
		return nullptr;
	});

	g_matSystem->RegisterShaderOverride("DrvSynVehicle", [](int instanceFormatId) -> const char* {
		return "BaseUnlitGame";
	});

	g_matSystem->RegisterShaderOverride("Skinned", [](int instanceFormatId) -> const char* {
		return "BaseUnlitGame";
	});

	g_matSystem->RegisterShaderOverride("BaseParticle", [](int instanceFormatId) -> const char* {
		return "BaseUnlit";
	});

	eqAppStateMng::SetCurrentStateType(APP_STATE_MAIN_GAMELOOP);

	return true;
}

void ShutdownAppStates()
{
	for (int i = 0; i < APP_STATE_COUNT; ++i)
		s_appStates[i] = nullptr;

#ifdef ENABLE_MULTIPLAYER
	Networking::ShutdownNetworking();
#endif

	g_studioCache->Shutdown();
	g_parallelJobs->Shutdown();
	g_eqCore->UnregisterInterface<CEmptyStudioShapeCache>();
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



