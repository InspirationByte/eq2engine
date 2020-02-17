//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Network player
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETPLAYER_H
#define NETPLAYER_H

#include "Player.h"

#include "net_game_defs.h"

#include "car.h"
#include "utils/eqstring.h"
#include "Network/NETThread.h"
#include "predictable_object.h"

using namespace Networking;

//
// Player car spawn info
//
struct netPlayerSpawnInfo_t
{
	netPlayerSpawnInfo_t()
	{
		netCarID = -1;
	}

	Vector3D			spawnPos;
	Vector3D			spawnRot;

	EqString			carName;
	EqString			teamName;
	carColorScheme_t	spawnColor;

	int					netCarID;
};

//------------------------------------------------------------------------------------

//
// Holds information about player of network state
//
class CNetPlayer : public CPlayer
{
	friend class CPredictable;

public:
						CNetPlayer( int clientID, const char* name );
						~CNetPlayer();

	const char*			GetName() const;
	const char*			GetCarName() const;
	void				SetControls(const playerControl_t& controls);
	CCar*				GetCar();

	float				GetLatency() const;

	// when server recieves controls from player
	void				SV_OnRecieveControls(int controls);

	// client recieve
	bool				CL_AddSnapshot( const netSnapshot_t& snapshot );

	void				CL_GetPredictedSnapshot( const netObjSnapshot_t& snapshot, float fDt_diff, netObjSnapshot_t& out ) const;
	float				CL_GetSnapshotLatency() const;

	void				CL_Spawn();

	bool				Net_Update(float fDt);

	void				Update(float fDt);

	bool				IsReady();

//protected:

	EqString				m_name;
	int						m_dupNameId;
	int						m_hasPlayerNameOf;

	int						m_clientID;
	int						m_id;

	bool					m_ready;

	//----------------------------------------------------------------------
	// controls and snapshot stuff 
	//		TODO: to the structure

	netPlayerSpawnInfo_t*	m_spawnInfo;

	CCar*					m_ownCar;

	float					m_fCurTime;
	float					m_fLastCmdTime;

	int						m_curControls;
	int						m_oldControls;

	netObjSnapshot_t		m_snapshots[2];
	int						m_curSnapshot;

	int						m_curTick;
	int						m_curSvTick;
	int						m_packetTick;
	int						m_lastPacketTick;

	int						m_lastCmdTick;
	int						m_lastPrevCmdTick;

	float					m_packetLatency;	// it's converted from tick difference * tick rate
	float					m_interpTime;
};

//----------------------------------------------------------------------------

class CNetConnectQueryEvent : public CNetEvent
{
public:
	CNetConnectQueryEvent( kvkeybase_t& kvs);
	CNetConnectQueryEvent();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_CONNECT;}

protected:
	int			m_clientID;
	sockaddr_in	m_clientAddr;

	kvkeybase_t	m_kvs;
};

//----------------------------------------------------------------------------

class CNetDisconnectEvent : public CNetEvent
{
public:
	CNetDisconnectEvent( int playerID, const char* reasonTokn );
	CNetDisconnectEvent();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_DISCONNECT;}

protected:
	int			m_clientID;
	int			m_playerID;
	EqString	m_reasonString;
};

//----------------------------------------------------------------------------

class CNetClientPlayerInfo : public CNetEvent
{
public:
	CNetClientPlayerInfo(const char* carName);
	CNetClientPlayerInfo();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_CLIENTPLAYER_INFO;}

protected:
	int			m_clientID;
	sockaddr_in	m_clientAddr;

	// TODO: keyvalues
	EqString	m_carName;
};

//----------------------------------------------------------------------------

class CNetServerPlayerInfo : public CNetEvent
{
public:
	CNetServerPlayerInfo( CNetPlayer* player );
	CNetServerPlayerInfo();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_SERVERPLAYER_INFO;}

protected:
	kvkeybase_t			m_kvs;

	float				m_sync_time;
};

//----------------------------------------------------------------------------

enum EPlayerPacketType
{
	PL_PACKET_CONTROLS = 0,	// player controls
	PL_PACKET_SNAPSHOT,		// car physical properties snapshot
};

class CNetPlayerPacket : public CNetEvent
{
public:
	CNetPlayerPacket( const netInputCmd_t& cmd, int nPlayerID, int curTick );
	CNetPlayerPacket( const netSnapshot_t& snapshot, int nPlayerID, int curTick );
	CNetPlayerPacket();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_PLAYERPACKET;}

protected:

	EPlayerPacketType	m_type;

	netInputCmd_t		m_inCmd;
	netSnapshot_t		m_snapshot;

	int					m_packetTick;

	int					m_playerID;
	int					m_clientID;
	sockaddr_in			m_clientAddr;
};

#endif // NETPLAYER_H
