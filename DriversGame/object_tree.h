//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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
	DECLARE_CLASS(CObject_Tree, CGameObject)

	CObject_Tree( kvkeybase_t* kvdata );
	~CObject_Tree();

	void				OnRemove();
	void				Spawn();

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

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