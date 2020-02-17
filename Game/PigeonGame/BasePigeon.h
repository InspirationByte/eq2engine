//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Pigeon
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEPIGEON_H
#define BASEPIGEON_H

#include "BaseActor.h"

class CBasePigeon : public CBaseActor
{
	DEFINE_CLASS_BASE( CBaseActor );
public:

							CBasePigeon();

	virtual void			Spawn();

	virtual void			Precache();

	MoveType_e				GetMoveType();

	virtual EntRelClass_e	Classify()			{return ENTCLASS_PIGEON;}

	virtual void			PainSound();
	virtual void			DeathSound();

	// traction by wind affection
	virtual void			ApplyTraction(Vector3D& vAddTraction);

	virtual void			AliveThink();

	// compute matrix transformation for this object
	void					ComputeTransformMatrix();

	virtual void			UpdateView();

protected:
	virtual void			ProcessMovement( movementdata_t& data );

	// flight control
	Vector3D				m_vecTraction;
	Vector3D				m_vecFlightDirection;		// flight direction
	float					m_fFlyVelocity;				// flying velocity (lower is landing, bigger if straight to the flying direction)
	Vector3D				m_vecFlyAngle;				// fly angle captured from mouse

	// TODO: head controller, and other functionals
	int						m_nHeadIKChain;

	DECLARE_DATAMAP();
};

#endif // BASEPIGEON_H