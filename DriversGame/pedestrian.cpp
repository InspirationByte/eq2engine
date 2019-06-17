//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2019
//////////////////////////////////////////////////////////////////////////////////
// Description: Pedestrian
//////////////////////////////////////////////////////////////////////////////////

#include "pedestrian.h"
#include "CameraAnimator.h"
#include "input.h"

#define PED_MODEL "models/characters/ped1.egf"

const float PEDESTRIAN_RADIUS = 0.85f;

CPedestrian::CPedestrian() : CGameObject(), CAnimatingEGF()
{
	m_physBody = nullptr;
	m_onGround = false;
	m_pedState = 0;
}

CPedestrian::CPedestrian(kvkeybase_t* kvdata) : CPedestrian()
{

}

CPedestrian::~CPedestrian()
{

}

void CPedestrian::Precache()
{
	PrecacheStudioModel(PED_MODEL);
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
	SetModel(PED_MODEL);

	m_physBody = new CEqRigidBody();
	m_physBody->Initialize(PEDESTRIAN_RADIUS);

	m_physBody->SetCollideMask(COLLIDEMASK_DEBRIS);
	m_physBody->SetContents(OBJECTCONTENTS_VEHICLE);

	m_physBody->SetPosition(m_vecOrigin);

	m_physBody->m_flags |= BODY_DISABLE_DAMPING | /*COLLOBJ_DISABLE_RESPONSE |*/ COLLOBJ_COLLISIONLIST | BODY_FROZEN;

	m_physBody->SetMass(85.0f);
	m_physBody->SetFriction(0.0f);
	m_physBody->SetRestitution(0.0f);
	m_physBody->SetAngularFactor(vec3_zero);
	m_physBody->m_erp = 0.15f;
	m_physBody->SetGravity(18.0f);
	
	m_physBody->SetUserData(this);

	m_bbox = m_physBody->m_aabb_transformed;

	g_pPhysics->m_physics.AddToWorld(m_physBody);

	BaseClass::Spawn();
}

void CPedestrian::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	conf.dist = 3.8f;
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

const float ACCEL_RATIO = 12.0f;
const float DECEL_RATIO = 25.0f;

const float MAX_WALK_VELOCITY = 1.5f;
const float MAX_RUN_VELOCITY = 9.0f;

void CPedestrian::Simulate(float fDt)
{
	m_vecOrigin = m_physBody->GetPosition();

	if(!m_physBody->IsFrozen())
		m_onGround = false;

	for (int i = 0; i < m_physBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& pair = m_physBody->m_collisionList[i];

		m_onGround = (pair.bodyB && pair.bodyB->GetContents() == OBJECTCONTENTS_SOLID_GROUND);

		if (m_onGround)
			break;
	}

	const Vector3D& velocity = m_physBody->GetLinearVelocity();

	Vector3D preferredMove(0.0f);

	Vector3D forwardVec;
	AngleVectors(m_vecAngles, &forwardVec);

	int controlButtons = GetControlButtons();

	Activity bestMoveActivity = (controlButtons & IN_BURNOUT) ? ACT_RUN : ACT_WALK;
	float bestMaxSpeed = (bestMoveActivity == ACT_RUN) ? MAX_RUN_VELOCITY : MAX_WALK_VELOCITY;

	if (fDt > 0.0f)
	{
		if (length(velocity) < bestMaxSpeed)
		{
			if (controlButtons & IN_ACCELERATE)
				preferredMove += forwardVec * ACCEL_RATIO;
			else if (controlButtons & IN_BRAKE)
				preferredMove -= forwardVec * ACCEL_RATIO;
		}

		{
			preferredMove.x -= (velocity.x - preferredMove.x) * DECEL_RATIO;
			preferredMove.z -= (velocity.z - preferredMove.z) * DECEL_RATIO;
		}

		m_physBody->ApplyLinearForce(preferredMove * m_physBody->GetMass());
	}

	if (controlButtons)
		m_physBody->TryWake(false);
	
	if (controlButtons & IN_TURNLEFT)
		m_vecAngles.y += 120.0f * fDt;

	if (controlButtons & IN_TURNRIGHT)
		m_vecAngles.y -= 120.0f * fDt;

	if (controlButtons & IN_BURNOUT)
		m_pedState = 1;
	else if (controlButtons & IN_EXTENDTURN)
		m_pedState = 2;
	else if (controlButtons & IN_HANDBRAKE)
		m_pedState = 0;

	Activity idleAct = ACT_IDLE;

	if (m_pedState == 1)
	{
		idleAct = ACT_IDLE_WPN;
	}
	else if (m_pedState == 2)
	{
		idleAct = ACT_IDLE_CROUCH;
	}
	else
		m_pedState = 0;

	Activity currentAct = GetCurrentActivity();

	if (currentAct != idleAct)
		SetPlaybackSpeedScale(length(velocity) / bestMaxSpeed);
	else
		SetPlaybackSpeedScale(1.0f);

	if (length(velocity.xz()) > 0.5f)
	{
		if (currentAct != bestMoveActivity)
			SetActivity(bestMoveActivity);
	}
	else if(currentAct != idleAct)
		SetActivity(idleAct);

	AdvanceFrame(fDt);
	DebugRender(m_worldMatrix);

	BaseClass::Simulate(fDt);
}

void CPedestrian::UpdateTransform()
{
	Vector3D offset(vec3_up*PEDESTRIAN_RADIUS);

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
	if(m_physBody)
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