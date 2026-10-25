//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics angular limit constraint
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "eqPhysics_Contstraint.h"

class CEqRigidBody;

// Limits rotation of body0 joint frame relative to body1 joint frame.
// Rotation is measured as a rotation vector in body1 joint frame axes
// and each component is kept within [minLimit, maxLimit]; equal limits lock the axis.
// Use together with CEqPointConstraint to make a ragdoll joint.
class CEqAngularLimitConstraint : public IEqPhysicsConstraint
{
public:
	CEqAngularLimitConstraint() = default;
	~CEqAngularLimitConstraint();

	// @body0Frame and @body1Frame are joint frame orientations relative to the bodies.
	// @minLimit and @maxLimit are angles in radians, swapped values are allowed.
	// @timescale indicates the timescale over which limit violation is eliminated
	void			Init(	CEqRigidBody* body0, const Quaternion& body0Frame,
							CEqRigidBody* body1, const Quaternion& body1Frame,
							const Vector3D& minLimit, const Vector3D& maxLimit,
							float timescale = 1.0f,
							int flags = 0);

	void			PreApply(float dt);
	bool			Apply(float dt);
	void			Destroy();

	// returns rotation of body0 joint frame relative to body1 joint frame
	Quaternion		GetJointRotation() const;

protected:
	Quaternion		m_body0Frame{ qidentity };
	CEqRigidBody*	m_body0{ nullptr };
	Quaternion		m_body1Frame{ qidentity };
	CEqRigidBody*	m_body1{ nullptr };
	Vector3D		m_minLimit{ vec3_zero };
	Vector3D		m_maxLimit{ vec3_zero };
	float			m_timescale{ 1.0f };

	// some values that we calculate once in PreApply
	Vector3D		m_axis{ vec3_zero };	///< world axis of the limit violation
	float			m_targetVel{ 0.0f };	///< relative angular velocity along axis that removes violation
};
