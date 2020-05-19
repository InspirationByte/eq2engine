//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Controllable
//////////////////////////////////////////////////////////////////////////////////

#ifndef CONTROLLABLE_H
#define CONTROLLABLE_H

#include "GameObject.h"

const float _oneBy1024 = 1.0f / 1023.0f;

class CControllableGameObject : public CGameObject
{
public:
	DECLARE_CLASS(CControllableGameObject, CGameObject)

	CControllableGameObject();
	virtual ~CControllableGameObject() {}

	virtual void	SetControlButtons(int flags);
	virtual int		GetControlButtons() const;

	void			SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering);
	void			GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering);

protected:
	short			m_controlButtons;
	short			m_oldControlButtons;

	short			m_accelRatio;
	short			m_brakeRatio;
	short			m_steerRatio;
};

//--------------------------------------------------------------
#if 0
class ICarDriver
{
	friend class CCar;
public:
	virtual ~ICarDriver() {}

	virtual void Init() = 0;

	// OnCarCollisionEvent
	virtual void OnHitByCar(const CollisionPairData_t& pair, CGameObject* hitBy) = 0;

	// called by OnPrePhysicsFrame
	virtual void Update(float fDt) = 0;

	// for type casting
	virtual ECarDriverType GetType() const = 0;

private:
	CCar* m_owner;
};
#endif

#endif // CONTROLLABLE_H