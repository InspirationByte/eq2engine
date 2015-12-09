//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: physics object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_TREE_H
#define OBJECT_TREE_H

#include "GameObject.h"
#include "world.h"
#include "BillboardList.h"

class CObject_Tree : public CGameObject
{
public:
	CObject_Tree( kvkeybase_t* kvdata );
	~CObject_Tree();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	Vector3D			GetOrigin();
	Vector3D			GetAngles();
	Vector3D			GetVelocity();

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const		{return GO_STATIC;}

	void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

protected:

	CEqCollisionObject*	m_pPhysicsObject;
	EqString			m_smashSound;
	CBillboardList*		m_blbList;
};

#endif // OBJECT_TREE_H