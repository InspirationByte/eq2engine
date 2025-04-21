//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Physics hinge joint
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "eqPhysics_Controller.h"
#include "eqPhysics_PointConstraint.h"
#include "eqPhysics_MaxDistConstraint.h"

class CEqRigidBody;

const float MAX_HINGE_ANGLE_LIMIT = DEG2RAD(150.0f);

class CEqHingeJoint : public IEqPhysController
{
public:
	void Init(	CEqRigidBody* body0, CEqRigidBody* body1, 
				const Vector3D& hingeAxis, 
				const Vector3D& hingePosRel0,
				const float hingeHalfWidth,
				const float hingeFwdAngle,	// angles are in radians
				const float hingeBckAngle,	// angles are in radians
				const float sidewaysSlack,
				const float damping = -1.0f,
				int flags = 0);				// constraint flags

	void Init(	CEqRigidBody* body0, CEqRigidBody* body1, 
			const Vector3D& hingeAxis, 
			const Vector3D& hingePosRel0,
			const Vector3D& hingePosRel1,
			const float hingeHalfWidth,
			const float hingeFwdAngle,	// angles are in radians
			const float hingeBckAngle,	// angles are in radians
			const float sidewaysSlack,
			const float damping = -1.0f,
			int flags = 0);				// constraint flags

	void				SetEnabled(bool enable);

    void				Break();		/// Just remove the limit constraint
    void				Restore();		/// Just enable the limit constraint

    bool				IsBroken() const;
    const Vector3D&		GetHingePosRelA() const			{ return m_hingePosRel0; }

	CEqRigidBody*		GetBodyA() const				{ return m_body0; }
	CEqRigidBody*		GetBodyB() const				{ return m_body1; }

    /// We can be asked to apply an extra torque to body0 (and
    /// opposite to body1) each time step.
    void				SetExtraTorque(float torque)	{ m_extraTorque = torque; }

	void				Update(float dt);

protected:
	void				AddedToWorld( CEqPhysics* physics );
	void				RemovedFromWorld( CEqPhysics* physics );

	CEqPointConstraint		m_midPointConstraint;
	CEqMaxDistConstraint	m_sidePointConstraints[2];
	CEqMaxDistConstraint	m_maxDistanceConstraint;

	Vector3D			m_hingeAxis;
	Vector3D			m_hingePosRel0;
	CEqRigidBody*		m_body0{ nullptr };
	CEqRigidBody*		m_body1{ nullptr };

	float				m_damping{ 0.0f };
	float				m_extraTorque{ 0.0f }; // allow extra torque applied per update
	int					m_flags{ 0 };
};
