//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics point constraint
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQPHYSICS_POINTCONSTRAINT_H
#define EQPHYSICS_POINTCONSTRAINT_H

#include "math/FVector.h"
#include "math/BoundingBox.h"

#include "eqPhysics_Contstraint.h"

class CEqRigidBody;

class CEqPhysicsPointConstraint : public IEqPhysicsConstraint
{
public:
	CEqPhysicsPointConstraint();
	~CEqPhysicsPointConstraint();

	// @allowedDistance indicated how much the points are allowed to deviate.
	// @timescale indicates the timescale over which deviation is eliminated
	// (suggest a few times dt - be careful if there's a variable timestep!)
	// if @timescale < 0 then the value indicates the number of dts
	void			Init(	CEqRigidBody* body0, const FVector3D& body0Pos,
							CEqRigidBody* body1, const FVector3D& body1Pos,
							float allowedDistance,
							float timescale = 1.0f);

	void			PreApply(float dt);
	bool			Apply(float dt);
	void			Destroy();

protected:
	FVector3D		m_body0Pos;
	CEqRigidBody*	m_body0;
	FVector3D		m_body1Pos;
	CEqRigidBody*	m_body1;
	float			m_allowedDistance;
	float			m_timescale;

	// some values that we calculate once in PreApply
	FVector3D	m_worldPos;	///< mid point of the two joint positions
	Vector3D	m_R0;		///< position relative to body 0 (in world space)
	Vector3D	m_R1;
	Vector3D	m_VrExtra;	///< extra vel for restoring deviation
};

#endif // EQPHYSICS_POINTCONSTRAINT_H