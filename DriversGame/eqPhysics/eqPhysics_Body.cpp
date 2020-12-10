//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Rigid body
//////////////////////////////////////////////////////////////////////////////////

#include "eqPhysics_Body.h"

#include "IDebugOverlay.h"

#include "../shared_engine/physics/BulletConvert.h"
using namespace EqBulletUtils;

#include "eqPhysics_Contstraint.h"

#include "ConVar.h"
#include "DebugInterface.h"

//-----------------------------------------------------------------------------------------

ConVar ph_showcollisionresponses("ph_showcollisionresponses", "0");
#define COLLRESPONSE_DEBUG_SCALE 0.005f

ConVar ph_debug_body("ph_debug_body", "0", "Print debug body information in screen");

const float frictionTimeScale = 10.0f;
const float dampingTimeScale = 1.0f;
const float minimumAngularVelocity = 0.000001f;	// squared value

const float gravity_const = 9.81f;
const float angVelDamp = 0.01f;//0.0125f;

const float inertiaMassScale = 1.5f;

const float BODY_FREEZE_TIME		= 0.5f;

const float BODY_MIN_VELOCITY		= 0.08f;
const float BODY_MIN_VELOCITY_ANG	= 0.08f;

const float BODY_MIN_VELOCITY_WAKE		= 0.002f;
const float BODY_MIN_VELOCITY_WAKE_ANG	= 0.005f;

CEqRigidBody::CEqRigidBody() : CEqCollisionObject()
{
	//m_linearMomentum = FVector3D(0.0f);
	m_linearVelocity = Vector3D(0.0f);
	m_angularVelocity = Vector3D(0.0f);
	m_totalTorque = Vector3D(0.0f);
	m_totalForce = Vector3D(0.0f);

	m_linearFactor = Vector3D(1.0f);
	m_angularFactor = Vector3D(1.0f);

	m_restitution = 0.1f;
	m_friction = 0.1f;

	m_centerOfMass = FVector3D(0.0f);
	m_centerOfMassTrans = m_centerOfMass;

	m_gravity = gravity_const;

	m_freezeTime = BODY_FREEZE_TIME;

	m_minFrameTime = 0.0f;
	m_frameTimeAccumulator = 0.0f;
	m_lastFrameTime = 0.0f;
	m_minFrameTimeIgnoreMotion = false;

	m_prevPosition = vec3_zero;
	m_prevOrientation = identity();
}

CEqRigidBody::~CEqRigidBody()
{
	RemoveAllConstraints();
}

void CEqRigidBody::ClearContacts()
{
	CEqCollisionObject::ClearContacts();
	m_contactPairs.clear(false);
}

// constraints
void CEqRigidBody::AddConstraint( IEqPhysicsConstraint* constraint )
{
	m_constraints.append(constraint);
}

void CEqRigidBody::RemoveConstraint( IEqPhysicsConstraint* constraint )
{
	m_constraints.fastRemove(constraint);
}

void CEqRigidBody::RemoveAllConstraints()
{
	for(int i = 0; i < m_constraints.numElem(); i++)
		m_constraints[i]->SetEnabled(false);

	m_constraints.clear();
}

void CEqRigidBody::SetConstraintsUnsatisfied()
{
	for(int i = 0; i < m_constraints.numElem(); i++)
		m_constraints[i]->m_satisfied = false;
}

void CEqRigidBody::ComputeInertia(float scale)
{
	ASSERTMSG(m_shape != NULL, "CEqRigidBody(CEqCollisionObject) - did you forgot to call Initialize()?");

	btVector3 inertia;
	m_shape->calculateLocalInertia(m_mass*scale, inertia);

	m_inertia = ConvertBulletToDKVectors(inertia);

	m_invInertia = Vector3D(	m_inertia.x != 0.0f ? 1.0f / m_inertia.x: 0.0f,
								m_inertia.y != 0.0f ? 1.0f / m_inertia.y: 0.0f,
								m_inertia.z != 0.0f ? 1.0f / m_inertia.z: 0.0f);

	UpdateInertiaTensor();
}

void CEqRigidBody::SetMass(float mass, float inertiaScale /*= 1.0f*/)
{
	m_mass = mass;

	if(m_mass > 0)
		m_invMass = 1.0f / m_mass;
	else
		m_invMass = 0.0f;

	ComputeInertia(inertiaScale);
}

float CEqRigidBody::GetMass() const
{
	return m_mass;
}

float CEqRigidBody::GetInvMass() const
{
	return m_invMass;
}

void CEqRigidBody::TryWake( bool velocityCheck )
{
	if(m_flags & BODY_FORCE_FREEZE)
		return;

	if( velocityCheck && 
		lengthSqr(m_linearVelocity) < BODY_MIN_VELOCITY_WAKE &&
		lengthSqr(m_angularVelocity) < BODY_MIN_VELOCITY_WAKE_ANG)
		return;

	m_flags &= ~BODY_FROZEN;
	m_freezeTime = BODY_FREEZE_TIME;
}

void CEqRigidBody::Wake()
{
	m_flags &= ~(BODY_FROZEN | BODY_FORCE_FREEZE);
	m_freezeTime = BODY_FREEZE_TIME;
}

void CEqRigidBody::Freeze()
{
	m_flags |= (BODY_FROZEN | BODY_FORCE_FREEZE);
}

bool CEqRigidBody::IsFrozen()
{
	int flags = m_flags;
	return ((flags & BODY_FROZEN) || (flags & BODY_FORCE_FREEZE));
}

bool CEqRigidBody::IsCanIntegrate(bool checkIgnore)
{
	if(m_frameTimeAccumulator == 0.0f || (checkIgnore == m_minFrameTimeIgnoreMotion))
		return true;

	return false;
}

void CEqRigidBody::SetMinFrameTime( float time, bool ignoreMotion )
{
	if (m_minFrameTime != time || m_minFrameTimeIgnoreMotion != ignoreMotion)
		m_frameTimeAccumulator = 0.0f;

	m_minFrameTime = time;
	m_minFrameTimeIgnoreMotion = ignoreMotion;
}

float CEqRigidBody::GetMinFrametime()
{
	return m_minFrameTime;
}

float CEqRigidBody::GetLastFrameTime()
{
	return m_lastFrameTime;
}

void CEqRigidBody::Integrate(float delta)
{
	if( IsFrozen() )
	{
		if(!(m_flags & BODY_FORCE_PRESERVEFORCES))
		{
			// zero some forces
			m_totalTorque = Vector3D(0);
			m_totalForce = Vector3D(0);
			m_linearVelocity = Vector3D(0);
			m_angularVelocity = Vector3D(0);
		}

		return;
	}

	if(m_mass <= 0.0f)
		return;

	float frameTimeAccumulator = m_frameTimeAccumulator;
	bool ignoreMotion = m_minFrameTimeIgnoreMotion;

	if( frameTimeAccumulator < m_minFrameTime )
	{
		frameTimeAccumulator += delta;

		if(!ignoreMotion)
			return;
	}
	else if(ignoreMotion)
	{
		frameTimeAccumulator = 0.0f;
	}

	if(ignoreMotion)
	{
		m_lastFrameTime = delta;
	}
	else
	{
		m_lastFrameTime = frameTimeAccumulator + delta;
		frameTimeAccumulator = 0.0f;
	}

	// store
	m_frameTimeAccumulator = frameTimeAccumulator;

	// store previous position
	m_prevPosition = m_position;
	m_prevOrientation = m_orientation;
	
	// accumulate with custom delta time
	AccumulateForces( m_lastFrameTime );

	if(ph_debug_body.GetBool())
	{
		debugoverlay->Text3D(m_position, 50.0f, ColorRGBA(1,1,1,1), 0.0f,
			"Position: [%.2f %.2f %.2f]\n"
			"Lin. vel: [%.2f %.2f %.2f] (%.2f)\n"
			"Ang. vel: [%.2f %.2f %.2f]\n"
			"mass: %g\n"
			"cell: %d",
			(float)m_position.x,(float)m_position.y,(float)m_position.z,
			(float)m_linearVelocity.x,(float)m_linearVelocity.y,(float)m_linearVelocity.z, (float)length(m_linearVelocity),
			(float)m_angularVelocity.x,(float)m_angularVelocity.y,(float)m_angularVelocity.z,
			(float)m_mass,
			m_cell != nullptr);
	}
}

void CEqRigidBody::Update(float time)
{
	Vector3D angularVelocity;

	// re-calculate velocity
	m_linearVelocity = (m_position - m_prevPosition) / time;

	float r_time = 1.0f / time;
	
	Quaternion dq  = m_orientation * !m_prevOrientation;

	angularVelocity = Vector3D(dq.x * 2.0 * r_time, dq.y * 2.0 * r_time, dq.z * 2.0 * r_time);

	if (dq.w < 0.0f)
		m_angularVelocity = -m_angularVelocity;

	m_angularVelocity = angularVelocity;

	// set for collision detection
	UpdateBoundingBoxTransform();
}

void CEqRigidBody::AccumulateForces(float time)
{
	Vector3D linearVelocity = m_linearVelocity;
	Vector3D angularVelocity = m_angularVelocity;
	Quaternion orientation = m_orientation;

	int flags = m_flags;

	if (!(flags & BODY_NO_AUTO_FREEZE))
	{
		if (lengthSqr(linearVelocity) < BODY_MIN_VELOCITY &&
			lengthSqr(angularVelocity) < BODY_MIN_VELOCITY_ANG)
		{
			m_freezeTime -= time;

			if (m_freezeTime < 0.0f)
				m_flags |= BODY_FROZEN;
		}
		else
			m_freezeTime = BODY_FREEZE_TIME;
	}

	// gravity to momentum
	linearVelocity += Vector3D(0,-m_gravity,0) * time;

	// apply force to momentum
	linearVelocity += m_totalForce * m_invMass * time;

	// apply torque
	angularVelocity += (m_invInertiaTensor * m_totalTorque) * time;

	if( lengthSqr(angularVelocity) < minimumAngularVelocity )
		angularVelocity = vec3_zero;

	if(!(flags & BODY_DISABLE_DAMPING))
	{
		// apply damping to angular velocity
		float scale = 1.0f - (angVelDamp * time * dampingTimeScale);

		if(scale > 0)
			angularVelocity *= scale;
		else
			angularVelocity = FVector3D(0);
	}

	// move by computed linear velocity
	m_position += linearVelocity * m_linearFactor * time;

	// convert angular velocity to spinning
	// and accumulate
	Quaternion angVelSpinning = AngularVelocityToSpin(orientation, angularVelocity*m_angularFactor);

	// encountered
	ASSERT(angVelSpinning.isNan() == false);

	orientation += (angVelSpinning * time);
	orientation.fastNormalize();
	m_orientation = orientation;

	// store applied velocities and orientation
	m_linearVelocity = linearVelocity;
	m_angularVelocity = angularVelocity;

	// clear them
	m_totalTorque = Vector3D(0);
	m_totalForce = Vector3D(0);

	m_flags |= COLLOBJ_TRANSFORM_DIRTY;

	UpdateInertiaTensor();
}

void CEqRigidBody::UpdateInertiaTensor()
{
	const Vector3D inertia = m_invInertia;

	//rotateVector(m_centerOfMass, m_orientation);

	Matrix3x3 orientMat(m_orientation);
	m_centerOfMassTrans = rotateVector(m_centerOfMass, m_orientation);// orientMat*Vector3D(m_centerOfMass);
	m_invInertiaTensor = orientMat*scale3(inertia.x, inertia.y, inertia.z)*transpose(orientMat);
}

void CEqRigidBody::SetCenterOfMass(const FVector3D& center)
{
	m_centerOfMass = center;
	UpdateInertiaTensor();
}

const FVector3D& CEqRigidBody::GetCenterOfMass() const
{
	return m_centerOfMass;
}

//--------------------

// applies local impulse
void CEqRigidBody::ApplyImpulse(const FVector3D& rel_pos, const Vector3D& impulse)
{
	/*
	Vector3D linearVelocity = impulse * m_invMass;
	Vector3D angularVelocity = m_invInertiaTensor*cross(Vector3D(rel_pos + m_centerOfMassTrans), impulse);
	Quaternion orientation = m_orientation;

	// move by computed linear velocity
	m_position += linearVelocity * m_linearFactor;

	// convert angular velocity to spinning
	// and accumulate
	Quaternion angVelSpinning = AngularVelocityToSpin(m_orientation, angularVelocity*m_angularFactor);

	// encountered
	ASSERT(angVelSpinning.isNan() == false);

	m_orientation += angVelSpinning;
	*/
	m_linearVelocity += impulse * m_invMass;
	m_angularVelocity += m_invInertiaTensor*cross(Vector3D(rel_pos + m_centerOfMassTrans), impulse);
}

// applies world impulse
void CEqRigidBody::ApplyWorldImpulse(const FVector3D& position, const Vector3D& impulse)
{
	m_linearVelocity += impulse * m_invMass;
	m_angularVelocity += m_invInertiaTensor * cross(Vector3D((m_position - position) + m_centerOfMassTrans), impulse);
}

// applies local force
void CEqRigidBody::ApplyForce(const FVector3D& rel_pos, const Vector3D& force)
{
	m_totalForce += force;
	Vector3D torqueAdd = cross(Vector3D(rel_pos+m_centerOfMassTrans), force);
	m_totalTorque += torqueAdd;
}

// applies world impulse
void CEqRigidBody::ApplyWorldForce(const FVector3D& position, const Vector3D& force)
{
	m_totalForce += force;
	Vector3D torqueAdd = cross(Vector3D((m_position - position) + m_centerOfMassTrans), force);
	m_totalTorque += torqueAdd;
}

void CEqRigidBody::ApplyAngularImpulse(const Vector3D& impulse)
{
	m_angularVelocity += m_invInertiaTensor*impulse;
}

void CEqRigidBody::ApplyAngularImpulseAt(const FVector3D& rel_pos, const Vector3D& impulse)
{
	m_angularVelocity += m_invInertiaTensor*cross(Vector3D(rel_pos), impulse);
}

void CEqRigidBody::ApplyLinearImpulse(const Vector3D& impulse)
{
	m_linearVelocity += impulse*m_invMass;
}

void CEqRigidBody::ApplyAngularForce(const Vector3D& force)
{
	m_totalTorque += force;
}

void CEqRigidBody::ApplyAngularForceAt(const FVector3D& rel_pos, const Vector3D& force)
{
	m_totalTorque += cross(Vector3D(rel_pos), force);
}

void CEqRigidBody::ApplyLinearForce(const Vector3D& force)
{
	m_totalForce += force;
}

//--------------------

void CEqRigidBody::SetPosition(const FVector3D& position)
{
	// explicitly reset previous position
	m_position = m_prevPosition = position;
	m_flags |= COLLOBJ_TRANSFORM_DIRTY;

	UpdateBoundingBoxTransform();
}

void CEqRigidBody::SetOrientation(const Quaternion& orient)
{
	m_orientation = m_prevOrientation = orient;
	m_flags |= COLLOBJ_TRANSFORM_DIRTY;

	UpdateBoundingBoxTransform();
}

//--------------------

const Vector3D& CEqRigidBody::GetLinearVelocity() const
{
	return m_linearVelocity;
}

const Vector3D& CEqRigidBody::GetAngularVelocity() const
{
	return m_angularVelocity;
}

//--------------------

Vector3D CEqRigidBody::GetVelocityAtLocalPoint(const FVector3D& point) const
{
	// THIS IS WRONG
	return m_linearVelocity + cross(m_angularVelocity, Vector3D(point));
}

Vector3D CEqRigidBody::GetVelocityAtWorldPoint(const FVector3D& point) const
{
	// THIS IS WRONG
	return m_linearVelocity + cross(m_angularVelocity, Vector3D(m_position-point));
}

float CEqRigidBody::ComputeImpulseDenominator(const FVector3D& pos, const Vector3D& normal) const
{
	Vector3D r0 = pos+m_centerOfMassTrans;
	Vector3D c0 = cross(r0, normal);

	Vector3D vec = cross(m_invInertiaTensor*c0, r0);

	return m_invMass + dot(normal, vec);
}

//--------------------

void CEqRigidBody::SetLinearVelocity(const Vector3D& velocity)
{
	// set linear momentum, BUT multiplied by it's mass
	m_linearVelocity = velocity;
}

void CEqRigidBody::SetAngularVelocity(const Vector3D& velocity)
{
	m_angularVelocity = velocity;
}

void CEqRigidBody::SetLinearFactor(const Vector3D& fac)
{
	m_linearFactor = fac;
}

void CEqRigidBody::SetAngularFactor(const Vector3D& fac)
{
	m_angularFactor = fac;
}


//------------------------------

float CEqRigidBody::GetGravity() const
{
	return m_gravity;
}

void CEqRigidBody::SetGravity(float value)
{
	m_gravity = value;
}

const Matrix3x3& CEqRigidBody::GetWorldInvInertiaTensor() const
{
	return m_invInertiaTensor;
}

//--------------------------------------------------------------------------------------------------------------------

//
// STATIC
//
Vector3D CEqRigidBody::ComputeFrictionVelocity(	const Vector3D& collNormal,
												const Vector3D& collPointVelocity,
												float normalImpulse, float denominator,
												float staticFriction, float dynamicFriction)
{
	Vector3D tangent_vel = collPointVelocity - dot(collPointVelocity, collNormal)  * collNormal;

	float tangent_speed = length(tangent_vel);

	if (tangent_speed > 0.0f)
	{
		Vector3D T = -tangent_vel / tangent_speed;

		float numerator = tangent_speed;

		if(denominator > 0.0f)
		{
			float impulseToReverse = numerator / denominator;

			float impulseFromNormal = staticFriction * normalImpulse;
			float frictionImpulse;

			if (impulseToReverse < impulseFromNormal)
				frictionImpulse = impulseToReverse;
			else
				frictionImpulse = dynamicFriction * normalImpulse;

			return T*frictionImpulse;
		}
	}

	return vec3_zero;
}

//
// STATIC
//
Vector3D CEqRigidBody::ComputeFrictionVelocity2(const Vector3D& collNormal,
												const Vector3D& collPointVelocityA, const Vector3D& collPointVelocityB,
												float normalImpulse, float denominator,
												float staticFriction, float dynamicFriction)
{
	Vector3D subVelocities = collPointVelocityA-collPointVelocityB;

	Vector3D tangent_vel = subVelocities - dot(subVelocities, collNormal) * collNormal;

	float tangent_speed = length(tangent_vel);

	if (tangent_speed > 0.0f)
	{
		Vector3D T = -tangent_vel / tangent_speed;

		float numerator = tangent_speed;

		if(denominator > 0.0f)
		{
			float impulseToReverse = numerator / denominator;

			float impulseFromNormal = staticFriction * normalImpulse;
			float frictionImpulse;

			if (impulseToReverse < impulseFromNormal)
				frictionImpulse = impulseToReverse;
			else
				frictionImpulse = dynamicFriction * normalImpulse;

			return T*frictionImpulse;
		}
	}

	return vec3_zero;
}

Vector3D GetVelocityAtLocalPointWrap(CEqCollisionObject* obj, const FVector3D& relativePos)
{
	if (!obj->IsDynamic())
		return vec3_zero;

	return ((CEqRigidBody*)obj)->GetVelocityAtLocalPoint(relativePos);
}

float ComputeImpulseDenominatorWrap(CEqCollisionObject* obj, const FVector3D& relativePos, const Vector3D& normal)
{
	if (!obj->IsDynamic())
		return 0.0f;

	return ((CEqRigidBody*)obj)->ComputeImpulseDenominator(relativePos, normal);
}

float CEqRigidBody::ApplyImpulseResponseTo(ContactPair_t& pair, float error_correction_factor)
{
	FVector3D contactPoint = pair.position;
	Vector3D contactNormal = pair.normal;
	int pairFlag = pair.flags;

	CEqCollisionObject* bodyA = pair.bodyA;
	CEqRigidBody* bodyB = (CEqRigidBody*)pair.bodyB;

	FVector3D contactRelativePosA(0);
	FVector3D contactRelativePosB;
	Vector3D contactVelocity;
	float denominator = 0.0f;

	Vector3D relVelA;
	Vector3D relVelB;
	bool bodyADynamic = bodyA->IsDynamic();
	
	// body B
	{
		contactRelativePosB = bodyB->GetPosition()-contactPoint;
		contactVelocity = relVelB = bodyB->GetVelocityAtLocalPoint(contactRelativePosB);
	}

	// body A
	if (bodyADynamic)
	{
		contactRelativePosA = bodyA->GetPosition()-contactPoint;
		relVelA = ((CEqRigidBody*)bodyA)->GetVelocityAtLocalPoint(contactRelativePosA);
		contactVelocity -= relVelA;
	}

	bool forceFrozenA = (bodyA->m_flags & BODY_FORCE_FREEZE);
	bool forceFrozenB = (bodyB->m_flags & BODY_FORCE_FREEZE);

	// check velocity from opposite object to add denominator
	// if object is frozen
	if(bodyADynamic && (!forceFrozenA || forceFrozenA && lengthSqr(relVelB) > 3.0f)) // TODO: unfreeze activation variable
		denominator += ((CEqRigidBody*)bodyA)->ComputeImpulseDenominator(contactRelativePosA, contactNormal);

	if(!forceFrozenB || forceFrozenB && lengthSqr(relVelA) > 3.0f) // TODO: unfreeze activation variable
		denominator += bodyB->ComputeImpulseDenominator(contactRelativePosB, contactNormal);

	if (denominator < 0.0000001)
		return 0.0f;

	const float combined_rest = 1.0f + (pair.restitutionA + bodyB->GetRestitution());
	const float combined_friction = (bodyA->GetFriction()+bodyB->GetFriction())*0.5f;

	const float impulse_speed = dot(contactVelocity, contactNormal);
	const float jacDiagABInv = 1.0f / denominator;

	const float penetrationImpulse = error_correction_factor * jacDiagABInv;
	const float velocityImpulse = impulse_speed * jacDiagABInv;

	float velocityImpulseRest = impulse_speed * combined_rest * jacDiagABInv;

	float normalImpulse = penetrationImpulse+velocityImpulse;
	normalImpulse =  max(0.0f, normalImpulse); //(0.0f > normalImpulse) ? 0.0f: normalImpulse;

	float normalImpulseRest = penetrationImpulse+velocityImpulseRest;
	normalImpulseRest = max(0.0f, normalImpulseRest); // (0.0f > normalImpulseRest) ? 0.0f : normalImpulseRest;

	// apply impact based on point velocity
	Vector3D impulseVector = contactNormal*normalImpulseRest;

	if(ph_showcollisionresponses.GetBool())
	{
		debugoverlay->Line3D(contactPoint, contactPoint+impulseVector*COLLRESPONSE_DEBUG_SCALE, ColorRGBA(1,0,0,1), ColorRGBA(1,1,0,1), 3.0f);
		debugoverlay->Line3D(contactPoint, contactPoint-impulseVector*COLLRESPONSE_DEBUG_SCALE, ColorRGBA(1,0,0,1), ColorRGBA(1,1,0,1), 3.0f);

		debugoverlay->Box3D(contactPoint-0.01f, contactPoint+0.01f, ColorRGBA(1,1,0,1), 3.0f);
	}


	Vector3D frictionImpulse = ComputeFrictionVelocity(contactNormal, 
		contactVelocity,
		normalImpulse, 
		denominator, 
		combined_friction, 
		bodyB->GetFriction());

	// apply now
	if( bodyADynamic && 
		!(pairFlag & COLLPAIRFLAG_OBJECTA_NO_RESPONSE) && 
		!(bodyA->m_flags & BODY_INFINITEMASS) && 
		!(bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE) &&
		!(bodyA->m_flags & BODY_FORCE_FREEZE))
	{
		((CEqRigidBody*)bodyA)->ApplyImpulse(contactRelativePosA, impulseVector - frictionImpulse);
		((CEqRigidBody*)bodyA)->TryWake();
	}

	if( !(pairFlag & COLLPAIRFLAG_OBJECTB_NO_RESPONSE) && 
		!(bodyB->m_flags & BODY_INFINITEMASS) && 
		!(bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE) &&
		!(bodyB->m_flags & BODY_FORCE_FREEZE))
	{
		bodyB->ApplyImpulse(contactRelativePosB, -impulseVector + frictionImpulse);
		bodyB->TryWake();
	}

	return normalImpulse;
}

//
// STATIC
//
float CEqRigidBody::ApplyImpulseResponseTo(CEqRigidBody* body, const FVector3D& point, const Vector3D& normal, float posError, float restitutionA, float frictionA, float percentage)
{
	if(!body)
		return 0.0f;

	// relative hit position to compute impact point velocity
	const FVector3D contactRelativePos = body->GetPosition()-point;

	const Vector3D impactpoint_velocity = body->GetVelocityAtLocalPoint(contactRelativePos);

	const float combined_rest = 1.0 + (restitutionA + body->GetRestitution());

	const float impulse_speed = -dot(impactpoint_velocity, normal);

	const float denom = body->ComputeImpulseDenominator(contactRelativePos, normal);

	const float jacDiagABInv = 1.0f / denom;

	const float penetrationImpulse = posError * jacDiagABInv;
	const float velocityImpulse = impulse_speed * jacDiagABInv;
	const float velocityImpulseRest = impulse_speed * combined_rest * jacDiagABInv;

    float normalImpulseRest = penetrationImpulse + velocityImpulseRest;
	normalImpulseRest = (0.0f > normalImpulseRest) ? 0.0f : normalImpulseRest;

	// compute non-restitution value for friction
    float normalImpulse = penetrationImpulse + velocityImpulse;
	normalImpulse = (0.0f > normalImpulse) ? 0.0f : normalImpulse;

	// apply impact based on point velocity
	Vector3D impactVel = normal*normalImpulseRest;

	if(ph_showcollisionresponses.GetBool())
		debugoverlay->Line3D(point, point+impactVel*COLLRESPONSE_DEBUG_SCALE, ColorRGBA(1,0,0,1), ColorRGBA(1,1,0,1), 3.0f);

	float combined_friction = (frictionA + body->GetFriction())*0.5f;

	Vector3D frictionImpulse = ComputeFrictionVelocity(normal, impactpoint_velocity, normalImpulse, denom, combined_friction, body->GetFriction());

	// apply friction of impact
	impactVel += frictionImpulse;

	// apply now
	body->ApplyImpulse(contactRelativePos, impactVel*percentage);
	
	// static collisions should not wake the body

	return normalImpulse*percentage;
}

//
// STATIC
//
float CEqRigidBody::ApplyImpulseResponseTo2( CEqRigidBody* bodyA, CEqRigidBody* bodyB, const FVector3D& point, const Vector3D& normal, float posError, int pairFlag)
{
	if(!bodyA || !bodyB)
		return 0.0f;

	// relative hit position to compute impact point velocity
	const FVector3D contactRelativePosA = bodyA->GetPosition()-point;
	const FVector3D contactRelativePosB = bodyB->GetPosition()-point;

	const Vector3D impactpoint_velocityA = bodyA->GetVelocityAtLocalPoint(contactRelativePosA);
	const Vector3D impactpoint_velocityB = bodyB->GetVelocityAtLocalPoint(contactRelativePosB);

	const float combined_rest = 1.0f + (bodyA->GetRestitution() + bodyB->GetRestitution());

	const Vector3D vel_sub = impactpoint_velocityB-impactpoint_velocityA;

	const float impulse_speed = dot(vel_sub, normal);

	const float denomA = bodyA->ComputeImpulseDenominator(contactRelativePosA, normal);
	const float denomB = bodyB->ComputeImpulseDenominator(contactRelativePosB, normal);

	const float jacDiagABInv = 1.0f / (denomA+denomB);

	const float penetrationImpulse = posError * jacDiagABInv;
	const float velocityImpulse = impulse_speed * jacDiagABInv;

	float velocityImpulseRest = impulse_speed * combined_rest * jacDiagABInv;

	Vector3D impDir = normal;

	Vector3D impactDirA = impDir;
	Vector3D impactDirB = -impDir;

	impactDirA = fastNormalize(impactDirA);
	impactDirB = fastNormalize(impactDirB);

    float normalImpulse = penetrationImpulse+velocityImpulse;
	normalImpulse = (0.0f > normalImpulse) ? 0.0f: normalImpulse;

    float normalImpulseRest = penetrationImpulse+velocityImpulseRest;
	normalImpulseRest = (0.0f > normalImpulseRest) ? 0.0f: normalImpulseRest;

	// apply impact based on point velocity
	Vector3D impactVelA = impactDirA*normalImpulseRest;
	Vector3D impactVelB = impactDirB*normalImpulseRest;

	if(ph_showcollisionresponses.GetBool())
	{
		debugoverlay->Line3D(point, point+impactVelA*COLLRESPONSE_DEBUG_SCALE, ColorRGBA(1,0,0,1), ColorRGBA(1,1,0,1), 3.0f);
		debugoverlay->Line3D(point, point+impactVelB*COLLRESPONSE_DEBUG_SCALE, ColorRGBA(1,0,0,1), ColorRGBA(1,1,0,1), 3.0f);

		debugoverlay->Box3D(point-0.01f, point+0.01f, ColorRGBA(1,1,0,1), 3.0f);
	}

	float combined_friction = (bodyA->GetFriction()+bodyB->GetFriction())*0.5f;

	Vector3D frictionImpulse = ComputeFrictionVelocity(normal, impactpoint_velocityA-impactpoint_velocityB, normalImpulse, denomA+denomB, combined_friction, combined_friction);

	impactVelA += frictionImpulse;
	impactVelB -= frictionImpulse;

	bodyA->TryWake();
	bodyB->TryWake();

	// apply now
	if( !(pairFlag & COLLPAIRFLAG_OBJECTA_NO_RESPONSE) && 
		!(bodyA->m_flags & BODY_INFINITEMASS) && 
		!(bodyB->m_flags & COLLOBJ_DISABLE_RESPONSE))
	{
		bodyA->ApplyImpulse(contactRelativePosA, impactVelA);
		bodyA->TryWake();
	}

	if( !(pairFlag & COLLPAIRFLAG_OBJECTB_NO_RESPONSE) && 
		!(bodyB->m_flags & BODY_INFINITEMASS) && 
		!(bodyA->m_flags & COLLOBJ_DISABLE_RESPONSE))
	{
		bodyB->ApplyImpulse(contactRelativePosB, impactVelB);
		bodyB->TryWake();
	}

	return normalImpulse;
}
