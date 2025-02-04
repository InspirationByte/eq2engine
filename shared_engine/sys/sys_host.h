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

struct SysVideoMode
{
	int		displayId;
	uint	bitsPerPixel;
	int		width;
	int		height;
	int		refreshRate;
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
	void				RenderFrame();

	bool				IsPauseAllowed() const;
	void				SignalPause();

	void				OnWindowResize(int width, int height);
	void				OnFocusChanged(bool inFocus);

	EQWNDHANDLE			GetWindowHandle() const { return m_window; }

	bool				IsWindowed() const;
	void				SetFullscreenMode(bool screenSize);
	void				SetWindowedMode();
	void				ApplyVideoMode();
	void				ToggleFullscreen();

	void				GetVideoModes(Array<SysVideoMode>& displayModes) const;

	//---------------------------------
	// INPUT
	//---------------------------------

	void				RequestTextInput();
	void				ReleaseTextInput();
	bool				IsTextInputShown() const;

	void				ProcessKeyChar( const char* utfChar );
	void				Key_Event( int key, bool down );
	void				Mouse_Event( float x, float y, int buttons, bool down );
	void				MouseMove_Event( int x, int y, int dx, int dy );
	void				MouseWheel_Event(int x, int y, int hscroll, int vscroll);

	void				JoyAxis_Event( short axis, short value );
	void				JoyButton_Event( short button, bool down);

	void				TouchMotion_Event( float x, float y, int finger );
	void				Touch_Event( float x, float y, int finger, bool down );

	void				SetCursorPosition(int x, int y);

	//---------------------------------

	void				SetWindowTitle(const char* windowTitle);

	double				GetFrameTime() const {return m_accumTime;}

	const IVector2D&	GetWindowSize() const {return m_winSize;}
	IEqFont*			GetDefaultFont() const {return m_defaultFont;}

	int					GetQuitState() const {return m_quitState;}

// static

	static void			HostQuitToDesktop();
	static void 		HostExitCmd(const ConCommand* cmd, ArrayCRef<EqStringRef> args);

protected:

	void				UpdateCursorState();
	void				SetCursorShow(bool bShow);

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

	bool				m_skipMouseMove{ false };
	bool				m_cursorCentered{ false };
	bool				m_wantsToggleFullscreen{ false };
};

extern CStaticAutoPtr<CGameHost> g_pHost;
extern SyncJob* g_beginSceneJob;
extern SyncJob* g_endSceneJob;