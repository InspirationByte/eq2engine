//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics joints system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <Jolt/Core/Core.h>
#include <Jolt/Physics/Constraints/TwoBodyConstraint.h>

#include "dkphysics/IPhysicsJoint.h"

class DkPhysicsObject;

class DkPhysicsJoint : public IPhysicsJoint
{
	friend class DkPhysics;
	friend class DkPhysicsObject;
public:
	~DkPhysicsJoint() = default;

	DkPhysicsJoint(JPH::TwoBodyConstraint* jphConstraint, DkPhysicsObject* objA, DkPhysicsObject* objB);

	IPhysicsObject* GetPhysicsObjectA() const;
	IPhysicsObject* GetPhysicsObjectB() const;

	Matrix4x4		GetGlobalTransformA() const;
	Matrix4x4		GetGlobalTransformB() const;

	Matrix4x4		GetFrameTransformA() const;
	Matrix4x4		GetFrameTransformB() const;

	void			SetLinearLowerLimit(const Vector3D& linearLower);
	void			SetLinearUpperLimit(const Vector3D& linearUpper);
	void			SetAngularLowerLimit(const Vector3D& angularLower);
    void			SetAngularUpperLimit(const Vector3D& angularUpper);

	Vector3D		GetLinearLowerLimit() const;
	Vector3D		GetLinearUpperLimit() const;
	Vector3D		GetAngularLowerLimit() const;
    Vector3D		GetAngularUpperLimit() const;

	void			UpdateTransform();
protected:

	DkPhysicsObject*	m_objA{ nullptr };
	DkPhysicsObject*	m_objB{ nullptr };
	JPH::Ref<JPH::TwoBodyConstraint>	m_jphContraint;
};
