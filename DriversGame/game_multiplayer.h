//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Networking game state
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_NET_H
#define GAME_NET_H

#include "session_stuff.h"

#include "net_game_defs.h"

#include "Network/NETThread.h"

using namespace Networking;


//--------------------------------------------------------------------------

#define SV_NEW_PLAYER (-1)	// for server: place player in a free slot

class CNetPlayer;
struct netPlayerSpawnInfo_t;

class CNetGameSession : public CGameSessionBase
{
	friend class CGameObject;

public:
	CNetGameSession();

	int						GetSessionType() const { return SESSION_NETWORK; }

	bool					Create_Server();
	bool					Create_Client();

	bool					DoConnect();

	void					Init();
	void					Shutdown();

	bool					IsClient() const;
	bool					IsServer() const;

	void					Update(float fDt);

	void					UpdateLocalControls(int nControls, float steering, float accel_brake);

	// makes new player
	CNetPlayer*				CreatePlayer(netPlayerSpawnInfo_t* spawnInfo, int clientID, int playerID, const char* name);

	// sends player disconnect message and removes from client/server list
	void					DisconnectPlayer( int playerID, const char* reason);

	// depends on server maxplayer
	int						GetFreePlayerSlots() const;

	int						GetMaxPlayers() const;
	int						GetNumPlayers() const;

	// initializes local player
	void					InitLocalPlayer(netPlayerSpawnInfo_t* spawnInfo, int clientID = -1, int playerID = 0);

	// sends netplayer spawns
	void					SendPlayerInfoList(int clientID);

	// sends object spawns for single client
	void					SendObjectSpawns( int clientID );

	void					SetLocalPlayer(CNetPlayer* player);
	CNetPlayer*				GetLocalPlayer() const;

	CCar*					GetPlayerCar() const;
	void					SetPlayerCar(CCar* pCar);

	// search for player
	CNetPlayer*				GetPlayerByClientID(int clientID) const;
	CNetPlayer*				GetPlayerByID(int playerID) const;

	CNetworkThread*			GetNetThread();

	CGameObject*			FindNetworkObjectById( int id ) const;

	float					GetLocalLatency() const;
private:

	void					UpdatePlayerControls();

	void					Net_SpawnObject( CGameObject* obj );
	void					Net_RemoveObject( CGameObject* obj );
	void					Net_SendObjectData( CGameObject* obj, int nClientID );

	void					SV_ProcessDuplicatePlayerName(CNetPlayer* player);
	void					SV_ScriptedPlayerProvision(CNetPlayer* player);

	int						FindUniqueNetworkObjectId();

	int						m_maxPlayers;

	CNetPlayer*				m_localNetPlayer;
	playerControl_t			m_localControls;
	float					m_localPlayerLatency;


	DkList<CNetPlayer*>		m_netPlayers;

	CNetworkThread			m_netThread;
	bool					m_isServer;
	bool					m_isClient;

	EConnectStatus			m_connectStatus;

	double					m_curTimeNetwork;
	double					m_prevTimeNetwork;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CNetGameSession, CGameSessionBase)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC_CONST(FindNetworkObjectById)
	//OOLUA_MFUNC_CONST(GetPlayerByClientID)
	//OOLUA_MFUNC_CONST(GetPlayerByID)

	OOLUA_MFUNC_CONST(GetFreePlayerSlots)

	OOLUA_MFUNC_CONST(GetMaxPlayers)
	OOLUA_MFUNC_CONST(GetNumPlayers)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

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
	CNetSpawnInfo(CGameObject* obj, ENetObjEventType evType);
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

	ENetObjEventType	m_objEvent;
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
