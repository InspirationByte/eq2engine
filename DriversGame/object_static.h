//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: physics object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_STATIC_H
#define OBJECT_STATIC_H

#include "GameObject.h"
#include "world.h"

class CObject_Static : public CGameObject
{
public:
	DECLARE_CLASS(CObject_Static, CGameObject)

	CObject_Static( kvkeybase_t* kvdata );
	~CObject_Static();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const		{return GO_STATIC;}

	void				OnCarCollisionEvent(const CollisionPairData_t& pai, CGameObject* hitByr);

	void				L_SetContents(int contents);
	void				L_SetCollideMask(int contents);

	int					L_GetContents() const;
	int					L_GetCollideMask() const;

protected:

	CEqCollisionObject*	m_pPhysicsObject;

	wlightdata_t		m_light;

	bool				m_flicker;
	bool				m_killed;
	float				m_flickerTime;
};

#endif // OBJECT_PHYSICS_H