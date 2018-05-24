//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
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

	int			maxPlayers;
	float		tickInterval;
};

extern server_client_info_t		g_svclientInfo;
extern CReplayData*				g_replayData;

#endif // SESSION_STUFF_H