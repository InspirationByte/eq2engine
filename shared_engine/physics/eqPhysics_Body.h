//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rigid body
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "eqCollision_Object.h"

class IEqPhysicsConstraint;

#define BODY_DISABLE_RESPONSE	COLLOBJ_DISABLE_RESPONSE
#define BODY_COLLISIONLIST		COLLOBJ_COLLISIONLIST

enum EBodyFlags
{
	// body is temporary frozen and can woke up
	BODY_FROZEN					= (1 << 16),

	// body is constantly frozen and can't woke up unless this flag active
	BODY_FORCE_FREEZE			= (1 << 17),

	// when object is frozen, it will preserve the forces
	BODY_FORCE_PRESERVEFORCES	= (1 << 18),

	// disables angular damping of body
	BODY_DISABLE_DAMPING		= (1 << 19),

	// disables body freezing
	BODY_NO_AUTO_FREEZE			= (1 << 20),

	// don't take response from all bodies (good for trains)
	BODY_INFINITEMASS			= (1 << 21),

	// cars have some specials like "Car vs Car" is a "Box vs Box" collision detection and faster game object detection
	BODY_ISCAR					= (1 << 22),

	//---------------
	// special flags

	// appears in moveable list
	BODY_MOVEABLE				= (1 << 24),
};

///
/// Converts angular velocity to spinning
///
inline Quaternion AngularVelocityToSpin( const Quaternion& orientation, const Vector3D& angularVelocity )
{
	Quaternion vel( 0, angularVelocity.x*0.5f, angularVelocity.y*0.5f, angularVelocity.z*0.5f );
    return vel * orientation;
}

//-----------------------------------------------------------------------------------------------------------------------------

///
/// Eq Rigid body
///
class CEqRigidBody : public CEqCollisionObject
{
	friend class CEqPhysics;
public:

	/// Computes friction velocity to apply
	static Vector3D		ComputeFrictionVelocity( const Vector3D& collNormal, const Vector3D& collVelocity, float normalImpulse, float denominator, float staticFriction, float dynamicFriction);

	/// Simply applies impulse response to single body
	static float		ApplyImpulseResponseTo(eqContactPair& pair, float error_correction_factor);

	static void			CopyValues(CEqRigidBody* dest, const CEqRigidBody* src);

	//---------------------------------------------------------------

	CEqRigidBody();
	~CEqRigidBody();

	void				ClearContacts();

	bool				IsDynamic() const {return true;}

	void				SetMass(float mass, float inertiaScale = 1.0f);
	float				GetMass() const;
	float				GetInvMass() const;

	void				ApplyImpulse(const FVector3D& rel_pos, const Vector3D& impulse);			///< apply impulse at relative position
	void				ApplyForce(const FVector3D& rel_pos, const Vector3D& force);				///< apply force at relative position

	void				ApplyAngularImpulse( const Vector3D& impulse );
	void				ApplyAngularImpulseAt(const FVector3D& rel_pos, const Vector3D& impulse);

	void				ApplyAngularForce(const Vector3D& force);
	void				ApplyAngularForceAt(const FVector3D& rel_pos, const Vector3D& force);

	void				ApplyLinearImpulse(const Vector3D& impulse);
	void				ApplyLinearForce(const Vector3D& force);

	void				ApplyWorldImpulse(const FVector3D& position, const Vector3D& impulse);		///< apply impulse at world position
	void				ApplyWorldForce(const FVector3D& position, const Vector3D& force);			///< apply impulse at world position

	void				SetPosition(const FVector3D& position);										///< sets new position
	void				SetOrientation(const Quaternion& orient);									///< sets new orientation and updates inertia tensor

	const FVector3D&	GetPrevPosition() const;													///< returns last frame body position
	const Quaternion&	GetPrevOrientation() const;													///< returns last frame body Quaternion orientation

	void				SetCenterOfMass(const FVector3D& center);									///< sets new center of mass
	const FVector3D&	GetCenterOfMass() const;													///< returns body center of mass

	Vector3D			GetVelocityAtLocalPoint(const FVector3D& point) const;						///< returns velocity at specified local point
	Vector3D			GetVelocityAtWorldPoint(const FVector3D& point) const;						///< returns velocity at specified world point

	const Vector3D&		GetLinearVelocity() const;													///< returns linear velocity
	const Vector3D&		GetAngularVelocity() const;													///< returns angular velocity

	void				SetLinearVelocity(const Vector3D& velocity);								///< sets new linear velocity (momentum / invMass)
	void				SetAngularVelocity(const Vector3D& velocity);								///< sets new angular velocity

	void				SetLinearFactor(const Vector3D& fac);										///< sets new linear factor
	void				SetAngularFactor(const Vector3D& fac);										///< sets new angular factor

	float				GetGravity() const;
	void				SetGravity(float value);													///< sets new gravity force

	const Matrix3x3&	GetWorldInvInertiaTensor() const;											///< returns world transformed inverse inertia tensor

	bool				TryWake( bool velocityCheck = true );										///< tries to wake the body up
	void				Wake();																		///< unfreezes the body even if it was forced to freeze
	void				Freeze();																	///< force freezes body and external powers will not wake it up
	bool				IsFrozen() const;															///< indicates that body has been frozen (forced or timed out)

	void				SetMinFrameTime( float time, bool ignoreMotion = true );					///< sets minimal frame time for collision detections
	float				GetMinFrametime() const;
	float				GetLastFrameTime() const;													///< returns last frame time (used if min frame time set)
	float				GetAccumDeltaTime() const;
	bool				IsCanIntegrate( bool checkIgnore = false) const;

	void				Integrate( float time );													///< integrates velocities on body and stores position
	void				Update( float time );														///< updates body position
	
	// constraints
	void				AddConstraint( IEqPhysicsConstraint* constraint );
	void				RemoveConstraint( IEqPhysicsConstraint* constraint );
	void				RemoveAllConstraints();

	void				SetConstraintsUnsatisfied();

	float				ComputeImpulseDenominator(const FVector3D& pos, const Vector3D& normal) const;

protected:
	void				ComputeInertia(float scale);
	void				UpdateInertiaTensor();		///< updates inertia tensor
	void				AccumulateForces(float time);	///< accumulates forces

	FixedArray<eqContactPair, 32>			m_contactPairs; // contact pair list in single frame
	FixedArray<IEqPhysicsConstraint*, 8>	m_constraints;

	Matrix3x3			m_invInertiaTensor{ identity3 };

	Quaternion			m_prevOrientation{ qidentity };
	FVector3D			m_prevPosition{ 0.0f };

	FVector3D			m_centerOfMassTrans{ 0.0f };

	Vector3D			m_linearVelocity{ vec3_zero };	// linear velocity - IS READ ONLY! To set: linearMomentum * mass
	Vector3D			m_angularVelocity{ vec3_zero };

	Vector3D			m_linearFactor{ 1.0f };
	Vector3D			m_angularFactor{ 1.0f };

	Vector3D			m_totalTorque{ vec3_zero };
	Vector3D			m_totalForce{ vec3_zero };

	// constant params
	Vector3D			m_inertia{ 1.0f };
	Vector3D			m_invInertia{ 1.0f };

	FVector3D			m_centerOfMass{ 0.0f };

	float				m_gravity{ 0.0f };
	float				m_mass{ 1.0f };
	float				m_invMass{ 1.0f };

	float				m_freezeTime{ 0.0f };

	float				m_minFrameTime{ 0.0f };
	float				m_frameTimeAccumulator{ 0.0f };
	float				m_lastFrameTime{ 0.0f };
	bool				m_minFrameTimeIgnoreMotion{ false };
};
