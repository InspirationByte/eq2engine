//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#ifndef PEDESTRIAN_H
#define PEDESTRIAN_H

#include "GameObject.h"
#include "ControllableObject.h"
#include "Animating.h"

class CPedestrian : public CGameObject, public CAnimatingEGF, public CControllableObject
{
	friend class CCar;
public:
	DECLARE_CLASS(CPedestrian, CGameObject)

	CPedestrian();
	CPedestrian(kvkeybase_t* kvdata);
	~CPedestrian();

	void				SetModelPtr(IEqModel* modelPtr);

	void				Precache();

	void				OnRemove();
	void				Spawn();

	void				ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const;

	void				Draw(int nRenderFlags);

	void				Simulate(float fDt);

	int					ObjType() const { return GO_PEDESTRIAN; }

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	const Vector3D&		GetOrigin();
	const Vector3D&		GetAngles();
	const Vector3D&		GetVelocity() const;

	void				UpdateTransform();

protected:

	void				HandleAnimatingEvent(AnimationEvent nEvent, char* options);

	CEqRigidBody*		m_physBody;
	bool				m_onGround;

};

#endif // PEDESTRIAN_H