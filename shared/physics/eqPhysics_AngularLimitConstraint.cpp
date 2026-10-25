//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2025
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics angular limit constraint
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_AngularLimitConstraint.h"
#include "eqPhysics_Body.h"

// rotation vector uses the same convention as CEqRigidBody angular velocity
static Vector3D QuaternionToRotationVector(const Quaternion& q)
{
	const Quaternion shortest = (q.w < 0.0f) ? -q : q;
	const Vector3D v = shortest.asVector4D().xyz();

	const float sinHalfAngle = length(v);
	if (sinHalfAngle < F_EPS)
		return v * 2.0f;

	const float angle = 2.0f * atan2f(sinHalfAngle, shortest.w);
	return v * (angle / sinHalfAngle);
}

static Quaternion RotationVectorToQuaternion(const Vector3D& rotVec)
{
	const float angle = length(rotVec);
	if (angle < F_EPS)
		return qidentity;

	return Quaternion(angle, rotVec / angle);
}

CEqAngularLimitConstraint::~CEqAngularLimitConstraint()
{
	if (m_body0)
		m_body0->RemoveConstraint(this);

	if (m_body1)
		m_body1->RemoveConstraint(this);

	Destroy();
}

void CEqAngularLimitConstraint::Init(CEqRigidBody* body0, const Quaternion& body0Frame,
									CEqRigidBody* body1, const Quaternion& body1Frame,
									const Vector3D& minLimit, const Vector3D& maxLimit,
									float timescale, int flags)
{
	m_body0 = body0;
	m_body0Frame = body0Frame;
	m_body1 = body1;
	m_body1Frame = body1Frame;
	m_timescale = timescale;
	m_flags = flags;

	for (int i = 0; i < 3; ++i)
	{
		m_minLimit[i] = min(minLimit[i], maxLimit[i]);
		m_maxLimit[i] = max(minLimit[i], maxLimit[i]);
	}

	if (m_body0)
		m_body0->AddConstraint(this);

	if (m_body1)
		m_body1->AddConstraint(this);
}

Quaternion CEqAngularLimitConstraint::GetJointRotation() const
{
	const Quaternion frame0 = m_body0->GetOrientation() * m_body0Frame;
	const Quaternion frame1 = m_body1->GetOrientation() * m_body1Frame;
	return !frame1 * frame0;
}

void CEqAngularLimitConstraint::PreApply(float dt)
{
	m_satisfied = false;
	m_targetVel = 0.0f;

	// rotateVector() rotates by the conjugate, so angles around the axes have the opposite sign
	const Vector3D jointAngles = -QuaternionToRotationVector(GetJointRotation());

	Vector3D limitedAngles;
	for (int i = 0; i < 3; ++i)
		limitedAngles[i] = clamp(jointAngles[i], m_minLimit[i], m_maxLimit[i]);

	if (lengthSqr(limitedAngles - jointAngles) < sqr(F_EPS))
		return;

	const Vector3D limitedRotVec = -limitedAngles;

	// orientation of body0 that keeps the joint at the limit
	const Quaternion frame1 = m_body1->GetOrientation() * m_body1Frame;
	const Quaternion targetOrientation0 = frame1 * RotationVectorToQuaternion(limitedRotVec) * !m_body0Frame;

	const Vector3D error = QuaternionToRotationVector(targetOrientation0 * !m_body0->GetOrientation());
	const float errorAmount = lengthSqr(error);
	if (errorAmount < sqr(F_EPS))
		return;

	m_axis = error / errorAmount;
	m_targetVel = errorAmount / max(m_timescale, dt);
}

bool CEqAngularLimitConstraint::Apply(float dt)
{
	m_satisfied = true;

	if (m_targetVel <= 0.0f)
		return false;

	const float relativeVel = dot(m_body0->GetAngularVelocity() - m_body1->GetAngularVelocity(), m_axis);
	if (relativeVel >= m_targetVel)
		return false;

	float denominator = 0.0f;
	if (!(m_flags & CONSTRAINT_FLAG_BODYA_NOIMPULSE))
		denominator += dot(m_axis, m_body0->GetWorldInvInertiaTensor() * m_axis);

	if (!(m_flags & CONSTRAINT_FLAG_BODYB_NOIMPULSE))
		denominator += dot(m_axis, m_body1->GetWorldInvInertiaTensor() * m_axis);

	if (denominator < F_EPS)
		return false;

	const Vector3D impulse = ((m_targetVel - relativeVel) / denominator) * m_axis;

	if (!(m_flags & CONSTRAINT_FLAG_BODYA_NOIMPULSE))
		m_body0->ApplyAngularImpulse(impulse);

	if (!(m_flags & CONSTRAINT_FLAG_BODYB_NOIMPULSE))
		m_body1->ApplyAngularImpulse(-impulse);

	m_body0->TryWake();
	m_body1->TryWake();

	m_body0->SetConstraintsUnsatisfied();
	m_body1->SetConstraintsUnsatisfied();

	return true;
}

void CEqAngularLimitConstraint::Destroy()
{
	m_body0 = m_body1 = nullptr;
	SetEnabled(false);
}
