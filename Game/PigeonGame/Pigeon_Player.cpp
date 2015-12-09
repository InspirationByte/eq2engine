//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: player-controlled pigeon
//////////////////////////////////////////////////////////////////////////////////

#include "GameInput.h"
#include "debugoverlay.h"
#include "Pigeon_Player.h"

// player FOV
ConVar g_viewFov("g_viewFov", "75", "Field of view", CV_CHEAT);

ConVar g_swingPigeonMod("g_swingPigeonMod", "0.12", "Swing modifier", CV_CHEAT);
ConVar g_swingReducePigeonMod("g_swingReducePigeonMod", "1.15", "Swing modifier", CV_CHEAT);
ConVar g_swingPlayerMod("g_swingPlayerMod", "0.1", "Swing modifier", CV_CHEAT);

ConVar g_tractionMouseFactor("g_tractionMouseFactor", "1.0", "Mouse factor for pigeon traction", CV_CHEAT);

ConVar m_invertFlyControls("m_invertFlyControls", "1", "Invert fly controller", CV_ARCHIVE);

CPlayerPigeon::CPlayerPigeon()
{
	m_vPunchAngleVelocity = vec3_zero;
	m_vPunchAngle = vec3_zero;
	m_vPunchAngle2 = vec3_zero;
	m_vRealEyeAngles = vec3_zero;
	m_vRealEyeOrigin = vec3_zero;
	m_fMass = 1.0f;

	m_vecFlyAngle = vec3_zero;

	m_fShakeDecayCurTime = 0;
	m_fShakeMagnitude = 0;
	m_vShake = vec3_zero;

	m_nMaxHealth = 100;
	m_nHealth = m_nMaxHealth;
	m_vHeadPos = Vector3D(0,20,15);
}

void CPlayerPigeon::Spawn()
{
	BaseClass::Spawn();

	PhysicsGetObject()->SetContents(COLLISION_GROUP_PLAYER);
	PhysicsGetObject()->SetCollisionMask(COLLIDE_PLAYER);

	pPigeonSwing = debugoverlay->Graph_AddBucket("pigeon 'swing'", ColorRGBA(0,1,1,1), 1.0f, 0.01f);
	pPlayerSwing = debugoverlay->Graph_AddBucket("player 'swing'", ColorRGBA(0,1,1,1), 2.0f, 0.01f);

	m_fPigeonSwingVelocity = 0.0f;
	m_fPlayerSwing = 0.0f;

	SetIKChainEnabled(m_nHeadIKChain, true);
	SetIKLocalTarget(m_nHeadIKChain, Vector3D(0,15,15));

}

void CPlayerPigeon::Activate()
{
	engine->SetCenterMouseCursor(true);

	UpdateView();

	BaseClass::Activate();
}

void CPlayerPigeon::Precache()
{
	BaseClass::Precache();
}

void CPlayerPigeon::OnRemove()
{
	debugoverlay->Graph_RemoveBucket(pPigeonSwing);
	debugoverlay->Graph_RemoveBucket(pPlayerSwing);

	BaseClass::OnRemove();
}

ConVar g_thirdPersonDist("g_thirdPersonDist", "80", "Enables thirdperson view", CV_CHEAT);
ConVar g_thirdPersonHeight("g_thirdPersonHeight", "12", "Enables thirdperson view", CV_CHEAT);

// entity view - computation for rendering
// use SetActiveViewEntity to set view from this entity
void CPlayerPigeon::CalcView(Vector3D &origin, Vector3D &angles, float &fov)
{
	fov = g_viewFov.GetFloat();

	Vector3D f,r,u;
	AngleVectors(m_vecEyeAngles, &f, &r, &u);

	IPhysicsObject* pIgnore = PhysicsGetObject();

	// TODO: apply external camera rotation since we've using freelook

	Vector3D targetDir = (f * -g_thirdPersonDist.GetFloat()) + Vector3D(0, g_thirdPersonHeight.GetFloat(), 0);

	internaltrace_t tr;
	physics->InternalTraceBox(m_vecEyeOrigin, m_vecEyeOrigin+targetDir, Vector3D(2), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr, &pIgnore);

	origin = m_vRealEyeOrigin + targetDir*tr.fraction + Vector3D(0, g_thirdPersonHeight.GetFloat(), 0);
	angles = m_vRealEyeAngles;
}

#define PUNCH_DAMPING		9.0f		// bigger number makes the response more damped, smaller is less damped
										// currently the system will overshoot, with larger damping values it won't
#define PUNCH_SPRING_CONSTANT	95.0f	// bigger number increases the speed at which the view corrects

void CPlayerPigeon::DecayPunchAngle()
{
	if ( dot(m_vPunchAngle, m_vPunchAngle) > 0.001 || dot(m_vPunchAngleVelocity, m_vPunchAngleVelocity) > 0.001 || dot(m_vPunchAngle2, m_vPunchAngle2) > 0.001 )
	{
		m_vPunchAngle2 -= m_vPunchAngle2 * gpGlobals->frametime * 8.0f;

		m_vPunchAngle += m_vPunchAngleVelocity * gpGlobals->frametime;
		float damping = 1 - (PUNCH_DAMPING * gpGlobals->frametime);
		
		if ( damping < 0 )
			damping = 0.0f;

		m_vPunchAngleVelocity *= damping;
		 
		// torsional spring
		float springForceMagnitude = PUNCH_SPRING_CONSTANT * gpGlobals->frametime;
		springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
		m_vPunchAngleVelocity -= m_vPunchAngle * springForceMagnitude;

		// don't wrap around
		m_vPunchAngle = Vector3D( 
			clamp(m_vPunchAngle.x, -89, 89 ), 
			clamp(m_vPunchAngle.y, -179, 179 ),
			clamp(m_vPunchAngle.z, -89, 89 ) );
	}
}

void CPlayerPigeon::ShakeView()
{
	if(m_fShakeDecayCurTime <= 0.01)
	{
		m_fShakeMagnitude = 0;
		return;
	}

	// maximum for 3 seconds shake, heavier is more
	float fShakeRate = m_fShakeDecayCurTime / 3.0f;

	m_fShakeMagnitude -= m_fShakeMagnitude*(1.0f-fShakeRate);

	m_fShakeDecayCurTime -= gpGlobals->frametime;

	m_vShake = 5*fShakeRate*Vector3D(sin(cos(m_fShakeMagnitude*fShakeRate+gpGlobals->curtime*44)), sin(cos(m_fShakeMagnitude*fShakeRate+gpGlobals->curtime*115)), sin(cos(m_fShakeMagnitude*fShakeRate+gpGlobals->curtime*77)));
}

void CPlayerPigeon::ViewPunch(Vector3D &angleOfs, bool bWeapon)
{
	if(bWeapon)
		m_vPunchAngle2 += angleOfs;
	else
		m_vPunchAngleVelocity += angleOfs * 30;
}

void CPlayerPigeon::ViewShake(float fMagnutude, float fTime)
{
	m_fShakeDecayCurTime += fTime;

	// clamp time to 5 seconds max
	if(m_fShakeDecayCurTime > 1.5f)
		m_fShakeDecayCurTime = 1.5f;

	m_fShakeMagnitude += fMagnutude;
}

void CPlayerPigeon::AliveThink()
{
	// TODO: Must be as control sender for active player
	m_nActorButtons = nClientButtons;

	int nGamePlayFlags = nGPFlags;

	if(this != g_pGameRules->GetLocalPlayer())
		nGamePlayFlags &= ~(GP_FLAG_NOCLIP | GP_FLAG_GOD);

	movementdata_t movedata;
	movedata.forward = 0.0f;
	movedata.right = 0.0f;

	//if(m_nActorButtons & IN_FORWARD)
	//	movedata.forward += 1.0f;

	movedata.forward = m_fPigeonSwingVelocity;

	// quickly jump out
	if(m_nActorButtons & IN_BACKWARD)
		movedata.forward -= 1.0f;

	Vector3D f, r, u;

	// TODO: compute from my angles, only keeping right vector
	EyeVectors(&f, &r, &u);

	// apply wing forces when pressing buttons
	if(m_nActorButtons & IN_WINGLEFT)
	{
		float fForce = 30.0f;
		float fSideDist = 5.0f;

		m_fFlyVelocity += 0.5f * gpGlobals->frametime;

		//m_vecTraction += Vector3D(u*fForce - r*fSideDist);
	}

	if(m_nActorButtons & IN_WINGRIGHT)
	{
		float fForce = 30.0f;
		float fSideDist = 5.0f;

		m_fFlyVelocity += 0.5f * gpGlobals->frametime;

		//m_vecTraction += Vector3D(u*fForce + r*fSideDist);
	}

	if(m_nCurPose < POSE_CINEMATIC)
	{
		if(m_nActorButtons & IN_DUCK)
			SetPose( POSE_CROUCH, 0.3f);
	}

	//length(m_vecAbsVelocity.xz())*g_swingPigeonMod.GetFloat()
	//m_fPigeonSwingVelocity = cos(gpGlobals->curtime * g_swingPigeonMod.GetFloat());

	m_fPigeonSwingVelocity -= g_swingReducePigeonMod.GetFloat()*gpGlobals->frametime;
	m_fPigeonSwingVelocity = clamp(m_fPigeonSwingVelocity, 0, 1);

	debugoverlay->Graph_AddValue(pPigeonSwing, m_fPigeonSwingVelocity);

	ProcessMovement( movedata );

	BaseClass::AliveThink();
}

void CPlayerPigeon::DeathThink()
{
	BaseClass::DeathThink();
}

void CPlayerPigeon::UpdateView()
{
	BaseClass::UpdateView();

	// update and decay the punch angle
	DecayPunchAngle();

	ShakeView();

	if(IsAlive())
	{
		MoveType_e type = GetMoveType();

		Vector3D cam_angle_target = m_vecEyeAngles;
		
		if( type == MOVE_TYPE_FLY )
		{
			if(!(m_nActorButtons & IN_SECONDARY))
				cam_angle_target = Vector3D(m_vecFlyAngle.x,m_vecFlyAngle.y,m_vecFlyAngle.z*0.25f);
			else
				cam_angle_target.z = 0.0f;
		}
		else if(type == MOVE_TYPE_WALK)
		{
			if(!(m_nActorButtons & IN_SECONDARY))
				cam_angle_target.x = 0.0f;

			cam_angle_target.z = 0.0f;
		}

		Vector3D angleDiff = AnglesDiff(NormalizeAngles180(cam_angle_target), NormalizeAngles180(m_vecEyeAngles));

		m_vecEyeAngles -= angleDiff*gpGlobals->frametime*5.65f;

		m_vecFlyAngle.z -= m_vecFlyAngle.z*gpGlobals->frametime*1.5f;
	}

	Vector3D f,r,u;
	AngleVectors(m_vecEyeAngles, &f,&r,&u);

	// compute real eye position
	IPhysicsObject* pIgnore = PhysicsGetObject();
	internaltrace_t trEyePos;
	physics->InternalTraceBox(m_vecAbsOrigin, m_vecEyeOrigin, Vector3D(2), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &trEyePos, &pIgnore);

	Vector3D eye_pos = lerp(m_vecAbsOrigin, m_vecEyeOrigin, trEyePos.fraction);

	m_vRealEyeAngles = (m_vecEyeAngles - m_vPunchAngle - m_vPunchAngle2 + m_vShake*0.75f);
	m_vRealEyeOrigin = eye_pos +  m_vShake;

	SetIKLocalTarget(m_nHeadIKChain, m_vHeadPos);
}

Vector3D CPlayerPigeon::GetEyeAngles()
{
	return m_vRealEyeAngles;
}

Vector3D CPlayerPigeon::GetEyeOrigin()
{
	return m_vRealEyeOrigin;
}

void CPlayerPigeon::MouseMoveInput(float x, float y)
{
	// TODO: allow rotate death camera

	float flyY = y;

	if(m_invertFlyControls.GetBool())
		flyY = -y;

	// do freelook
	if(IsDead() || (m_nActorButtons & IN_SECONDARY))
	{
		m_vecEyeAngles.y += x;
		m_vecEyeAngles.x += y;

		m_vHeadPos.z += y;

		return;
	}

	if(GetMoveType() == MOVE_TYPE_FLY)
	{
		if(m_nActorButtons & IN_FASTSTEER)
		{
			// add more power to the steering
			m_vecFlyAngle.y += x;
			m_vecFlyAngle.x += y;
		}
		else
		{
			Matrix4x4 angleAdded = rotateZ(DEG2RAD(x)) * rotateX(DEG2RAD(flyY));

			// this is very odd that you should input negative Y angle
			Matrix4x4 angleCurrent = rotateZXY(DEG2RAD(m_vecFlyAngle.x), DEG2RAD(-m_vecFlyAngle.y), DEG2RAD(m_vecFlyAngle.z));

			Matrix4x4 rotation = angleAdded*angleCurrent;

			m_vecFlyAngle = EulerMatrixZXY(transpose(rotation));
			m_vecFlyAngle = NormalizeAngles180(VRAD2DEG(m_vecFlyAngle));
		}

		m_vecFlyAngle.x = clamp(m_vecFlyAngle.x, -80, 80);
		m_vecFlyAngle.z = clamp(m_vecFlyAngle.z, -80, 80);
	}
	else
	{
		m_vecEyeAngles.y += x;

		// Y is accelerator

		float fPrevSwing = m_fPlayerSwing;

		m_fPlayerSwing -= y*g_swingPlayerMod.GetFloat();
		m_fPlayerSwing = clamp(m_fPlayerSwing, -1, 1);

		float fSwingDiff = fabs(m_fPlayerSwing-fPrevSwing);

		m_fPigeonSwingVelocity += fSwingDiff*g_swingPigeonMod.GetFloat();

		debugoverlay->Graph_AddValue(pPlayerSwing, m_fPlayerSwing+1.0f);
	}
}

BEGIN_DATAMAP(CPlayerPigeon)
	DEFINE_FIELD( m_vPunchAngleVelocity, VTYPE_ANGLES),
	DEFINE_FIELD( m_vPunchAngle, VTYPE_ANGLES),
	DEFINE_FIELD( m_vRealEyeAngles, VTYPE_ANGLES),
	DEFINE_FIELD( m_vPunchAngle2, VTYPE_ANGLES),

	DEFINE_FIELD( m_vShake, VTYPE_VECTOR3D),
	DEFINE_FIELD( m_fShakeDecayCurTime, VTYPE_FLOAT),
	DEFINE_FIELD( m_fShakeMagnitude, VTYPE_FLOAT),
END_DATAMAP()

DECLARE_ENTITYFACTORY(player, CPlayerPigeon)