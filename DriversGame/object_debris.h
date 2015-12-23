//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: debris object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_DEBRIS_H
#define OBJECT_DEBRIS_H

#include "GameObject.h"

class CObject_Debris : public CGameObject
{
public:
	CObject_Debris( kvkeybase_t* kvdata );
	~CObject_Debris();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	const Vector3D&		GetOrigin();
	const Vector3D&		GetAngles();
	const Vector3D&		GetVelocity() const;

	void				Draw( int nRenderFlags );

	void				OnPhysicsFrame(float fDt);
	void				Simulate(float fDt);

	int					ObjType() const		{return GO_DEBRIS;}

	void				L_SetContents(int contents);
	void				L_SetCollideMask(int contents);

	int					L_GetContents() const;
	int					L_GetCollideMask() const;

protected:

	CEqRigidBody*		m_physBody;
	bool				m_collOccured;
	float				m_fTimeToRemove;
	EqString			m_smashSound;
};

#endif // OBJECT_DEBRIS_H