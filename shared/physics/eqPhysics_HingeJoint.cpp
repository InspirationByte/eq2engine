//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics hinge joint
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "eqPhysics_HingeJoint.h"
#include "eqPhysics_Body.h"
#include "eqPhysics.h"

enum EHingeFlags
{
	HINGE_FLAG_USELIMITS	= (1 << 15),
	HINGE_FLAG_BROKEN		= (1 << 16),		// state flag only
	HINGE_FLAG_ENABLED		= (1 << 17),		// state flag only
};

CEqPhysicsHingeJoint::CEqPhysicsHingeJoint() :
	m_body0(nullptr), m_body1(nullptr), m_extraTorque(0.0f), m_damping(0.0f), m_flags(0)
{

}

CEqPhysicsHingeJoint::~CEqPhysicsHingeJoint()
{

}

void CEqPhysicsHingeJoint::AddedToWorld( CEqPhysics* physics )
{
	physics->AddConstraint(&m_midPointConstraint);
	physics->AddConstraint(&m_maxDistanceConstraint);
	physics->AddConstraint(&m_sidePointConstraints[0]);
	physics->AddConstraint(&m_sidePointConstraints[1]);
}

void CEqPhysicsHingeJoint::RemovedFromWorld( CEqPhysics* physics )
{
	physics->RemoveConstraint(&m_midPointConstraint);
	physics->RemoveConstraint(&m_maxDistanceConstraint);
	physics->RemoveConstraint(&m_sidePointConstraints[0]);
	physics->RemoveConstraint(&m_sidePointConstraints[1]);
}

void CEqPhysicsHingeJoint::Init(CEqRigidBody* body0, CEqRigidBody* body1, 
								const Vector3D & hingeAxis, 
								const FVector3D & hingePosRel0,
								const float hingeHalfWidth,
								const float hingeFwdAngle,
								const float hingeBckAngle,
								const float sidewaysSlack,
								const float damping,
								int flags)
{
	m_body0 = body0;
	m_body1 = body1;
	m_hingeAxis = hingeAxis;
	m_hingePosRel0 = hingePosRel0;
	m_damping = damping;
	m_flags = flags;

	FVector3D hingePosRel1 = rotateVector(body0->GetPosition() + rotateVector(hingePosRel0, m_body0->GetOrientation()) - body1->GetPosition(), !m_body1->GetOrientation());

	// generate the two positions relative to each body
	FVector3D relPos0a = hingePosRel0 + hingeHalfWidth * m_hingeAxis;
	FVector3D relPos0b = hingePosRel0 - hingeHalfWidth * m_hingeAxis;

	FVector3D relPos1a = hingePosRel1 + hingeHalfWidth * m_hingeAxis;
	FVector3D relPos1b = hingePosRel1 - hingeHalfWidth * m_hingeAxis;

	float timescale = 1.0f / 20.0f;
	float allowedDistanceMid = 0.01f;
	float allowedDistanceSide = sidewaysSlack * hingeHalfWidth;

	m_sidePointConstraints[0].Init(body0, relPos0a, body1, relPos1a, allowedDistanceSide, flags);
	m_sidePointConstraints[1].Init(body0, relPos0b, body1, relPos1b, allowedDistanceSide, flags);

	m_midPointConstraint.Init(body0, hingePosRel0, body1, hingePosRel1, allowedDistanceMid, timescale, flags);

	if (hingeFwdAngle <= MAX_HINGE_ANGLE_LIMIT)
	{
		// choose a direction that is perpendicular to the hinge
		Vector3D perpDir(0.0f, 0.0f, 1.0f);

		if (dot(perpDir, m_hingeAxis) > 0.1f)
			perpDir = Vector3D(0.0f, 1.0f, 0.0f);

		// now make it perpendicular to the hinge
		Vector3D sideAxis = cross(m_hingeAxis, perpDir);
		perpDir = normalize( cross(sideAxis, m_hingeAxis) );
    
		// the length of the "arm" TODO take this as a parameter? what's
		// the effect of changing it?
		float len = 10.0f * hingeHalfWidth;
    
		// Choose a position using that dir. this will be the anchor point
		// for body 0. relative to hinge
		Vector3D hingeRelAnchorPos0 = perpDir * len;

		

		// anchor point for body 2 is chosen to be in the middle of the
		// angle range.  relative to hinge
		float angleToMiddle = 0.5f * (hingeFwdAngle - hingeBckAngle);
		Vector3D hingeRelAnchorPos1 = rotateAxis3(m_hingeAxis, -angleToMiddle) * hingeRelAnchorPos0;

		hingeRelAnchorPos0 = rotateVector(hingeRelAnchorPos0, m_body0->GetOrientation());
		hingeRelAnchorPos1 = rotateVector(hingeRelAnchorPos1, m_body1->GetOrientation());
    
		// work out the "string" length
		float hingeHalfAngle = 0.5f * (hingeFwdAngle + hingeBckAngle);
		float allowedDistance = len * 2.0f * sinf(hingeHalfAngle * 0.5f);

		FVector3D hingePos = body1->GetPosition() + rotateVector(hingePosRel0, m_body0->GetOrientation());
		FVector3D relPos0c = rotateVector(hingePos + hingeRelAnchorPos0 - body0->GetPosition(), !m_body0->GetOrientation());
		FVector3D relPos1c = rotateVector(hingePos + hingeRelAnchorPos1 - body1->GetPosition(), !m_body1->GetOrientation());

		m_maxDistanceConstraint.Init(	body0, relPos0c, 
										body1, relPos1c,
										allowedDistance, flags);
		m_flags |= HINGE_FLAG_USELIMITS;
	}

	if (m_damping <= 0.0f)
		m_damping = -1.0f; // just make sure that a value of 0.0 doesn't mess up...
	else
		m_damping = clamp(m_damping, 0.0f, 1.0f);
}

void CEqPhysicsHingeJoint::SetEnabled(bool enable)
{
	if(enable)
	{
		if (m_body0)
		{
			m_midPointConstraint.SetEnabled(true);
			m_sidePointConstraints[0].SetEnabled(true);
			m_sidePointConstraints[1].SetEnabled(true);

			if ((m_flags & HINGE_FLAG_USELIMITS) && !(m_flags & HINGE_FLAG_BROKEN))
				m_maxDistanceConstraint.SetEnabled(true);
		}
	}
	else
	{
		if (m_body0)
		{
			m_midPointConstraint.SetEnabled(false);
			m_sidePointConstraints[0].SetEnabled(false);
			m_sidePointConstraints[1].SetEnabled(false);

			if ((m_flags & HINGE_FLAG_USELIMITS) && !(m_flags & HINGE_FLAG_BROKEN))
				m_maxDistanceConstraint.SetEnabled(false);
		}
	}

	m_enabled = enable;
}

void CEqPhysicsHingeJoint::Break()
{
	if(IsBroken())
		return;

	if(m_flags & HINGE_FLAG_USELIMITS)
		m_maxDistanceConstraint.SetEnabled(false);

	m_flags |= HINGE_FLAG_BROKEN;
}

void CEqPhysicsHingeJoint::Restore()
{
	if(!IsBroken())
		return;

	if(m_flags & HINGE_FLAG_USELIMITS)
		m_maxDistanceConstraint.SetEnabled(true);

	m_flags &= ~HINGE_FLAG_BROKEN;
}

 bool CEqPhysicsHingeJoint::IsBroken() const
 {
	return (m_flags & HINGE_FLAG_BROKEN);
 }

void CEqPhysicsHingeJoint::Update(float dt)
{
	ASSERT(m_body0 != nullptr);
	ASSERT(m_body1 != nullptr);

	if(m_damping > 0.0f)
	{
		// Some hinges can bend in wonky ways. Derive the effective hinge axis
		// using the relative rotation of the bodies.
		Vector3D hingeAxis = m_body1->GetAngularVelocity() - m_body0->GetAngularVelocity();
		hingeAxis = fastNormalize(hingeAxis);

		const float angRot1 = dot(m_body0->GetAngularVelocity(), hingeAxis);
		const float angRot2 = dot(m_body1->GetAngularVelocity(), hingeAxis);

		const float avAngRot = 0.5f * (angRot1 + angRot2);

		const float frac = 1.0f - m_damping;
		const float newAngRot1 = avAngRot + (angRot1 - avAngRot) * frac;
		const float newAngRot2 = avAngRot + (angRot2 - avAngRot) * frac;

		const Vector3D newAngVel1 = 
			m_body0->GetAngularVelocity() + (newAngRot1 - angRot1) * hingeAxis;
		const Vector3D newAngVel2 = 
			m_body1->GetAngularVelocity() + (newAngRot2 - angRot2) * hingeAxis;

		if(!(m_flags & CONSTRAINT_FLAG_BODYA_NOIMPULSE))
			m_body0->SetAngularVelocity(newAngVel1);

		if(!(m_flags & CONSTRAINT_FLAG_BODYB_NOIMPULSE))
			m_body1->SetAngularVelocity(newAngVel2);
	}
	/*
	// the extra torque
	if (m_extraTorque != 0.0f)
	{
		Vector3D torque1 = m_extraTorque * (m_body0->GetOrientation() * m_hingeAxis);
		m_body0->AddWorldTorque(torque1);
		m_body1->AddWorldTorque(-torque1);
	}*/
}