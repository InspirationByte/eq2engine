//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics point constraint
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_MaxDistConstraint.h"
#include "eqPhysics_Body.h"

CEqPhysicsMaxDistConstraint::CEqPhysicsMaxDistConstraint() : m_body0(nullptr), m_body1(nullptr)
{
}

CEqPhysicsMaxDistConstraint::~CEqPhysicsMaxDistConstraint()
{
	if (m_body0)
		m_body0->RemoveConstraint(this);

	if (m_body1)
		m_body1->RemoveConstraint(this);

	Destroy();
}

void CEqPhysicsMaxDistConstraint::Init(CEqRigidBody* body0, const FVector3D& body0Pos,
										CEqRigidBody* body1, const FVector3D& body1Pos,
										float maxDistance, int flags)
{
	m_body0Pos = body0Pos;
	m_body1Pos = body1Pos;
	m_body0 = body0;
	m_body1 = body1;
	m_maxDistance = maxDistance;
	m_flags = flags;

	if (m_body0)
		m_body0->AddConstraint(this);

	if (m_body1)
		m_body1->AddConstraint(this);
}

void CEqPhysicsMaxDistConstraint::PreApply(float dt)
{
	m_satisfied = false;

	m_R0 = rotateVector(m_body0Pos, m_body0->GetOrientation());
	m_R1 = rotateVector(m_body1Pos, m_body1->GetOrientation());

	const FVector3D worldPos0(m_body0->GetPosition() + m_R0);
	const FVector3D worldPos1(m_body1->GetPosition() + m_R1);
	m_worldPos = 0.5f * (worldPos0 + worldPos1);

	// current location of point 0 relative to point 1
	m_currentRelPos0 = worldPos0 - worldPos1;
}

bool CEqPhysicsMaxDistConstraint::Apply(float dt)
{
	const float MaxVelocityMagnitude = 20.0f;
	const float minVelForProcessing = 0.01f;

	m_satisfied = true;

	//bool body0FrozenPre = (m_body0->m_flags & BODY_FROZEN);
	//bool body1FrozenPre = (m_body1->m_flags & BODY_FROZEN);
	//  if (body0FrozenPre && body1FrozenPre)
	//    return false;

	const Vector3D currentVel0(m_body0->GetLinearVelocity() + cross(m_R0, m_body0->GetAngularVelocity()));
	const Vector3D currentVel1(m_body1->GetLinearVelocity() + cross(m_R1, m_body1->GetAngularVelocity()));

	// predict a new location
	Vector3D predRelPos0(m_currentRelPos0 + (currentVel0 - currentVel1) * dt);

	// if the new position is out of range then clamp it
	Vector3D clampedRelPos0 = predRelPos0;

	float clampedRelPos0Mag = length(clampedRelPos0);
	if (clampedRelPos0Mag <= F_EPS)
		return false;

	if (clampedRelPos0Mag > m_maxDistance)
		clampedRelPos0 *= m_maxDistance / clampedRelPos0Mag;

	// now claculate desired vel based on the current pos, new/clamped
	// pos and dt
	Vector3D desiredRelVel0((clampedRelPos0 - m_currentRelPos0) / max(dt, F_EPS));

	// Vr is -ve the total velocity change
	Vector3D Vr((currentVel0 - currentVel1) - desiredRelVel0);

	float normalVel = length(Vr);

	// limit it
	if (normalVel > MaxVelocityMagnitude)
	{
		Vr *= (MaxVelocityMagnitude / normalVel);
		normalVel = MaxVelocityMagnitude;
	}
	else if (normalVel < minVelForProcessing)
	{
		return false;
	}

	const Vector3D N = Vr / normalVel;

	float denominator = m_body0->GetInvMass() + m_body1->GetInvMass() + 
						dot(N, cross(m_body0->GetWorldInvInertiaTensor() * (cross(m_R0, N)), m_R0)) + 
						dot(N, cross(m_body1->GetWorldInvInertiaTensor() * (cross(m_R1, N)), m_R1));

	if (denominator < F_EPS)
		return false;

	float normalImpulse = -normalVel / denominator;

	if(!(m_flags & CONSTRAINT_FLAG_BODYA_NOIMPULSE))
		m_body0->ApplyWorldImpulse(m_worldPos, normalImpulse * N);

	if(!(m_flags & CONSTRAINT_FLAG_BODYB_NOIMPULSE))
		m_body1->ApplyWorldImpulse(m_worldPos, -normalImpulse * N);

	m_body0->TryWake();
	m_body1->TryWake();

	m_body0->SetConstraintsUnsatisfied();
	m_body1->SetConstraintsUnsatisfied();

	m_satisfied = true;

	return true;
}

void CEqPhysicsMaxDistConstraint::Destroy()
{
	m_body0 = m_body1 = nullptr;
	SetEnabled(false);
}
