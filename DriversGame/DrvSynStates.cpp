//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: States of Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "DrvSynStates.h"

#include "LuaBinding_Drivers.h"

#include "sys_host.h"

#include "session_stuff.h"

#include "state_title.h"
#include "state_mainmenu.h"
#include "state_lobby.h"

#include "physics.h"
#include "world.h"


#define GAME_WINDOW_TITLE	"The Driver Syndicate" //varargs("Driver Syndicate Alpha [%s] build %d", __DATE__, GetEngineBuildNumber())

CBaseStateHandler* g_states[GAME_STATE_COUNT] = { nullptr };

extern void DrvSyn_RegisterShaderOverrides();
extern void DrvSyn_RegisterEqUIControls();

extern ConVar sv_maxplayers;

namespace EqStateMgr
{

bool InitRegisterStates()
{
	// initialize shader overrides after libraries are loaded
	DrvSyn_RegisterShaderOverrides();

	// register UIs
	DrvSyn_RegisterEqUIControls();

	g_states[GAME_STATE_GAME] = g_State_Game;
	g_states[GAME_STATE_TITLESCREEN] = g_State_Title;
	g_states[GAME_STATE_MAINMENU] = g_State_MainMenu;
	g_states[GAME_STATE_MPLOBBY] = g_State_NetLobby;
	

	// Lua binding is initialized from here
	if (!LuaBinding_InitDriverSyndicateBindings(GetLuaState()))
	{
		ErrorMsg("Lua base initialization error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());
		return false;
	}

	g_pHost->SetWindowTitle(GAME_WINDOW_TITLE);

	return true;
}

bool IsMultiplayerGameState()
{
	// maxplayers set and ingame
	return GetCurrentStateType() == GAME_STATE_GAME && sv_maxplayers.GetInt() > 1;
}

bool IsInGameState()
{
	return GetCurrentStateType() != GAME_STATE_GAME;
}

void SignalPause()
{
	CState_Game* gameState = (CState_Game*)EqStateMgr::GetCurrentState();

	if (gameState->GetPauseMode() == PAUSEMODE_NONE)
	{
		gameState->SetPauseState(true);
		g_sounds->Update();
	}
}

}