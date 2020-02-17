//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Ladder
//////////////////////////////////////////////////////////////////////////////////

#ifndef LADDER_H
#define LADDER_H

#include "BaseEntity.h"

class CInfoLadderPoint : public BaseEntity
{
public:
	DEFINE_CLASS_BASE(BaseEntity);

	CInfoLadderPoint();

	void		Spawn();

	// checks object for usage ray/box
	bool		CheckUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir);

	// onUse handler
	bool		OnUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir);

	Vector3D	GetLadderDestPoint() {return m_vLadderDestPoint;}
	Vector3D	GetLadderDestAngles() {return m_vLadderDestAngles;}

	DECLARE_DATAMAP();

protected:
	Vector3D	m_vLadderDestPoint;
	Vector3D	m_vLadderDestAngles;
};

#endif // LADDER_H