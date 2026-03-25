//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2026
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"

#include "DkJoltPCH.h"
#include <Jolt/Physics/PhysicsSystem.h>

#include "DkPhysicsObject.h"

#include "render/IDebugOverlay.h"
#include "physics/BulletConvert.h"

DkPhysicsObject::DkPhysicsObject(JPH::PhysicsSystem& jphPhysSys, JPH::BodyID jphBodyId)
	: m_jphPhysSys(jphPhysSys)
	, m_jphObjId(jphBodyId)
{
}

JPH::BodyInterface&	DkPhysicsObject::GetJPHBodyIface() const
{
	return m_jphPhysSys.GetBodyInterface(); 
}

// Sets collides with group flags
void DkPhysicsObject::SetCollisionMask(uint group)
{
	m_nCollidesWith = group;
}

// Returns collides with group flags
uint DkPhysicsObject::GetCollisionMask() const
{
	return m_nCollidesWith;
}

// Sets collision group
void DkPhysicsObject::SetContents(uint group)
{
	m_nCollisionGroup = group;
}

// Returns collision group
uint DkPhysicsObject::GetContents() const
{
	return m_nCollisionGroup;
}

// Sets collision group
void DkPhysicsObject::SetEntityObjectPtr(void* ptr)
{
	m_pParentObject = ptr;
}

// Returns entity associated with this physics object
void* DkPhysicsObject::GetEntityObjectPtr() const
{
	return m_pParentObject;
}

// Sets user data pointer
void DkPhysicsObject::SetUserData(void* ptr)
{
	m_pUserData = ptr;
}

// Returns user data
void* DkPhysicsObject::GetUserData() const
{
	return m_pUserData;
}

// Various flags
void DkPhysicsObject::AddFlags(int nFlags)
{
	m_nFlags |= nFlags;
}

// Various flags
void DkPhysicsObject::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

int	DkPhysicsObject::GetFlags() const
{
	return m_nFlags;
}

void DkPhysicsObject::SetActivationState(EPhysActivationState nState)
{
	if(nState == PS_ACTIVE)
		GetJPHBodyIface().ActivateBody(m_jphObjId);
	else if(nState == PS_INACTIVE)
		GetJPHBodyIface().DeactivateBody(m_jphObjId);
	// TODO: freeze
}

EPhysActivationState DkPhysicsObject::GetActivationState() const
{
	return GetJPHBodyIface().IsActive(m_jphObjId) ? PS_ACTIVE : PS_INACTIVE;
}

void DkPhysicsObject::WakeUp()
{
	GetJPHBodyIface().ResetSleepTimer(m_jphObjId);
}

// Sets angular factor (may help with up-right vector)
void DkPhysicsObject::SetAngularFactor(const Vector3D &factor)
{
	ASSERT_FAIL("Unimplemented");
}

Vector3D DkPhysicsObject::GetAngularFactor() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

// Sets linear factor
void DkPhysicsObject::SetLinearFactor(const Vector3D &factor)
{
	ASSERT_FAIL("Unimplemented");
}

Vector3D DkPhysicsObject::GetLinearFactor() const
{
	ASSERT_FAIL("Unimplemented");
	return vec3_unit;
}

// Sets linear factor
void DkPhysicsObject::SetRestitution(float rest)
{
	GetJPHBodyIface().SetRestitution(m_jphObjId, rest);
}

// gets linear factor
float DkPhysicsObject::GetRestitution() const
{
	return GetJPHBodyIface().GetRestitution(m_jphObjId);
}

// Sets linear factor
void DkPhysicsObject::SetDamping(float linear, float angular)
{
	ASSERT_FAIL("Unimplemented");
}

// Sets linear factor
float DkPhysicsObject::GetDampingLinear() const
{
	ASSERT_FAIL("Unimplemented");
	return 0;
}

// Sets linear factor
void DkPhysicsObject::SetFriction(float fric)
{
	GetJPHBodyIface().SetFriction(m_jphObjId, fric);
}

// Sets linear factor
float DkPhysicsObject::GetFriction() const
{
	return GetJPHBodyIface().GetFriction(m_jphObjId);
}

// Sets linear factor
float DkPhysicsObject::GetDampingAngular() const
{
	ASSERT_FAIL("Unimplemented");
	return 0;
}

float DkPhysicsObject::GetInvMass() const
{
	JPH::BodyLockRead jphLock(m_jphPhysSys.GetBodyLockInterface(), m_jphObjId);
	if (!jphLock.Succeeded())
		return 0.0f;

	return jphLock.GetBody().GetMotionProperties()->GetInverseMass();
}

float DkPhysicsObject::GetMass() const
{
	const float invMass = GetInvMass();
	if (invMass <= 0.0f)
		return 0.0f;
	return 1.0f / invMass;
}

void DkPhysicsObject::SetMass(float fMass)
{
	JPH::BodyLockWrite jphLock(m_jphPhysSys.GetBodyLockInterface(), m_jphObjId);
	if (jphLock.Succeeded())
		return;

	return jphLock.GetBody().GetMotionProperties()->SetInverseMass(1.0f / fMass);
}

const char* DkPhysicsObject::GetName() const
{
	return m_name;
}

void DkPhysicsObject::SetCollisionResponseEnabled(bool bEnabled)
{
}

// returns object static state
bool DkPhysicsObject::IsStatic() const
{
	return GetJPHBodyIface().GetMotionType(m_jphObjId) == JPH::EMotionType::Static;
}

// returns object dynamic state
bool DkPhysicsObject::IsDynamic() const
{
	return GetJPHBodyIface().GetMotionType(m_jphObjId) == JPH::EMotionType::Dynamic;
}

void DkPhysicsObject::ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos)
{
	GetJPHBodyIface().AddImpulse(m_jphObjId, Convert::ToVec3(impulse), Convert::ToVec3(relativePos));
}

void DkPhysicsObject::AddTorque(const Vector3D &torque)
{
	GetJPHBodyIface().AddTorque(m_jphObjId, Convert::ToVec3(torque));
}

void DkPhysicsObject::AddForce(const Vector3D &force)
{
	GetJPHBodyIface().AddForce(m_jphObjId, Convert::ToVec3(force));
}

void DkPhysicsObject::AddForceAtPosition(const Vector3D &force,const Vector3D &position)
{
	GetJPHBodyIface().AddForce(m_jphObjId, Convert::ToVec3(force), Convert::ToVec3(position));
}

void DkPhysicsObject::SetPosition(const Vector3D &position)
{
	GetJPHBodyIface().SetPosition(m_jphObjId, Convert::ToVec3(position), JPH::EActivation::DontActivate);
}

void DkPhysicsObject::SetAngles(const Vector3D &ang)
{
	Quaternion quat = rotateXYZ(DEG2RAD(ang.x),DEG2RAD(ang.y),DEG2RAD(ang.z));
	GetJPHBodyIface().SetRotation(m_jphObjId, Convert::ToQuat(quat), JPH::EActivation::DontActivate);
}

void DkPhysicsObject::SetVelocity(const Vector3D &linear)
{
	GetJPHBodyIface().SetLinearVelocity(m_jphObjId, Convert::ToVec3(linear));
}

void DkPhysicsObject::SetAngularVelocity(const Vector3D &vAxis, float velocity)
{
	GetJPHBodyIface().SetAngularVelocity(m_jphObjId, Convert::ToVec3(vAxis * velocity));
}

void DkPhysicsObject::GetBoundingBox(Vector3D &mins, Vector3D &maxs) const
{
	JPH::BodyLockRead jphLock(m_jphPhysSys.GetBodyLockInterface(), m_jphObjId);
	if(jphLock.Succeeded())
		return;
	const JPH::AABox& jphBox = jphLock.GetBody().GetWorldSpaceBounds();
	
	mins = Convert::FromVec3(jphBox.mMin);
	maxs = Convert::FromVec3(jphBox.mMax);
}

Matrix4x4 DkPhysicsObject::GetTransformMatrix() const
{
	return Convert::FromMat44Transposed(GetJPHBodyIface().GetWorldTransform(m_jphObjId));
}

// set transformation of object
void DkPhysicsObject::SetTransformFromMatrix(const Matrix4x4 &matrix)
{
	Quaternion rot(matrix.getRotationComponent());
	renormalize(rot);
	GetJPHBodyIface().SetPositionAndRotation(m_jphObjId, Convert::ToVec3(matrix.getTranslationComponent()), Convert::ToQuat(rot), JPH::EActivation::DontActivate);
}

Vector3D DkPhysicsObject::GetAngles() const
{
	const JPH::Quat jphQuat = GetJPHBodyIface().GetRotation(m_jphObjId);
	return eulersXYZ(Convert::FromQuat(jphQuat)) * M_RAD2DEG;
}

Vector3D DkPhysicsObject::GetPosition() const
{
	return Convert::FromVec3(GetJPHBodyIface().GetPosition(m_jphObjId));
}

Vector3D DkPhysicsObject::GetVelocity() const
{
	return Convert::FromVec3(GetJPHBodyIface().GetLinearVelocity(m_jphObjId));
}

Vector3D DkPhysicsObject::GetAngularVelocity() const
{
	return Convert::FromVec3(GetJPHBodyIface().GetAngularVelocity(m_jphObjId));
}

Vector3D DkPhysicsObject::GetVelocityAtPoint(const Vector3D &point) const
{
	return Convert::FromVec3(GetJPHBodyIface().GetPointVelocity(m_jphObjId, Convert::ToVec3(point)));
}

//void DkPhysicsObject::AddContactEventFromManifoldPoint(btManifoldPoint* pt, DkPhysicsObject* hitB)
//{
//	// TODO: m_ContactEvents.append
//}

// Contacts
void DkPhysicsObject::ClearContactEvents()
{
	m_ContactEvents.clear();
}

int DkPhysicsObject::GetContactEventCount() const
{
	return m_ContactEvents.numElem();
}

physContactEvt_t* DkPhysicsObject::GetContactEvent(int idx)
{
	return &m_ContactEvents[idx];
}

void DkPhysicsObject::AddObjectToIgnore(IPhysicsObject* pObject)
{
	m_IgnoreCollisionList.append(pObject);
}

void DkPhysicsObject::RemoveIgnoredObject(IPhysicsObject* pObject)
{
	m_IgnoreCollisionList.remove(pObject);
}

bool DkPhysicsObject::ShouldCollideWith(IPhysicsObject* pObject) const
{
	if(arrayFindIndex(m_IgnoreCollisionList, pObject ) != -1)
		return false;

	int nContentsObject1 = GetContents();
	int nCollMaskObject1 = GetCollisionMask();

	int nContentsObject2 = pObject->GetContents();
	int nCollMaskObject2 = pObject->GetCollisionMask();

	bool shouldCollide = (nContentsObject1 & nCollMaskObject2) != 0;
	shouldCollide = shouldCollide && (nContentsObject2 & nCollMaskObject1);

	return shouldCollide;
}