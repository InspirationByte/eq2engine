//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech engine host - header
//////////////////////////////////////////////////////////////////////////////////

#ifndef SYS_ENGINEHOST_H
#define SYS_ENGINEHOST_H

#include "DebugInterface.h"
#include "IEngineHost.h"

// Used to optimize mouse input/output
struct inputCommand_t
{
	HWND hwnd;
	UINT message;
	WPARAM wParam;
	LPARAM lParam;
};

//-----------------------------------------------------------------------------
// Equilibrium Engine host
//-----------------------------------------------------------------------------


class CEngineHost : public IEngineHost
{
public:
	
	friend class			CFont;

							CEngineHost();
	virtual					~CEngineHost();

	bool					Init();

	void					Shutdown();
	int						Frame();

	bool					FilterTime( float dTime );

	void					BeginScene();
	void					EndScene();

	double					GetFrameTime();
	double					GetCurTime();
	
	void					ProcessKeyChar( ubyte chr );
	void					TrapKey_Event( int key, bool down );
	void					TrapMouse_Event(float x, float y, int buttons, bool down );
	void					TrapMouseMove_Event( int x, int y, float deltaX, float deltaY );
	void					TrapMouseWheel_Event(int x, int y, int scroll);

	void					SetWindowSize(int width,int height);
	void					SetCursorPosition(const IVector2D &pos);
	IVector2D				GetCursorPosition();

	IVector2D				GetWindowSize();

	void					SetCursorShow(bool bShow, bool console = false);

	void					StartTrapMode();
	bool					IsTrapping();
	bool					CheckDoneTrapping( int& buttons, int& key );

	int						GetQuitting();
	void					SetQuitting( int quittype );

	EQWNDHANDLE				GetWindowHandle();
	IEqFont*				GetDefaultFont();

	IShaderAPI*				GetRenderer();
	IPhysics*				GetPhysics();
	IMaterialSystem*		GetMaterialSystem();

	IEqFont*				LoadFont(const char* pszFontName);

	void					EnterResourceLoading();
	void					EndResourceLoading();

	int						GetResourceLoadingTimes();

protected:
	bool					LoadRenderer();
	bool					LoadPhysics();
	bool					LoadGameLibrary();

	bool					InitSubSystems();

private:
	int						m_nQuitting;

	double					m_fCurTime;
	double					m_fFrameTime;
	double					m_fOldTime;

	bool					m_bTrapMode;
	bool					m_bDoneTrapping;
	int						m_nTrapKey;
	int						m_nTrapButtons;

	int						m_nWidth;
	int						m_nHeight;

	IVector2D				m_vCursorPosition;

	IEqFont*				m_pDefaultFont;

	EQWNDHANDLE				m_hHWND;

	bool					m_bDisableMouseUpdatePerFrame;

	int						m_nLoadingResourceTimes;
	float					m_fResourceLoadTimes;
};

extern DkList<inputCommand_t> g_inputCommands;

// Initializes engine
bool InitEngineHost();
void ShutdownEngineHost();

bool RegisterWindowClass(HINSTANCE hInst);
int  RunEngine(HINSTANCE hInst);


#endif //SYS_ENGINEHOST_H