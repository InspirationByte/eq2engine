//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Networking game defintions
//////////////////////////////////////////////////////////////////////////////////

#ifndef NET_GAME_DEFS_H
#define NET_GAME_DEFS_H

#define TICK_INTERVAL			(g_svclientInfo.tickInterval)//(.015)  // 15 msec ticks
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)dt / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )

enum EConnectStatus
{
	CONN_NONE = -1,
	CONN_OK = 0,
	CONN_ERROR,
};

enum EGameMsgs
{
	CMSG_CONNECT = 0,
	CMSG_DISCONNECT,

	CMSG_CLIENTPLAYER_INFO,

	CMSG_SERVERPLAYER_INFO,

	CMSG_PLAYER_SYNC,
	CMSG_PLAYERPACKET,

	CMSG_OBJECT_SPAWN,
	CMSG_OBJECT_FRAME,
};

#endif // NET_GAME_DEFS_H