//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics hinge joint
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "eqPhysics_Contstraint.h"

class CEqRigidBody;

class CEqPhysicsMaxDistConstraint : public IEqPhysicsConstraint
{
public:
	CEqPhysicsMaxDistConstraint();
	~CEqPhysicsMaxDistConstraint();

	// @allowedDistance indicated how much the points are allowed to deviate.
	// @timescale indicates the timescale over which deviation is eliminated
	// (suggest a few times dt - be careful if there's a variable timestep!)
	// if @timescale < 0 then the value indicates the number of dts
	void			Init(	CEqRigidBody* body0, const FVector3D& body0Pos,
							CEqRigidBody* body1, const FVector3D& body1Pos,
							float maxDistance,
							int flags = 0);

	void			PreApply(float dt);
	bool			Apply(float dt);
	void			Destroy();

protected:
	// configuration
	CEqRigidBody*	m_body0;
	CEqRigidBody*	m_body1;
	FVector3D		m_body0Pos;
	FVector3D		m_body1Pos;
	float			m_maxDistance;
	// stuff that gets updated
	Vector3D		m_R0;
	Vector3D		m_R1;
	FVector3D		m_worldPos;
	Vector3D		m_currentRelPos0;
};
