//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: State of game
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_GAME_H
#define STATE_GAME_H

#include "ui_luaMenu.h"
#include "StateManager.h"

enum EPauseMode
{
	PAUSEMODE_NONE = 0,
	PAUSEMODE_PAUSE,
	PAUSEMODE_GAMEOVER,
	PAUSEMODE_COMPLETE,
};

//--------------------------------------------------------------------------------------

class CState_Game : public CBaseStateHandler, public CLuaMenu
{
public:
				CState_Game();
				~CState_Game();

	int			GetType() const {return GAME_STATE_GAME;}

	bool		Update( float fDt );
	bool		UpdatePauseState();

	void		DrawMenu( float fDt );

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	void		HandleKeyPress( int key, bool down );
	void		HandleMouseMove( int x, int y, float deltaX, float deltaY );
	void		HandleMouseWheel(int x,int y,int scroll);

	void		HandleJoyAxis( short axis, short value );

	int			GetPauseMode() const;
	void		SetPauseState( bool pause );

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

	EqString	m_gameMenuName;
};

//--------------------------------------------------------------------------------------

class CGameSession;

extern CGameSession* g_pGameSession;
extern CState_Game* g_State_Game;

//--------------------------------------------------------------------------------------

bool	LoadMissionScript( const char* name );

#endif // STATE_GAME_H
