//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Various game session stuff
//////////////////////////////////////////////////////////////////////////////////

#ifndef SESSION_STUFF_H
#define SESSION_STUFF_H

#include "state_game.h"
#include "session_base.h"
#include "replay.h"

struct server_client_info_t
{
	int			clientID;
	int			playerID;

	EqString	playerName;

	int			maxPlayers;
	float		tickInterval;
};

extern server_client_info_t		g_svclientInfo;
extern CReplayTracker*				g_replayTracker;

#endif // SESSION_STUFF_H