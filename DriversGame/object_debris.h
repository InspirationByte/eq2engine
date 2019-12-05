//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: debris object
//////////////////////////////////////////////////////////////////////////////////

#ifndef OBJECT_DEBRIS_H
#define OBJECT_DEBRIS_H

#include "GameObject.h"

struct breakablePart_t
{
	int bodyGroupIdx;
	int physObjIdx;
	Vector3D offset;
};

struct breakSpawn_t
{
	char objectDefName[32];
	Vector3D offset;
};

class CObject_Debris : public CGameObject
{
	friend class CCar;

public:
	DECLARE_CLASS(CObject_Debris, CGameObject)

	CObject_Debris( kvkeybase_t* kvdata );
	~CObject_Debris();

	void				OnRemove();

	void				Spawn();
	void				SpawnAsHubcap(IEqModel* model, int8 bodyGroup);

	void				SpawnAsBreakablePart(IEqModel* model, int8 bodyGroup, int physObj);

	void				SetOrigin(const Vector3D& origin);
	void				SetAngles(const Vector3D& angles);
	void				SetVelocity(const Vector3D& vel);

	const Vector3D&		GetOrigin() const;
	const Vector3D&		GetAngles() const;
	const Vector3D&		GetVelocity() const;

	void				Draw( int nRenderFlags );

	void				Simulate(float fDt);

	int					ObjType() const		{ return GO_DEBRIS; }

	bool				IsSmashed() const { return m_physBody && (m_physBody->m_flags & BODY_MOVEABLE) > 0; }

	void				L_SetContents(int contents);
	void				L_SetCollideMask(int contents);

	int					L_GetContents() const;
	int					L_GetCollideMask() const;

protected:

	void				BreakAndSpawnDebris();

	CEqRigidBody*		m_physBody;
	float				m_fTimeToRemove;
	EqString			m_smashSound;
	eqPhysSurfParam_t*	m_surfParams;

	float				m_breakMinForce;
	breakablePart_t*	m_breakable;
	int					m_numBreakParts;

	EqString			m_smashSpawn;
	CGameObject*		m_smashSpawnedObject;

	breakSpawn_t*		m_breakSpawn;
};

#endif // OBJECT_DEBRIS_H