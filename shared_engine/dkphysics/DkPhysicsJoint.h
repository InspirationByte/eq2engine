//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics joints system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IPhysicsJoint.h"

class btGeneric6DofConstraint;

class DkPhysicsJoint : public IPhysicsJoint
{
	friend class DkPhysics;
	friend class CPhysicsObject;

public:

	DkPhysicsJoint();
	~DkPhysicsJoint();

	IPhysicsObject* GetPhysicsObjectA(); // returns pointer to first physics object
	IPhysicsObject* GetPhysicsObjectB(); // returns pointer to second physics object

	Matrix4x4		GetGlobalTransformA(); // returns global transform of object a
	Matrix4x4		GetGlobalTransformB() ; // returns global transform of object b

	Matrix4x4		GetFrameTransformA(); // returns local transform of object a
	Matrix4x4		GetFrameTransformB() ; // returns local transform of object b

	// setters

	void			SetLinearLowerLimit(const Vector3D& linearLower); // sets lower linear limit (in radians)

	void			SetLinearUpperLimit(const Vector3D& linearUpper); // sets upper linear limit (in radians)

	void			SetAngularLowerLimit(const Vector3D& angularLower); // sets lower angular limit (in radians)

    void			SetAngularUpperLimit(const Vector3D& angularUpper); // sets upper angular limit (in radians)

	// getters

	Vector3D		GetLinearLowerLimit(); // sets lower linear limit (in radians)
	Vector3D		GetLinearUpperLimit(); // returns upper linear limit (in radians)
	Vector3D		GetAngularLowerLimit(); // returns lower angular limit (in radians)
    Vector3D		GetAngularUpperLimit(); // returns upper angular limit (in radians)

	void			UpdateTransform(); // updates transform of joints
protected:

	IPhysicsObject*				m_pObjectA;
	IPhysicsObject*				m_pObjectB;

	btGeneric6DofConstraint*	m_pJointPointer;
};
