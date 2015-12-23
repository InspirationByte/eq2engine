//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: physics object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_PHYSICS_H
#define OBJECT_PHYSICS_H

#include "GameObject.h"

class CObject_Physics : public CGameObject
{
public:
	DECLARE_NETWORK_TABLE();
	DECLARE_CLASS( CObject_Physics, CGameObject )

	CObject_Physics( kvkeybase_t* kvdata );
	~CObject_Physics();

	void					OnRemove();
	void					Spawn();

	void					SetOrigin(const Vector3D& origin);
	void					SetAngles(const Vector3D& angles);
	void					SetVelocity(const Vector3D& vel);

	const Vector3D&			GetOrigin();
	const Vector3D&			GetAngles();
	const Vector3D&			GetVelocity() const;

	void					Draw( int nRenderFlags );

	void					Simulate(float fDt);

	int						ObjType() const		{return GO_PHYSICS;}

	void					OnUnpackMessage( CNetMessageBuffer* buffer );

	void					L_SetContents(int contents);
	void					L_SetCollideMask(int contents);

	int						L_GetContents() const;
	int						L_GetCollideMask() const;

protected:

	CNetworkVar(FVector3D,	m_netPos);
	CNetworkVar(FVector3D,	m_netAngles);

	EqString				m_smashSound;

	CEqRigidBody*			m_physBody;
	
};

#endif // OBJECT_PHYSICS_H