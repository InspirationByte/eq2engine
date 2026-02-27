//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics joints system
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#include "DkJoltPCH.h"
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

#include "DkPhysicsObject.h"
#include "DkPhysicsJoint.h"

DkPhysicsJoint::DkPhysicsJoint(JPH::TwoBodyConstraint* jphConstraint, DkPhysicsObject* objA, DkPhysicsObject* objB)
	: m_jphContraint(jphConstraint)
	, m_objA(objA)
	, m_objB(objB)
{
}

IPhysicsObject* DkPhysicsJoint::GetPhysicsObjectA() const
{
	return m_objA;
}

IPhysicsObject* DkPhysicsJoint::GetPhysicsObjectB() const
{
	return m_objB;
}

Matrix4x4 DkPhysicsJoint::GetGlobalTransformA() const
{
	return identity4;
}

Matrix4x4 DkPhysicsJoint::GetGlobalTransformB() const
{
	return identity4;
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformA() const
{
	return Convert::FromMat44(m_jphContraint->GetConstraintToBody1Matrix());
}

Matrix4x4 DkPhysicsJoint::GetFrameTransformB() const
{
	return Convert::FromMat44(m_jphContraint->GetConstraintToBody2Matrix());
}

void DkPhysicsJoint::SetLinearLowerLimit(const Vector3D& linearLower)
{
}

void DkPhysicsJoint::SetLinearUpperLimit(const Vector3D& linearUpper)
{
}

void DkPhysicsJoint::SetAngularLowerLimit(const Vector3D& angularLower)
{
}

void DkPhysicsJoint::SetAngularUpperLimit(const Vector3D& angularUpper)
{
}

Vector3D DkPhysicsJoint::GetLinearLowerLimit() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

Vector3D DkPhysicsJoint::GetLinearUpperLimit() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

Vector3D DkPhysicsJoint::GetAngularLowerLimit() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

Vector3D DkPhysicsJoint::GetAngularUpperLimit() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

void DkPhysicsJoint::UpdateTransform()
{
}