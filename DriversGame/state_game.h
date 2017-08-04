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

class CCar;

class CState_Game : public CBaseStateHandler, public CLuaMenu
{
public:
				CState_Game();
				~CState_Game();

	int			GetType() const {return GAME_STATE_GAME;}

	bool		Update( float fDt );

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	void		HandleKeyPress( int key, bool down );

	void		HandleMouseClick( int x, int y, int buttons, bool down );
	void		HandleMouseMove( int x, int y, float deltaX, float deltaY );
	void		HandleMouseWheel(int x,int y,int scroll);

	void		HandleJoyAxis( short axis, short value );

	int			GetPauseMode() const;
	void		SetPauseState( bool pause );

	void		StartReplay( const char* path );

	void		GetMouseCursorProperties(bool &visible, bool& centered);

	//---------------------------------------------

	void		UnloadGame();
	void		LoadGame();

	bool		LoadMissionScript( const char* name );
	const char*	GetMissionScriptName() const;

	void		StopStreams();

	void		QuickRestart(bool replay);

	void		SetDemoMode(bool mode) {m_demoMode = mode;}

	bool		IsGameRunning() {return m_isGameRunning;}
	
	void		SetupMenuStack( const char* name );

protected:

	bool		DoLoadingFrame();

	bool		UpdatePauseState();

	void		DoGameFrame( float fDt );
	void		DoCameraUpdates( float fDt );

	void		RenderMainView3D( float fDt );
	void		RenderMainView2D( float fDt );

	void		DrawMenu( float fDt );
	void		DrawLoadingScreen();

	void		OnEnterSelection( bool isFinal );
	void		OnMenuCommand( const char* command );

	Vector3D	GetViewVelocity() const;
	CCar*		GetViewCar() const;

	//----------------------------------------------
	int			m_doLoadingFrames;

	bool		m_isGameRunning;
	bool		m_pauseState;
	float		m_fade;

	bool		m_loadingError;

	bool		m_demoMode;
	bool		m_exitGame;

	bool		m_showMenu;

	bool		m_scheduledRestart;
	int			m_scheduledQuickReplay;

	EqString	m_gameMenuName;

	EqString	m_missionScriptName;
};

//--------------------------------------------------------------------------------------

class CGameSession;

extern CGameSession* g_pGameSession;
extern CState_Game* g_State_Game;

//--------------------------------------------------------------------------------------

#endif // STATE_GAME_H
