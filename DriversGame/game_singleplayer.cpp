//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#include "game_singleplayer.h"
#include "pedestrian.h"
#include "world.h"
#include "input.h"
#include "replay.h"

#include "DrvSynHUD.h"

void fng_car_variants(const ConCommandBase* base, DkList<EqString>& list, const char* query)
{
	if (g_pGameSession)
		g_pGameSession->GetCarNames(list);
}

ConVar g_car("g_car", "rollo", fng_car_variants, "player car", CV_ARCHIVE);

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

	memset(&m_playerControl, 0, sizeof(m_playerControl));

	CGameSessionBase::Init();

	// start recorder
	if (g_replayData->m_state <= REPL_RECORDING)
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

	// load regions on player car position
	if (GetPlayerCar())
	{
		// also enable headlights on player car if needed
		bool headLightsEnabled = GetPlayerCar()->IsEnabled() && (g_pGameWorld->m_envConfig.lightsType & WLIGHTS_CARS);
		GetPlayerCar()->SetLight(headLightsEnabled, headLightsEnabled);

		g_pGameWorld->QueryNearestRegions(GetPlayerCar()->GetOrigin(), false);
	}
	else if (GetPlayerPedestrian())
		g_pGameWorld->QueryNearestRegions(GetPlayerPedestrian()->GetOrigin(), false);
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

	// FIXME: multiple HUD instances for split screen
	g_pGameHUD->SetDisplayMainVehicle(m_playerCar);

	if (m_playerCar)
		m_playerCar->m_isLocalCar = true;
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