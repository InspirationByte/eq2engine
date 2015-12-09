//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Pigeon
//////////////////////////////////////////////////////////////////////////////////

#include "BasePigeon.h"
#include "gameinput.h"

#define DEFAULT_PIGEON_MODEL	"models/pigeons/pigeon_test.egf"

#define PIGEON_HEIGHT			12.0f
#define PIGEON_EYEHEIGHT		10.0f

#define PIGEON_MAX_FLIGHT_SPEED 380.0f
#define PIGEON_WALK_MAX_SPEED	90.0f

CBasePigeon::CBasePigeon()
{
	m_vecTraction = vec3_zero;
	m_vecFlightDirection = Vector3D(0,0,1);
	m_fFlyVelocity = 0.0f;
	m_fJumpPower = 150.0f;
	m_fMaxSpeed = PIGEON_WALK_MAX_SPEED;
}

void CBasePigeon::Precache()
{
	PrecacheStudioModel( DEFAULT_PIGEON_MODEL );

	BaseClass::Precache();
}

void CBasePigeon::Spawn()
{
	// actor height control
	m_fActorHeight			= PIGEON_HEIGHT;
	m_fEyeHeight			= PIGEON_EYEHEIGHT;

	SetModel( DEFAULT_PIGEON_MODEL );

	m_nHeadIKChain = FindIKChain("Head");

	BaseClass::Spawn();
}

void CBasePigeon::ProcessMovement( movementdata_t& data )
{
	MoveType_e type = GetMoveType();

	BaseEntity* pActorIgnore = this;

	m_bLastOnGround = m_bOnGround;
	//m_bOnGround = false;

	Matrix4x4 physics_transform = m_pPhysicsObject->GetTransformMatrix();

	BoundingBox bbox;
	m_pPhysicsObject->GetAABB(bbox.minPoint, bbox.maxPoint);

	// process basic things
	BaseClass::ProcessMovement(data);

	Vector3D velocity = m_pPhysicsObject->GetVelocity();

	if(type == MOVE_TYPE_FLY)
	{
		m_pPhysicsObject->SetCollisionMask(COLLIDE_ACTOR);

		Vector3D physObjectUpVec = physics_transform.rows[1].xyz();
		physObjectUpVec.x *= -1;

		float box_size_mult = 1.0f;

		if(m_vPrevPhysicsVelocity.y < MIN_FALLDMG_SPEED)
			box_size_mult += fabs(m_vPrevPhysicsVelocity.y*0.05f);

		Vector3D box_size(16,5+box_size_mult,16);

		Vector3D player_bottom = Vector3D(GetAbsOrigin().x, bbox.minPoint.y + box_size.y*0.5f,GetAbsOrigin().z);

		trace_t ground_trace;
		UTIL_TraceBox( player_bottom + Vector3D(0.0f,5,0.0f), player_bottom, box_size, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &ground_trace, &pActorIgnore, 1 );

		m_bOnGround = (ground_trace.fraction < 1.0f);

		CheckLanding();

		m_vecFlyAngle = NormalizeAngles360( m_vecFlyAngle );

		m_fFlyVelocity = clamp(m_fFlyVelocity, 0.0f, 1.0f);

		Vector3D rightDir, upDir;
		AngleVectors(m_vecFlyAngle, &m_vecFlightDirection, &rightDir, &upDir);

		Vector3D physVelocity = lerp(velocity, (m_vecFlightDirection*PIGEON_MAX_FLIGHT_SPEED), m_fFlyVelocity);

		m_pPhysicsObject->SetVelocity(physVelocity);

		float fAngleFactor = -((m_vecFlyAngle.x-90)/180.0f);

		// reduce fly velocity
		m_fFlyVelocity -= gpGlobals->frametime*fAngleFactor*0.15f;

		// don't reduce speed fully
		if(m_fFlyVelocity < 0.1 && ((GetCurrentActivity() == ACT_FLY) || (GetCurrentActivity() == ACT_FLY_WINGS)))
			m_fFlyVelocity = 0.1f;

		debugoverlay->Line3D(GetAbsOrigin(), GetAbsOrigin() + m_vecFlightDirection*PIGEON_MAX_FLIGHT_SPEED, ColorRGBA(0,0,0,1.0f), ColorRGBA(0,0,1.0f,1.0f));
		debugoverlay->Line3D(GetAbsOrigin(), GetAbsOrigin() + rightDir*85, ColorRGBA(0,0,0,1.0f), ColorRGBA(1.0f,0,0,1.0f));
		debugoverlay->Line3D(GetAbsOrigin(), GetAbsOrigin() + upDir*25, ColorRGBA(0,0,0,1.0f), ColorRGBA(0,1.0f,0,1.0f));

		// TODO: lerping
		m_vecAbsAngles = m_vecFlyAngle;

		if(m_fFlyVelocity >= 0.05f)
		{
			if((m_nActorButtons & IN_WINGLEFT) || (m_nActorButtons & IN_WINGRIGHT))
			{
				SetPlaybackSpeedScale(5.0f - m_fFlyVelocity*4.0f);

				if(GetCurrentActivity() != ACT_FLY_WINGS)
					SetActivity(ACT_FLY_WINGS);
			}
			else if(GetCurrentActivity() != ACT_FLY)
			{
				SetActivity(ACT_FLY);
				SetPlaybackSpeedScale(1.0f);
			}
		}

	}
	else
	{
		m_vecTraction = vec3_zero;
		m_vecFlyAngle = m_vecEyeAngles;
		EyeVectors(&m_vecFlightDirection);
		m_fFlyVelocity = 0.0f;

		float speed = length(velocity.xz());

		m_vecAbsAngles.x = 0;
		m_vecAbsAngles.z = 0;

		// process model controls
		if(speed > 5.0f)
		{
			if(GetCurrentActivity() != ACT_WALK)
				SetActivity(ACT_WALK);

			float speed_turn_rate = clamp((speed+5.0f) / 2.0f, 0.0f, 1.0f);

			float speed_anim_rate = clamp((speed+5.0f) / PIGEON_WALK_MAX_SPEED, 0.0f, 1.0f);

			SetPlaybackSpeedScale(speed_anim_rate);

			float angleDiff = AngleDiff(m_vecAbsAngles.y, m_vecEyeAngles.y);
				m_vecAbsAngles.y += angleDiff*gpGlobals->frametime*speed_turn_rate*5.0f;
		}
		else
		{
			if(GetCurrentActivity() != ACT_IDLE)
				SetActivity(ACT_IDLE);

			SetPlaybackSpeedScale(1.0f);
		}
	}
}

// compute matrix transformation for this object
void CBasePigeon::ComputeTransformMatrix()
{
	m_vecAbsAngles = NormalizeAngles180(m_vecAbsAngles);
	Matrix4x4 rotation = rotateY(DEG2RAD(m_vecAbsAngles.y)) * rotateX(DEG2RAD(m_vecAbsAngles.x)) * rotateZ(DEG2RAD(m_vecAbsAngles.z));//rotateXYZ(DEG2RAD(m_vecAbsAngles.x),DEG2RAD(m_vecAbsAngles.y),DEG2RAD(-m_vecAbsAngles.z));

	// rotateYZX(DEG2RAD(m_vecAbsAngles.x),DEG2RAD(m_vecAbsAngles.y),DEG2RAD(m_vecAbsAngles.z));

	m_matWorldTransform = translate(m_vecAbsOrigin) * rotation * scale(1.0f, 1.0f, 1.0f);
}

MoveType_e CBasePigeon::GetMoveType()
{
	MoveType_e type = BaseClass::GetMoveType();

	if(type == MOVE_TYPE_WALK && !m_bOnGround)
		return MOVE_TYPE_FLY;

	return type;
}

void CBasePigeon::PainSound()
{

}

void CBasePigeon::DeathSound()
{

}

void CBasePigeon::ApplyTraction(Vector3D& vAddTraction)
{
	m_vecTraction += vAddTraction;
}

void CBasePigeon::AliveThink()
{
	// update traction, movement


	// apply traction on a movement velocity

	// also control pigeon wings automatically if he's on ground using traction
	BaseClass::AliveThink();
}

void CBasePigeon::UpdateView()
{
	m_vecEyeOrigin = GetAbsOrigin() + Vector3D(0,m_fEyeHeight/2,0);
}

BEGIN_DATAMAP(CBasePigeon)
	DEFINE_FIELD(m_vecTraction, VTYPE_VECTOR3D)
END_DATAMAP()