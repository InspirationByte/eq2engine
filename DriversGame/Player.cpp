//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate player data holder
//////////////////////////////////////////////////////////////////////////////////

#include "Player.h"

CPlayer::CPlayer() : m_ownCar(nullptr)
{
}

CPlayer::~CPlayer()
{
}

CCar* CPlayer::GetCar()
{
	return m_ownCar;
}

void CPlayer::SetCar(CCar* car)
{
	if (m_ownCar)
		m_ownCar->m_isLocalCar = false;

	m_ownCar = car;

	if(m_ownCar)
		m_ownCar->m_isLocalCar = true;
}

bool CPlayer::IsCarAlive()
{
	if (m_ownCar)
		return m_ownCar->IsAlive();

	return false;
}

void CPlayer::SetControls(const playerControl_t& controls)
{
	m_controls = controls;
}