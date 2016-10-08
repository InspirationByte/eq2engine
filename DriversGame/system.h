//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers system and modules
//////////////////////////////////////////////////////////////////////////////////

#ifndef SYSTEM_H
#define SYSTEM_H

#include "materialsystem/IMaterialSystem.h"
#include "GameSoundEmitterSystem.h"
#include "in_keys_ident.h"
#include "IFont.h"
#include "eqParallelJobs.h"

#include "ConCommand.h"

#include "utils/eqtimer.h"

class CGameHost
{
public:
	enum
	{
		QUIT_NOTQUITTING = 0,
		QUIT_TODESKTOP,
		QUIT_RESTART
	};

						CGameHost();

	bool				LoadModules();
	bool				InitSystems( EQWNDHANDLE pWindow, bool bWindowed );
	void				ShutdownSystems();

	bool				Frame();

	bool				IsInMultiplayerGame() const;
	void				SignalPause();

	void				OnWindowResize(int width, int height);

	void				ProcessKeyChar( int chr );
	void				TrapKey_Event( int key, bool down );
	void				TrapMouse_Event( float x, float y, int buttons, bool down );
	void				TrapMouseMove_Event( int x, int y );
	void				TrapMouseWheel_Event(int x, int y, int scroll);

	void				TrapJoyAxis_Event( short axis, short value );
	void				TrapJoyBall_Event( short ball, short xrel, short yrel );
	void				TrapJoyButton_Event( short button, bool down);

	void				TouchMotion_Event( float x, float y, int finger );
	void				Touch_Event( float x, float y, int finger, bool down );

	void				StartTrapMode();
	bool				IsTrapping();
	bool				CheckDoneTrapping( int& buttons, int& key );

	void				SetCursorPosition(int x, int y);
	void				SetCursorShow(bool bShow);

	void				SetCenterMouseEnable(bool center);

	double				GetCurTime() const {return m_fCurTime;}
	double				GetFrameTime() const {return m_fGameFrameTime;}

	const IVector2D&	GetWindowSize() const {return m_winSize;}
	IEqFont*			GetDefaultFont() const {return m_pDefaultFont;}

	int					GetQuitState() const {return m_nQuitState;}

// static

	static void			HostQuitToDesktop();
	static void 		HostExitCmd(CONCOMMAND_ARGUMENTS);

protected:

	void				BeginScene();
	void				EndScene();
	bool				FilterTime( double fDt );

	EQWNDHANDLE	m_pWindow;

	IVector2D	m_winSize;
	IVector2D	m_mousePos;
	IVector2D	m_prevMousePos;
	Vector2D	m_mouseDelta;

	int			m_nQuitState;

	bool		m_bCenterMouse;
	bool		m_cursorVisible;

	bool		m_bTrapMode;
	bool		m_bDoneTrapping;
	int			m_nTrapKey;
	int			m_nTrapButtons;

	IEqFont*	m_pDefaultFont;

	CEqTimer	m_timer;

	double		m_fCurTime;
	double		m_fFrameTime;
	double		m_fOldTime;

	double		m_fGameCurTime;
	double		m_fGameFrameTime;
	double		m_fGameOldTime;
};

extern CGameHost* g_pHost;

#endif // SYSTEM_H
