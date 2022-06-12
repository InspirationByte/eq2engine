//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects public header
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// flags
enum PhysObjectFlags_e
{
	PO_NO_EVENTS		= (1 << 0), // don't generate events for specified object
	PO_BLOCKEVENTS		= (1 << 1), // block events for object (dynamic flag)
	PO_NO_EVENT_BLOCK	= (1 << 2),	// disables distant locking of object
};

enum phys_freeze_state_e
{
	PS_INVALID = -1,
	PS_FROZEN = 0,		// object is not moveable
	PS_ACTIVE,			// object is active and moveable
	PS_INACTIVE			// object is inactive, but any collision activates it
};

struct phySurfaceMaterial_t;
struct dkCollideData_t;
struct pritimiveinfo_t;

class IPhysicsObject;

struct physicsContactEvent_t
{
	IPhysicsObject* pHitB;

	float			fImpulse;
	float			fImpulseLateral_1;
	float			fImpulseLateral_2;

	Vector3D		vWorldHitOriginA;
	Vector3D		vWorldHitOriginB;

	Vector3D		vWorldHitNormal;

	float			fCombinedFriction;
	float			fCombinedRest;

	float			fDistance;
};

class IPhysicsObject
{
public:

	//-----------------------------
	// Object properties
	//-----------------------------

	virtual void					SetCollisionMask(uint mask) = 0;				// Sets collides with group flags
	virtual uint					GetCollisionMask() = 0;							// Returns collides with group flags

	virtual void					SetContents(uint contents) = 0;					// Sets collision group
	virtual uint					GetContents() = 0;								// Returns collision group

	virtual void					SetEntityObjectPtr(void* ptr) = 0;				// Sets entity pointer
	virtual void*					GetEntityObjectPtr() = 0;						// Returns entity associated with this physics object

	virtual void					SetUserData(void* ptr) = 0;						// Sets user data pointer
	virtual void*					GetUserData() = 0;								// Returns user data

	virtual void					SetActivationState(phys_freeze_state_e nState) = 0;	// freeze states
	virtual phys_freeze_state_e		GetActivationState() = 0;							// freeze states

	virtual void					SetAngularFactor(const Vector3D &factor) = 0;			// Sets angular factor (may help with up-right vector)
	virtual Vector3D				GetAngularFactor() = 0;							// Sets angular factor (may help with up-right vector)

	virtual void					SetLinearFactor(const Vector3D &factor) = 0;			// Sets linear factor
	virtual Vector3D				GetLinearFactor() = 0;							// Gets linear factor

	virtual void					SetRestitution(float rest) = 0;					// Sets restitution
	virtual float					GetRestitution() = 0;							// Gets restitution

	virtual void					SetDamping(float linear, float angular) = 0;	// Sets damping
	virtual float					GetDampingLinear() = 0;							// Gets linear damping
	virtual float					GetDampingAngular() = 0;						// Gets angular damping

	virtual void					SetFriction(float fric) = 0;					// Sets friction
	virtual float					GetFriction() = 0;								// Gets friction

	virtual void					SetSleepTheresholds(float lin, float ang) = 0;	// Sleep thereshold

	virtual void					AddFlags(int nFlags) = 0;						// add object flags
	virtual void					RemoveFlags(int nFlags) = 0;					// remove object flags
	virtual int						GetFlags() = 0;

	virtual float					GetInvMass() = 0;								// returns 1.0 / object mass
	virtual void					SetMass(float fMass) = 0;						// sets object mass

	virtual const char*				GetName() = 0;									// Get object name
	virtual phySurfaceMaterial_t*	GetMaterial() = 0;								// object material parameters

	virtual int						GetContactEventCount() = 0;						// returns contact event count
	virtual physicsContactEvent_t*	GetContactEvent(int idx) = 0;					// returns contact event by index

	virtual void					WakeUp() = 0;									// wakes object if status is PS_INACTIVE
	//virtual void					Freeze() = 0;									// freezes object if status is PS_ACTIVE

	virtual void					SetCollisionResponseEnabled(bool bEnabled) = 0;	// enable/disable response of collisions (enabled by default)

	virtual bool					IsStatic() = 0;									// returns object static state
	virtual bool					IsDynamic() = 0;								// returns object dynamic state

	//-----------------------------
	// Object physics actions
	//-----------------------------

	// Adds torque and force
	virtual void					ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos) = 0;
	virtual void					AddLocalTorqueImpulse(const Vector3D &torque) = 0;
	virtual void					AddForce(const Vector3D &force) = 0;
	virtual void					AddLocalForce(const Vector3D &force) = 0;
	virtual void					AddForceAtPosition(const Vector3D &force,const Vector3D &pos) = 0;
	virtual void					AddForceAtLocalPosition(const Vector3D &force,const Vector3D &pos) = 0;
	virtual void					AddLocalForceAtPosition(const Vector3D &force,const Vector3D &pos) = 0;
	virtual void					AddLocalForceAtLocalPosition(const Vector3D &force,const Vector3D &pos) = 0;

	// Sets position of object and other
	virtual void					SetPosition(const Vector3D &pos) = 0;
	virtual void					SetAngles(const Vector3D &ang) = 0;
	virtual void					SetVelocity(const Vector3D &linear) = 0;
	virtual void					SetAngularVelocity(const Vector3D &vAxis,float velocity) = 0;

	// returns mins and maxs of transformed aabb
	virtual void					GetAABB(Vector3D &mins, Vector3D &maxs) = 0;

	// Get transformation matrix for renderer purposes
	virtual Matrix4x4				GetTransformMatrix() = 0;

	// Get transformation matrix for renderer purposes
	virtual Matrix4x4				GetOrientationMatrix() = 0;

	// set transformation of object
	virtual void					SetTransformFromMatrix(const Matrix4x4 &matrix) = 0;

	// Get position, angles and other stuff
	virtual Vector3D				GetPosition() = 0;
	virtual Vector3D				GetAngles() = 0;
	virtual Vector3D				GetVelocity() = 0;
	virtual Vector3D				GetAngularVelocity() = 0;

	virtual Vector3D				GetVelocityAtPoint(const Vector3D &point) = 0;

	// returns child shapes count
	virtual int						GetChildShapeCount() = 0;

	// setup child transform
	virtual void					SetChildShapeTransform(int shapeNum, const Vector3D &localOrigin, const Vector3D &localAngles) = 0;

// SAVE - RESTORE functions

	virtual void					AddObjectToIgnore(IPhysicsObject* pObject) = 0;
	virtual void					RemoveIgnoredObject(IPhysicsObject* pObject) = 0;

	virtual bool					ShouldCollideWith(IPhysicsObject* pObject) = 0;
};
