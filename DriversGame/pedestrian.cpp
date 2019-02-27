//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#include "pedestrian.h"

const Vector3D PEDESTRIAN_BOX(0.45f, 0.8f, 0.45f);
const Vector3D PEDESTRIAN_OFFS(0.0f, 0.8f, 0.0f);

CPedestrian::CPedestrian() : CPedestrian(nullptr)
{
}

CPedestrian::CPedestrian(kvkeybase_t* kvdata) : CGameObject(), CAnimatingEGF()
{
	m_physBody = nullptr;
}

CPedestrian::~CPedestrian()
{

}

void CPedestrian::Precache()
{
	PrecacheStudioModel("models/characters/test.egf");
}

void CPedestrian::SetModelPtr(IEqModel* modelPtr)
{
	BaseClass::SetModelPtr(modelPtr);

	InitAnimating(m_pModel);
}

void CPedestrian::OnRemove()
{
	if (m_physBody)
	{
		g_pPhysics->m_physics.DestroyBody(m_physBody);
		m_physBody = NULL;
	}

	BaseClass::OnRemove();
}

void CPedestrian::Spawn()
{
	SetModel("models/characters/test.egf");

	SetActivity(ACT_RUN);

	m_physBody = new CEqRigidBody();
	m_physBody->Initialize(-PEDESTRIAN_BOX + PEDESTRIAN_OFFS, PEDESTRIAN_BOX + PEDESTRIAN_OFFS);

	m_physBody->SetCollideMask(COLLIDEMASK_OBJECT);
	m_physBody->SetContents(OBJECTCONTENTS_OBJECT);

	m_physBody->SetPosition(GetOrigin());

	m_physBody->SetMass(100.0f);
	m_physBody->SetFriction(0.9f);
	m_physBody->SetRestitution(0.5f);
	m_physBody->SetAngularFactor(vec3_zero);
	m_physBody->m_erp = 0.15f;

	m_physBody->SetUserData(this);

	m_bbox = m_physBody->m_aabb_transformed;

	g_pPhysics->m_physics.AddToWorld(m_physBody);

	BaseClass::Spawn();
}

void CPedestrian::Draw(int nRenderFlags)
{
	RecalcBoneTransforms();

	m_physBody->ConstructRenderMatrix(m_worldMatrix);

	DrawEGF(nRenderFlags, m_boneTransforms);
}

void CPedestrian::Simulate(float fDt)
{
	m_vecOrigin = m_physBody->GetPosition();

	m_vecAngles = eulers(m_physBody->GetOrientation());
	m_vecAngles = VRAD2DEG(m_vecAngles);

	AdvanceFrame(fDt);
	DebugRender(m_worldMatrix);

	BaseClass::Simulate(fDt);
}

void CPedestrian::SetOrigin(const Vector3D& origin)
{
	if (m_physBody)
		m_physBody->SetPosition(origin);

	m_vecOrigin = origin;
}

void CPedestrian::SetAngles(const Vector3D& angles)
{
	if (m_physBody)
		m_physBody->SetOrientation(Quaternion(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CPedestrian::SetVelocity(const Vector3D& vel)
{
	if (m_physBody)
		m_physBody->SetLinearVelocity(vel);
}

const Vector3D& CPedestrian::GetOrigin()
{
	m_vecOrigin = m_physBody->GetPosition();
	return m_vecOrigin;
}

const Vector3D& CPedestrian::GetAngles()
{
	m_vecAngles = eulers(m_physBody->GetOrientation());
	m_vecAngles = VRAD2DEG(m_vecAngles);

	return m_vecAngles;
}

const Vector3D& CPedestrian::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}

void CPedestrian::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
{

}