//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_SINGLEPLAYER_H
#define GAME_SINGLEPLAYER_H

#include "session_base.h"

class CSingleGameSession : public CGameSessionBase
{
public:
	CSingleGameSession();
	virtual				~CSingleGameSession();

	void				Init();
	void				Shutdown();

	virtual int			GetSessionType() const { return SESSION_SINGLE; }

	virtual bool		IsClient() const { return true; }
	virtual bool		IsServer() const { return true; }

	CCar*				GetPlayerCar() const;
	void				SetPlayerCar(CCar* pCar);

	void				UpdateLocalControls(int nControls, float steering, float accel_brake);

protected:
	void				UpdatePlayerControls();

	CCar*				m_playerCar;
	playerControl_t		m_playerControl;
};


#endif // GAME_SINGLEPLAYER_H