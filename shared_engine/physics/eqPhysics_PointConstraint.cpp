//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics point constraint
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_PointConstraint.h"
#include "eqPhysics_Body.h"

CEqPhysicsPointConstraint::CEqPhysicsPointConstraint() : m_body0(nullptr), m_body1(nullptr)
{
}

CEqPhysicsPointConstraint::~CEqPhysicsPointConstraint()
{
	if (m_body0)
		m_body0->RemoveConstraint(this);

	if (m_body1)
		m_body1->RemoveConstraint(this);

	Destroy();
}

void CEqPhysicsPointConstraint::Init(CEqRigidBody* body0, const FVector3D& body0Pos,
									CEqRigidBody* body1, const FVector3D& body1Pos,
									float allowedDistance,
									float timescale, int flags)
{
	m_body0Pos = body0Pos;
	m_body1Pos = body1Pos;
	m_body0 = body0;
	m_body1 = body1;
	m_allowedDistance = allowedDistance;
	m_timescale = timescale;
	m_flags = flags;

	if (m_body0)
		m_body0->AddConstraint(this);

	if (m_body1)
		m_body1->AddConstraint(this);
}

void CEqPhysicsPointConstraint::PreApply(float dt)
{
	m_satisfied = false;

	m_R0 = rotateVector(m_body0Pos, m_body0->GetOrientation());
	m_R1 = rotateVector(m_body1Pos, m_body1->GetOrientation());

	const FVector3D worldPos0(m_body0->GetPosition() + m_R0);
	const FVector3D worldPos1(m_body1->GetPosition() + m_R1);
	m_worldPos = 0.5f * (worldPos0 + worldPos1);

	// add a "correction" based on the deviation of point 0
	const Vector3D deviation(worldPos0 - worldPos1);
	float deviationAmount = length(deviation);
	
	if (deviationAmount > m_allowedDistance)
	{
		m_VrExtra = ((deviationAmount - m_allowedDistance) / (deviationAmount * max(m_timescale, dt)))* deviation;
	}
	else
	{
		m_VrExtra = vec3_zero;
	}
}

bool CEqPhysicsPointConstraint::Apply(float dt)
{
	const float MaxVelocityMagnitude = 200.0f;
	const float minVelForProcessing = 0.01f;
	const int CONSTRAINT_STEPS = 8;

	m_satisfied = true;

	const float compliance = 0.0f;
	const float stepSize = dt / CONSTRAINT_STEPS;
	const float dstep = compliance / (stepSize * stepSize);
	
	for(int i = 0; i < CONSTRAINT_STEPS; i++)
	{		
		// velocity at point, but computed a bit differently
		const Vector3D currentVel0(m_body0->GetLinearVelocity() + cross(m_R0, m_body0->GetAngularVelocity()));
		const Vector3D currentVel1(m_body1->GetLinearVelocity() + cross(m_R1, m_body1->GetAngularVelocity()));
		
		// add a "correction" based on the deviation of point 0
		Vector3D Vr(m_VrExtra + currentVel0 - currentVel1);

		float normalVel = length(Vr);
		if (normalVel < minVelForProcessing)
			return false;
	
		// limit things
		if (normalVel > MaxVelocityMagnitude)
		{
			Vr *= MaxVelocityMagnitude / normalVel;
			normalVel = MaxVelocityMagnitude;
		}

		const Vector3D N = Vr / normalVel;

		const float numerator = -normalVel - dstep;
		const float denominator = dstep + m_body0->GetInvMass() + m_body1->GetInvMass() + 
			dot(N, cross(m_body0->GetWorldInvInertiaTensor() * (cross(m_R0, N)), m_R0)) + 
			dot(N, cross(m_body1->GetWorldInvInertiaTensor() * (cross(m_R1, N)), m_R1));

		if (denominator < F_EPS)
			return false;
	
		const Vector3D normalImpulse = (numerator / denominator) * N;

		if (!(m_flags & CONSTRAINT_FLAG_BODYA_NOIMPULSE))
			m_body0->ApplyWorldImpulse(m_worldPos, normalImpulse);

		if (!(m_flags & CONSTRAINT_FLAG_BODYB_NOIMPULSE))
			m_body1->ApplyWorldImpulse(m_worldPos, -normalImpulse);

		m_body0->TryWake();
		m_body1->TryWake();
	}

	m_body0->SetConstraintsUnsatisfied();
	m_body1->SetConstraintsUnsatisfied();

	m_satisfied = true;

	return true;
}

void CEqPhysicsPointConstraint::Destroy()
{
	m_body0 = m_body1 = nullptr;
	SetEnabled(false);
}
