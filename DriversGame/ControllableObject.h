//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Controllable
//////////////////////////////////////////////////////////////////////////////////

#ifndef CONTROLLABLE_H
#define CONTROLLABLE_H

const float _oneBy1024 = 1.0f / 1023.0f;

class CControllableObject
{
public:
	CControllableObject();

	virtual void	SetControlButtons(int flags);
	int				GetControlButtons() const;

	void			SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering);
	void			GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering);

protected:
	short			m_controlButtons;
	short			m_oldControlButtons;

	short			m_accelRatio;
	short			m_brakeRatio;
	short			m_steerRatio;
};

#endif // CONTROLLABLE_H