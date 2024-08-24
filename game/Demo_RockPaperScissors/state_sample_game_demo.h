#pragma once
#include "states.h"

class CParticleBatch;
class CMoviePlayer;
class IGPURenderPassRecorder;
struct RPSObject;

class CState_SampleGameDemo : public CAppStateBase
{
public:
	CState_SampleGameDemo();
	int			GetType() const { return APP_STATE_SAMPLE_GAME_DEMO; }

	// when changed to this state
	// @from - used to transfer data
	void		OnEnter(CAppStateBase* from);

	// when the state changes to something
	// @to - used to transfer data
	void		OnLeave(CAppStateBase* to);

	// when 'false' returned the next state goes on
	bool		Update(float fDt);

	void		HandleKeyPress(int key, bool down);
	void		HandleMouseClick(int x, int y, int buttons, bool down);
	void		HandleMouseMove(int x, int y, float deltaX, float deltaY);
	void		HandleMouseWheel(int x, int y, int scroll);

	void		HandleJoyAxis(short axis, short value);

	void		GetMouseCursorProperties(bool& visible, bool& centered);

private:

	void		InitGame();

	void		InitMovie();
	void		ShowMovie(IGPURenderPassRecorder* cmdRecorder);
	
	Array<RPSObject>	m_objects{ PP_SL };

	CParticleBatch*		m_pfxGroup{ nullptr };
	CRefPtr<CMoviePlayer>	m_moviePlayer;

	float				m_zoomLevel{ 1.0f };
	Vector2D			m_pan{ 0.0f };
	bool				m_mouseDown{ false };
};

extern CStaticAutoPtr<CState_SampleGameDemo> g_State_SampleGameDemo;