//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers system and modules
//////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef KeyPress
#undef KeyPress
#endif

class IEqFont;
class ConCommand;
class CGameSystemJob;
class SyncJob;

struct VideoMode_t
{
	int displayId;
	uint bitsPerPixel;
	int width;
	int height;
	int refreshRate;
};

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
	bool				InitSystems();
	void				ShutdownSystems();

	bool				Frame();

	bool				IsInMultiplayerGame() const;
	void				SignalPause();

	void				OnWindowResize(int width, int height);
	void				OnFocusChanged(bool inFocus);

	EQWNDHANDLE			GetWindowHandle() const { return m_window; }

	bool				IsWindowed() const;
	void				SetFullscreenMode(bool screenSize);
	void				SetWindowedMode();
	void				ApplyVideoMode();
	void				ToggleFullscreen();

	void				GetVideoModes(Array<VideoMode_t>& displayModes) const;

	//---------------------------------
	// INPUT
	//---------------------------------

	void				RequestTextInput();
	void				ReleaseTextInput();
	bool				IsTextInputShown() const;

	void				ProcessKeyChar( const char* utfChar );
	void				TrapKey_Event( int key, bool down );
	void				TrapMouse_Event( float x, float y, int buttons, bool down );
	void				TrapMouseMove_Event( int x, int y, int dx, int dy );
	void				TrapMouseWheel_Event(int x, int y, int hscroll, int vscroll);

	void				TrapJoyAxis_Event( short axis, short value );
	void				TrapJoyButton_Event( short button, bool down);

	void				TouchMotion_Event( float x, float y, int finger );
	void				Touch_Event( float x, float y, int finger, bool down );

	void				StartTrapMode();
	bool				IsTrapping();
	bool				CheckDoneTrapping( int& buttons, int& key );

	void				SetCursorPosition(int x, int y);

	//---------------------------------

	void				SetWindowTitle(const char* windowTitle);

	double				GetFrameTime() const {return m_accumTime;}

	const IVector2D&	GetWindowSize() const {return m_winSize;}
	IEqFont*			GetDefaultFont() const {return m_defaultFont;}

	int					GetQuitState() const {return m_quitState;}

// static

	static void			HostQuitToDesktop();
	static void 		HostExitCmd(const ConCommand* cmd, Array<EqString>& args);

protected:

	void				UpdateCursorState();
	void				SetCursorShow(bool bShow);

	void				BeginScene();
	void				EndScene();

	bool				FilterTime( double fDt );

	IVector2D			m_winSize{ 0 };
	IVector2D			m_mousePos{ 0 };
	IVector2D			m_prevMousePos{ 0 };
	Vector2D			m_mouseDelta{ 0.0f };

	EqString			m_windowTitle;
	EQWNDHANDLE			m_window{ nullptr };

	IEqFont*			m_defaultFont{ nullptr };

	CEqTimer			m_timer;
	double				m_accumTime{ 0.0 };

	int					m_quitState{ QUIT_NOTQUITTING };

	int					m_trapKey{ 0 };
	int					m_trapButtons{ 0 };
	bool				m_keyTrapMode{ false };
	bool				m_keyDoneTrapping{ false };
	bool				m_skipMouseMove{ false };
	bool				m_cursorCentered{ false };
	bool				m_wantsToggleFullscreen{ false };
};

extern CStaticAutoPtr<CGameHost> g_pHost;
extern SyncJob* g_beginSceneJob;
extern SyncJob* g_endSceneJob;