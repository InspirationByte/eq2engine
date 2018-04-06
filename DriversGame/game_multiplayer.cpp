//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Networking game state
//////////////////////////////////////////////////////////////////////////////////

#include "luabinding/LuaBinding.h"

#include "game_multiplayer.h"
#include "NetPlayer.h"
#include "DrvSynStates.h"
#include "IDebugOverlay.h"
#include "sys_host.h"

extern ConVar			sv_maxplayers;
extern ConVar			g_car;

DECLARE_NETEVENT_LIST( SERVER_EVENTS );
DECLARE_NETEVENT_LIST( CLIENT_EVENTS );

ConVar					net_name("net_name", "unnamed", "Player name", CV_ARCHIVE);

ConVar					net_address("net_address", "localhost", "Network address to connect");
ConVar					net_clientport("net_clientport", varargs("%d", DEFAULT_CLIENTPORT), "Network client port");
ConVar					net_serverport("net_serverport", varargs("%d", DEFAULT_SERVERPORT), "Network server port");

ConVar					sv_rate("sv_rate", "60", "server rate (message count per second)", CV_ARCHIVE);
ConVar					cl_cmdrate("cl_cmdrate", "60", "client rate (message count per second)", CV_ARCHIVE);

ConVar					net_server("net_server", "1", "Is server");

CNetworkClient*			g_pClientInterface = new CNetworkClient();

server_client_info_t	g_svclientInfo;

//------------------------------------------------------------------------------------------------------------------------------

DECLARE_CMD(connect, "connects to server/game/lobby", 0)
{
	if(CMD_ARGC == 0)
	{
		MsgError("address not specified\n");
		return;
	}

	net_address.SetValue(CMD_ARGV(0).c_str());

	g_pClientInterface->SetClientID(-1);

	if( g_pClientInterface->Connect(net_address.GetString(), net_serverport.GetInt(), net_clientport.GetInt()))
	{
		CNetworkThread connectionThread(g_pClientInterface);

		connectionThread.RegisterEventList(NETEVENT_LIST(CLIENT_EVENTS), true);

		connectionThread.StartThread("ClientConnectionThread");

		CNetMessageBuffer outBuf;
		if( connectionThread.SendWaitDataEvent( new CNetConnectQueryEvent(net_name.GetString()), CMSG_CONNECT, &outBuf, -1) )
		{
            MsgInfo("Got response from server '%s' on connect\n", net_name.GetString());

			kvkeybase_t kvs;
			outBuf.ReadKeyValues( &kvs );

			EqString statusString(KV_GetValueString(kvs.FindKeyBase("status"), 0, "error"));

			if( !statusString.Compare("error") )
			{
				MsgError("Cannot connect, reason: %s\n", KV_GetValueString(kvs.FindKeyBase("desc")));
				return;
			}
			else if( !statusString.Compare("ok") )
			{
				connectionThread.StopWork();
				connectionThread.SetNetworkInterface(NULL);

				// set level name, set other info, and change states
				net_server.SetInt(0);

				EqString levName = KV_GetValueString(kvs.FindKeyBase("levelname"), 0, "default");
				int clientID = KV_GetValueInt(kvs.FindKeyBase("clientID"), 0, -1);
				int playerID = KV_GetValueInt(kvs.FindKeyBase("playerID"), 0, -1);
				int maxPlayers = KV_GetValueInt(kvs.FindKeyBase("maxPlayers"), 0, -1);
				int numPlayers = KV_GetValueInt(kvs.FindKeyBase("numPlayers"), 0, -1);
				int tickInterval = KV_GetValueInt(kvs.FindKeyBase("tickInterval"), 0, -1);
				EqString envName = KV_GetValueString(kvs.FindKeyBase("environment"), 0, "day_clear");

				MsgInfo("---- Server game info ----\n");
				MsgInfo("max players: %d\n", maxPlayers);
				MsgInfo("num players: %d\n", numPlayers);
				MsgInfo("tick intrvl: %d\n", tickInterval);
				MsgInfo("level: %s\n", levName.c_str());
				MsgInfo("env: %s\n", envName.c_str());

				sv_maxplayers.SetInt(maxPlayers);

				if( numPlayers <= maxPlayers )
				{
                    MsgInfo("Loading game\n");

					g_pClientInterface->SetClientID(clientID);
					g_svclientInfo.clientID = clientID;
					g_svclientInfo.playerID = playerID;
					g_svclientInfo.maxPlayers = maxPlayers;
					g_svclientInfo.tickInterval = tickInterval;

					// TODO: set client info to new session
					g_pGameWorld->SetLevelName( levName.c_str() );

					g_pGameWorld->SetEnvironmentName( envName.c_str() );

					// load game
					EqStateMgr::SetCurrentState(g_states[GAME_STATE_GAME]);
				}
			}
		}
		else
		{
			MsgWarning("Server not found at '%s:%d'\n", net_address.GetString(), net_serverport.GetInt());
			connectionThread.StopWork();
			connectionThread.SetNetworkInterface(NULL);
			g_pClientInterface->Shutdown();
		}
	}
}

DECLARE_CMD(disconnect, "shutdown game", 0)
{
	if(!g_pGameSession)
		return;

	if(g_pGameSession->GetSessionType() == SESSION_NETWORK)
	{
		CNetGameSession* ses = (CNetGameSession*)g_pGameSession;

		CNetPlayer* player = ses->GetLocalPlayer();

		if(player)
			ses->GetNetThread()->SendEvent( new CNetDisconnectEvent(player->m_id, "Disconnect by user"), CMSG_DISCONNECT, NM_SENDTOALL, CDPSEND_GUARANTEED );
	}

	if(EqStateMgr::GetCurrentStateType() == GAME_STATE_GAME)
		EqStateMgr::SetCurrentState(g_states[GAME_STATE_MAINMENU]);
}

DECLARE_CMD(ping, "shows ping of the clients", 0)
{
	if(!g_pGameSession)
		return;

	if(g_pGameSession->GetSessionType() != SESSION_NETWORK)
	{
		MsgError("No network game running\n");
		return;
	}

	CNetGameSession* ses = (CNetGameSession*)g_pGameSession;
	int numPlayers = ses->GetMaxPlayers();

	MsgInfo("--- Players ping on the server ---");
	for(int i = 0; i < numPlayers;i ++)
	{
		CNetPlayer* player = ses->GetPlayerByID(i);

		if(!player)
			continue;

		Msg("%s: %.1f ms\n", player->GetName(), player->GetLatency()*500.0f);
	}
}

//------------------------------------------------------------------------------------------------------------------------------

// intervention protection
EventFilterResult_e FilterServerUnwantedMessages( CNetworkThread* pNetThread, CNetMessageBuffer* pMsg, int nEventType )
{
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
}

//------------------------------------------------------------------------------------------------------------------------------

CNetGameSession::CNetGameSession() : m_netThread(NULL)
{
	m_curTimeNetwork = 0.0f;
	m_prevTimeNetwork = 0.0f;

	m_isClient = false;
	m_isServer = false;

	m_connectStatus = CONN_NONE;
	m_localPlayer = NULL;
}

bool CNetGameSession::Create_Server()
{
	CNetworkServer* pServer = new CNetworkServer();

	if(!pServer->Init(net_serverport.GetInt()))
	{
		delete pServer;

		MsgError("can't init network server\n");
		return false;
	}

	m_netThread.SetNetworkInterface(pServer);

	// register server messages
	m_netThread.RegisterEventList( NETEVENT_LIST(SERVER_EVENTS), true );
	m_netThread.SetEventFilterCallback( FilterServerUnwantedMessages );


	m_netThread.StartThread("DServerThread");

	m_isServer = true;

	m_maxPlayers = sv_maxplayers.GetInt();

	m_players.setNum( m_maxPlayers );

	for(int i = 0; i < m_players.numElem(); i++)
		m_players[i] = NULL;

	// TODO: get spawn point from mission script

	netPlayerSpawnInfo_t* spawn = new netPlayerSpawnInfo_t;
	spawn->m_spawnPos = Vector3D(0, 0.5, 10);
	spawn->m_spawnRot = Vector3D(0,0,0);
	spawn->m_spawnColor = Vector4D(0);
	spawn->m_useColor = false;

	// INIT SERVER LOCAL PLAYER
	InitLocalPlayer( spawn, NM_SERVER );

	return true;
}

void CNetGameSession::InitLocalPlayer(netPlayerSpawnInfo_t* spawnInfo, int clientID, int playerID)
{
	CNetPlayer* player = CreatePlayer(spawnInfo, clientID, playerID, net_name.GetString());

	if(!player)
	{
		MsgError("Cannot create local player (clid = %d)\n", clientID);
		return;
	}

	player->m_carName = g_car.GetString();
	player->m_isLocal = true;

	// server player is always ready
	if(IsServer())
		player->m_ready = true;

	player->m_ownCar = g_pGameSession->CreateCar( g_car.GetString() );

	if( player->m_ownCar )
	{
		player->m_ownCar->Spawn();

		player->m_ownCar->SetOrigin( spawnInfo->m_spawnPos );
		player->m_ownCar->SetAngles( spawnInfo->m_spawnRot );

		if(spawnInfo->m_useColor)
			player->m_ownCar->SetCarColour( spawnInfo->m_spawnColor );

		g_pGameWorld->AddObject( player->m_ownCar );

		if(IsClient())
			player->m_ownCar->m_networkID = spawnInfo->m_netCarID;

		SetLocalPlayer( player );
	}
}

void CNetGameSession::SetLocalPlayer(CNetPlayer* player)
{
	m_localPlayer = player;

	if(m_localPlayer)
	{
		SetPlayerCar( m_localPlayer->m_ownCar );
		g_pGameWorld->m_level.QueryNearestRegions( m_localPlayer->m_ownCar->GetOrigin(), true);
	}
}

CNetPlayer*	CNetGameSession::GetLocalPlayer() const
{
	return m_localPlayer;
}

CCar* CNetGameSession::GetPlayerCar() const
{
	if(m_localPlayer)
	{
		return m_localPlayer->m_ownCar;
	}

	return NULL;
}

CNetPlayer*	CNetGameSession::GetPlayerByClientID(int clientID) const
{
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		if(m_players[i]->m_clientID == clientID)
			return m_players[i];
	}

	return NULL;
}

CNetPlayer* CNetGameSession::GetPlayerByID(int playerID) const
{
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		if(m_players[i]->m_id == playerID)
			return m_players[i];
	}

	return NULL;
}

bool CNetGameSession::Create_Client()
{
	ASSERT(g_pClientInterface);

	m_netThread.SetNetworkInterface(g_pClientInterface);

	// register client messages
	m_netThread.RegisterEventList(NETEVENT_LIST(CLIENT_EVENTS), true);

	m_netThread.StartThread("DClientThread");
	m_isServer = false;
	m_isClient = true;

	m_maxPlayers = g_svclientInfo.maxPlayers;

	m_players.setNum( m_maxPlayers );

	for(int i = 0; i < m_players.numElem(); i++)
		m_players[i] = NULL;

	CNetMessageBuffer buffer;

    MsgInfo("Retrieving client info...\n");

	// send connection ready
	if( m_netThread.SendWaitDataEvent( new CNetClientPlayerInfo(g_car.GetString()), CMSG_CLIENTPLAYER_INFO, &buffer, -1 ) )
	{
		// TODO: get spawn position

		// update once - spawn
		g_pGameWorld->UpdateWorld(0.01f);
		g_pGameWorld->m_level.WaitForThread();

		// send syncronization status
		m_netThread.SendEvent( new CNetSyncronizePlayerEvent(), CMSG_PLAYER_SYNC, NM_SERVER, CDPSEND_GUARANTEED );
	}
	else
	{
        MsgError(" - Server has no response!\n");
        return false;
	}


	return true;
}

CGameObject* CNetGameSession::FindNetworkObjectById( int id ) const
{
	DkList<CGameObject*>& objList = g_pGameWorld->m_gameObjects;

	for(int i = 0; i < objList.numElem(); i++)
	{
		if( objList[i]->m_networkID == NETWORK_ID_OFFLINE )
			continue;

		if(objList[i]->m_networkID == id)
			return objList[i];
	}

	return NULL;
}

int SortObjectsById(CGameObject* const& a, CGameObject* const& b)
{
	return a->m_networkID - b->m_networkID;
}

int	CNetGameSession::FindUniqueNetworkObjectId()
{
	int nMinUnused = 0;

	// copy list of objects
	static DkList<CGameObject*> objList;
	objList.clear(false);
	objList.append(g_pGameWorld->m_gameObjects);

	objList.sort(SortObjectsById);

	int enumIdx = 0;

	for(int idx = 0; idx < objList.numElem(); idx++)
	{
		if(objList[idx]->m_networkID == NETWORK_ID_OFFLINE)
			continue;

        if ( objList[idx]->m_networkID == enumIdx )
		{
			enumIdx++;
			continue;
		}
        else
		{
			nMinUnused = enumIdx;
			break;
		}
	}

	int nBestId = nMinUnused;

	for(int i = 0; i < objList.numElem(); i++)
	{
		if( objList[i]->m_networkID == NETWORK_ID_OFFLINE )
			continue;

		if(nBestId == objList[i]->m_networkID)
			nBestId++;
	}

	return nBestId;
}

//--------------------------------------------------------------------------------------------------

void CNetGameSession::Net_SpawnObject( CGameObject* obj )
{
	int netId = FindUniqueNetworkObjectId();

	obj->m_networkID = netId;

	if(netId != NETWORK_ID_OFFLINE)
	{
		//Msg("Emit_NetSpawnObject - make object id %d\n", netId);

		// send spawn for existing clients
		m_netThread.SendEvent(new CNetSpawnInfo(obj, NETOBJ_SPAWN), CMSG_OBJECT_SPAWN, NM_SENDTOALL, 0);
	}
}

void CNetGameSession::Net_RemoveObject( CGameObject* obj )
{
	// remove
	//m_netThread.SendEvent(new CNetSpawnInfo(obj, NETOBJ_REMOVE), CMSG_OBJECT_SPAWN, NM_SENDTOALL, 0);
}

void CNetGameSession::Net_SendObjectData( CGameObject* obj, int nClientID )
{
	if(obj->m_networkID == NETWORK_ID_OFFLINE)
		return;

	obj->m_isNetworkStateChanged = false;

	m_netThread.SendEvent(new CNetObjectFrame(obj), CMSG_OBJECT_FRAME, nClientID);
}

void CNetGameSession::SendObjectSpawns( int clientID )
{
	for(int j = 0; j < g_pGameWorld->m_gameObjects.numElem(); j++)
	{
		CGameObject* obj = g_pGameWorld->m_gameObjects[j];

		if(obj->m_networkID == NETWORK_ID_OFFLINE)
			continue;

		// send spawn for existing clients
		m_netThread.SendEvent(new CNetSpawnInfo(obj, NETOBJ_SPAWN), CMSG_OBJECT_SPAWN, clientID, CDPSEND_GUARANTEED);
	}
}

//--------------------------------------------------------------------------------------------------

void CNetGameSession::SendPlayerInfoList( int clientID )
{
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		//if( !m_players[i]->IsReady() )
		//	continue;

		//Msg("[SERVER] sending player info %s %s %d\n", m_players[i]->m_carName.c_str(), m_players[i]->m_name.c_str(), m_players[i]->m_id);

		m_netThread.SendEvent( new CNetServerPlayerInfo( m_players[i] ), CMSG_SERVERPLAYER_INFO, NM_SENDTOALL, CDPSEND_GUARANTEED );
	}
}

void CNetGameSession::Init()
{
	LoadCarData();

	if(net_server.GetBool())
	{
		Create_Server();
	}
	else
	{
		Create_Client();
	}
}

void CNetGameSession::Shutdown()
{
	// TODO:	send player disconnect
	//			destroy thread

	if(m_isServer)
	{
		// send disconnect to all clients
		for(int i = 0; i < m_players.numElem(); i++)
		{
			if(m_players[i] == NULL)
				continue;

			// send to other clients
			m_netThread.SendEvent( new CNetDisconnectEvent(m_players[i]->m_id, "server shutdown"), CMSG_DISCONNECT, NM_SENDTOALL, CDPSEND_GUARANTEED );
		}


		m_netThread.StopWork();
		INetworkInterface* pNetInterface =  m_netThread.GetNetworkInterface();

		if(pNetInterface)
		{
			pNetInterface->Shutdown();
			delete pNetInterface;

			m_netThread.SetNetworkInterface(NULL);
		}
	}
	else
	{
		CNetPlayer* player = GetLocalPlayer();

		if(player)
		{
			m_netThread.SendEvent( new CNetDisconnectEvent(player->m_id, "Disconnect by user"), CMSG_DISCONNECT, NM_SERVER, CDPSEND_GUARANTEED );
		}

		// send disconnect
		INetworkInterface* pNetInterface = m_netThread.GetNetworkInterface();

		if(pNetInterface)
		{
			m_netThread.StopWork();
			m_netThread.SetNetworkInterface(NULL);

			g_pClientInterface->Shutdown();
		}
	}

	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] != NULL)
		{
			delete m_players[i];
			m_players[i] = NULL;
		}
	}

	m_localPlayer = NULL;

	CGameSession::Shutdown();
}

bool CNetGameSession::IsClient() const
{
	return !m_isServer || m_isClient;
}

bool CNetGameSession::IsServer() const
{
	return m_isServer;
}

CNetworkThread*	CNetGameSession::GetNetThread()
{
	return &m_netThread;
}

void CNetGameSession::Update(float fDt)
{
	if(m_localPlayer)
	{
		m_localPlayer->m_ready = true;

		CCar* playerCar = m_localPlayer->GetCar();

		if(playerCar)
		{
			g_pGameWorld->m_level.QueryNearestRegions( playerCar->GetOrigin(), false);

			playerCar->SetControlButtons( m_localControls );

			debugoverlay->Text(ColorRGBA(1,1,0,1), "Car speed: %.1f MPH", playerCar->GetSpeed());
		}
		else
		{
			g_pGameWorld->m_level.QueryNearestRegions( g_pGameWorld->m_view.GetOrigin(), false);
		}
	}

	float phys_begin = MEASURE_TIME_BEGIN();

	g_pPhysics->Simulate( fDt, PHYSICS_ITERATION_COUNT, NULL );

	debugoverlay->Text(ColorRGBA(1,1,0,1), "physics time, ms: %g", abs(MEASURE_TIME_STATS(phys_begin)));

	g_pGameWorld->UpdateWorld( fDt );

	//------------------------------------------------------------

	m_curTimeNetwork = g_pHost->GetCurTime();

	double fNetRate = IsServer() ? sv_rate.GetFloat() : cl_cmdrate.GetFloat();

	double fRateMs = 1.0 / fNetRate;

	float localPlayerLatency = 0.0;

	if(IsServer())
		g_svclientInfo.tickInterval = fRateMs;

	// simulate objects of world
	for(int i = 0; i < g_pGameWorld->m_gameObjects.numElem(); i++)
	{
		CGameObject* obj = g_pGameWorld->m_gameObjects[i];

		//obj->Simulate( fDt );

		// send object
		if(	IsServer() &&
			obj->m_isNetworkStateChanged &&
			(m_curTimeNetwork-m_prevTimeNetwork > fRateMs))
		{
			Net_SendObjectData(obj, NM_SENDTOALL);
		}
	}

	if(m_curTimeNetwork-m_prevTimeNetwork > fRateMs)
	{
		for(int i = 0; i < m_players.numElem(); i++)
		{
			if(m_players[i] == NULL)
				continue;

			if(m_players[i]->m_ready)
			{
				// set our controls
				if( m_players[i]->m_isLocal )
				{
					m_players[i]->SetControls( m_localControls );
				}

				m_players[i]->NetUpdate( fRateMs );

				if( m_players[i]->m_isLocal )
					localPlayerLatency = m_players[i]->m_packetLatency;
			}

			// disconnect me if needed
			if(m_players[i]->m_disconnect)
			{
				DisconnectPlayer( m_players[i]->m_id, "unknown" );
			}
		}

		m_prevTimeNetwork = m_curTimeNetwork;
	}

	m_netThread.UpdateDispatchEvents();

	int numPlayers = GetNumPlayers();

	debugoverlay->Text(ColorRGBA(1,1,0,1), "Num players: %d\n", numPlayers);

	if(IsClient())
		debugoverlay->Text(ColorRGBA(1,1,0,1), "Running CLIENT");
	else if(IsServer())
	{
		CNetworkServer* pServer = (CNetworkServer*)m_netThread.GetNetworkInterface();
		debugoverlay->Text(ColorRGBA(1,1,0,1), "Running SERVER");
		debugoverlay->Text(ColorRGBA(1,1,0,1), "	number of clients: %d", pServer->GetClientCount());
	}

	// refresh players in standard way
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		if(m_players[i]->m_ready)
		{
			// make latency equal for non-local players on client
			if( IsClient() && !m_players[i]->m_isLocal )
				m_players[i]->m_packetLatency = localPlayerLatency;

			m_players[i]->Update( fDt );

			// query nearest regions for servers to load
			if(	IsServer() &&
				m_players[i]->GetCar())
			{
				g_pGameWorld->m_level.QueryNearestRegions( m_players[i]->GetCar()->GetOrigin() );
			}
		}
	}
}

CNetPlayer* CNetGameSession::CreatePlayer(netPlayerSpawnInfo_t* spawnInfo, int clientID, int playerID, const char* name)
{
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		if(m_players[i]->m_clientID == clientID &&
			m_players[i]->m_id == playerID)
		{
			MsgError("Player is already connected (playerID=%d clientID = %d)\n", playerID, clientID);
			return NULL;
		}
	}

	for(int i = 0; i < m_players.numElem(); i++)
	{
		// place in the right slot if specified
		if(playerID != SV_NEW_PLAYER && playerID != i)
			continue;

		if(m_players[i] == NULL)
		{
			playerID = i;
			m_players[playerID] = new CNetPlayer( clientID, name );
			m_players[playerID]->m_spawnInfo = spawnInfo;
			m_players[playerID]->m_id = i;
			break;
		}
	}

	Msg("[SERVER] Player slot %d created\n", playerID);

	if(playerID == -1)
		return NULL;

	return m_players[playerID];
}

void CNetGameSession::DisconnectPlayer( int playerID, const char* reason )
{
	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
			continue;

		if(m_players[i]->m_id == playerID)
		{
			// first we need to remove the car
			if(m_players[i]->m_ownCar)
			{
				g_pGameWorld->RemoveObject(m_players[i]->m_ownCar);
				m_players[i]->m_ownCar = NULL;
			}

			if( m_players[i]->m_ready )
			{
				// TODO: send disconnected message to all clients
				Msg("%s left the game (%s)\n", m_players[i]->m_name.c_str(), reason);
			}
			else
			{
				Msg("Player '%s' timed out (connection lost?)\n", m_players[i]->m_name.c_str());
			}

			if( m_players[i]->m_isLocal )
			{
				m_localPlayer = NULL;
				m_playerCar = NULL;
			}
			else if(IsServer())
			{
				// remove client from server send list
				CNetworkServer* srv = (CNetworkServer*)m_netThread.GetNetworkInterface();
				srv->RemoveClientById( m_players[i]->m_clientID );

				// send to other clients
				m_netThread.SendEvent( new CNetDisconnectEvent(playerID, reason), CMSG_DISCONNECT, NM_SENDTOALL, CDPSEND_GUARANTEED );
			}

			delete m_players[i];
			m_players[i] = NULL;
		}
	}

	if(!m_localPlayer)
		EqStateMgr::SetCurrentState( g_states[GAME_STATE_MAINMENU] );
}

int CNetGameSession::GetMaxPlayers() const
{
	return m_maxPlayers;
}

int CNetGameSession::GetNumPlayers() const
{
	int nSlots = 0;

	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] != NULL)
		{
			nSlots++;
		}
	}

	return nSlots;
}

int CNetGameSession::GetFreePlayerSlots() const
{
	int nSlots = 0;

	for(int i = 0; i < m_players.numElem(); i++)
	{
		if(m_players[i] == NULL)
		{
			nSlots++;
		}
	}

	return nSlots;
}

bool CNetGameSession::DoConnect()
{
	net_server.SetInt(0);

	if( Create_Client() )
	{
		if(m_connectStatus != CONN_OK)
		{
			Shutdown();
			return false;
		}
		else
		{
			EqStateMgr::SetCurrentState( g_states[GAME_STATE_GAME] );
			return true;
		}
	}

	return false;
}

OOLUA_EXPORT_FUNCTIONS( CNetGameSession )
OOLUA_EXPORT_FUNCTIONS_CONST( CNetGameSession, FindNetworkObjectById, GetFreePlayerSlots, GetMaxPlayers, GetNumPlayers )

//---------------------------------------------------------------------------------------------

CNetSpawnInfo::CNetSpawnInfo(CGameObject* obj, int evType)
{
	m_object = obj;
	m_objectId = m_object->m_networkID;
	m_objEvent = evType;
}

CNetSpawnInfo::CNetSpawnInfo()
{
	m_object = NULL;
	m_objectId = -1;
}

void CNetSpawnInfo::Process( CNetworkThread* pNetThread )
{

}

void CNetSpawnInfo::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_objEvent = pStream->ReadInt();
	int objType = pStream->ReadInt();
	m_objectId = pStream->ReadInt();
	EqString objectName = pStream->ReadString();

	CNetGameSession* ses = (CNetGameSession*)g_pGameSession;

	if(m_objEvent == NETOBJ_SPAWN)
		m_object = g_pGameWorld->FindObjectByName(objectName.c_str());

	if(!m_object)
		m_object = ses->FindNetworkObjectById(m_objectId);

	if( m_object )
	{
		ASSERTMSG(m_object->ObjType() == objType, varargs("Object '%s' type is WRONG, check your network code", objectName.c_str()));

		if( m_objEvent == NETOBJ_SPAWN )
		{
			//Msg("Assign object ID %d on '%s' (%s)", m_objectId, m_object->GetName(), s_objectTypeNames[m_object->ObjType()]);
			m_object->m_networkID = m_objectId;

			m_object->OnUnpackMessage( pStream );
		}
		else
		{
			//Msg("Requested remove object ID %d on '%s'\n", m_objectId, objectName.c_str());
			g_pGameWorld->RemoveObject( m_object );
		}
	}
	else
	{
		// spawn object on client and assign ID
		//m_object->m_networkID = m_objectId;
	}
}

void CNetSpawnInfo::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	//Msg("Sending objectid=%d type=%s\n", m_objectId, s_objectTypeNames[m_object->ObjType()]);

	pStream->WriteInt( m_objEvent );
	pStream->WriteInt( m_object->ObjType() );
	pStream->WriteInt( m_objectId );
	pStream->WriteString( m_object->GetName() );

	if(m_objEvent == NETOBJ_SPAWN)
	{
		//Msg("Spawn pack object\n");
		m_object->OnPackMessage( pStream );
	}
}

//---------------------------------------------------------------------------------------------

CNetObjectFrame::CNetObjectFrame(CGameObject* obj)
{
	m_object = obj;
	m_objectId = m_object->m_networkID;
}

CNetObjectFrame::CNetObjectFrame()
{
	m_object = NULL;
	m_objectId = -1;
}

void CNetObjectFrame::Process( CNetworkThread* pNetThread )
{
	// find object


	// read in states
}

void CNetObjectFrame::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_objectId = pStream->ReadInt();
	int objType = pStream->ReadInt();

	CNetGameSession* ses = (CNetGameSession*)g_pGameSession;

	m_object = ses->FindNetworkObjectById(m_objectId);

	if(m_object && m_object->ObjType() == objType)
	{
		m_object->OnUnpackMessage( pStream );
	}
}

void CNetObjectFrame::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteInt( m_objectId );
	pStream->WriteInt( m_object->ObjType() );

	m_object->OnPackMessage( pStream );
}

//---------------------------------------------------------------------------------------------
CNetSyncronizePlayerEvent::CNetSyncronizePlayerEvent()
{
}

void CNetSyncronizePlayerEvent::Process( CNetworkThread* pNetThread )
{
	CNetGameSession* ses = (CNetGameSession*)g_pGameSession;
	ses->SendObjectSpawns( m_clientID );
}

void CNetSyncronizePlayerEvent::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_clientID = pStream->GetClientID();
}

//---------------------------------------------------------------------------------------------

DECLARE_NETEVENT(CMSG_CONNECT,			CNetConnectQueryEvent,		SERVER_EVENTS);

BEGIN_DECLARE_NETEVENT(CMSG_DISCONNECT, CNetDisconnectEvent)
	REGISTER_THREAD_NETEVENT( CLIENT_EVENTS );
	REGISTER_THREAD_NETEVENT( SERVER_EVENTS );
END_NETEVENT(CNetDisconnectEvent)

// Client player info - that is sent to the server, server responds with event below
DECLARE_NETEVENT(CMSG_CLIENTPLAYER_INFO,CNetClientPlayerInfo,		SERVER_EVENTS);

// Server player info - player spawn information, sync time and other things that confirms client connection
DECLARE_NETEVENT(CMSG_SERVERPLAYER_INFO,CNetServerPlayerInfo,		CLIENT_EVENTS);

DECLARE_NETEVENT(CMSG_PLAYER_SYNC,		CNetSyncronizePlayerEvent,	SERVER_EVENTS);

// register player controls packet
BEGIN_DECLARE_NETEVENT(CMSG_PLAYERPACKET, CNetPlayerPacket)
	REGISTER_THREAD_NETEVENT( CLIENT_EVENTS );
	REGISTER_THREAD_NETEVENT( SERVER_EVENTS );
END_NETEVENT(CNetPlayerPacket)

// register object frame on client and server
BEGIN_DECLARE_NETEVENT(CMSG_OBJECT_FRAME, CNetObjectFrame)
	REGISTER_THREAD_NETEVENT( CLIENT_EVENTS );
	REGISTER_THREAD_NETEVENT( SERVER_EVENTS );
END_NETEVENT(CNetObjectFrame)

// register object frame on client and server
BEGIN_DECLARE_NETEVENT(CMSG_OBJECT_SPAWN, CNetSpawnInfo)
	REGISTER_THREAD_NETEVENT( CLIENT_EVENTS );
	REGISTER_THREAD_NETEVENT( SERVER_EVENTS );
END_NETEVENT(CNetSpawnInfo)
