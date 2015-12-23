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

#include "utils/eqtimer.h"

using namespace Threading;

EQWNDHANDLE CreateEngineWindow();

void InitWindowAndRun();

#ifdef _WIN32
bool RegisterWindowClass(HINSTANCE hInst);
#endif // _WIN32

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

	bool		LoadModules();
	bool		InitSystems( EQWNDHANDLE pWindow, bool bWindowed );
	void		ShutdownSystems();

	bool		Frame();
	bool		FilterTime( double fDt );

	void		SetWindowSize(int width, int height);

	void		BeginScene();

	void		EndScene();

	void		ProcessKeyChar( int chr );
	void		TrapKey_Event( int key, bool down );
	void		TrapMouse_Event( float x, float y, int buttons, bool down );
	void		TrapMouseMove_Event( int x, int y, float deltaX, float deltaY );
	void		TrapMouseWheel_Event(int x, int y, int scroll);

	void		TrapJoyAxis_Event( short axis, short value );
	void		TrapJoyBall_Event( short ball, short xrel, short yrel );
	void		TrapJoyButton_Event( short button, bool down);

	void		StartTrapMode();
	bool		IsTrapping();
	bool		CheckDoneTrapping( int& buttons, int& key );

	void		SetCursorPosition(int x, int y);
	void		SetCursorShow(bool bShow);

	void		SetCenterMouseEnable(bool center);

// protected:

	EQWNDHANDLE	m_pWindow;

	int			m_nWidth;
	int			m_nHeight;

	Vector2D	m_mousePos;

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
