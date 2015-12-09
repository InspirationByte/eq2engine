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

using namespace Networking;

struct netsnapshot_s
{
	TVec4D<half>	car_rot;
	FVector3D		car_pos;
	Vector3D		car_vel;
	//Vector3D		car_angvel;

	// TODO: send damage info of vehicle

	int				controls;

	int				timeStamp;
};

ALIGNED_TYPE(netsnapshot_s,4) netsnapshot_t;

struct netinputcmd_s
{
	int controls;

	// FIXME: more controller info?

	int	timeStamp;
};

ALIGNED_TYPE(netinputcmd_s,4) netinputcmd_t;

struct playersnapshot_t
{
	playersnapshot_t() {}

	playersnapshot_t(const netsnapshot_t& snap)
	{
		car_rot = Quaternion(snap.car_rot.w,snap.car_rot.x,snap.car_rot.y,snap.car_rot.z);
		car_pos = snap.car_pos;
		car_vel = snap.car_vel;
		//car_angvel = snap.car_angvel;

		controls = snap.controls;
		timeStamp = snap.timeStamp;
	}

	Quaternion	car_rot;

	FVector3D	car_pos;
	Vector3D	car_vel;
	//Vector3D	car_angvel;

	int			controls;
	int			timeStamp;

	float		latency;

	//int			pad;
};

struct netspawninfo_t
{
	netspawninfo_t()
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

//
// Holds information about player of network state
//
class CNetPlayer
{
public:
						CNetPlayer( int clientID, const char* name );
						~CNetPlayer();

	const char*			GetCarName() const;
	void				SetControls(int controls);
	CCar*				GetCar();

	bool				IsAlive();

	// client recieve
	bool				AddSnapshot( const netsnapshot_t& snapshot );
	void				AddControlSnapshot(int controls);

	void				GetPredictedSnapshot( const playersnapshot_t& snapshot, float fDt_diff, playersnapshot_t& out );

	float				CalcLatency();

	void				NETSpawn();
	void				NetUpdate(float fDt);
	void				Update(float fDt);

	bool				IsReady();

//protected:

	netspawninfo_t*		m_spawnInfo;

	int					m_curControls;
	int					m_oldControls;

	playersnapshot_t	m_snapshots[2];
	int					m_curSnapshot;

	EqString			m_name;
	EqString			m_carName;
	CCar*				m_ownCar;

	int					m_clientID;
	int					m_id;

	bool				m_ready;
	bool				m_disconnect;

	bool				m_isLocal;
	float				m_fNotreadyTime;

	float				m_fCurTime;
	float				m_fLastCmdTime;

	//float				m_fPrevLastCmdTime;
	//float				m_packetTime;

	int					m_curTick;
	int					m_curSvTick;
	int					m_packetTick;
	int					m_lastPacketTick;

	int					m_lastCmdTick;
	int					m_lastPrevCmdTick;

	float				m_packetLatency;	// it's converted from tick difference * tick rate
	float				m_interpTime;

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
	CNetPlayerPacket( const netinputcmd_t& cmd, int nPlayerID, int curTick );
	CNetPlayerPacket( const netsnapshot_t& snapshot, int nPlayerID, int curTick );
	CNetPlayerPacket();

	void		Process( CNetworkThread* pNetThread );

	void		Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	void		Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream );

	int			GetEventType() const {return CMSG_PLAYERPACKET;}

protected:

	EPlayerPacketType	m_type;

	netinputcmd_t		m_inCmd;
	netsnapshot_t		m_snapshot;

	int					m_packetTick;

	int					m_playerID;
	int					m_clientID;
	sockaddr_in			m_clientAddr;
};

#endif // NETPLAYER_H
