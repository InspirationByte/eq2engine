//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: In-game lobby for managing multiplayer game
//////////////////////////////////////////////////////////////////////////////////

#include "state_lobby.h"
#include "game_multiplayer.h"

const int	DEFAULT_LOBBYSERVERPORT		= 12508;

ConVar net_lobbyport("net_lobbyport", varargs("%d", DEFAULT_LOBBYSERVERPORT), "Lobby server port");

// intervention protection
EventFilterResult_e FilterLobbyUnwantedMessages( CNetworkThread* pNetThread, CNetMessageBuffer* pMsg, int nEventType )
{
/*
	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(!netSes)
		return EVENTFILTER_ERROR_NOTALLOWED;

	int clid = pMsg->GetClientID();

	if( netSes->GetPlayerByClientID(clid) == NULL)
	{
		bool allow = (nEventType == CMSG_CONNECT) ||
					 (nEventType == CMSG_CLIENTPLAYER_INFO);

		return allow ? EVENTFILTER_OK : EVENTFILTER_ERROR_NOTALLOWED;
	}
	else
	{
		bool disallow = (nEventType == CMSG_CONNECT);

		return disallow ? EVENTFILTER_ERROR_NOTALLOWED : EVENTFILTER_OK;
	}
	*/

	bool disallow = (nEventType == CMSG_CONNECT);

	return disallow ? EVENTFILTER_ERROR_NOTALLOWED : EVENTFILTER_OK;
}

CState_NetGameLobby::CState_NetGameLobby() : m_server(true), m_netThread(NULL)
{

}

CState_NetGameLobby::~CState_NetGameLobby()
{

}

void CState_NetGameLobby::OnEnter( CBaseStateHandler* from )
{
	// Init net interface and thread

}

void CState_NetGameLobby::OnLeave( CBaseStateHandler* to )
{
	// destroy net thread and interface
}

bool CState_NetGameLobby::Update( float fDt )
{
	return true;
}

void CState_NetGameLobby::HandleKeyPress( int key, bool down )
{

}

void CState_NetGameLobby::OnEnterSelection( bool isFinal )
{

}

CState_NetGameLobby* g_State_NetLobby = new CState_NetGameLobby();