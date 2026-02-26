//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IPhysicsObject.h"

namespace JPH {
class PhysicsSystem;
class BodyInterface;
}

using PhysEventArray = FixedArray<physContactEvt_t, 4>;

class IMaterial;

class DkPhysicsObject : public IPhysicsObject
{
public:
	// Make it friend because we accessing the private members
	friend class DkPhysics;
	friend class DkPhysicsRope;

	~DkPhysicsObject() = default;
	DkPhysicsObject(JPH::PhysicsSystem& jphPhysSys, JPH::BodyID jphBodyId);

	void					SetCollisionMask(uint mask);
	uint					GetCollisionMask() const;

	void					SetContents(uint contents);
	uint					GetContents() const;

	void					SetEntityObjectPtr(void* ptr);
	void*					GetEntityObjectPtr() const;

	void					SetUserData(void* ptr);
	void*					GetUserData() const;

	void					SetActivationState(EPhysActivationState nState);
	EPhysActivationState	GetActivationState() const;

	void					SetAngularFactor(const Vector3D &factor);
	Vector3D				GetAngularFactor() const;

	void					SetLinearFactor(const Vector3D &factor);
	Vector3D				GetLinearFactor() const;

	void					SetRestitution(float rest);
	float					GetRestitution() const;

	void					SetDamping(float linear, float angular);
	float					GetDampingLinear() const;
	float					GetDampingAngular() const;

	void					SetFriction(float fric);
	float					GetFriction() const;

	void					AddFlags(int nFlags);
	void					RemoveFlags(int nFlags);
	int						GetFlags() const;

	float					GetInvMass() const;
	float					GetMass() const;
	void					SetMass(float fMass);

	void					SetCollisionResponseEnabled(bool bEnabled);

	const char*				GetName() const;
	const physSurfaceInfo_t*	GetMaterial() const { return m_physMaterial; }

	//void					AddContactEventFromManifoldPoint(btManifoldPoint* pt, DkPhysicsObject* hitB);

	void					ClearContactEvents();
	int						GetContactEventCount() const;
	physContactEvt_t*		GetContactEvent(int idx);

	bool					IsStatic() const;
	bool					IsDynamic() const;

	void					ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos);
	void					AddTorque(const Vector3D &torque);
	void					AddForce(const Vector3D &force);
	void					AddForceAtPosition(const Vector3D &force,const Vector3D &pos);

	void					SetPosition(const Vector3D &pos);
	void					SetAngles(const Vector3D &ang);
	void					SetVelocity(const Vector3D &linear);
	void					SetAngularVelocity(const Vector3D &vAxis,float velocity);

	void					GetBoundingBox(Vector3D &mins, Vector3D &maxs) const;
	Matrix4x4				GetTransformMatrix() const;
	void					SetTransformFromMatrix(const Matrix4x4 &matrix);

	Vector3D				GetPosition() const;
	Vector3D				GetAngles() const;
	Vector3D				GetVelocity() const;
	Vector3D				GetAngularVelocity() const;

	Vector3D				GetVelocityAtPoint(const Vector3D &point) const;

	void					WakeUp();
	//void					Freeze();

	void					AddObjectToIgnore(IPhysicsObject* pObject);
	void					RemoveIgnoredObject(IPhysicsObject* pObject);

	bool					ShouldCollideWith(IPhysicsObject* pObject) const;

private:

	JPH::BodyInterface&			GetJPHBodyIface() const;

	JPH::PhysicsSystem& 		m_jphPhysSys;
	JPH::BodyID					m_jphObjId;

	PhysEventArray				m_ContactEvents;
	Array<IPhysicsObject*>		m_IgnoreCollisionList{ PP_SL };
	EqString					m_name;

	const physSurfaceInfo_t*	m_physMaterial = nullptr;
	void*						m_pParentObject = nullptr;
	void*						m_pUserData = nullptr;
	IMaterial*					m_pRMaterial = nullptr;

	uint						m_nCollisionGroup = 0;
	uint						m_nCollidesWith = 0;
	int							m_nFlags = 0;
};
