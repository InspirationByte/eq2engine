//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#include "pedestrian.h"
#include "CameraAnimator.h"
#include "input.h"

const float PEDESTRIAN_RADIUS = 0.35f;
const float PEDESTRIAN_HEIGHT = 0.85f;

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

	m_physBody = new CEqRigidBody();
	m_physBody->Initialize(PEDESTRIAN_RADIUS, PEDESTRIAN_HEIGHT);

	m_physBody->SetCollideMask(COLLIDEMASK_OBJECT);
	m_physBody->SetContents(OBJECTCONTENTS_OBJECT);

	m_physBody->SetPosition(GetOrigin());

	m_physBody->SetMass(100.0f);
	m_physBody->SetFriction(0.0f);
	m_physBody->SetRestitution(0.5f);
	m_physBody->SetAngularFactor(vec3_up);
	m_physBody->m_erp = 0.1f;

	m_physBody->SetUserData(this);

	m_bbox = m_physBody->m_aabb_transformed;

	g_pPhysics->m_physics.AddToWorld(m_physBody);

	BaseClass::Spawn();
}

void CPedestrian::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	conf.dist = 4.8f;
	conf.height = 1.0f;
	conf.distInCar = 0.0f;
	conf.widthInCar = 0.0f;
	conf.heightInCar = 0.72f;
	conf.fov = 60.0f;

	filter.AddObject(m_physBody);
}

void CPedestrian::Draw(int nRenderFlags)
{
	RecalcBoneTransforms();

	//m_physBody->ConstructRenderMatrix(m_worldMatrix);

	UpdateTransform();
	DrawEGF(nRenderFlags, m_boneTransforms);
}

void CPedestrian::Simulate(float fDt)
{
	m_vecOrigin = m_physBody->GetPosition();

	const Vector3D& velocity = m_physBody->GetLinearVelocity();

	Vector3D newVelocity(0.0f, velocity.y, 0.0f);

	Vector3D forwardVec;
	AngleVectors(m_vecAngles, &forwardVec);

	if (m_controlButtons & IN_ACCELERATE)
	{
		newVelocity += forwardVec * 8.0f;
	}
	else if (m_controlButtons & IN_ACCELERATE)
	{
		newVelocity += forwardVec * 2.0f;
	}

	m_physBody->ApplyLinearForce(newVelocity * m_physBody->GetMass() * 2.0f);
	m_physBody->TryWake();

	if (m_controlButtons & IN_TURNLEFT)
		m_vecAngles.y += 120.0f * fDt;

	if (m_controlButtons & IN_TURNRIGHT)
		m_vecAngles.y -= 120.0f * fDt;

	Activity currentAct = GetCurrentActivity();

	if (length(velocity.xz()) > 0.5f)
	{
		if (currentAct != ACT_RUN)
		{
			SetActivity(ACT_RUN);
		}
	}
	else if(currentAct != ACT_IDLE)
	{
		SetActivity(ACT_IDLE);
	}

	AdvanceFrame(fDt);
	DebugRender(m_worldMatrix);

	BaseClass::Simulate(fDt);
}

void CPedestrian::UpdateTransform()
{
	Vector3D offset(vec3_up*PEDESTRIAN_HEIGHT);

	// refresh it's matrix
	m_worldMatrix = translate(m_vecOrigin - offset)*rotateXYZ4(DEG2RAD(m_vecAngles.x), DEG2RAD(m_vecAngles.y), DEG2RAD(m_vecAngles.z));
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
	//m_vecAngles = eulers(m_physBody->GetOrientation());
	//m_vecAngles = VRAD2DEG(m_vecAngles);

	return m_vecAngles;
}

const Vector3D& CPedestrian::GetVelocity() const
{
	return m_physBody->GetLinearVelocity();
}

void CPedestrian::HandleAnimatingEvent(AnimationEvent nEvent, char* options)
{

}