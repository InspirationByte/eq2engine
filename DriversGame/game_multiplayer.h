//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Networking game state
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_NET_H
#define GAME_NET_H

#include "session_stuff.h"
#include "Network/NETThread.h"

using namespace Networking;

#define TICK_INTERVAL			(g_svclientInfo.tickInterval)//(.015)  // 15 msec ticks
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)dt / TICK_INTERVAL ) )
#define TICKS_TO_TIME( t )		( TICK_INTERVAL *( t ) )
#define ROUND_TO_TICKS( t )		( TICK_INTERVAL * TIME_TO_TICKS( t ) )

enum EConnectStatus
{
	CONN_NONE				= -1,
	CONN_OK					= 0,
	CONN_ERROR,
};

enum EGameMsgs
{
	CMSG_CONNECT		= 0,
	CMSG_DISCONNECT,

	CMSG_CLIENTPLAYER_INFO,

	CMSG_SERVERPLAYER_INFO,

	CMSG_PLAYER_SYNC,
	CMSG_PLAYERPACKET,

	CMSG_OBJECT_SPAWN,
	CMSG_OBJECT_FRAME,
};

struct server_client_info_t
{
	int			clientID;
	int			playerID;

	int			maxPlayers;
	float		tickInterval;
};

extern server_client_info_t	g_svclientInfo;

//--------------------------------------------------------------------------

#define SV_NEW_PLAYER (-1)	// for server: place player in a free slot

class CNetPlayer;
struct netspawninfo_t;

class CNetGameSession : public CGameSession
{
	friend class CGameObject;

public:
	CNetGameSession();

	bool					Create_Server();
	bool					Create_Client();

	bool					DoConnect();

	void					Init();
	void					Shutdown();

	bool					IsClient() const;
	bool					IsServer() const;

	void					Update(float fDt);

	// makes new player
	CNetPlayer*				CreatePlayer(netspawninfo_t* spawnInfo, int clientID, int playerID, const char* name);

	// sends player disconnect message and removes from client/server list
	void					DisconnectPlayer( int playerID, const char* reason);

	// depends on server maxplayer
	int						GetFreePlayerSlots() const;

	int						GetMaxPlayers() const;
	int						GetNumPlayers() const;

	// initializes local player
	void					InitLocalPlayer(netspawninfo_t* spawnInfo, int clientID = -1, int playerID = 0);

	// sends netplayer spawns
	void					SendPlayerInfoList(int clientID);

	// sends object spawns for single client
	void					SendObjectSpawns( int clientID );

	void					SetLocalPlayer(CNetPlayer* player);
	CNetPlayer*				GetLocalPlayer() const;

	CCar*					GetPlayerCar() const;

	// search for player
	CNetPlayer*				GetPlayerByClientID(int clientID) const;
	CNetPlayer*				GetPlayerByID(int playerID) const;

	CNetworkThread*			GetNetThread();

	CGameObject*			FindNetworkObjectById( int id ) const;

public:
	int						GetSessionType() const {return SESSION_NETWORK;}
private:

	void					Net_SpawnObject( CGameObject* obj );
	void					Net_RemoveObject( CGameObject* obj );
	void					Net_SendObjectData( CGameObject* obj, int nClientID );

	int						FindUniqueNetworkObjectId();

	int						m_maxPlayers;

	CNetPlayer*				m_localPlayer;
	DkList<CNetPlayer*>		m_players;

	CNetworkThread			m_netThread;
	bool					m_isServer;
	bool					m_isClient;

	EConnectStatus			m_connectStatus;

	double					m_curTimeNetwork;
	double					m_prevTimeNetwork;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CNetGameSession, CGameSession)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC_CONST(FindNetworkObjectById)
	//OOLUA_MFUNC_CONST(GetPlayerByClientID)
	//OOLUA_MFUNC_CONST(GetPlayerByID)

	OOLUA_MFUNC_CONST(GetFreePlayerSlots)

	OOLUA_MFUNC_CONST(GetMaxPlayers)
	OOLUA_MFUNC_CONST(GetNumPlayers)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif NO_LUA

//---------------------------------------------------------------

enum ENetObjEventType
{
	NETOBJ_SPAWN = 0,
	NETOBJ_REMOVE
};

// object spawn/remove info
class CNetSpawnInfo : public CNetEvent
{
public:
	CNetSpawnInfo(CGameObject* obj, int evType);
	CNetSpawnInfo();

	void			Process( CNetworkThread* pNetThread );

	void			Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void			Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int				GetEventType() const {return CMSG_OBJECT_SPAWN;}

protected:
	int				m_clientID;
	sockaddr_in		m_clientAddr;

	CGameObject*	m_object;
	int				m_objectId;

	int				m_objEvent;
};

// object frame
class CNetObjectFrame : public CNetEvent
{
public:
	CNetObjectFrame(CGameObject* obj);
	CNetObjectFrame();

	void			Process( CNetworkThread* pNetThread );

	void			Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void			Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int				GetEventType() const {return CMSG_OBJECT_FRAME;}

protected:
	int				m_clientID;
	sockaddr_in		m_clientAddr;

	CGameObject*	m_object;
	int				m_objectId;
};

// object frame
class CNetSyncronizePlayerEvent : public CNetEvent
{
public:
	CNetSyncronizePlayerEvent();

	void			Process( CNetworkThread* pNetThread );

	void			Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void			Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream ) {}

	int				GetEventType() const {return CMSG_PLAYER_SYNC;}

protected:
	int				m_clientID;
};

#endif // GAME_NET_H