//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium physics objects
//////////////////////////////////////////////////////////////////////////////////

#include "DkBulletObject.h"

#include "render/IDebugOverlay.h"
#include "utils/strtools.h"

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
	m_pPhyObjectPointer = NULL;

	m_pParentObject = NULL;
	m_pUserData = NULL;

	m_nCollisionGroup = 0;
	m_nCollidesWith = 0;

	m_pRMaterial = NULL;

	m_pPhysMaterial = NULL;

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
void CPhysicsObject::SetAngularFactor(Vector3D &factor)
{
	m_pPhyObjectPointer->setAngularFactor(ConvertDKToBulletVectors(factor));
}

Vector3D CPhysicsObject::GetAngularFactor()
{
	return ConvertBulletToDKVectors((btVector3)m_pPhyObjectPointer->getAngularFactor());
}

// Sets linear factor
void CPhysicsObject::SetLinearFactor(Vector3D &factor)
{
	m_pPhyObjectPointer->setLinearFactor(ConvertDKToBulletVectors(factor));
}

Vector3D CPhysicsObject::GetLinearFactor()
{
	return ConvertBulletToDKVectors((btVector3)m_pPhyObjectPointer->getLinearFactor());
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

	m_pPhyObjectPointer->applyImpulse(ConvertPositionToBullet((Vector3D)impulse)*METERS_PER_UNIT_INV, ConvertPositionToBullet((Vector3D)relativePos));
}

void CPhysicsObject::AddLocalTorqueImpulse(const Vector3D &torque)
{
	m_pPhyObjectPointer->applyTorque(ConvertPositionToBullet(torque));
}

void CPhysicsObject::AddForce(const Vector3D &force)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();

	m_pPhyObjectPointer->applyCentralForce(ConvertPositionToBullet((Vector3D)force));
}

void CPhysicsObject::AddLocalForce(const Vector3D &force)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();

	m_pPhyObjectPointer->applyCentralForce(ConvertPositionToBullet((Vector3D)force));
}

void CPhysicsObject::AddForceAtPosition(const Vector3D &force,const Vector3D &pos)
{
	if(m_pPhyObjectPointer->getActivationState() == DISABLE_SIMULATION)
		return;

	m_pPhyObjectPointer->activate();
	m_pPhyObjectPointer->applyForce(ConvertPositionToBullet((Vector3D)force), ConvertPositionToBullet((Vector3D)pos));
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

void CPhysicsObject::SetPosition(const Vector3D &pos)
{
	m_pPhyObjectPointer->getWorldTransform().setOrigin(ConvertPositionToBullet((Vector3D)pos));
}

void CPhysicsObject::SetAngles(const Vector3D &ang)
{
	btVector3 origin = m_pPhyObjectPointer->getWorldTransform().getOrigin();

	btMatrix3x3 rotational;
	Matrix3x3 rotation = rotateXYZ3(DEG2RAD(ang.x),DEG2RAD(ang.y),DEG2RAD(ang.z));

	rotational[0] = ConvertDKToBulletVectors(rotation.rows[0]);
	rotational[1] = ConvertDKToBulletVectors(rotation.rows[1]);
	rotational[2] = ConvertDKToBulletVectors(rotation.rows[2]);
	
	m_pPhyObjectPointer->getWorldTransform().setBasis(rotational);
	m_pPhyObjectPointer->getWorldTransform().setOrigin(origin);
}

void CPhysicsObject::SetVelocity(const Vector3D &linear)
{
	//m_pPhyObjectPointer->setInterpolationLinearVelocity(ConvertPositionToBullet((Vector3D)linear));
	m_pPhyObjectPointer->setLinearVelocity(ConvertPositionToBullet((Vector3D)linear));
}

void CPhysicsObject::SetAngularVelocity(const Vector3D &vAxis, float velocity)
{
	m_pPhyObjectPointer->setAngularVelocity(ConvertPositionToBullet(vAxis*velocity));	
}

void CPhysicsObject::GetAABB(Vector3D &mins, Vector3D &maxs)
{
	btVector3 bmins, bmaxs;

	m_pPhyObjectPointer->getAabb(bmins, bmaxs);

	mins = ConvertPositionToEq(bmins);
	maxs = ConvertPositionToEq(bmaxs);
}

Matrix4x4 CPhysicsObject::GetTransformMatrix()
{
	/*
	Matrix4x4 mat;
	btTransform trans = m_pPhyObjectPointer->getWorldTransform();
	trans.setOrigin(trans.getOrigin() * (1.0f / METERS_PER_UNIT));
	m_pPhyObjectPointer->getWorldTransform().getOpenGLMatrix((float*)&mat);
	*/
	return transpose( ConvertMatrix4ToEq( m_pPhyObjectPointer->getWorldTransform() ) );
}

Matrix4x4 CPhysicsObject::GetOrientationMatrix()
{
	return identity4();
}

// set transformation of object
void CPhysicsObject::SetTransformFromMatrix(Matrix4x4 &matrix)
{
	/*
	btTransform trans;
	trans.setFromOpenGLMatrix(matrix);
	trans.setOrigin(trans.getOrigin()*METERS_PER_UNIT);
	*/
	m_pPhyObjectPointer->setWorldTransform(ConvertMatrix4ToBullet(matrix));
}

Vector3D CPhysicsObject::GetAngles()
{
	Matrix4x4 eq_matrix;
	m_pPhyObjectPointer->getWorldTransform().getOpenGLMatrix(eq_matrix.rows[0]);

	Vector3D vec = EulerMatrixXYZ(transpose(eq_matrix.getRotationComponent()));

	return VRAD2DEG(vec);
}

Vector3D CPhysicsObject::GetPosition()
{
	return ConvertPositionToEq( m_pPhyObjectPointer->getWorldTransform().getOrigin() );
}

Vector3D CPhysicsObject::GetVelocity()
{
	btVector3 vec = m_pPhyObjectPointer->getLinearVelocity();
	return ConvertPositionToEq(vec);
}

Vector3D CPhysicsObject::GetAngularVelocity()
{
	btVector3 vec = m_pPhyObjectPointer->getAngularVelocity();
	return ConvertPositionToEq(vec);
}

Vector3D CPhysicsObject::GetVelocityAtPoint(Vector3D &point)
{
	btVector3 vec = m_pPhyObjectPointer->getVelocityInLocalPoint(ConvertPositionToBullet(point));

	return ConvertPositionToEq(vec);
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

	Vector3D normal = ConvertBulletToDKVectors(pt->m_normalWorldOnB);

	Vector3D speed = ConvertPositionToEq(m_pPhyObjectPointer->getVelocityInLocalPoint(pt->m_positionWorldOnB - m_pPhyObjectPointer->getWorldTransform().getOrigin()));

	float fImpulse = fabs(dot(speed, normal));

	IPhysicsObject* objB = (IPhysicsObject*)hitB;

	m_ContactEvents[m_numEvents].pHitB				= objB;

	m_ContactEvents[m_numEvents].vWorldHitNormal	= normal;
	m_ContactEvents[m_numEvents].vWorldHitOriginA	= ConvertPositionToEq(pt->m_positionWorldOnA);
	m_ContactEvents[m_numEvents].vWorldHitOriginB	= ConvertPositionToEq(pt->m_positionWorldOnB);

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
void CPhysicsObject::SetChildShapeTransform(int shapeNum, Vector3D &localOrigin, Vector3D &localAngles)
{
	if(m_pPhyObjectPointer->getCollisionShape()->getShapeType() == COMPOUND_SHAPE_PROXYTYPE)
	{
		btTransform trans;
		trans.setIdentity();
		trans.setOrigin(ConvertPositionToBullet(localOrigin));

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