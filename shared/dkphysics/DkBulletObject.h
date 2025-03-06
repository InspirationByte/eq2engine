//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "dkphysics/IPhysicsObject.h"

#define MAX_CONTACT_EVENTS 4 // per single object

class IMaterial;

class CEqRigidBody : public btRigidBody
{
public:
	///btRigidBody constructor using construction info
	CEqRigidBody(	const btRigidBodyConstructionInfo& constructionInfo) : btRigidBody(constructionInfo)
	{
		m_checkCollideWith = 1;
	}

	///btRigidBody constructor for backwards compatibility. 
	///To specify friction (etc) during rigid body construction, please use the other constructor (using btRigidBodyConstructionInfo)
	CEqRigidBody(	btScalar mass, btMotionState* motionState, btCollisionShape* collisionShape, const btVector3& localInertia=btVector3(0,0,0)) 
		: btRigidBody( mass, motionState, collisionShape, localInertia)
	{
		m_checkCollideWith = 1;
	}

	inline bool checkCollideWithOverride(const  btCollisionObject* co) const
	{
		IPhysicsObject* pObject1 = (IPhysicsObject*)getUserPointer();
		IPhysicsObject* pObject2 = (IPhysicsObject*)co->getUserPointer();

		if(pObject1 && pObject2)
		{
			if(!pObject1->ShouldCollideWith(pObject2) || !pObject2->ShouldCollideWith(pObject1))
				return false;
		}

		return btRigidBody::checkCollideWithOverride(co);
	}
};

class CPhysicsObject : public IPhysicsObject
{
public:
	// Make it friend because we accessing the private members
	friend class DkPhysics;
	friend class DkPhysicsRope;

	CPhysicsObject();
	~CPhysicsObject();

	//-----------------------------
	// Object properties
	//-----------------------------

	void					SetCollisionMask(uint mask);				// Sets collides with group flags
	uint					GetCollisionMask();							// Returns collides with group flags

	void					SetContents(uint contents);					// Sets collision group
	uint					GetContents();								// Returns collision group

	void					SetEntityObjectPtr(void* ptr);				// Sets entity pointer
	void*					GetEntityObjectPtr();						// Returns entity associated with this physics object

	void					SetUserData(void* ptr);						// Sets user data pointer
	void*					GetUserData();								// Returns user data

	void					SetActivationState(phys_freeze_state_e nState);	// freeze states
	phys_freeze_state_e		GetActivationState();							// freeze states

	void					SetAngularFactor(const Vector3D &factor);			// Sets angular factor (may help with up-right vector)
	Vector3D				GetAngularFactor();							// Sets angular factor (may help with up-right vector)

	void					SetLinearFactor(const Vector3D &factor);			// Sets linear factor
	Vector3D				GetLinearFactor();							// Gets linear factor

	void					SetRestitution(float rest);					// Sets restitution
	float					GetRestitution();							// Gets restitution

	void					SetDamping(float linear, float angular);	// Sets damping
	float					GetDampingLinear();							// Gets linear damping
	float					GetDampingAngular();						// Gets angular damping

	void					SetFriction(float fric);					// Sets friction
	float					GetFriction();								// Gets friction

	void					SetSleepTheresholds(float lin, float ang);	// Sleep thereshold

	void					AddFlags(int nFlags);						// add object flags
	void					RemoveFlags(int nFlags);					// remove object flags
	int						GetFlags();

	float					GetInvMass();								// returns 1.0 / object mass
	void					SetMass(float fMass);						// sets object mass

	void					SetCollisionResponseEnabled(bool bEnabled);	// enable/disable response of collisions (enabled by default)

	const char*				GetName();									// Get object name
	phySurfaceMaterial_t*	GetMaterial();								// object material parameters

	// internal use, adds contact event
	void					AddContactEventFromManifoldPoint(btManifoldPoint* pt, CPhysicsObject* hitB);

	// Contacts
	void					ClearContactEvents();
	int						GetContactEventCount();
	physicsContactEvent_t*	GetContactEvent(int idx);

	bool					IsStatic();									// returns object static state
	bool					IsDynamic();								// returns object dynamic state

	//-----------------------------
	// Object physics actions
	//-----------------------------

	// Adds torque and force
	void					ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos);
	void					AddLocalTorqueImpulse(const Vector3D &torque);
	void					AddForce(const Vector3D &force);
	void					AddLocalForce(const Vector3D &force);
	void					AddForceAtPosition(const Vector3D &force,const Vector3D &pos);
	void					AddForceAtLocalPosition(const Vector3D &force,const Vector3D &pos);
	void					AddLocalForceAtPosition(const Vector3D &force,const Vector3D &pos);
	void					AddLocalForceAtLocalPosition(const Vector3D &force,const Vector3D &pos);

	// Sets position of object and other
	void					SetPosition(const Vector3D &pos);
	void					SetAngles(const Vector3D &ang);
	void					SetVelocity(const Vector3D &linear);
	void					SetAngularVelocity(const Vector3D &vAxis,float velocity);


	void					GetBoundingBox(Vector3D &mins, Vector3D &maxs);

	// Get transformation matrix for renderer purposes
	Matrix4x4				GetTransformMatrix();

	// Get transformation matrix for renderer purposes
	Matrix4x4				GetOrientationMatrix();

	// set transformation of object
	void					SetTransformFromMatrix(const Matrix4x4 &matrix);

	// Get position, angles and other stuff
	Vector3D				GetPosition();
	Vector3D				GetAngles();
	Vector3D				GetVelocity();
	Vector3D				GetAngularVelocity();

	Vector3D				GetVelocityAtPoint(const Vector3D &point);

	// returns child shapes count
	int						GetChildShapeCount();

	// setup child transform
	void					SetChildShapeTransform(int shapeNum, const Vector3D &localOrigin, const Vector3D &localAngles);

	void					WakeUp();			// wakes object if status is PS_INACTIVE
	//void					Freeze();			// freezes object if status is PS_ACTIVE

// SAVE - RESTORE functions

	void					AddObjectToIgnore(IPhysicsObject* pObject);
	void					RemoveIgnoredObject(IPhysicsObject* pObject);

	bool					ShouldCollideWith(IPhysicsObject* pObject);

private:

	btRigidBody*			m_pPhyObjectPointer;

	Array<IPhysicsObject*>	m_IgnoreCollisionList{ PP_SL };

	phySurfaceMaterial_t*	m_pPhysMaterial;

	uint					m_nCollisionGroup; // collision group
	uint					m_nCollidesWith;
	void*					m_pParentObject; // entity specifier
	void*					m_pUserData; // user data

	EqString				m_name;

	float					m_fMass;

	physicsContactEvent_t	m_ContactEvents[MAX_CONTACT_EVENTS];
	int						m_numEvents;

	IMaterial*				m_pRMaterial;
public:
	int						m_nFlags;
};
