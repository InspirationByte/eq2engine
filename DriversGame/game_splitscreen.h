//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_SPLITSCREEN_H
#define GAME_SPLITSCREEN_H

#include "session_base.h"

#define SPLITSCREEN_MAXPLAYERS 4

struct playerControl_t
{
	int		buttons;
	float	steeringValue;
	float	accelBrakeValue;
};

struct playerData_t
{
	CCar*				car;
	playerControl_t		control;
};

//------------------------------------------------------

class CSplitScreenGameSession : public CGameSessionBase
{
public:
	CSplitScreenGameSession();
	virtual						~CSplitScreenGameSession();

	virtual void				Init();
	virtual void				Shutdown();

	virtual int					GetSessionType() const { return SESSION_SPLITSCREEN; }

	virtual bool				IsClient() const { return true; }
	virtual bool				IsServer() const { return true; }

protected:

	playerData_t				m_localPlayers[SPLITSCREEN_MAXPLAYERS];
	int							m_numLocalPlayers;
};


#endif // GAME_SINGLEPLAYER_H