//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network player
//////////////////////////////////////////////////////////////////////////////////

#ifndef NETPLAYER_H
#define NETPLAYER_H

#include "game_multiplayer.h"

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
		m_useColor = true;
		m_netCarID = -1;
	}

	Vector3D			m_spawnPos;
	Vector3D			m_spawnRot;
	carColorScheme_t	m_spawnColor;

	int					m_netCarID;

	bool				m_useColor;
};

//------------------------------------------------------------------------------------

//
// Holds information about player of network state
//
class CNetPlayer
{
	friend class CPredictable;

public:
						CNetPlayer( int clientID, const char* name );
						~CNetPlayer();

	const char*			GetCarName() const;
	void				SetControls(int controls);
	CCar*				GetCar();

	bool				IsAlive();

	float				GetLatency() const;

	// when server recieves controls from player
	void				OnServerRecieveControls(int controls);

	// client recieve
	bool				AddSnapshot( const netSnapshot_t& snapshot );

	void				GetPredictedSnapshot( const netObjSnapshot_t& snapshot, float fDt_diff, netObjSnapshot_t& out ) const;
	float				GetSnapshotLatency() const;


	void				NETSpawn();
	void				NetUpdate(float fDt);
	void				Update(float fDt);

	bool				IsReady();

//protected:

	netPlayerSpawnInfo_t*	m_spawnInfo;

	EqString				m_name;
	EqString				m_carName;
	CCar*					m_ownCar;

	int						m_clientID;
	int						m_id;

	bool					m_ready;
	bool					m_disconnect;

	bool					m_isLocal;
	float					m_fNotreadyTime;

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
	CNetConnectQueryEvent( const char* playername );
	CNetConnectQueryEvent();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_CONNECT;}

protected:
	int			m_clientID;
	sockaddr_in	m_clientAddr;
	EqString	m_playerName;
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
	int					m_clientID;
	int					m_playerID;
	int					m_carNetID;

	Vector3D			m_position;
	Quaternion			m_rotation;

	carColorScheme_t	m_carColor;

	float				m_sync_time;

	EqString			m_carName;
	EqString			m_playerName;

	sockaddr_in			m_clientAddr;
};

//----------------------------------------------------------------------------

enum EPlayerPacketType
{
	PL_PACKET_CONTROLS = 0,	// player controls
	PL_PACKET_PROPS,		// properties like position
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
