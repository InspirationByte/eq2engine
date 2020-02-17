//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Controllable
//////////////////////////////////////////////////////////////////////////////////

#include "ControllableObject.h"
#include "input.h"

CControllableGameObject::CControllableGameObject() :
	m_controlButtons(0),
	m_oldControlButtons(0),
	m_accelRatio(1023),
	m_brakeRatio(1023),
	m_steerRatio(1023)
{

}

void CControllableGameObject::SetControlButtons(int flags)
{
	// make no misc controls here
	flags &= ~IN_MISC;

	m_controlButtons = flags;
}

int	CControllableGameObject::GetControlButtons() const
{
	return m_controlButtons;
}

void CControllableGameObject::SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering)
{
	m_accelRatio = min(fAccelRatio, 1.0f)*1023.0f;
	m_brakeRatio = min(fBrakeRatio, 1.0f)*1023.0f;
	m_steerRatio = clamp(fSteering, -1.0f, 1.0f)*1023.0f;

	if (m_accelRatio > 1023)
		m_accelRatio = 1023;

	if (m_brakeRatio > 1023)
		m_brakeRatio = 1023;

	if (m_steerRatio > 1023)
		m_steerRatio = 1023;

	if (m_steerRatio < -1023)
		m_steerRatio = -1023;
}

void CControllableGameObject::GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering)
{
	fAccelRatio = (double)m_accelRatio * _oneBy1024;
	fBrakeRatio = (double)m_brakeRatio * _oneBy1024;
	fSteering = (double)m_steerRatio * _oneBy1024;
}