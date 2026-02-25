//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics joints system
//////////////////////////////////////////////////////////////////////////////////

#pragma once

class IPhysicsObject;

class IPhysicsJoint
{
public:
	virtual ~IPhysicsJoint() = default;

	virtual IPhysicsObject* GetPhysicsObjectA() const = 0; // returns pointer to first physics object
	virtual IPhysicsObject* GetPhysicsObjectB() const = 0; // returns pointer to second physics object

	virtual Matrix4x4		GetGlobalTransformA() const = 0; // returns global transform of object a
	virtual Matrix4x4		GetGlobalTransformB() const = 0; // returns global transform of object b

	virtual Matrix4x4		GetFrameTransformA() const = 0; // returns local transform of object a
	virtual Matrix4x4		GetFrameTransformB() const = 0; // returns local transform of object b

	// setters

	virtual void			SetLinearLowerLimit(const Vector3D& linearLower) = 0; // sets lower linear limit (in radians)
	virtual void			SetLinearUpperLimit(const Vector3D& linearUpper) = 0; // sets upper linear limit (in radians)
    virtual void			SetAngularLowerLimit(const Vector3D& angularLower) = 0; // sets lower angular limit (in radians)
    virtual void			SetAngularUpperLimit(const Vector3D& angularUpper) = 0; // sets upper angular limit (in radians)

	// getters

	virtual Vector3D		GetLinearLowerLimit() const = 0; // returns lower linear limit (in radians)
	virtual Vector3D		GetLinearUpperLimit() const = 0; // returns upper linear limit (in radians)
	virtual Vector3D		GetAngularLowerLimit() const = 0; // returns lower angular limit (in radians)
    virtual Vector3D		GetAngularUpperLimit() const = 0; // returns upper angular limit (in radians)

	virtual void			UpdateTransform() = 0; // updates transform of joints
};
