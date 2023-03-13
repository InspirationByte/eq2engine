//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#define __BT_SKIP_UINT64_H	// for SDL2
#include <btBulletDynamicsCommon.h>

#include "core/core_common.h"
#include "DkBulletObject.h"

#include "dkphysics/physcoord.h"
#include "render/IDebugOverlay.h"

#include "dkphysics/IDkPhysics.h"

#include "physics/BulletConvert.h"
using namespace EqBulletUtils;

static int island_params_translate[3] =
{
	DISABLE_SIMULATION,
	WANTS_DEACTIVATION,
	ISLAND_SLEEPING
};

static phys_freeze_state_e  back_to_freeze_state[6] =
{
	PS_INVALID,

	PS_ACTIVE,
	PS_INACTIVE,
	PS_ACTIVE,
	PS_ACTIVE,
	PS_FROZEN
};

CPhysicsObject::CPhysicsObject()
{
	m_pPhyObjectPointer = nullptr;

	m_pParentObject = nullptr;
	m_pUserData = nullptr;

	m_nCollisionGroup = 0;
	m_nCollidesWith = 0;

	m_pRMaterial = nullptr;

	m_pPhysMaterial = nullptr;

	m_numEvents = 0;

	m_nFlags = 0;
}

CPhysicsObject::~CPhysicsObject()
{

}

// Sets collides with group flags
void CPhysicsObject::SetCollisionMask(uint group)
{
	m_nCollidesWith = group;
}

// Returns collides with group flags
uint CPhysicsObject::GetCollisionMask()
{
	return m_nCollidesWith;
}

// Sets collision group
void CPhysicsObject::SetContents(uint group)
{
	m_nCollisionGroup = group;
}

// Returns collision group
uint CPhysicsObject::GetContents()
{
	return m_nCollisionGroup;
}

// Sets collision group
void CPhysicsObject::SetEntityObjectPtr(void* ptr)
{
	m_pParentObject = ptr;
}

// Returns entity associated with this physics object
void* CPhysicsObject::GetEntityObjectPtr()
{
	return m_pParentObject;
}

// Sets user data pointer
void CPhysicsObject::SetUserData(void* ptr)
{
	m_pUserData = ptr;
}

// Returns user data
void* CPhysicsObject::GetUserData()
{
	return m_pUserData;
}

// Various flags
void CPhysicsObject::AddFlags(int nFlags)
{
	m_nFlags |= nFlags;
}

// Various flags
void CPhysicsObject::RemoveFlags(int nFlags)
{
	m_nFlags &= ~nFlags;
}

int	CPhysicsObject::GetFlags()
{
	return m_nFlags;
}

void CPhysicsObject::SetActivationState(phys_freeze_state_e nState)
{
	//m_pPhyObjectPointer->setActivationState(island_params_translate[nState]);
	m_pPhyObjectPointer->forceActivationState(island_params_translate[nState]);
}

phys_freeze_state_e CPhysicsObject::GetActivationState()
{
	return back_to_freeze_state[m_pPhyObjectPointer->getActivationState()];
}

void CPhysicsObject::WakeUp()
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();
}

// Sets angular factor (may help with up-right vector)
void CPhysicsObject::SetAngularFactor(const Vector3D &factor)
{
	btVector3 vec;
	ConvertDKToBulletVectors(vec, factor);
	m_pPhyObjectPointer->setAngularFactor(vec);
}

Vector3D CPhysicsObject::GetAngularFactor()
{
	Vector3D out;
	ConvertBulletToDKVectors(out, m_pPhyObjectPointer->getAngularFactor());
	return out;
}

// Sets linear factor
void CPhysicsObject::SetLinearFactor(const Vector3D &factor)
{
	btVector3 vec;
	ConvertDKToBulletVectors(vec, factor);
	m_pPhyObjectPointer->setLinearFactor(vec);
}

Vector3D CPhysicsObject::GetLinearFactor()
{
	Vector3D out;
	ConvertBulletToDKVectors(out, m_pPhyObjectPointer->getLinearFactor());
	return out;
}

// Sets linear factor
void CPhysicsObject::SetRestitution(float rest)
{
	m_pPhyObjectPointer->setRestitution(rest);
}

// gets linear factor
float CPhysicsObject::GetRestitution()
{
	return m_pPhyObjectPointer->getRestitution();
}

// Sets linear factor
void CPhysicsObject::SetDamping(float linear, float angular)
{
	m_pPhyObjectPointer->setDamping(linear,angular);
}

// Sets linear factor
float CPhysicsObject::GetDampingLinear()
{
	return m_pPhyObjectPointer->getLinearDamping();
}

// Sets linear factor
void CPhysicsObject::SetFriction(float fric)
{
	m_pPhyObjectPointer->setFriction(fric);
}

// Sets linear factor
float CPhysicsObject::GetFriction()
{
	return m_pPhyObjectPointer->getFriction();
}

// Sets linear factor
float CPhysicsObject::GetDampingAngular()
{
	return m_pPhyObjectPointer->getAngularDamping();
}

float CPhysicsObject::GetInvMass()
{
	return m_pPhyObjectPointer->getInvMass();
}

void CPhysicsObject::SetMass(float fMass)
{
	btVector3 localInertia;
	m_pPhyObjectPointer->getCollisionShape()->calculateLocalInertia(fMass * METERS_PER_UNIT_INV, localInertia);

	m_pPhyObjectPointer->setMassProps(fMass * METERS_PER_UNIT_INV, localInertia);
}

const char* CPhysicsObject::GetName()
{
	return m_name.GetData();
}

void CPhysicsObject::SetCollisionResponseEnabled(bool bEnabled)
{
	if(bEnabled)
	{
		int nFlags = m_pPhyObjectPointer->getCollisionFlags();
		nFlags &= ~btCollisionObject::CF_NO_CONTACT_RESPONSE;

		m_pPhyObjectPointer->setCollisionFlags(nFlags);
	}
	else
	{
		int nFlags = m_pPhyObjectPointer->getCollisionFlags();
		nFlags |= btCollisionObject::CF_NO_CONTACT_RESPONSE;

		m_pPhyObjectPointer->setCollisionFlags(nFlags);
	}
}

// returns object static state
bool CPhysicsObject::IsStatic()
{
	return m_pPhyObjectPointer->isStaticObject();
}

// returns object dynamic state
bool CPhysicsObject::IsDynamic()
{
	return !m_pPhyObjectPointer->isStaticObject();
}

// Sleep thereshold
void CPhysicsObject::SetSleepTheresholds(float lin, float ang)
{
	m_pPhyObjectPointer->setSleepingThresholds(lin, ang);
}

void CPhysicsObject::ApplyImpulse(const Vector3D &impulse, const Vector3D &relativePos)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();

	btVector3 vec, pos;
	ConvertPositionToBullet(vec, impulse);
	ConvertPositionToBullet(pos, relativePos);
	m_pPhyObjectPointer->applyImpulse(vec * METERS_PER_UNIT_INV, pos);
}

void CPhysicsObject::AddLocalTorqueImpulse(const Vector3D &torque)
{
	btVector3 vec;
	ConvertPositionToBullet(vec, torque);
	m_pPhyObjectPointer->applyTorque(vec);
}

void CPhysicsObject::AddForce(const Vector3D &force)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();

	btVector3 vec;
	ConvertPositionToBullet(vec, force);
	m_pPhyObjectPointer->applyCentralForce(vec);
}

void CPhysicsObject::AddLocalForce(const Vector3D &force)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();

	btVector3 vec;
	ConvertPositionToBullet(vec, force);
	m_pPhyObjectPointer->applyCentralForce(vec);
}

void CPhysicsObject::AddForceAtPosition(const Vector3D &force,const Vector3D &position)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	btVector3 vec, pos;
	ConvertPositionToBullet(vec, force);
	ConvertPositionToBullet(pos, position);

	m_pPhyObjectPointer->activate();
	m_pPhyObjectPointer->applyForce(vec, pos);
}

void CPhysicsObject::AddForceAtLocalPosition(const Vector3D &force,const Vector3D &pos)
{
	
}

void CPhysicsObject::AddLocalForceAtPosition(const Vector3D &force,const Vector3D &pos)
{

}

void CPhysicsObject::AddLocalForceAtLocalPosition(const Vector3D &force,const Vector3D &pos)
{

}

void CPhysicsObject::SetPosition(const Vector3D &position)
{
	btVector3 pos;
	ConvertPositionToBullet(pos, position);
	m_pPhyObjectPointer->getWorldTransform().setOrigin(pos);
}

void CPhysicsObject::SetAngles(const Vector3D &ang)
{
	btVector3 origin = m_pPhyObjectPointer->getWorldTransform().getOrigin();

	btMatrix3x3 rotational;
	Matrix3x3 rotation = rotateXYZ3(DEG2RAD(ang.x),DEG2RAD(ang.y),DEG2RAD(ang.z));

	ConvertDKToBulletVectors(rotational[0], rotation.rows[0]);
	ConvertDKToBulletVectors(rotational[1], rotation.rows[1]);
	ConvertDKToBulletVectors(rotational[2], rotation.rows[2]);
	
	m_pPhyObjectPointer->getWorldTransform().setBasis(rotational);
	m_pPhyObjectPointer->getWorldTransform().setOrigin(origin);
}

void CPhysicsObject::SetVelocity(const Vector3D &linear)
{
	btVector3 vec;
	ConvertPositionToBullet(vec, linear);
	m_pPhyObjectPointer->setLinearVelocity(vec);
}

void CPhysicsObject::SetAngularVelocity(const Vector3D &vAxis, float velocity)
{
	btVector3 vel;
	ConvertPositionToBullet(vel, vAxis * velocity);
	m_pPhyObjectPointer->setAngularVelocity(vel);
}

void CPhysicsObject::GetBoundingBox(Vector3D &mins, Vector3D &maxs)
{
	btVector3 bmins, bmaxs;

	m_pPhyObjectPointer->getAabb(bmins, bmaxs);

	ConvertPositionToEq(mins, bmins);
	ConvertPositionToEq(maxs, bmaxs);
}

Matrix4x4 CPhysicsObject::GetTransformMatrix()
{
	Matrix4x4 mat;
	ConvertMatrix4ToEq(mat, m_pPhyObjectPointer->getWorldTransform());
	return transpose(mat);
}

Matrix4x4 CPhysicsObject::GetOrientationMatrix()
{
	return identity4;
}

// set transformation of object
void CPhysicsObject::SetTransformFromMatrix(const Matrix4x4 &matrix)
{
	btTransform trans;
	ConvertMatrix4ToBullet(trans, matrix);
	m_pPhyObjectPointer->setWorldTransform(trans);
}

Vector3D CPhysicsObject::GetAngles()
{
	Matrix4x4 eq_matrix;
	m_pPhyObjectPointer->getWorldTransform().getOpenGLMatrix(eq_matrix.rows[0]);

	Vector3D vec = EulerMatrixXYZ(transpose(eq_matrix.getRotationComponent()));

	return RAD2DEG(vec);
}

Vector3D CPhysicsObject::GetPosition()
{
	Vector3D out;
	ConvertPositionToEq(out, m_pPhyObjectPointer->getWorldTransform().getOrigin() );
	return out;
}

Vector3D CPhysicsObject::GetVelocity()
{
	Vector3D out;
	ConvertPositionToEq(out, m_pPhyObjectPointer->getLinearVelocity());
	return out;
}

Vector3D CPhysicsObject::GetAngularVelocity()
{
	Vector3D out;
	ConvertBulletToDKVectors(out, m_pPhyObjectPointer->getAngularVelocity());
	return out;
}

Vector3D CPhysicsObject::GetVelocityAtPoint(const Vector3D &point)
{
	btVector3 pt;
	ConvertPositionToBullet(pt, point);
	
	Vector3D out;
	ConvertPositionToEq(out, m_pPhyObjectPointer->getVelocityInLocalPoint(pt));

	return out;
}

phySurfaceMaterial_t* CPhysicsObject::GetMaterial()
{
	return m_pPhysMaterial;
}

// internal use, adds contact event
void CPhysicsObject::AddContactEventFromManifoldPoint(btManifoldPoint* pt, CPhysicsObject* hitB)
{
	if(m_numEvents >= MAX_CONTACT_EVENTS)
		return;

	Vector3D normal;
	ConvertBulletToDKVectors(normal, pt->m_normalWorldOnB);

	Vector3D speed;
	ConvertPositionToEq(speed, m_pPhyObjectPointer->getVelocityInLocalPoint(pt->m_positionWorldOnB - m_pPhyObjectPointer->getWorldTransform().getOrigin()));

	float fImpulse = fabs(dot(speed, normal));

	IPhysicsObject* objB = (IPhysicsObject*)hitB;

	m_ContactEvents[m_numEvents].pHitB				= objB;

	m_ContactEvents[m_numEvents].vWorldHitNormal	= normal;
	ConvertPositionToEq(m_ContactEvents[m_numEvents].vWorldHitOriginA, pt->m_positionWorldOnA);
	ConvertPositionToEq(m_ContactEvents[m_numEvents].vWorldHitOriginB, pt->m_positionWorldOnB);

	m_ContactEvents[m_numEvents].fImpulse			= fImpulse;
	m_ContactEvents[m_numEvents].fCombinedFriction	= pt->m_combinedFriction;
	m_ContactEvents[m_numEvents].fCombinedRest		= pt->m_combinedRestitution;
	m_ContactEvents[m_numEvents].fImpulseLateral_1	= pt->m_appliedImpulseLateral1;
	m_ContactEvents[m_numEvents].fImpulseLateral_2	= pt->m_appliedImpulseLateral2;
	m_ContactEvents[m_numEvents].fDistance			= pt->m_distance1;

	m_numEvents++;
}

// Contacts
void CPhysicsObject::ClearContactEvents()
{
	m_numEvents = 0;
}

int CPhysicsObject::GetContactEventCount()
{
	return m_numEvents;
}

physicsContactEvent_t* CPhysicsObject::GetContactEvent(int idx)
{
	return &m_ContactEvents[idx];
}

// returns child shapes count
int CPhysicsObject::GetChildShapeCount()
{
	if(m_pPhyObjectPointer->getCollisionShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		btCompoundShape* pShape = (btCompoundShape*)m_pPhyObjectPointer->getCollisionShape();
		return pShape->getNumChildShapes();
	}

	return 0;
}

// setup child transform
void CPhysicsObject::SetChildShapeTransform(int shapeNum, const Vector3D &localOrigin, const Vector3D &localAngles)
{
	if(m_pPhyObjectPointer->getCollisionShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		btVector3 vec;
		ConvertPositionToBullet(vec, localOrigin);

		btTransform trans;
		trans.setIdentity();
		trans.setOrigin(vec);

		btCompoundShape* pShape = (btCompoundShape*)m_pPhyObjectPointer->getCollisionShape();
		pShape->updateChildTransform(shapeNum, trans);
	}
}

// SAVE - RESTORE functions

void CPhysicsObject::AddObjectToIgnore(IPhysicsObject* pObject)
{
	m_IgnoreCollisionList.append(pObject);
}

void CPhysicsObject::RemoveIgnoredObject(IPhysicsObject* pObject)
{
	m_IgnoreCollisionList.remove(pObject);
}

bool CPhysicsObject::ShouldCollideWith(IPhysicsObject* pObject)
{
	if(m_IgnoreCollisionList.findIndex( pObject ) != -1)
		return false;

	int nContentsObject1 = GetContents();
	int nCollMaskObject1 = GetCollisionMask();

	int nContentsObject2 = pObject->GetContents();
	int nCollMaskObject2 = pObject->GetCollisionMask();

	bool shouldCollide = (nContentsObject1 & nCollMaskObject2) != 0;
	shouldCollide = shouldCollide && (nContentsObject2 & nCollMaskObject1);

	return shouldCollide;
}