//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
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
	bool				InitSystems( EQWNDHANDLE pWindow );
	void				ShutdownSystems();

	bool				Frame();

	bool				IsInMultiplayerGame() const;
	void				SignalPause();

	void				OnWindowResize(int width, int height);
	void				OnFocusChanged(bool inFocus);

	EQWNDHANDLE			GetWindowHandle() const { return m_pWindow; }
	void				SetFullscreenMode();
	void				SetWindowedMode();
	void				ApplyVideoMode();
	void				GetVideoModes(Array<VideoMode_t>& displayModes);

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
	IEqFont*			GetDefaultFont() const {return m_pDefaultFont;}

	int					GetQuitState() const {return m_nQuitState;}

// static

	static void			HostQuitToDesktop();
	static void 		HostExitCmd(ConCommand* cmd, Array<EqString>& args);

#ifdef PLAT_ANDROID
	void*				GetAndroidNativeWindowFromSDL() const;
	void*				GetEGLSurfaceFromSDL() const;
#endif // PLAT_ANDROID

protected:

	void				UpdateCursorState();
	void				SetCursorShow(bool bShow);

	void				BeginScene();
	void				EndScene();

	bool				FilterTime( double fDt );

	IVector2D			m_winSize;
	IVector2D			m_mousePos;
	IVector2D			m_prevMousePos;
	Vector2D			m_mouseDelta;

	CEqTimer			m_timer;

	EQWNDHANDLE			m_pWindow;

	IEqFont*			m_pDefaultFont;

	int					m_nQuitState;

	bool				m_bTrapMode;
	bool				m_bDoneTrapping;
	bool				m_skipMouseMove;

	int					m_nTrapKey;
	int					m_nTrapButtons;

	bool				m_cursorCentered;

	double				m_accumTime;
};

extern CGameHost* g_pHost;
