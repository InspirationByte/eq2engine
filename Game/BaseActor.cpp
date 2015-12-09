//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base actor class
//////////////////////////////////////////////////////////////////////////////////

#include "GameInput.h"
#include "BaseActor.h"
#include "BaseWeapon.h"
#include "Effects.h"

extern int nGPFlags;
extern int nGPLastFlags;

#define ACTOR_STEP_HEIGHT				50.0f
#define ACTOR_STEP_LENGTH				60.0f
#define ACTOR_FOOT_SIZE					Vector3D(8,8,8)
#define ACTOR_STEPUP_NORMAL_EPSILON		0.85f
#define ACTOR_WALK_NORMAL_EPSILON		0.35f

#ifdef PIGEONGAME
#define ACTOR_CAPSULE_SIZE				(8.0f)
#else
#define ACTOR_CAPSULE_SIZE				(22.0f)
#endif

#define USE_DISTANCE					(64.0f)

ConVar g_noclip_speed("g_noclip_speed","800","Editor camera speed",CV_ARCHIVE);
ConVar g_ladder_speed("g_ladder_speed","90","Editor camera speed",CV_CHEAT);

// declare data table for actor
BEGIN_DATAMAP( CBaseActor )

	DEFINE_FIELD( m_nPose, VTYPE_INTEGER),
	DEFINE_FIELD( m_nCurPose, VTYPE_INTEGER),

	DEFINE_FIELD( m_fMaxSpeed, VTYPE_FLOAT),
	DEFINE_FIELD( m_fNextPoseChangeTime, VTYPE_TIME),
	DEFINE_FIELD( m_fNextPoseTime, VTYPE_TIME),
	DEFINE_FIELD( m_fNextStepTime, VTYPE_TIME),
	
	DEFINE_FIELD( m_fEyeHeight, VTYPE_FLOAT),
	

	DEFINE_FIELD( m_fPainPercent, VTYPE_FLOAT),

	DEFINE_FIELD( m_vecEyeOrigin, VTYPE_ORIGIN),
	DEFINE_FIELD( m_vecEyeAngles, VTYPE_ANGLES),
	DEFINE_FIELD( m_vPrevPhysicsVelocity, VTYPE_VECTOR3D),
	DEFINE_FIELD( m_vMoveDir, VTYPE_VECTOR3D),

	DEFINE_FIELD( m_bOnGround, VTYPE_BOOLEAN),
	DEFINE_FIELD( m_bLastOnGround, VTYPE_BOOLEAN),
	

	DEFINE_LISTFIELD(m_pEquipment, VTYPE_ENTITYPTR),
	DEFINE_ARRAYFIELD(m_pEquipmentSlots, VTYPE_ENTITYPTR, WEAPONSLOT_COUNT),
	DEFINE_ARRAYFIELD(m_nClipCount, VTYPE_INTEGER, AMMOTYPE_MAX),
	DEFINE_FIELD( m_nCurrentSlot, VTYPE_INTEGER),

	DEFINE_FIELD(m_pLadder, VTYPE_ENTITYPTR),
	
	DEFINE_FIELD(m_pActiveWeapon, VTYPE_ENTITYPTR),

	DEFINE_THINKFUNC(AliveThink),
	DEFINE_THINKFUNC(DeathThink),

	// DEFINE_RAGDOLL(m_pRagdoll),

END_DATAMAP()

CBaseActor::CBaseActor() : BaseClass()
{
	m_nPose					= POSE_STAND;
	m_nCurPose				= POSE_STAND;

	m_pRagdoll				= NULL;

	m_fMaxSpeed				= MIN_SPEED;

	m_pLastDamager			= NULL;
	m_vecEyeAngles			= vec3_zero;
	m_fMass					= 60;
	m_bOnGround				= true;
	m_bLastOnGround			= true;
	m_fPainPercent			= 0.0f;
	m_nActorButtons			= 0;
	m_nOldActorButtons		= 0;
	m_fJumpTime				= 0.0f;
	m_pCurrentMaterial		= NULL;
	m_fNextStepTime			= 0.0f;
	m_fJumpPower			= 220;
	m_fJumpMoveDirPower		= 209;
	m_fMaxFallingVelocity	= 0.0f;

	m_vMoveDir				= vec3_zero;
	m_vPrevPhysicsVelocity	= vec3_zero;

	memset(m_pEquipmentSlots,NULL,sizeof(m_pEquipmentSlots));
	memset(m_nClipCount, 0, sizeof(m_nClipCount));
	m_nCurrentSlot			= -1;

	m_pActiveWeapon			= NULL;

	m_pLadder				= NULL;
	m_fNextPoseChangeTime	= 0;
	m_fNextPoseTime			= 0;

	// actor height control
	m_fActorHeight			= DEFAULTACTOR_HEIGHT;
	m_fEyeHeight			= DEFAULT_ACTOR_EYE_HEIGHT;
}

CBaseActor::~CBaseActor()
{
	
}

void CBaseActor::Spawn()
{
	PhysicsCreateObjects();

	SetNextThink(gpGlobals->curtime);
	SetThink(&CBaseActor::AliveThink);

	m_nHealth = m_nMaxHealth;

	BaseClass::Spawn();

	m_pPhysicsObject->SetPosition( GetAbsOrigin() );
}

void CBaseActor::Precache()
{
	PrecacheScriptSound("DefaultActor.Fraction");
	 
	BaseClass::Precache();
}

void CBaseActor::PhysicsCreateObjects()
{
	if(m_pPhysicsObject)
		return;

	// first non-moveable primitive
	pritimiveinfo_t halfActor;
	halfActor.primType = PHYSPRIM_CAPSULE;

	halfActor.capsuleInfo.radius = ACTOR_CAPSULE_SIZE;
	halfActor.capsuleInfo.height = PART_CAPSULE_SIZE(m_fActorHeight);

	// add shapes to physics engine
	int actor_half_shape = physics->AddPrimitiveShape(halfActor);

	/*
	physobjectdata_t actor_object_info;
	actor_object_info.object.mass = m_fMass;
	actor_object_info.object.mass_center = vec3_zero;
	actor_object_info.object.offset = vec3_zero;
	actor_object_info.object.numShapes = 2;
	actor_object_info.cached_shape_indexes[0] = actor_half_shape;
	actor_object_info.cached_shape_indexes[1] = actor_half_shape;

	actor_object_info.object.surfaceprops[0] = '\0';
	strcpy(actor_object_info.object.surfaceprops, "body");

	PhysicsInitCustom(COLLISION_GROUP_ACTORS, false, &actor_object_info);
	*/

	int shape_idxs[2] = {actor_half_shape, actor_half_shape};

	PhysicsInitCustom(COLLISION_GROUP_ACTORS, false, 2, shape_idxs, "body", m_fMass);

	if(!m_pPhysicsObject)
		MsgError("ACTOR ERROR: Failed to create physics object\n");
	else
	{
		m_pPhysicsObject->SetCollisionMask( COLLIDE_ACTOR );

		m_pPhysicsObject->SetChildShapeTransform(0, Vector3D(0, -PART_HEIGHT_OFFSET(m_fActorHeight), 0), vec3_zero);
		m_pPhysicsObject->SetChildShapeTransform(1, Vector3D(0, PART_HEIGHT_OFFSET(m_fActorHeight), 0), vec3_zero);

		m_pPhysicsObject->SetPosition(GetAbsOrigin());

		m_pPhysicsObject->SetSleepTheresholds(0,0);
		m_pPhysicsObject->SetAngularFactor(Vector3D(0,1,0));
		m_pPhysicsObject->SetFriction(0.00001f);
		m_pPhysicsObject->SetRestitution(1.0f);
		m_pPhysicsObject->SetDamping(0.0001f,1.0f);

		m_pPhysicsObject->WakeUp();
	}

	// make ragdoll
	m_pRagdoll = CreateRagdoll(m_pModel);
	if(m_pRagdoll)
	{
		for(int i = 0; i< m_pRagdoll->m_numParts; i++)
		{
			if(m_pRagdoll->m_pParts[i])
			{
				m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_RAGDOLLBONES );
				m_pRagdoll->m_pParts[i]->SetEntityObjectPtr( this );
				m_pRagdoll->m_pParts[i]->SetActivationState( PS_FROZEN );
				m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( false );
			}
		}
	}
}

void CBaseActor::FirstUpdate()
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->SetActivationState(PS_ACTIVE);
}

Vector3D CBaseActor::GetEyeOrigin()
{
	return m_vecEyeOrigin;
}

Vector3D CBaseActor::GetEyeAngles()
{
	return m_vecEyeAngles;
}

void CBaseActor::EyeVectors(Vector3D* forward, Vector3D* right, Vector3D* up )
{
	AngleVectors(m_vecEyeAngles, forward, right, up);
}

// entity view - computation for rendering
// use SetActiveViewEntity to set view from this entity
void CBaseActor::CalcView(Vector3D &origin, Vector3D &angles, float &fov)
{
	origin = m_vecEyeOrigin;
	angles = m_vecEyeAngles;
	fov = 90;
}

void CBaseActor::SnapEyeAngles(const Vector3D &angles)
{
	m_vecEyeAngles = angles;
	m_vecEyeAngles.z = 0;
}

void CBaseActor::OnRemove()
{
	for(int i = 0; i < m_pEquipment.numElem(); i++)
	{
		m_pEquipment[i]->SetOwner(NULL);
	}

	if(m_pRagdoll)
	{
		DestroyRagdoll(m_pRagdoll);
	}

	BaseClass::OnRemove();
}

void CBaseActor::UpdateView()
{
	if(m_nCurPose == POSE_STAND)
	{
		float target_dist = fabs(DEFAULT_ACTOR_EYE_HEIGHT-m_fEyeHeight);

		m_fEyeHeight += clamp(target_dist*CROUCH_STAND_ACCEL, 0, CROUCH_STAND_SPEED)*gpGlobals->frametime;

		if(m_fEyeHeight > DEFAULT_ACTOR_EYE_HEIGHT)
		{
			m_fEyeHeight = DEFAULT_ACTOR_EYE_HEIGHT;
		}
	}
	else if(m_nCurPose == POSE_CROUCH || m_nCurPose == POSE_CINEMATIC_LADDER)
	{
		float height = DEFAULT_ACTOR_EYE_HEIGHT-(m_fActorHeight*0.5f);

		float target_dist = fabs(height-m_fEyeHeight);

		m_fEyeHeight -= clamp(target_dist*CROUCH_STAND_ACCEL, 0, CROUCH_STAND_SPEED)*gpGlobals->frametime;

		if(m_fEyeHeight < height)
		{
			m_fEyeHeight = height;
		}
	}

	if(m_nCurPose != POSE_CINEMATIC)
		m_vecEyeOrigin = GetAbsOrigin() + Vector3D(0,m_fEyeHeight/2,0);
}

void CBaseActor::OnPreRender()
{
	//UpdateView();

	if(m_pModel)
		g_modelList->AddRenderable(this);
}

void CBaseActor::OnPostRender()
{

}

void CBaseActor::Update(float decaytime)
{
	if(m_pPhysicsObject)
		m_vPrevPhysicsVelocity = m_pPhysicsObject->GetVelocity();

	// update base class (for thinking)
	BaseClass::Update(decaytime);

	UpdateView();

	static float lastPlaceTime = 0;

	NormalizeAngles360(m_vecEyeAngles);

	nGPLastFlags = 0;

	// movement isn't processed by CBaseActor - it does AI or actor

	m_nOldActorButtons = m_nActorButtons;

	if(IsDead() && m_pRagdoll)
	{
		for(int i = 0; i< m_pRagdoll->m_numParts; i++)
		{
			if(m_pRagdoll->m_pParts[i])
				ObjectSoundEvents(m_pRagdoll->m_pParts[i], this);
		}
	}
}

Vector3D CBaseActor::GetFootOrigin()
{
	Vector3D offset = Vector3D( 0, -m_vBBox.GetSize().y*0.5f, 0);
	return GetAbsOrigin() + offset;
}

// updates parent transformation
void CBaseActor::UpdateTransform()
{
	if(m_pRagdoll && IsDead())
	{
		m_pRagdoll->GetVisualBonesTransforms( m_BoneMatrixList );

		SetAbsOrigin( m_pRagdoll->GetPosition() );
		SetAbsAngles( vec3_zero );

		BaseClass::UpdateTransform();
	}
	else
	{
		UpdateBones();

		if(m_pRagdoll)
			m_pRagdoll->SetBoneTransform( m_BoneMatrixList, transpose(m_matWorldTransform) );

		BaseClass::UpdateTransform();

		Vector3D offset = Vector3D( 0, -m_vBBox.GetSize().y*0.5f, 0);
		m_matWorldTransform = translate(offset) * m_matWorldTransform;
	}
}

ConVar g_autojump("g_autojump","1","Enable autojump?",CV_CHEAT);
ConVar g_autojump_powerscale("g_autojump_powerscale","1.2","Autojump power scale",CV_CHEAT);


void CBaseActor::SetLadderEntity(CInfoLadderPoint* pLadder)
{
	m_pLadder = pLadder;
}

void CBaseActor::SetPose(int nPose, float fNextTimeToChange)
{
	if(m_nPose == nPose)
		return;

	m_nPose = nPose;

	if(nPose >= POSE_CINEMATIC)
		m_nCurPose = nPose;

	m_fNextPoseTime = fNextTimeToChange;
}

MoveType_e CBaseActor::GetMoveType()
{
	int nGamePlayFlags = nGPFlags;

	if(this != g_pGameRules->GetLocalPlayer())
		nGamePlayFlags &= ~(GP_FLAG_NOCLIP | GP_FLAG_GOD);

	MoveType_e move_type = MOVE_TYPE_WALK;

	if(m_pLadder && (m_nCurPose == POSE_CINEMATIC_LADDER))
		move_type = MOVE_TYPE_LADDER;

	if(nGamePlayFlags & GP_FLAG_NOCLIP)
		move_type = MOVE_TYPE_NOCLIP;

	return move_type;
}

ConVar g_player_accel("g_player_accel", "8.0f");
ConVar g_player_deccel("g_player_deccel", "8.0f");
ConVar g_autojump_deccel("g_autojump_deccel", "2.0f");

void CBaseActor::ProcessMovement( movementdata_t& data )
{
	if(!m_pPhysicsObject)
		return;

	if(m_nCurPose != m_nPose && m_fNextPoseChangeTime < gpGlobals->curtime)
	{
		m_fNextPoseChangeTime = gpGlobals->curtime + m_fNextPoseTime;
		m_nCurPose = m_nPose;
	}

	MoveType_e move_type = GetMoveType();

	BaseEntity* pActorIgnore = this;

	m_bLastOnGround = m_bOnGround;
	m_bOnGround = false;

	Matrix4x4 physics_transform = m_pPhysicsObject->GetTransformMatrix();

	BoundingBox bbox;
	m_pPhysicsObject->GetAABB(bbox.minPoint, bbox.maxPoint);

	m_pPhysicsObject->WakeUp();

	if(!m_pLadder && m_nCurPose == POSE_CINEMATIC_LADDER)
	{
		m_nCurPose = POSE_CROUCH;
		m_fNextPoseChangeTime = gpGlobals->curtime + 0.25f;
		m_nPose = POSE_STAND;
	}

	m_vMoveDir = vec3_zero;

	if(move_type == MOVE_TYPE_WALK)
	{
		m_pPhysicsObject->SetCollisionMask(COLLIDE_ACTOR);

		Vector3D velocity = m_pPhysicsObject->GetVelocity();

		Vector3D physObjectUpVec = physics_transform.rows[1].xyz();
		physObjectUpVec.x *= -1;

		float box_size_mult = 1.0f;

		if(m_vPrevPhysicsVelocity.y < MIN_FALLDMG_SPEED)
			box_size_mult += fabs(m_vPrevPhysicsVelocity.y*0.05f);

		Vector3D physMins, physMaxs;
		m_pPhysicsObject->GetAABB(physMins, physMaxs);

		Vector3D physSize = (physMaxs-physMins)*0.5f;

		// do first trace
		Vector3D box_size(physSize.x*0.75f,6+box_size_mult,physSize.z*0.75f);

		Vector3D player_bottom = Vector3D(GetAbsOrigin().x, bbox.minPoint.y + box_size.y*0.5f,GetAbsOrigin().z);

		trace_t ground_trace;
		UTIL_TraceBox( player_bottom + Vector3D(0.0f,5,0.0f), player_bottom, box_size, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &ground_trace, &pActorIgnore, 1 );

		// shound trace twice to detect difference
		float fGroundSphere = physSize.x*0.25;

		trace_t ground_trace2;
		UTIL_TraceSphere( player_bottom + Vector3D(0.0f,5,0.0f), player_bottom, fGroundSphere, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &ground_trace2, &pActorIgnore, 1 );

		// set onground state
		m_bOnGround = (ground_trace.fraction < 1.0f);

		// display box
		debugoverlay->Box3D(ground_trace.traceEnd - box_size, ground_trace.traceEnd + box_size, Vector4D(1,m_bOnGround,0,1));

		float decel_scale = 1.0f;

		bool bDoFootsteps = m_bOnGround;

		if(m_bOnGround)
		{
			// surface slope
			float fSurfaceNormalDot = dot(ground_trace.normal/*normalize(ground_trace.normal + ground_trace2.normal)*/, vec3_up);

			//debugoverlay->Text3D(ground_trace.traceEnd, ColorRGBA(1,1,1,1), "fSurfaceNormalDot=%g", fSurfaceNormalDot);

			// set current material
			m_pCurrentMaterial = ground_trace.hitmaterial;

			// Compute movement vectors
			Vector3D forward;
			Vector3D right;

			AngleVectors(Vector3D(0, m_vecEyeAngles.y, 0),&forward,&right,NULL);

			bool bMovementPressed = false;

			// check movement pressed (the deadzone applyed for joystick usage)
			if(fabs(data.forward) > 0.01f || fabs(data.right) > 0.01f)
			{
				bMovementPressed = true;

				// perform normalized balancing, it saves controller affection
				Vector2D dirBalanced = balance(Vector2D(data.forward, data.right));

				m_vMoveDir = (forward*dirBalanced.x + right*dirBalanced.y);
			}

			float fPlayerSpeed = length(velocity.xz());

			if(bMovementPressed)
			{
				trace_t compare_box;
				UTIL_TraceSphere(GetAbsOrigin(), GetAbsOrigin() - Vector3D(0.0f, ACTOR_STEP_HEIGHT+6.0f, 0.0f), 8, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &compare_box, &pActorIgnore, 1);

				float comp_to_up_dot = dot(compare_box.normal, Vector3D(0,1,0));

				//float fSurfaceToUp = dot(ground_trace.normal, Vector3D(0,1,0));

				//if(fSurfaceToUp < 0.2f)
				//	bForceMove = true;

				debugoverlay->Box3D(compare_box.traceEnd + -ACTOR_FOOT_SIZE,compare_box.traceEnd + ACTOR_FOOT_SIZE, Vector4D(0,1,0,1));

				// perform autojump if any movement buttons was pressed
				if(g_autojump.GetBool() && (m_pPhysicsObject->GetVelocity().y > -80) && (fPlayerSpeed > 50 && m_bOnGround) && comp_to_up_dot > 0.2f)
				{
					float step_length = ACTOR_STEP_LENGTH;

					Vector3D actorStepPos = GetAbsOrigin() + m_vMoveDir * step_length;

					trace_t autojumpbox;
					UTIL_TraceSphere(GetAbsOrigin(), actorStepPos - Vector3D(0.0f, ACTOR_STEP_HEIGHT+6.0f, 0.0f), 8, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &autojumpbox, &pActorIgnore, 1);

					float surface_to_up_dot = dot(autojumpbox.normal, Vector3D(0,1,0));

					debugoverlay->Box3D(autojumpbox.traceEnd + -ACTOR_FOOT_SIZE,autojumpbox.traceEnd + Vector3D(16,32,16), Vector4D(1,1,0,1));
					debugoverlay->Box3D(autojumpbox.traceEnd + -ACTOR_FOOT_SIZE,autojumpbox.traceEnd + ACTOR_FOOT_SIZE, Vector4D(1,0,0,1));
					//debugoverlay->Text3D(autojumpbox.traceEnd, Vector4D(1,1,1,1), "surface dot: %f", dot(autojumpbox.normal, Vector3D(0,1,0)));

					float comp = (autojumpbox.traceEnd.y - compare_box.traceEnd.y);

					// second for ideal surface
					if(surface_to_up_dot > ACTOR_STEPUP_NORMAL_EPSILON)
					{
						if(comp > 4 && comp < 32)
						{
							m_bOnGround = true;

							Vector3D jumpVel(0,comp * g_autojump_powerscale.GetFloat(),0);

							decel_scale = g_autojump_deccel.GetFloat();

							velocity += jumpVel;
						}
					}
				}

				Vector3D move_velocity = m_vMoveDir*m_fMaxSpeed;

				if(length(m_vecAbsVelocity) < m_fMaxSpeed)
					m_vecAbsVelocity += move_velocity * g_player_accel.GetFloat() * gpGlobals->frametime;
			}

			// do friction movement if actor standing on slopes
			if(fSurfaceNormalDot > 0.8f)
			{
				velocity.x = 0.0f;
				velocity.z = 0.0f;
			}
			else
			{
				decel_scale = 10.0f;
				m_vecAbsVelocity *= 0.1f;
				bDoFootsteps = false;
			}

			// handle jumping
			if((m_nActorButtons & IN_JUMP) && !(m_nOldActorButtons & IN_JUMP) && m_bOnGround && (m_fJumpTime < gpGlobals->curtime))
			{
				Vector3D jumpVel = Vector3D(0,m_fJumpPower,0);

				m_fJumpTime = gpGlobals->curtime + 0.5f;

				// apply impulse to ground object
				velocity += jumpVel;

				m_fMaxFallingVelocity = 0.0f;
				m_bOnGround = false;
			}

			m_vecAbsVelocity.y = 0.0f;

			m_pPhysicsObject->SetVelocity( velocity + m_vecAbsVelocity );

			if(length(m_vecAbsVelocity.xz()) > 0.1f)
				m_pPhysicsObject->SetLinearFactor(Vector3D(1,1,1));
			else
				m_pPhysicsObject->SetLinearFactor(Vector3D(0.0f,1,0.0f));

			m_vecAbsVelocity -= m_vecAbsVelocity*gpGlobals->frametime*g_player_deccel.GetFloat() * decel_scale;
		}
		else
			m_vecAbsVelocity = m_pPhysicsObject->GetVelocity();

		CheckLanding();

		// handle footsteps TODO: make from model IK attacments
		if(bDoFootsteps && 
			m_fNextStepTime < gpGlobals->curtime && 
			length(m_vPrevPhysicsVelocity.xz()) > 50)
		{
			float vol_perc = clamp(length(m_vPrevPhysicsVelocity.xz()) / 250,0,1);

			if(m_pCurrentMaterial)
				PlayStepSound(m_pCurrentMaterial->footStepSound, vol_perc);
			else
				PlayStepSound("physics.footstep", vol_perc);

			m_fNextStepTime = gpGlobals->curtime + 0.28f * (2.0-(length(m_vPrevPhysicsVelocity.xz()) / 360));
		}
	}
	else if(move_type == MOVE_TYPE_LADDER)
	{
		Vector3D velocity = m_pPhysicsObject->GetVelocity();

		Vector3D physObjectUpVec = physics_transform.rows[1].xyz();
		physObjectUpVec.x *= -1;

		float box_size_mult = 1.0f;

		if(m_vPrevPhysicsVelocity.y < MIN_FALLDMG_SPEED)
			box_size_mult += fabs(m_vPrevPhysicsVelocity.y*0.05f);

		Vector3D box_size(8,20,8);

		Vector3D player_bottom = Vector3D(GetAbsOrigin().x, bbox.minPoint.y + box_size.y*0.5f,GetAbsOrigin().z);

		Matrix4x4 ladder_endpoint_rot = !rotateXYZ4(DEG2RAD(m_pLadder->GetLadderDestAngles().x),DEG2RAD(m_pLadder->GetLadderDestAngles().y),DEG2RAD(m_pLadder->GetLadderDestAngles().z));
		Vector3D lforward = ladder_endpoint_rot.rows[2].xyz();
		
		Vector3D ladderVec = normalize(m_pLadder->GetLadderDestPoint() - m_pLadder->GetAbsOrigin());

		//if(fDotSignMoveDir > 0)
		Vector3D offs = lforward*32.0f;

		box_size = Vector3D(8,20,8);

		trace_t ground_trace;
		UTIL_TraceBox( player_bottom + Vector3D(0.0f,5,0.0f), player_bottom + offs, box_size, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PLAYERCLIP, &ground_trace, &pActorIgnore, 1 );

		m_bOnGround = (ground_trace.fraction < 1.0f);

		debugoverlay->Box3D(ground_trace.traceEnd - box_size, ground_trace.traceEnd + box_size, Vector4D(1,m_bOnGround,0,1));

		m_pPhysicsObject->SetChildShapeTransform(1, Vector3D(0, -PART_HEIGHT_OFFSET(m_fActorHeight), 0), vec3_zero);

		bool bMovementPressed = false;

		if(fabs(data.forward) > 0.05f || fabs(data.right) > 0.05f)
			bMovementPressed = true;

		if(m_nActorButtons & IN_JUMP)
		{
			m_pLadder = NULL;

			Vector3D forward;
			AngleVectors(GetEyeAngles(), &forward);

			forward *= m_fMaxSpeed;

			m_vecAbsVelocity = forward;

			m_pPhysicsObject->SetVelocity( m_vecAbsVelocity );

			m_nCurPose = POSE_STAND;
			m_fNextPoseChangeTime = gpGlobals->curtime + 0.2f;
			m_nPose = POSE_STAND;

			m_bOnGround = false;

			m_fMaxFallingVelocity = 0.0f;
			return;
		}

		// if we at the end of ladder, set it's pointer and start from crouching
		if(m_pLadder)
		{
			m_pPhysicsObject->SetCollisionMask(COLLIDE_ACTOR);

			Vector3D ladderVec = normalize(m_pLadder->GetLadderDestPoint() - m_pLadder->GetAbsOrigin());

			// check movement pressed (the deadzone applyed for joystick usage)
			if(fabs(data.forward) > 0.01f)
			{
				m_vMoveDir = ladderVec*data.forward;
			}

			float intrp = lineProjection(m_pLadder->GetAbsOrigin(), m_pLadder->GetLadderDestPoint(), GetAbsOrigin());

			// getting out from ladder
			if((intrp < 0.0f || intrp > 1.0f) && ground_trace.fraction < 1.0f && dot(ground_trace.normal, Vector3D(0,1,0)) > 0.5f)
			{
				m_pLadder = NULL;

				Vector3D forward;
				AngleVectors(Vector3D(0,GetEyeAngles().y, 0), &forward);

				m_vecAbsVelocity = forward*m_fMaxSpeed;

				m_pPhysicsObject->SetVelocity(m_vecAbsVelocity);

				m_fMaxFallingVelocity = 0.0f;
				return;
			}

			if(!bMovementPressed)
			{
				m_pPhysicsObject->SetPosition(GetAbsOrigin());
				m_pPhysicsObject->SetVelocity(vec3_zero);
			}
			else
			{
				m_vMoveDir *= g_ladder_speed.GetFloat();

				if(data.forward < 0 && (m_nActorButtons & IN_FORCERUN))
				{
					m_vMoveDir.y = m_pPhysicsObject->GetVelocity().y;
					if(m_vMoveDir.y < -450)
						m_vMoveDir.y = -450;
				}
				else
				{
					// handle footsteps
					if(m_fNextStepTime < gpGlobals->curtime && fabs(m_vMoveDir.y) > 50 && m_bOnGround)
					{
						PlayStepSound("physics.footstepmetal", 0.25f);

						m_fNextStepTime = gpGlobals->curtime + 0.43f;
					}
				}


				m_pPhysicsObject->SetVelocity(m_vMoveDir);
				SetAbsOrigin( m_pPhysicsObject->GetPosition() );
			}
		}
	}
	else if(move_type == MOVE_TYPE_NOCLIP)
	{
		m_pPhysicsObject->SetCollisionMask(0);

		m_fMaxFallingVelocity = 0.0f;

		// Compute movement vectors
		Vector3D forward;
		Vector3D right;

		AngleVectors(m_vecEyeAngles,&forward,&right,NULL);

		bool bMovementPressed = false;

		// check movement pressed (the deadzone applyed for joystick usage)
		if(fabs(data.forward) > 0.01f || fabs(data.right) > 0.01f)
		{
			bMovementPressed = true;

			// perform normalized balancing, it saves controller affection
			Vector2D dirBalanced = balance(Vector2D(data.forward, data.right));

			m_vMoveDir = (forward*dirBalanced.x + right*dirBalanced.y);

			if(length(m_vecAbsVelocity) < g_noclip_speed.GetFloat())
				m_vecAbsVelocity += m_vMoveDir * g_noclip_speed.GetFloat() * g_player_accel.GetFloat() * gpGlobals->frametime;
		}

		m_vecAbsOrigin += m_vecAbsVelocity * gpGlobals->frametime;

		m_pPhysicsObject->SetVelocity(GetAbsVelocity());
		m_pPhysicsObject->SetPosition(GetAbsOrigin());

		m_vecAbsVelocity -= m_vecAbsVelocity*gpGlobals->frametime*g_player_deccel.GetFloat();
	}

	// crouching handling
	if(m_nCurPose == POSE_CROUCH)
	{
		m_pPhysicsObject->SetChildShapeTransform(1, Vector3D(0, -PART_HEIGHT_OFFSET(m_fActorHeight), 0), vec3_zero);

		if(!(m_nActorButtons & IN_DUCK))
		{
			// check the actor can stand up here
			trace_t crouchRay;
			UTIL_TraceBox(GetAbsOrigin(),GetAbsOrigin()+Vector3D(0,PART_CAPSULE_SIZE(m_fActorHeight),0), Vector3D(20,PART_CAPSULE_SIZE(m_fActorHeight),20), COLLIDE_ACTOR, &crouchRay, &pActorIgnore, 1);

			if(crouchRay.fraction == 1.0f)
				SetPose(POSE_STAND, 0.3f);
		}
	}
	else
		m_pPhysicsObject->SetChildShapeTransform(1, Vector3D(0, PART_HEIGHT_OFFSET(m_fActorHeight), 0), vec3_zero);

	BaseClass::SetAbsOrigin( m_pPhysicsObject->GetPosition() );

	if(m_bOnGround)
		m_fMaxFallingVelocity = 0.0f;
}

void CBaseActor::PlayStepSound(const char* soundname, float fVol)
{
	EmitSound_t ep;
	ep.fPitch = 1.0;
	ep.fRadiusMultiplier = 0.8f;
	ep.fVolume = fVol;
	ep.origin = Vector3D(0,-1,0);

	ep.name = (char*)soundname;

	// attach this sound to entity
	ep.pObject = this;

	EmitSoundWithParams(&ep);
}

void CBaseActor::CheckLanding()
{
	float fCurrentYawSpeed = -m_vPrevPhysicsVelocity.y;
	if(fCurrentYawSpeed < 0)
		fCurrentYawSpeed = 0.0f;

	if(fCurrentYawSpeed > m_fMaxFallingVelocity)
		m_fMaxFallingVelocity = fCurrentYawSpeed;

	if(!m_bLastOnGround && m_bOnGround && m_fMaxFallingVelocity > MIN_FALLDMG_SPEED)
	{
		float fFactor = clamp(fabs(m_fMaxFallingVelocity-MIN_FALLDMG_SPEED) / 250,0,10000);

		
		int dmg = 100*fFactor;

		if(dmg > 0)
		{
			// damage event
			PlayStepSound("DefaultActor.Fraction", 1.0f);

			damageInfo_t info;
			info.m_fDamage = dmg;
			info.m_fImpulse = 0.0f;
			info.m_nDamagetype = DAMAGE_TYPE_UNKNOWN;
			info.m_pInflictor = NULL;
			info.m_pTarget = this;
			info.m_vDirection = Vector3D(0,1,0);
			info.m_vOrigin = GetAbsOrigin();

			// make damage from worldspawn
			MakeHit( info, false );
		}
	}

	if(!m_bLastOnGround && m_bOnGround && m_fMaxFallingVelocity > 30 && m_pCurrentMaterial)
	{
		float vol_perc = clamp(fabs(m_fMaxFallingVelocity) / 250,0,1);

		PlayStepSound(m_pCurrentMaterial->footStepSound, vol_perc);
	}

	m_vPrevPhysicsVelocity.y = 0.0f;

	if(!m_bLastOnGround && m_bOnGround)
		m_vecAbsVelocity = m_vPrevPhysicsVelocity;
}

void CBaseActor::DeathSound()
{
	EmitSound_t ep;
	ep.fPitch = 1.0;
	ep.fRadiusMultiplier = 1.0f;
	ep.fVolume = 1;
	ep.origin = Vector3D(0,0,0);

	ep.name = (char*)"Aslan.Death";

	ep.nFlags |= EMITSOUND_FLAG_FORCE_CACHED;

	// attach this sound to entity
	ep.pObject = this;

	ses->EmitSound(&ep);
}


void CBaseActor::PainSound()
{
	EmitSound_t ep;
	ep.fPitch = 1.0;
	ep.fRadiusMultiplier = 1.0f;
	ep.fVolume = 1;
	ep.origin = Vector3D(0,0,0);

	ep.name = (char*)"Aslan.Pain";

	// attach this sound to entity
	ep.pObject = this;

	ep.nFlags |= EMITSOUND_FLAG_FORCE_CACHED;

	ses->EmitSound(&ep);
}

void CBaseActor::MakeHit(const damageInfo_t& info, bool sound)
{
	if(info.m_fDamage <= 0)
		return;

	int nClampedDamage = info.m_fDamage;

	m_fPainPercent += ((float)info.m_fDamage /(float) m_nMaxHealth)*4.0f;
	m_fPainPercent = clamp(m_fPainPercent, 0.0f, 1.0f);

	nClampedDamage *= m_fPainPercent;

	nClampedDamage+=1;

	if(nClampedDamage > m_nMaxHealth)
		nClampedDamage = m_nMaxHealth;
	if(nClampedDamage < 0)
		nClampedDamage = 0;

	int nCurrentHealth = GetHealth();

	int nNewHealth = nCurrentHealth - nClampedDamage;

	if(nNewHealth > m_nMaxHealth)
		nNewHealth = m_nMaxHealth;

	if(nNewHealth < 0)
		nNewHealth = 0;

	damageInfo_t copy = info;
	copy.m_fDamage = nClampedDamage;

	BaseClass::ApplyDamage(copy);

	SetHealth(nNewHealth);

	//if(pDamager)
	//	Msg("Damage taken from \"%s\", %d percents\n", pDamager->GetClassname(), nClampedDamage);

	if(nNewHealth > 0 && m_fPainPercent > 0.45f && sound)
		PainSound();

	m_pLastDamager = info.m_pInflictor;

	if(m_nHealth <= 0)
		OnDeath( info );
}

void CBaseActor::SetHealth(int nHealth)
{
	// dispath fakedeath
	if(!m_pLastDamager && nHealth <= 0)
	{
		damageInfo_t info;
		info.m_fDamage = m_nHealth-nHealth;
		info.m_fImpulse = 0.0f;
		info.m_nDamagetype = DAMAGE_TYPE_UNKNOWN;
		info.m_pTarget = this;
		info.m_vOrigin = GetAbsOrigin();
		info.m_vDirection = Vector3D(0,0,1);

		OnDeath( info );
	}

	BaseClass::SetHealth(nHealth);
}

void CBaseActor::ApplyDamage( const damageInfo_t& info  )
{
	if(IsDead())
		return;

	damageInfo_t newInfo = info;

	float fDamage = info.m_fDamage;

	if(m_pRagdoll && info.trace.pt.hitObj)
	{
		// get hit group
		int hitGroup = (int)info.trace.pt.hitObj->GetUserData();

		switch( hitGroup )
		{
			case BODYPART_ARM_L:
			case BODYPART_ARM_R:
				fDamage *= 0.6f;
				break;
			case BODYPART_LEG_L:
			case BODYPART_LEG_R:
				fDamage *= 1.4f;
				break;
			case BODYPART_CHEST:
				fDamage *= 0.9f;
				break;
			case BODYPART_HEAD:
				fDamage *= 10.0f;
				break;
		}
	}

	newInfo.m_fDamage = fDamage;

	MakeHit( newInfo, true );
}

// thinking functions
void CBaseActor::AliveThink()
{
	if(m_fPainPercent > (1.0f-(float)GetHealth()/(float)m_nMaxHealth)*0.7f)
	{
		m_fPainPercent -= gpGlobals->frametime*0.37f;
		m_fPainPercent = clamp(m_fPainPercent,0.0f,1.0f);
	}

	if(IsDead())
	{
		SetNextThink(gpGlobals->curtime);
		SetThink(&CBaseActor::DeathThink);

		if(m_pLastDamager && stricmp(m_pLastDamager->GetClassname(), "worldinfo"))
			DeathSound();

		return;
	}

	if(m_nActorButtons & IN_USE && !(m_nOldActorButtons & IN_USE))
	{
		trace_t useBoxTrace;

		Vector3D forward;
		AngleVectors(GetEyeAngles(), &forward);

		UTIL_TraceBox(GetEyeOrigin(), GetEyeOrigin() + forward * USE_DISTANCE, Vector3D(8), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_ACTORS, &useBoxTrace);

		debugoverlay->Box3D(useBoxTrace.traceEnd - Vector3D(8), useBoxTrace.traceEnd + Vector3D(8), ColorRGBA(1,0,0,1),0);

		// if has hit use object
		if( useBoxTrace.hitEnt)
		{
			BaseEntity* pEnt = useBoxTrace.hitEnt;

			pEnt->OnUse(this, useBoxTrace.traceEnd, forward);
		}
		else
		{
			// we only picking first object
			// TODO: sectoral check
			for(int i = 0; i < UTIL_EntCount(); i++)
			{
				BaseEntity* pEnt = (BaseEntity*)UTIL_EntByIndex(i);

				if(pEnt->CheckUse(this, useBoxTrace.traceEnd, forward))
				{
					pEnt->OnUse(this, useBoxTrace.traceEnd, forward);
					break;
				}
			}
		}
	}

	// update pose change time
	if(m_nCurPose != m_nPose && m_fNextPoseChangeTime < gpGlobals->curtime)
	{
		m_fNextPoseChangeTime = gpGlobals->curtime + m_fNextPoseTime;
		m_nCurPose = m_nPose;
	}

	SetNextThink(gpGlobals->curtime);
}

void CBaseActor::OnDeath( const damageInfo_t& info )
{
	if(m_pModel)
	{
		StopAnimation();

		if(m_pRagdoll)
		{
			// freeze main object
			if(PhysicsGetObject())
			{
				PhysicsGetObject()->SetActivationState( PS_FROZEN );
				PhysicsGetObject()->SetCollisionResponseEnabled(false);

				PhysicsDestroyObject();
			}

			//m_pRagdoll->SetBoneTransform( m_BoneMatrixList, transpose(m_matWorldTransform) );

			for(int i = 0; i< m_pRagdoll->m_numParts; i++)
			{
				if(m_pRagdoll->m_pParts[i])
				{
					m_pRagdoll->m_pParts[i]->SetContents( COLLISION_GROUP_DEBRIS );
					m_pRagdoll->m_pParts[i]->SetCollisionMask( COLLIDE_DEBRIS );

					m_pRagdoll->m_pParts[i]->SetActivationState(PS_ACTIVE);
					//m_pRagdoll->m_pParts[i]->SetVelocity( m_vPrevPhysicsVelocity );
					m_pRagdoll->m_pParts[i]->SetVelocity( vec3_zero );
					m_pRagdoll->m_pParts[i]->SetAngularVelocity( Vector3D(1,1,1), 0.0f );
					m_pRagdoll->m_pParts[i]->SetCollisionResponseEnabled( true );
				}
			}

			m_pRagdoll->Wake();

			SetAbsOrigin( m_pRagdoll->GetPosition() );
			SetAbsAngles( vec3_zero );

			// update transformation on death to prevent flicker
			UpdateTransform();

			// TODO: apply damage forces to the ragdoll (we need to trace it first)
			//if( info.trace.pt.m_pHit )
			//	info.trace.pt.m_pHit->ApplyImpulse(info.m_vDirection * info.m_fImpulse, info.m_vOrigin - info.trace.pt.m_pHit->GetPosition());
		}
	}
}

void CBaseActor::DeathThink()
{
	m_nActorButtons = 0;

	SetNextThink(gpGlobals->curtime);
}

int	CBaseActor::GetClipCount(int nAmmoType)
{
	if(nAmmoType <= AMMOTYPE_INVALID)
		return 0;

	return m_nClipCount[nAmmoType];
}

void CBaseActor::SetClipCount(int nAmmoType, int nClipCount)
{
	if(nAmmoType <= AMMOTYPE_INVALID)
		return;

	m_nClipCount[nAmmoType] = nClipCount;
}

// picks up weapon
bool CBaseActor::PickupWeapon(CBaseWeapon *pWeapon)
{
	ASSERT(pWeapon);

	if(HasWeaponClass( pWeapon->GetClassname() ))
	{
		int ammo_type = pWeapon->GetAmmoType();

		// give ammo if place available
		if( pWeapon->GetClip() > 0 && GetClipCount( ammo_type ) < g_ammotype[ammo_type].max_ammo )
		{
			// empty pickupable
			pWeapon->SetClip( 0 );

			SetClipCount( pWeapon->GetAmmoType(), GetClipCount( ammo_type ) + 1);
			EmitSound( "mg.pickup" );

			// should destroy this if we spawning with cheats
			return false;
		}
	}

	if(!IsWeaponSlotEmpty(pWeapon->GetSlot()))
		return false;

	if( IsEquippedWithWeapon(pWeapon) )
	{
		MsgError("Already equipped with that weapon.\n");
		return false;
	}

	EmitSound( "mg.pickup" );

	m_pEquipment.append( pWeapon );

	pWeapon->SetOwner(this);

	m_pEquipmentSlots[pWeapon->GetSlot()] = pWeapon;

	return true;
}

// drops weapon
void CBaseActor::DropWeapon( CBaseWeapon *pWeapon )
{
	if(GetActiveWeapon() == pWeapon)
		SetActiveWeapon(NULL);

	if(!pWeapon)
		return;

	if(!IsEquippedWithWeapon(pWeapon))
		return;

	m_pEquipment.remove( pWeapon );

	pWeapon->SetOwner(NULL);

	pWeapon->PhysicsGetObject()->SetActivationState(PS_ACTIVE);
	pWeapon->PhysicsGetObject()->WakeUp();

	// Empty weapon slot
	m_pEquipmentSlots[pWeapon->GetSlot()] = NULL;

	m_nCurrentSlot = WEAPONSLOT_NONE;
}

// returns equipped weapons list
DkList<CBaseWeapon*>* CBaseActor::GetEquippedWeapons()
{
	return &m_pEquipment;
}

// returns current active weapon
CBaseWeapon* CBaseActor::GetActiveWeapon()
{
	return m_pActiveWeapon;
}

// sets active weapon (if weapon is not in list, it will be ignored.)
void CBaseActor::SetActiveWeapon(CBaseWeapon* pWeapon)
{
	if(GetActiveWeapon() == pWeapon)
		return;

	if(!pWeapon)
		m_pActiveWeapon = NULL;

	if(!IsEquippedWithWeapon(pWeapon))
		return;

	m_pActiveWeapon = pWeapon;

	if(m_pActiveWeapon)
		m_pActiveWeapon->Deploy();

	m_nCurrentSlot = m_pActiveWeapon->GetSlot();
}

bool CBaseActor::IsEquippedWithWeapon(CBaseWeapon* pWeapon)
{
	for(int i = 0; i < m_pEquipment.numElem(); i++)
	{
		if(m_pEquipment[i] == pWeapon)
			return true;
	}

	return false;
}

// Is empty slot?
bool CBaseActor::IsWeaponSlotEmpty(int slot)
{
	if(slot >= WEAPONSLOT_COUNT)
		return false;

	if(slot == WEAPONSLOT_NONE)
		return false;

	return (m_pEquipmentSlots[slot] == NULL);
}

// actor has this weapon class?
bool CBaseActor::HasWeaponClass(const char* classname)
{
	for(int i = 0; i < m_pEquipment.numElem(); i++)
	{
		if(!stricmp(m_pEquipment[i]->GetClassname(), classname))
			return true;
	}

	return false;
}

// attack buttons checking
void CBaseActor::CheckAttacking()
{
	// base actors can't attack!
}