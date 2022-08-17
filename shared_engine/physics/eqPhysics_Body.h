//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
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

	// forces to use Box instead of this object's shape
	BODY_BOXVSDYNAMIC			= (1 << 23),

	//---------------
	// special flags

	BODY_MOVEABLE				= (1 << 30),
};

///
/// Converts angular velocity to spinning
///
inline Quaternion AngularVelocityToSpin( const Quaternion& orientation, const Vector3D& angularVelocity )
{
	Quaternion vel( 0, angularVelocity.x*0.5f, angularVelocity.y*0.5f, angularVelocity.z*0.5f );
	//Quaternion vel( 0, angularVelocity.x, angularVelocity.y, angularVelocity.z);

	//vel.normalize();

    return vel * orientation;
}

inline FMatrix4x4 RigidBodyInverse( const FMatrix4x4 & matrix )
{
	FMatrix4x4 inverse = matrix;
	FVector4D translation = matrix.rows[3];

	inverse.rows[3] = FVector4D(0,0,0,1);
	inverse = transpose( inverse );

	FVector4D x = matrix.rows[0];
	FVector4D y = matrix.rows[1];
	FVector4D z = matrix.rows[2];

	inverse.rows[3] = FVector4D(-dot( x, translation ),
								-dot( y, translation ),
								-dot( z, translation ),
								1.0f );
	return inverse;
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
	static float		ApplyImpulseResponseTo(ContactPair_t& pair, float error_correction_factor);

	static void			CopyValues(CEqRigidBody* dest, const CEqRigidBody* src);

	//---------------------------------------------------------------

							CEqRigidBody();
							~CEqRigidBody();

	void					ClearContacts();

	bool					IsDynamic() const {return true;}

	void					SetMass(float mass, float inertiaScale = 1.0f);
	float					GetMass() const;
	float					GetInvMass() const;

	void					ApplyImpulse(const FVector3D& rel_pos, const Vector3D& impulse);			///< apply impulse at relative position
	void					ApplyForce(const FVector3D& rel_pos, const Vector3D& force);				///< apply force at relative position

	void					ApplyAngularImpulse( const Vector3D& impulse );
	void					ApplyAngularImpulseAt(const FVector3D& rel_pos, const Vector3D& impulse);

	void					ApplyAngularForce(const Vector3D& force);
	void					ApplyAngularForceAt(const FVector3D& rel_pos, const Vector3D& force);

	void					ApplyLinearImpulse(const Vector3D& impulse);
	void					ApplyLinearForce(const Vector3D& force);

	void					ApplyWorldImpulse(const FVector3D& position, const Vector3D& impulse);		///< apply impulse at world position
	void					ApplyWorldForce(const FVector3D& position, const Vector3D& force);			///< apply impulse at world position

	void					SetPosition(const FVector3D& position);										///< sets new position
	void					SetOrientation(const Quaternion& orient);									///< sets new orientation and updates inertia tensor

	void					SetCenterOfMass(const FVector3D& center);									///< sets new center of mass
	const FVector3D&		GetCenterOfMass() const;													///< returns body center of mass

	Vector3D				GetVelocityAtLocalPoint(const FVector3D& point) const;						///< returns velocity at specified local point
	Vector3D				GetVelocityAtWorldPoint(const FVector3D& point) const;						///< returns velocity at specified world point

	const Vector3D&			GetLinearVelocity() const;													///< returns linear velocity
	const Vector3D&			GetAngularVelocity() const;													///< returns angular velocity

	void					SetLinearVelocity(const Vector3D& velocity);								///< sets new linear velocity (momentum / invMass)
	void					SetAngularVelocity(const Vector3D& velocity);								///< sets new angular velocity

	void					SetLinearFactor(const Vector3D& fac);										///< sets new linear factor
	void					SetAngularFactor(const Vector3D& fac);										///< sets new angular factor

	float					GetGravity() const;
	void					SetGravity(float value);													///< sets new gravity force

	const Matrix3x3&		GetWorldInvInertiaTensor() const;											///< returns world transformed inverse inertia tensor

	void					TryWake( bool velocityCheck = true );										///< tries to wake the body up
	void					Wake();																		///< unfreezes the body even if it was forced to freeze
	void					Freeze();																	///< force freezes body and external powers will not wake it up
	bool					IsFrozen();																	///< indicates that body has been frozen (forced or timed out)

	void					SetMinFrameTime( float time, bool ignoreMotion = true );					///< sets minimal frame time for collision detections
	float					GetMinFrametime();
	float					GetLastFrameTime();															///< returns last frame time (used if min frame time set)
	bool					IsCanIntegrate( bool checkIgnore = false);

	void					Integrate( float time );													///< integrates velocities on body and stores position
	void					Update( float time );														///< updates body position
	
	// constraints
	void					AddConstraint( IEqPhysicsConstraint* constraint );
	void					RemoveConstraint( IEqPhysicsConstraint* constraint );
	void					RemoveAllConstraints();

	void					SetConstraintsUnsatisfied();

	float					ComputeImpulseDenominator(const FVector3D& pos, const Vector3D& normal) const;

protected:


	void					ComputeInertia(float scale);
	void					UpdateInertiaTensor();		///< updates inertia tensor
	void					AccumulateForces(float time);	///< accumulates forces

	Array<ContactPair_t>			m_contactPairs{ PP_SL }; // contact pair list in single frame
	Array<IEqPhysicsConstraint*>	m_constraints{ PP_SL };

	Matrix3x3						m_invInertiaTensor;

	Vector3D						m_linearVelocity;	// linear velocity - IS READ ONLY! To set: linearMomentum * mass
	Vector3D						m_angularVelocity;

	Vector3D						m_linearFactor;
	Vector3D						m_angularFactor;

	Vector3D						m_totalTorque;
	Vector3D						m_totalForce;

	// constant params
	Vector3D						m_inertia;
	Vector3D						m_invInertia;

	FVector3D						m_centerOfMass;

	float							m_freezeTime;

	float							m_minFrameTime;
	float							m_frameTimeAccumulator;
	float							m_lastFrameTime;
	bool							m_minFrameTimeIgnoreMotion;

public:
	FVector3D						m_centerOfMassTrans;

	Quaternion						m_prevOrientation;
	FVector3D						m_prevPosition;

protected:

	float				m_gravity;

	float				m_mass;
	float				m_invMass;
};
