//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian AI
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIPEDESTRIAN_H
#define AIPEDESTRIAN_H

#include "GameObject.h"

class CPedestrian : public CGameObject
{
	friend class CCar;
public:
	DECLARE_CLASS(CPedestrian, CGameObject)

	CPedestrian(kvkeybase_t* kvdata);
	~CPedestrian();

	void				OnRemove();
	void				Spawn();

	void				Draw(int nRenderFlags);

	void				Simulate(float fDt);

	int					ObjType() const { return GO_PEDESTRIAN; }

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	const Vector3D&		GetOrigin() const;
	const Vector3D&		GetAngles() const;
	const Vector3D&		GetVelocity() const;

protected:

	CEqRigidBody*		m_physBody;

};

#endif // PEDESTRIAN_H