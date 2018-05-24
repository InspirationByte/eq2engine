//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Network player
//////////////////////////////////////////////////////////////////////////////////

#include "NetPlayer.h"
#include "IDebugOverlay.h"

#pragma todo("joystick parameters")

ConVar cl_predict("cl_predict", "1", "Prediction");
ConVar cl_predict_ratio("cl_predict_ratio", "0.3", "Client prediction ratio");
ConVar cl_predict_tolerance("cl_predict_tolerance", "5.0", "Interpolation tolerance");
ConVar cl_predict_angtolerance("cl_predict_angtolerance", "0.7", "Angular Interpolation tolerance");
ConVar cl_predict_correct_power("cl_predict_correct_power", "2.0", "Tolerance correction power");

ConVar cl_predict_debug("cl_debug_predict", "0", "Client prediction debug");

float GetSnapshotInterp( const netObjSnapshot_t& a, const netObjSnapshot_t& b, int curTick, float fTime )
{
	int snapTimeA = a.in.timeStamp;
	int snapTimeB = b.in.timeStamp;

	int tickDiff = snapTimeB-snapTimeA;

	float timeDiff = TICKS_TO_TIME(tickDiff);

	float fLerp = fTime / timeDiff;

	debugoverlay->Text(ColorRGBA(1,1,0,1), "GetSnapshotInterp: fTime=%.3f s, timeDiff=%.3f s numTickDiff=%d LERP=%g\n", fTime, timeDiff, tickDiff, fLerp);

	return fLerp;
}

void InterpolateSnapshots(const netObjSnapshot_t& a, const netObjSnapshot_t& b, float factor, netObjSnapshot_t& result)
{
	float fLerp = clamp(factor, -1.0f, 2.0f);

	result.position = lerp(a.position, b.position, fLerp);

	// NaN test
	if(!isnan(fLerp))
	{
		Quaternion resRot = slerp(a.rotation, b.rotation, fLerp);
		resRot.fastNormalize();
		result.rotation = resRot;
	}
	else
		result.rotation = identity();

	//result.car_angvel = lerp(a.car_angvel, b.car_angvel, fLerp);
	result.velocity = lerp(a.velocity, b.velocity, fLerp);
	result.in.controls = (fLerp < 0.5f) ? a.in.controls : b.in.controls;
	result.in.timeStamp = 0;
}

//
// Holds information about player of network state
//
CNetPlayer::CNetPlayer( int clientID, const char* name )
{
	m_name = name;
	m_carName = "buick";

	m_spawnInfo = NULL;

	m_clientID = clientID;
	m_ready = false;
	m_disconnectSignal = false;
	m_fNotreadyTime = 0.0f;
	m_ownCar = NULL;
	m_curControls = 0;
	m_oldControls = 0;

	m_fCurTime = 0.0f;
	//m_fLastCmdTime = 0.0f;
	//m_fPrevLastCmdTime = 0.0f;

	m_curSnapshot = 0;
	memset(m_snapshots, 0, sizeof(m_snapshots));
	m_snapshots[0].rotation = identity();
	m_snapshots[1].rotation = identity();

	m_packetLatency = 0.0f;
	m_interpTime = 0.0f;

	//--
	m_lastPacketTick = 0;
	m_curTick = 0;
	m_curSvTick = 0;
	m_packetTick = 0;
	m_lastCmdTick = 0;
	m_lastPrevCmdTick = 0;
}

CNetPlayer::~CNetPlayer()
{

}

const char* CNetPlayer::GetName() const
{
	return m_name.c_str();
}

const char* CNetPlayer::GetCarName() const
{
	return m_carName.c_str();
}

void CNetPlayer::SetControls(int controls)
{
	m_oldControls = m_curControls;
	m_curControls = controls;
}

CCar* CNetPlayer::GetCar()
{
	return m_ownCar;
}

bool CNetPlayer::IsAlive()
{
	return true;
}

// client recieve
bool CNetPlayer::CL_AddSnapshot( const netSnapshot_t& snapshot )
{
	if( m_snapshots[0].in.timeStamp >= snapshot.in.timeStamp)
		return false;

	if( m_snapshots[1].in.timeStamp >= snapshot.in.timeStamp )
		return false;

	m_snapshots[m_curSnapshot] = snapshot;

	int tickDiff = m_lastCmdTick - m_lastPrevCmdTick;

	m_snapshots[m_curSnapshot].latency = TICKS_TO_TIME(tickDiff);

	m_curSnapshot++;

	if(m_curSnapshot > 1)
		m_curSnapshot = 0;

	m_interpTime = 0.0f;
	m_fLastCmdTime = m_fCurTime;

	m_lastPrevCmdTick = m_lastCmdTick;
	m_lastCmdTick = m_curTick;

	return true;
}

void CNetPlayer::SV_OnRecieveControls(int controls)
{
	m_lastPrevCmdTick = m_lastCmdTick;
	m_lastCmdTick = m_curTick;

	m_fLastCmdTime = m_fCurTime;

	m_curControls = controls;
}

void CNetPlayer::NETSpawn()
{
	m_ownCar = g_pGameSession->CreateCar( m_carName.c_str() );

	if(!m_ownCar)
		return;

	if(!m_spawnInfo)
		return;

	m_ownCar->Spawn();

	m_ownCar->SetOrigin(m_spawnInfo->m_spawnPos);
	m_ownCar->SetAngles(m_spawnInfo->m_spawnRot);
	g_pGameWorld->AddObject( m_ownCar );

	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(netSes == NULL)
		return;

	if(m_spawnInfo->m_useColor)
		m_ownCar->SetCarColour(m_spawnInfo->m_spawnColor);

	// work on client side spawns
	if(netSes->IsClient())
		m_ownCar->m_networkID = m_spawnInfo->m_netCarID;

	Msg("[SERVER] %s spawn\n", m_name.c_str());

	delete m_spawnInfo;
	m_spawnInfo = NULL;
}

void UTIL_DebugDrawOBB(const FVector3D& pos, const Vector3D& mins, const Vector3D& maxs, const Matrix4x4& matrix, const ColorRGBA& color);

void CNetPlayer::Update(float fDt)
{
	if(!m_ownCar)
		return;

	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if( netSes == NULL )
		return;

	if( netSes->IsServer() )
	{
		if(netSes->GetLocalPlayer() == this)
		{
			m_lastPrevCmdTick = m_lastCmdTick;
			m_lastCmdTick = m_curTick;
			m_fLastCmdTime = m_fCurTime;
		}
	}

	if(m_ready && m_fCurTime-m_fLastCmdTime > 5.0f)
	{
#pragma todo("proper warning for connection problems")
		if(netSes->IsClient())
			MsgWarning("WARNING: connection lost...\n");

		m_ready = false;
		m_disconnectSignal = true;
	}

	float fOverallLatency = CL_GetSnapshotLatency() * 0.5f;

	debugoverlay->Text(ColorRGBA(1,1,1,1), "latency : %.2f ms (pck+snap=%.2f ms) (tick=%d)\n", m_packetLatency*500.0f, fOverallLatency*1000.0f, m_curTick);

	// update snapshot interpolation
	// update prediction
	// all clientside-only
	if(netSes->IsClient())
	{
		// interpolate snapshot
		netObjSnapshot_t interpSnap;

		float fSnapDelay = fOverallLatency;
		float fLerp = 0.0f;

		if(m_snapshots[0].in.timeStamp > m_snapshots[1].in.timeStamp)
		{
			fLerp = GetSnapshotInterp(m_snapshots[1], m_snapshots[0], m_curTick, m_interpTime);
			InterpolateSnapshots(m_snapshots[1], m_snapshots[0], fLerp, interpSnap);
		}
		else
		{
			fLerp = GetSnapshotInterp(m_snapshots[0], m_snapshots[1], m_curTick, m_interpTime);
			InterpolateSnapshots(m_snapshots[0], m_snapshots[1], fLerp, interpSnap);
		}

		netObjSnapshot_t finSnap;

		// find prediction snapshot
		netObjSnapshot_t predSnap;
		CL_GetPredictedSnapshot(interpSnap, (fDt+fSnapDelay)*cl_predict_ratio.GetFloat(), predSnap);

		FVector3D car_pos	= m_ownCar->GetPhysicsBody()->GetPosition();
		Quaternion car_rot	= m_ownCar->GetOrientation();
		Vector3D car_vel	= m_ownCar->GetVelocity();
		Vector3D car_angvel	= m_ownCar->GetAngularVelocity();

		if(cl_predict.GetBool())
		{
			finSnap = predSnap;
		}
		else
		{
			finSnap = interpSnap;
		}

		float pos_diff = length(car_pos - finSnap.position);
		float vel_diff = length(car_vel-finSnap.velocity);
		float angDiffAngle = acos((car_rot * !finSnap.rotation).w)*2.0f;

		// debug render
		if(cl_predict_debug.GetBool())
		{
			Matrix4x4 mat(interpSnap.rotation);
			UTIL_DebugDrawOBB(interpSnap.position,m_ownCar->GetPhysicsBody()->m_aabb.minPoint, m_ownCar->GetPhysicsBody()->m_aabb.maxPoint, mat, ColorRGBA(0.2, 1, 0.2, 0.1f));

			Matrix4x4 mat2(predSnap.rotation);
			UTIL_DebugDrawOBB(predSnap.position,m_ownCar->GetPhysicsBody()->m_aabb.minPoint, m_ownCar->GetPhysicsBody()->m_aabb.maxPoint, mat2, ColorRGBA(1.0, 1.0, 0.2, 0.1f));
		}

		// don't correct if it out of time
		// should help in lagging, but brokes prediction

		float fInterpTolerance = 1.0f;
		float fAngInterTolerance = 1.0f;

		float fTolerFactor = fOverallLatency+cl_predict_tolerance.GetFloat();
		float fTolerAngFactor = fOverallLatency+cl_predict_angtolerance.GetFloat();

		if(cl_predict.GetBool())
		{
			float fCarVel = length(m_ownCar->GetVelocity());

			if(pos_diff < cl_predict_tolerance.GetFloat())
			{
				// position difference - provide nearest is not so interpolated
				float fPosLerp = clamp((1.0f+pos_diff) / fTolerFactor, 0.0f, 1.0f);
				fPosLerp = pow(fPosLerp, cl_predict_correct_power.GetFloat());

				fInterpTolerance += fPosLerp + cl_predict_tolerance.GetFloat()*0.5f;

				// interpolate to predicted one accurately
				m_ownCar->GetPhysicsBody()->SetPosition( lerp(car_pos, predSnap.position, fPosLerp) );
			}

			if(angDiffAngle < cl_predict_angtolerance.GetFloat())
			{
				// position difference - provide nearest is not so interpolated
				float fAngDiffLerp = clamp((1.0f+angDiffAngle) / fTolerAngFactor, 0.0f, 1.0f);
				fAngDiffLerp = pow(fAngDiffLerp, cl_predict_correct_power.GetFloat());

				fAngInterTolerance += fAngDiffLerp + cl_predict_angtolerance.GetFloat()*0.5f;

				Quaternion predRot = slerp(car_rot, predSnap.rotation, fAngDiffLerp);
				predRot.fastNormalize();

				m_ownCar->GetPhysicsBody()->SetOrientation(predRot);
			}
		}
		else
		{
			finSnap = interpSnap;
		}

		float fMaxPosDiff = (fOverallLatency+fInterpTolerance) * cl_predict_tolerance.GetFloat();
		float fMaxVelDiff = (fOverallLatency+fInterpTolerance) * 2.5f;
		float fMaxAngDiff = (fOverallLatency+fAngInterTolerance) * cl_predict_angtolerance.GetFloat();

		if(pos_diff > fMaxPosDiff || vel_diff > fMaxVelDiff)
		{
			m_ownCar->GetPhysicsBody()->SetPosition(finSnap.position);
			m_ownCar->SetVelocity(finSnap.velocity);
		}

		if(angDiffAngle > fMaxAngDiff)
			m_ownCar->GetPhysicsBody()->SetOrientation(finSnap.rotation);

		// set controls for non-local player
		if(netSes->GetLocalPlayer() != this)
			SetControls(finSnap.in.controls);
	}

	m_ownCar->UpdateLightsState();

	m_interpTime += fDt;
	m_fCurTime += fDt;
}

void CNetPlayer::CL_GetPredictedSnapshot( const netObjSnapshot_t& in, float fDt_diff, netObjSnapshot_t& out ) const
{
	Quaternion spin = AngularVelocityToSpin(in.rotation, m_ownCar->GetAngularVelocity());

	out.position = in.position + m_ownCar->GetVelocity()*fDt_diff;
	out.rotation = in.rotation + spin*fDt_diff;
	out.rotation.fastNormalize();
	out.in.timeStamp = fDt_diff;

	// FIXME: 50% ratio?
	//out.car_angvel = in.car_angvel + (m_ownCar->GetAngularVelocity()-in.car_angvel)*fDt_diff;
	out.velocity = in.velocity + (m_ownCar->GetVelocity()-in.velocity)*fDt_diff;

	out.in.controls = in.in.controls;
}

float CNetPlayer::CL_GetSnapshotLatency() const
{
	float fSnapLatency = (m_snapshots[0].latency + m_snapshots[1].latency) * 0.5f;

	if(fSnapLatency < TICK_INTERVAL)	// if it less than tick interval, it will be jerky
		fSnapLatency = TICK_INTERVAL;

	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	return fSnapLatency + netSes->GetLocalLatency();
}

float CNetPlayer::GetLatency() const
{
	return m_packetLatency;
}

void CNetPlayer::NetUpdate(float fDt)
{
	if(!m_ownCar)
		NETSpawn();

	if(!m_ownCar)
		return;

	// send snapshots to the players
	m_ownCar->SetControlButtons( m_curControls );

	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(netSes == NULL)
		return;

	if( netSes->IsClient() && netSes->GetLocalPlayer() == this)
	{
		// send local player controls
		netInputCmd_t cmd;
		cmd.controls = m_curControls;
		cmd.timeStamp = m_curSvTick;

		// send sv tick and current time tick in controls
		netSes->GetNetThread()->SendEvent(new CNetPlayerPacket(cmd, m_id, m_curTick), CMSG_PLAYERPACKET, NM_SERVER);
	}
	else if( netSes->IsServer() )
	{
		// send snapshots to all clients
		netSnapshot_t snap;
		Quaternion orient = m_ownCar->GetPhysicsBody()->GetOrientation();

		snap.car_pos = m_ownCar->GetPhysicsBody()->GetPosition();
		snap.car_rot = TVec4D<half>(orient.x,orient.y,orient.z,orient.w);
		snap.car_vel = m_ownCar->GetVelocity();

		//snap.car_angvel = m_ownCar->GetAngularVelocity();

		snap.in.controls = m_curControls;
		snap.in.controls &= ~IN_SIREN;	// don't send siren toggle

		snap.in.timeStamp = m_curTick;

		netSes->GetNetThread()->SendEvent(new CNetPlayerPacket(snap, m_id, m_packetTick), CMSG_PLAYERPACKET, NM_SENDTOALL);
	}

	// simulate and predict all ticks together
	m_curSvTick += TIME_TO_TICKS(fDt);
	m_curTick += TIME_TO_TICKS(fDt);
	m_packetTick += TIME_TO_TICKS(fDt);
}

bool CNetPlayer::IsReady()
{
	return m_ready;
}

//------------------------------------------------------

CNetConnectQueryEvent::CNetConnectQueryEvent()
{
	m_playerName = "";
}

CNetConnectQueryEvent::CNetConnectQueryEvent( const char* playername )
{
	m_playerName = playername;
}

void CNetConnectQueryEvent::Process( CNetworkThread* pNetThread )
{
	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(netSes == NULL)
		return;

	netPeer_t* pClient = new netPeer_t;
	pClient->client_addr = m_clientAddr;
	pClient->client_id = -1;

	CNetworkServer* srv = (CNetworkServer*)pNetThread->GetNetworkInterface();

	// check forc client from this address already connected
	if (srv->GetClientByAddr(&m_clientAddr) != NULL)
	{
		Msg("Player attempt to connect while CONNECTED\n");

		kvkeybase_t kvs;
		CNetMessageBuffer buffer;

		kvs.SetKey("status", "error");
		kvs.SetKey("desc", "#ALREADY_CONNECTED");
		kvs.SetKey("clientID", "-1");
		kvs.SetKey("playerID", "-1");

		buffer.WriteKeyValues(&kvs);

		// add him to remove...
		int cl_id = srv->AddClient(pClient);
		pNetThread->SendData(&buffer, m_nEventID, cl_id, CDPSEND_GUARANTEED | NETSERVER_FLAG_REMOVECLIENT);

		return;
	}


	// we're adding this client because we need to send back statuses, POOR API DESIGN!!!
	// be sure to use NETSERVER_FLAG_REMOVECLIENT flag if we fail
	int cl_id = srv->AddClient(pClient);

	if(netSes->GetFreePlayerSlots() == 0)
	{
		kvkeybase_t kvs;
		CNetMessageBuffer buffer;

		kvs.SetKey("status", "error");
		kvs.SetKey("desc", "#SERVER_IS_FULL");
		kvs.SetKey("clientID", "-1");
		kvs.SetKey("playerID", "-1");

		buffer.WriteKeyValues(&kvs);

		pNetThread->SendData(&buffer, m_nEventID, cl_id, CDPSEND_GUARANTEED | Networking::NETSERVER_FLAG_REMOVECLIENT);

		return;
	}

	// TODO: select spawn point

	netPlayerSpawnInfo_t* spawn = new netPlayerSpawnInfo_t;
	spawn->m_spawnPos = Vector3D(0, 0.5, 10);
	spawn->m_spawnRot = Vector3D(0,0,0);
	spawn->m_spawnColor = Vector4D(0);
	spawn->m_useColor = false;

	CNetPlayer* newPlayer = netSes->CreatePlayer(spawn, cl_id, SV_NEW_PLAYER, m_playerName.c_str());

	if(!newPlayer)
	{
		kvkeybase_t kvs;
		CNetMessageBuffer buffer;

		kvs.SetKey("status", "error");
		kvs.SetKey("desc", "#SERVER_UNKNOWN_ERROR");
		kvs.SetKey("clientID", "-1");
		kvs.SetKey("playerID", "-1");

		buffer.WriteKeyValues(&kvs);

		pNetThread->SendData(&buffer, m_nEventID, cl_id, CDPSEND_GUARANTEED | Networking::NETSERVER_FLAG_REMOVECLIENT);

		return;
	}

	MsgInfo("Player '%s' joining the game...\n", m_playerName.c_str(), newPlayer->m_id);

	CNetMessageBuffer buffer;
	kvkeybase_t kvs;

	kvs.SetKey("status", "ok");
	kvs.SetKey("levelname", g_pGameWorld->GetLevelName());
	kvs.SetKey("environment", g_pGameWorld->GetEnvironmentName());
	kvs.SetKey("clientID", varargs("%d", cl_id));
	kvs.SetKey("playerID", varargs("%d", newPlayer->m_id));
	kvs.SetKey("maxPlayers", varargs("%d", netSes->GetMaxPlayers()));
	kvs.SetKey("numPlayers", varargs("%d", netSes->GetNumPlayers()));

	buffer.WriteKeyValues(&kvs);

	pNetThread->SendData(&buffer, m_nEventID, cl_id, CDPSEND_GUARANTEED);
}

void CNetConnectQueryEvent::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->GetClientInfo(m_clientAddr, m_clientID);
	m_playerName = pStream->ReadString();
}

void CNetConnectQueryEvent::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteString(m_playerName);
}

//------------------------------------------------------

CNetDisconnectEvent::CNetDisconnectEvent( int playerID, const char* reasonTokn )
{
	m_playerID = playerID;
	m_reasonString = reasonTokn;
}

CNetDisconnectEvent::CNetDisconnectEvent()
{

}

void CNetDisconnectEvent::Process( CNetworkThread* pNetThread )
{
	// Msg( "disconnected reason: %s", m_reasonString.c_str() );
	CNetGameSession* ses = (CNetGameSession*)g_pGameSession;

	ses->DisconnectPlayer( m_playerID, m_reasonString.c_str() );
}

void CNetDisconnectEvent::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_clientID = pStream->GetClientID();

	m_playerID = pStream->ReadInt();
	m_reasonString = pStream->ReadString();
}

void CNetDisconnectEvent::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteInt(m_playerID);
	pStream->WriteString( m_reasonString );
}

//--------------------------------------------------

CNetClientPlayerInfo::CNetClientPlayerInfo()
{
	m_carName = "";
}

CNetClientPlayerInfo::CNetClientPlayerInfo(const char* carName)
{
	m_carName = carName;
}

void CNetClientPlayerInfo::Process( CNetworkThread* pNetThread )
{
	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(netSes == NULL)
		return;

	CNetPlayer* player = netSes->GetPlayerByClientID(m_clientID);

	if(!player)
	{
        MsgError("CNetClientPlayerInfo: failed to get player by clientId=%d!!!\n", m_clientID);
		return;
	}

	player->m_carName = m_carName;

	// add player to list and send back message

	// don't make him ready unless recieved controls
	player->NETSpawn();

	if(netSes->IsServer())
	{
        MsgInfo("[SYNC] Sending player info...\n");
        netSes->SendPlayerInfoList( m_clientID );
	}

	kvkeybase_t kvs;
	kvs.SetKey("status", "ok");

	CNetMessageBuffer buffer;
	buffer.WriteKeyValues(&kvs);

	pNetThread->SendData(&buffer, m_nEventID, m_clientID, CDPSEND_GUARANTEED);
}

void CNetClientPlayerInfo::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_carName = pStream->ReadString();
	pStream->GetClientInfo(m_clientAddr, m_clientID);
}

void CNetClientPlayerInfo::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteString(m_carName);
}

//--------------------------------------------------

CNetServerPlayerInfo::CNetServerPlayerInfo()
{
	m_carName = "";
	m_playerName = "";
	m_playerID = -1;
	m_carNetID = -1;
}

CNetServerPlayerInfo::CNetServerPlayerInfo( CNetPlayer* player )
{
	m_carName = player->m_carName.c_str();
	m_playerName = player->m_name.c_str();
	m_playerID = player->m_id;
	m_sync_time = player->m_fCurTime;

	if( player->m_ownCar )
	{
		m_position = player->m_ownCar->GetOrigin();
		m_rotation = player->m_ownCar->GetOrientation();
		m_carColor = player->m_ownCar->GetCarColour();
		m_carNetID = player->m_ownCar->m_networkID;
	}
}

void CNetServerPlayerInfo::Process( CNetworkThread* pNetThread )
{
	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(netSes == NULL)
		return;

	CNetworkClient* client = (CNetworkClient*)pNetThread->GetNetworkInterface();

	Msg("[SYNC] got player info %s (time: %.2fs)\n", m_playerName.c_str(), m_sync_time);

	netPlayerSpawnInfo_t* spawn = new netPlayerSpawnInfo_t;
	spawn->m_spawnPos = m_position;
	spawn->m_spawnRot = eulers(m_rotation);
	spawn->m_spawnColor = m_carColor;
	spawn->m_netCarID = m_carNetID;

	// if it's our car
	if( g_svclientInfo.playerID == m_playerID )
	{
		netSes->InitLocalPlayer( spawn, client->GetClientID(),  g_svclientInfo.playerID);
	}
	else
	{
		CNetPlayer* player = netSes->GetPlayerByID(m_playerID);

		if(player && player->IsReady())
		{
			return;
		}

		// set to a server-controlled
		player = netSes->CreatePlayer(spawn, NM_SERVER, m_playerID, m_playerName.c_str());

		if(player)
		{
			player->m_carName = m_carName;

			player->m_fCurTime = m_sync_time;	// set sync time
			player->NETSpawn();
			player->m_ready = true;
		}
	}
}

void CNetServerPlayerInfo::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	m_playerID = pStream->ReadInt();
	m_carNetID = pStream->ReadInt();
	m_position = pStream->ReadVector3D();
	m_rotation = pStream->ReadVector4D();
	pStream->ReadData(&m_carColor, sizeof(m_carColor));
	m_sync_time = pStream->ReadFloat();

	m_carName = pStream->ReadString();
	m_playerName = pStream->ReadString();

	pStream->GetClientInfo(m_clientAddr, m_clientID);
}

void CNetServerPlayerInfo::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteInt(m_playerID);
	pStream->WriteInt(m_carNetID);
	pStream->WriteVector3D(m_position);
	pStream->WriteVector4D(m_rotation.asVector4D());
	pStream->WriteData(&m_carColor, sizeof(m_carColor));
	pStream->WriteFloat(m_sync_time);

	pStream->WriteString(m_carName);
	pStream->WriteString(m_playerName);
}

//----------------------------------------------------------------------------

CNetPlayerPacket::CNetPlayerPacket( const netInputCmd_t& cmd, int nPlayerID, int curTick )
{
	m_type = PL_PACKET_CONTROLS;
	m_snapshot.car_rot.w = 1.0f;
	m_inCmd = cmd;
	m_playerID = nPlayerID;
	m_packetTick = curTick;
}

CNetPlayerPacket::CNetPlayerPacket( const netSnapshot_t& snapshot, int nPlayerID, int curTick )
{
	m_type = PL_PACKET_PROPS;
	m_snapshot = snapshot;
	m_playerID = nPlayerID;
	m_packetTick = curTick;
}

CNetPlayerPacket::CNetPlayerPacket()
{
	m_type = PL_PACKET_CONTROLS;
	memset(&m_snapshot, 0, sizeof(netSnapshot_t));
	m_snapshot.car_rot.w = 1.0f;
	m_playerID = -1;
	m_packetTick = 0;
}

void CNetPlayerPacket::Process( CNetworkThread* pNetThread )
{
	// add player to list and send back message
	CNetGameSession* netSes = (CNetGameSession*)g_pGameSession;

	if(!netSes)
		return;

	CNetPlayer* player = netSes->GetPlayerByID( m_playerID );

	if(!player)
		return;

	// make player ready
	//player->m_ready = true;

	if( netSes->IsServer() && m_type == PL_PACKET_CONTROLS )
	{
		player->m_ready = true;

		// restricted
		if(player->m_clientID != m_clientID)
			return;

		if( m_packetTick > player->m_lastPacketTick )
		{
			player->m_lastPacketTick = m_packetTick;

			player->SV_OnRecieveControls( m_inCmd.controls );

			int tickDiff = (player->m_curTick - m_inCmd.timeStamp) + (m_packetTick - player->m_packetTick);

			player->m_packetLatency = TICKS_TO_TIME(tickDiff);
			player->m_packetTick = m_packetTick;
		}
	}
	else if( netSes->IsClient() && m_type == PL_PACKET_PROPS )
	{
		// make player ready
		player->m_ready = true;


		if( m_packetTick > player->m_lastPacketTick &&
			player->CL_AddSnapshot( m_snapshot ) )
		{
			player->m_lastPacketTick = m_packetTick;
			
			int tickDiff = (player->m_curTick - m_packetTick) + (m_packetTick - player->m_packetTick);

			if(tickDiff > 0)
				player->m_packetLatency = TICKS_TO_TIME(tickDiff);

			player->m_packetTick = m_packetTick;
			player->m_curSvTick = m_snapshot.in.timeStamp;
		}
	}
	else
	{
		ASSERTMSG(false, "CNetPlayerPacket: unsupported data combination\n");
	}
}

void CNetPlayerPacket::Unpack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->GetClientInfo(m_clientAddr, m_clientID);

	m_type = (EPlayerPacketType)pStream->ReadInt();
	m_playerID = pStream->ReadInt();
	m_packetTick = pStream->ReadInt();
	float tick_interval = pStream->ReadFloat();

	if(m_type == PL_PACKET_CONTROLS)
	{
		pStream->ReadData( &m_inCmd, sizeof(netInputCmd_t) );
	}
	else
	{
		pStream->ReadData( &m_snapshot, sizeof(netSnapshot_t) );

		// set new tick interval
		g_svclientInfo.tickInterval = tick_interval;
	}
}

void CNetPlayerPacket::Pack( CNetworkThread* pNetThread, CNetMessageBuffer* pStream )
{
	pStream->WriteInt(m_type);
	pStream->WriteInt(m_playerID);
	pStream->WriteInt(m_packetTick);
	pStream->WriteFloat(TICK_INTERVAL);

	if(m_type == PL_PACKET_CONTROLS)
	{
		pStream->WriteData( &m_inCmd, sizeof(netInputCmd_t) );
	}
	else
	{
		pStream->WriteData( &m_snapshot, sizeof(m_snapshot) );
	}
}
