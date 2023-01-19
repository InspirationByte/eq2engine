#pragma once
#include "states.h"

class CParticleBatch;

enum class RPSType
{
	ROCK,
	PAPER,
	SCISSORS,

	COUNT,
};

struct RPSObject
{
	Vector2D	pos;
	RPSType		type;
};

class CState_SampleGameDemo : public CBaseStateHandler
{
public:
	CState_SampleGameDemo();
	int			GetType() const { return GAME_STATE_SAMPLE_GAME_DEMO; }

	// when changed to this state
	// @from - used to transfer data
	void		OnEnter(CBaseStateHandler* from);

	// when the state changes to something
	// @to - used to transfer data
	void		OnLeave(CBaseStateHandler* to);

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
	int			CheckWhoDefeats(const RPSObject& a, const RPSObject& b) const;

	CParticleBatch*		m_pfxGroup;

	Array<RPSObject>	m_objects{ PP_SL };

	float				m_zoomLevel{ 1.0f };
	Vector2D			m_pan{ 0.0f };
	bool				m_mouseDown{ false };
};

extern CState_SampleGameDemo* g_State_SampleGameDemo;