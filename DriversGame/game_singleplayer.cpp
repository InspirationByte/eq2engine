//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#include "game_singleplayer.h"
#include "pedestrian.h"
#include "world.h"
#include "input.h"
#include "replay.h"

#include "DrvSynHUD.h"

const float THRILL_TIMESCALE = 0.65f;
const float THRILL_TIMEOUT = 7.0f * THRILL_TIMESCALE;
const float THRILL_LEAD_TO_PLAYER_MAX_DISTANCE = 80.0f;

void fng_car_variants(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	if (g_pGameSession)
		g_pGameSession->GetCarNames(list);
}

ConVar g_car("g_car", "rollo", fng_car_variants, "player car", CV_ARCHIVE);

DECLARE_CMD(cam_go_thrill, "Thrill camera", 0)
{
	if (g_pGameSession && g_pGameSession->GetSessionType() == SESSION_SINGLE)
	{
		CSingleGameSession* session = (CSingleGameSession*)g_pGameSession;

		if (session->IsInThrill())
			session->LeaveThrill();
		else
			session->GoThrill();
	}
}

CSingleGameSession::CSingleGameSession()
{

}

CSingleGameSession::~CSingleGameSession()
{

}

void CSingleGameSession::Init()
{
	m_playerCar = nullptr;
	m_playerPedestrian = nullptr;
	m_thrillTimeout = 0.0f;

	memset(&m_playerControl, 0, sizeof(m_playerControl));

	CGameSessionBase::Init();

	// start recorder
	if (g_replayTracker->m_state <= REPL_RECORDING)
	{
		//
		// Spawn default car if script not did
		//
		if (!GetPlayerCar())
		{
			CPedestrian* playerPed = (CPedestrian*)g_pGameWorld->CreateGameObject("pedestrian", nullptr);

			playerPed->SetOrigin(Vector3D(0.0f,1.0f,0.0f));

			playerPed->Spawn();
			g_pGameWorld->AddObject(playerPed);

			SetPlayerPedestrian(playerPed);
		}
	}

	CCar* playerCar = GetPlayerCar();
	CPedestrian* playerPed = GetPlayerPedestrian();

	Vector3D queryPos = playerCar ? playerCar->GetOrigin() : (playerPed ? playerPed->GetOrigin() : 0.0f);

	// load regions player car position
	if (playerCar || playerPed)
		g_pGameWorld->QueryNearestRegions(queryPos, false);

	// also enable headlights on player car if needed
	if (playerCar)
	{
		bool headLightsEnabled = playerCar->IsEnabled() && (g_pGameWorld->m_envConfig.lightsType & WLIGHTS_CARS);
		playerCar->SetLight(CAR_LIGHT_LOWBEAMS, headLightsEnabled);
	}
}

void CSingleGameSession::Shutdown()
{
	CGameSessionBase::Shutdown();

	m_playerCar = nullptr;

	memset(&m_playerControl, 0, sizeof(m_playerControl));
}

CCar* CSingleGameSession::GetPlayerCar() const
{
	return m_playerCar;
}

void CSingleGameSession::SetPlayerCar(CCar* pCar)
{
	if (m_playerCar)
		m_playerCar->m_isLocalCar = false;

	m_playerCar = pCar;
	g_pGameHUD->SetDisplayMainVehicle(pCar);

	if (pCar)
		pCar->m_isLocalCar = true;
}

CGameObject* CSingleGameSession::GetViewObject() const
{
	if (m_playerPedestrian)
		return m_playerPedestrian;

	return CGameSessionBase::GetViewObject();
}

CPedestrian* CSingleGameSession::GetPlayerPedestrian() const
{
	return m_playerPedestrian;
}

void CSingleGameSession::SetPlayerPedestrian(CPedestrian* ped)
{
	m_playerPedestrian = ped;
}

void CSingleGameSession::UpdateLocalControls(int nControls, float steering, float accel_brake)
{
	m_playerControl.buttons = nControls;
	m_playerControl.steeringValue = steering;
	m_playerControl.accelBrakeValue = accel_brake;
}

void CSingleGameSession::UpdateAsPlayerPedestrian(const playerControl_t& control, CPedestrian* ped)
{
	if (!g_pGameWorld->IsValidObject(ped))
		return;

	g_pGameWorld->QueryNearestRegions(ped->GetOrigin(), false);

	ped->SetControlButtons(control.buttons);

	ped->SetControlVars(
		(control.buttons & IN_ACCELERATE) ? control.accelBrakeValue : 0.0f,
		(control.buttons & IN_BRAKE) ? control.accelBrakeValue : 0.0f,
		control.steeringValue);
}

void CSingleGameSession::UpdatePlayerControls()
{
	if (m_playerPedestrian)
		UpdateAsPlayerPedestrian(m_playerControl, m_playerPedestrian);
	else
		UpdateAsPlayerCar(m_playerControl, m_playerCar);
}

bool CSingleGameSession::IsInThrill() const
{
	return (m_thrillTimeout > 0.0f);
}

void CSingleGameSession::LeaveThrill()
{
	m_thrillTimeout = 0.0f;

	if (!g_pCameraAnimator->IsScripted())
		g_pCameraAnimator->SetMode(CAM_MODE_OUTCAR);

	g_pGameHUD->ShowMotionBlur(false);
}

void CSingleGameSession::GoThrill()
{
	// thrill camera is valid for game only, not replay
	if (g_replayTracker->m_state != REPL_RECORDING)
		return;

	if (g_pCameraAnimator->IsScripted())
		return;

	CCar* focusCar = GetLeadCar();

	if (focusCar != GetPlayerCar())
	{
		if (distance(focusCar->GetOrigin(), GetPlayerCar()->GetOrigin()) > THRILL_LEAD_TO_PLAYER_MAX_DISTANCE)
			focusCar = GetPlayerCar();
	}

	if (focusCar == nullptr)
		return;

	if (focusCar->IsInWater())
		return;

	m_thrillTimeout = THRILL_TIMEOUT;

	// trace far back to determine distToTarget
	{
		eqPhysCollisionFilter filter(focusCar->GetPhysicsBody());

		const Vector3D& velocity = focusCar->GetVelocity();
		const Vector3D forward = focusCar->GetForwardVector();
		const Vector3D right = focusCar->GetRightVector();

		float rightVecFactor = RandomFloat(-1, 1);

		Vector3D traceStart = focusCar->GetOrigin() + vec3_up * 2.0f;
		Vector3D targetPosFront = traceStart + forward * 15.0f + velocity * 0.5f + right * sign(rightVecFactor) * 4.0f;
		Vector3D targetPosBack = traceStart + forward * -15.0f + velocity * -0.5f + right * sign(rightVecFactor) * 4.0f;

		int props = OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_VEHICLE;

		// trace sphere forward and back
		btBoxShape sphere(btVector3(0.5f, 0.5f, 0.5f));

		CollisionData_t traceForward;
		CollisionData_t traceBack;
		g_pPhysics->TestConvexSweep(&sphere, identity(), traceStart, targetPosFront, traceForward, props, &filter);
		g_pPhysics->TestConvexSweep(&sphere, identity(), traceStart, targetPosBack, traceBack, props, &filter);

		// adjust target pos
		targetPosFront = lerp(traceStart, targetPosFront, traceForward.fract);
		targetPosBack = lerp(traceStart, targetPosBack, traceBack.fract);

		// select best target pos
		// use furthest trace. Front is in priority (>=)
		Vector3D target_pos = (traceForward.fract >= traceBack.fract) ? targetPosFront : targetPosBack;

		// trace down
		if (g_pPhysics->TestConvexSweep(&sphere, identity(), target_pos, target_pos - Vector3D(0, 10.0f, 0.0f), traceForward, props, &filter))
			target_pos = traceForward.position + 1.0f;

		g_pCameraAnimator->SetMode(CAM_MODE_TRIPOD_FOLLOW_ZOOM);
		g_pCameraAnimator->SetOrigin(target_pos);
	}

	g_pGameHUD->ShowMotionBlur(true);
}

void CSingleGameSession::UpdateMission(float fDt)
{
	CGameSessionBase::UpdateMission(fDt);

	float thrillTime = m_thrillTimeout;
	if (thrillTime > 0)
	{
		thrillTime -= fDt;

		// if player has changed the mode, get out from thrill
		if (IsCarCamera((ECameraMode)g_pCameraAnimator->GetMode()))
			thrillTime = 0.0f;

		//g_pCameraAnimator->Update(fDt, g_nClientButtons, GetPlayerCar());

		if (thrillTime <= 0.0f || g_pCameraAnimator->IsScripted())
		{
			LeaveThrill();
			thrillTime = 0.0f;
		}
	}
	else if (m_missionStatus == MIS_STATUS_FAILED)
	{
		GoThrill();
		return;
	}

	m_thrillTimeout = thrillTime;
}

float CSingleGameSession::GetTimescale() const
{
	if(m_thrillTimeout > 0.0f)
		return THRILL_TIMESCALE;

	return 1.0f;
}