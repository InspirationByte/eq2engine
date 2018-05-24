//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Single player game session
//////////////////////////////////////////////////////////////////////////////////

#include "game_singleplayer.h"
#include "DrvSynHUD.h"

CSingleGameSession::CSingleGameSession()
{

}

CSingleGameSession::~CSingleGameSession()
{

}

void CSingleGameSession::Init()
{
	m_playerCar = nullptr;

	memset(&m_playerControl, 0, sizeof(m_playerControl));

	CGameSessionBase::Init();
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

void CSingleGameSession::UpdateLocalControls(int nControls, float steering, float accel_brake)
{
	m_playerControl.buttons = nControls;
	m_playerControl.steeringValue = steering;
	m_playerControl.accelBrakeValue = accel_brake;
}

void CSingleGameSession::UpdatePlayerControls()
{
	UpdateAsPlayerCar(m_playerControl, m_playerCar);
}