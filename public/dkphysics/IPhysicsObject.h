//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects public header
//////////////////////////////////////////////////////////////////////////////////

#pragma once

// flags
enum EPhysObjectFlags : int
{
	PO_NO_EVENTS		= (1 << 0), // don't generate events for specified object
	PO_BLOCKEVENTS		= (1 << 1), // block events for object (dynamic flag)
	PO_NO_EVENT_BLOCK	= (1 << 2),	// disables distant locking of object
};

enum EPhysActivationState : int
{
	PS_INVALID = -1,
	PS_FROZEN = 0,		// object is not moveable
	PS_ACTIVE,			// object is active and moveable
	PS_INACTIVE			// object is inactive, but any collision activates it
};

struct physSurfaceInfo_t;
struct physShapeInfo_t;
struct physPrimitiveInfo_t;

class IPhysicsObject;

struct physContactEvt_t
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
	virtual ~IPhysicsObject() = default;
	
	virtual void					SetCollisionMask(uint mask) = 0;
	virtual uint					GetCollisionMask() const = 0;

	virtual void					SetContents(uint contents) = 0;
	virtual uint					GetContents() const = 0;

	virtual void					SetEntityObjectPtr(void* ptr) = 0;
	virtual void*					GetEntityObjectPtr() const = 0;	

	virtual void					SetUserData(void* ptr) = 0;
	virtual void*					GetUserData() const = 0;

	virtual void					SetActivationState(EPhysActivationState nState) = 0;
	virtual EPhysActivationState	GetActivationState() const = 0;

	virtual void					SetAngularFactor(const Vector3D &factor) = 0;
	virtual Vector3D				GetAngularFactor() const = 0;

	virtual void					SetLinearFactor(const Vector3D &factor) = 0;
	virtual Vector3D				GetLinearFactor() const = 0;

	virtual void					SetRestitution(float rest) = 0;
	virtual float					GetRestitution() const = 0;

	virtual void					SetDamping(float linear, float angular) = 0;
	virtual float					GetDampingLinear() const = 0;
	virtual float					GetDampingAngular() const = 0;

	virtual void					SetFriction(float fric) = 0;
	virtual float					GetFriction() const = 0;

	virtual void					AddFlags(int nFlags) = 0;
	virtual void					RemoveFlags(int nFlags) = 0;
	virtual int						GetFlags() const = 0;

	virtual float					GetInvMass() const = 0;
	virtual float					GetMass() const = 0;
	virtual void					SetMass(float fMass) = 0;

	virtual const char*				GetName() const = 0;
	virtual const physSurfaceInfo_t*	GetMaterial() const = 0;

	virtual int						GetContactEventCount() const = 0;
	virtual physContactEvt_t*		GetContactEvent(int idx) = 0;

	virtual void					WakeUp() = 0;
	//virtual void					Freeze() = 0;

	virtual void					SetCollisionResponseEnabled(bool bEnabled) = 0;

	virtual bool					IsStatic() const = 0;
	virtual bool					IsDynamic() const = 0;

	virtual void					ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos) = 0;
	virtual void					AddTorque(const Vector3D &torque) = 0;
	virtual void					AddForce(const Vector3D &force) = 0;
	virtual void					AddForceAtPosition(const Vector3D &force,const Vector3D &pos) = 0;

	virtual void					SetPosition(const Vector3D &pos) = 0;
	virtual void					SetAngles(const Vector3D &ang) = 0;
	virtual void					SetVelocity(const Vector3D &linear) = 0;
	virtual void					SetAngularVelocity(const Vector3D &vAxis,float velocity) = 0;

	virtual void					GetBoundingBox(Vector3D &mins, Vector3D &maxs) const = 0;
	virtual Matrix4x4				GetTransformMatrix() const = 0;
	virtual void					SetTransformFromMatrix(const Matrix4x4 &matrix) = 0;

	virtual Vector3D				GetPosition() const = 0;
	virtual Vector3D				GetAngles() const = 0;
	virtual Vector3D				GetVelocity() const = 0;
	virtual Vector3D				GetAngularVelocity() const = 0;

	virtual Vector3D				GetVelocityAtPoint(const Vector3D &point) const = 0;

	virtual void					AddObjectToIgnore(IPhysicsObject* pObject) = 0;
	virtual void					RemoveIgnoredObject(IPhysicsObject* pObject) = 0;

	virtual bool					ShouldCollideWith(IPhysicsObject* pObject) const = 0;
};
