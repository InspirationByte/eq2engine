//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: State of game
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_GAME_H
#define STATE_GAME_H

#include "StateManager.h"
#include "ui_luaMenu.h"

//--------------------------------------------------------------------------------------

class CState_Game : public CBaseStateHandler, public CLuaMenu
{
public:
				CState_Game();
				~CState_Game();

	int			GetType() {return GAME_STATE_GAME;}

	bool		Update( float fDt );

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	void		HandleKeyPress( int key, bool down );
	void		HandleMouseMove( int x, int y, float deltaX, float deltaY );
	void		HandleMouseWheel(int x,int y,int scroll);

	void		HandleJoyAxis( short axis, short value );

	//---------------------------------------------

	void		UnloadGame();
	void		LoadGame();

	void		StopStreams();

	void		QuickRestart(bool replay);

	void		SetDemoMode(bool mode) {m_demoMode = mode;}

	bool		IsGameRunning() {return m_isGameRunning;}

	void		OnEnterSelection( bool isFinal );

	void		OnMenuCommand( const char* command );

	void		SetupMenuStack( const char* name );

protected:

	bool		m_isGameRunning;
	bool		m_pauseState;
	float		m_fade;

	bool		m_loadingError;

	bool		m_demoMode;
	bool		m_exitGame;

	bool		m_showMenu;

	bool		m_sheduledRestart;
	bool		m_sheduledQuickReplay;
};

//--------------------------------------------------------------------------------------

class CGameSession;

extern CGameSession* g_pGameSession;
extern CState_Game* g_State_Game;

//--------------------------------------------------------------------------------------

bool	LoadMissionScript( const char* name );

#endif // STATE_GAME_H