#pragma once
#include "states.h"

class CParticleBatch;
class IGPURenderPassRecorder;

class CState_GpuDrivenDemo : public CAppStateBase
{
public:
	CState_GpuDrivenDemo();
	int			GetType() const { return APP_STATE_MAIN_GAMELOOP; }

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

	void		InitGame();

	int			m_cameraButtons{ 0 };
private:

	void		StepGame(float fDt);
};

extern CStaticAutoPtr<CState_GpuDrivenDemo> g_State_Demo;