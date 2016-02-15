//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2016
//////////////////////////////////////////////////////////////////////////////////
// Description: In-game lobby for managing multiplayer game
//////////////////////////////////////////////////////////////////////////////////

#ifndef STATE_LOBBY_H
#define STATE_LOBBY_H

#include "session_stuff.h"
#include "Network/NETThread.h"

enum ELobbyMsgs
{
	CMSG_LOBBY_CONNECT		= 0,
	CMSG_LOBBY_DISCONNECT,

	CMSG_LOBBY_CLIENT_INFO,	// send/recieve
	CMSG_LOBBY_SERVER_INFO,	// send/recieve
};

//
// lobby menu state class
//
class CState_NetGameLobby : public CBaseStateHandler, public CLuaMenu
{
public:
				CState_NetGameLobby();
				~CState_NetGameLobby();

	bool		IsServer() const {return m_server;}
	void		SetServer( bool isServer ) {m_server = isServer;}

	int			GetType() const {return GAME_STATE_MPLOBBY;}

	void		OnEnter( CBaseStateHandler* from );
	void		OnLeave( CBaseStateHandler* to );

	bool		Update( float fDt );

	void		HandleKeyPress( int key, bool down );

	void		HandleMouseMove( int x, int y, float deltaX, float deltaY ) {}
	void		HandleMouseWheel(int x,int y,int scroll) {}

	void		HandleJoyAxis( short axis, short value ) {}

	void		OnEnterSelection( bool isFinal );

protected:
	bool				m_server;

	INetworkInterface*	m_netIface;
	CNetworkThread		m_netThread;
};

extern CState_NetGameLobby* g_State_NetLobby;

#endif // LOBBY_H