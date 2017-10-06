//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: White cage player
//////////////////////////////////////////////////////////////////////////////////

#include "Player.h"
#include "GameInput.h"
#include "BaseWeapon.h"
#include "GameRenderer.h"
#include "viewmodel.h"

#include "Math/math_util.h"

#define MAX_ANGLE_PITCH		(-86.0f)
#define MIN_ANGLE_PITCH		(86.0f)

#define WEAPONDROP_VELOCITY (300.0f)

// player FOV
ConVar g_viewFov("g_viewFov", "65", "Field of view", CV_CHEAT);

int nImpulseCheat = -1;

// declare data table for actor
BEGIN_DATAMAP( CWhiteCagePlayer )

	DEFINE_FIELD( m_bFlashlight, VTYPE_BOOLEAN),

	DEFINE_FIELD( m_vPunchAngleVelocity, VTYPE_ANGLES),
	DEFINE_FIELD( m_vPunchAngle, VTYPE_ANGLES),
	DEFINE_FIELD( m_vRealEyeAngles, VTYPE_ANGLES),
	DEFINE_FIELD( m_vPunchAngle2, VTYPE_ANGLES),
	

	DEFINE_FIELD( m_vShake, VTYPE_VECTOR3D),
	DEFINE_FIELD( m_fShakeDecayCurTime, VTYPE_FLOAT),
	DEFINE_FIELD( m_fShakeMagnitude, VTYPE_FLOAT),

	DEFINE_FIELD( m_fNextHealthAdd, VTYPE_TIME),
	DEFINE_FIELD( m_fAddEffect, VTYPE_FLOAT),

	DEFINE_FIELD( m_pViewmodel, VTYPE_ENTITYPTR),

	DEFINE_FIELD(m_fZoomMultiplier, VTYPE_FLOAT),
	

END_DATAMAP()

DECLARE_CMD(impulse, "No description",CV_CLIENTCONTROLS)
{
	if(CMD_ARGC)
	{
		nImpulseCheat = atoi(CMD_ARGV(0).c_str());
	}
}

static Vector3D lastPos = vec3_zero;

CWhiteCagePlayer::CWhiteCagePlayer() : CBaseActor()
{
	m_fBobTime				= 0;
	m_fBobReminderFactor	= 0;

	m_fBobAmplitudeRun		= 0.51f;

	m_fBobSpeedRun			= 10.0f;

	m_vPunchAngleVelocity = vec3_zero;
	m_vPunchAngle = vec3_zero;
	m_vPunchAngle2 = vec3_zero;
	m_vRealEyeAngles = vec3_zero;
	m_vRealEyeOrigin = vec3_zero;
	m_fMass = 60.0f;

	m_fZoomMultiplier = 1.0f;

	m_interpSignedSpeed = 0.0f;

	m_bFlashlight = false;

	m_fNextPickupCheckTime = 0.0f;

	flashlight_glitch_time = 0.0f;
	flashlight_glitch_time2 = 0.0f;

	m_fNextHealthAdd = 0.0f;
	m_fAddEffect = 0.0f;

	m_fShakeDecayCurTime = 0;
	m_fShakeMagnitude = 0;
	m_vShake = vec3_zero;

	m_nMaxHealth = 100;
	m_nHealth = m_nMaxHealth;

	m_pViewmodel = NULL;
}

#define BOB_CYCLE 0.8f

void CWhiteCagePlayer::Update(float decaytime)
{
	float health_perc = (float)m_nHealth/(float)m_nMaxHealth;

	float health_seg = health_perc*6.0f;

	if(health_seg < floor(health_seg+0.99f) && m_fNextHealthAdd < gpGlobals->curtime && m_nHealth > 0)
	{
		m_fNextHealthAdd = gpGlobals->curtime+0.2f; // 4 points per sec?

		int nOldHealth = GetHealth();
		int nNewHealth = nOldHealth+1; 

		float new_health_perc = (float)nNewHealth/(float)m_nMaxHealth;

		if(floor(new_health_perc*6.0f+1.0f) > floor(health_seg+1.0f))
			nNewHealth = (floor(health_seg+1.0f) / 6.0f) * m_nMaxHealth;
			
		SetHealth( nNewHealth );
	}

	BaseClass::Update(decaytime);
}

void CWhiteCagePlayer::SetHealth(int nHealth)
{
	int nOldHealth = m_nHealth;

	m_nHealth = nHealth;
	
	if(m_nHealth > m_nMaxHealth)
		m_nHealth = m_nMaxHealth;

	if(m_nHealth < 0)
		m_nHealth = 0;

	// special fx
	if(m_nHealth > nOldHealth)
		m_fAddEffect = 1.0f;
}

void CWhiteCagePlayer::Precache()
{
	PrecacheScriptSound("items.flashlightswitch");
	BaseClass::Precache();
}

void CWhiteCagePlayer::Spawn()
{
	BaseClass::Spawn();

	PhysicsGetObject()->SetContents(COLLISION_GROUP_PLAYER);
	PhysicsGetObject()->SetCollisionMask(COLLIDE_PLAYER);

	m_pViewmodel = (CBaseViewmodel*)entityfactory->CreateEntityByName("viewmodel");
	m_pViewmodel->Spawn();
	m_pViewmodel->Activate();
	m_pViewmodel->SetOwner(this);
}

void CWhiteCagePlayer::Activate()
{
	engine->SetCenterMouseCursor(true);

	UpdateView();

	BaseClass::Activate();
}

void CWhiteCagePlayer::DeathThink()
{
	BaseClass::DeathThink();

	movementdata_t movedata;
	movedata.forward = 0.0f;
	movedata.right = 0.0f;

	ProcessMovement( movedata );

	m_nPose = POSE_CROUCH;

	for(int i = 0; i < WEAPONSLOT_COUNT; i++)
		DropWeapon(m_pEquipmentSlots[i]);
}

#define PUNCH_DAMPING		9.0f		// bigger number makes the response more damped, smaller is less damped
										// currently the system will overshoot, with larger damping values it won't
#define PUNCH_SPRING_CONSTANT	95.0f	// bigger number increases the speed at which the view corrects

void CWhiteCagePlayer::DecayPunchAngle( )
{
	if ( dot(m_vPunchAngle, m_vPunchAngle) > 0.001 || dot(m_vPunchAngleVelocity, m_vPunchAngleVelocity) > 0.001 || dot(m_vPunchAngle2, m_vPunchAngle2) > 0.001 )
	{
		m_vPunchAngle2 -= m_vPunchAngle2 * gpGlobals->frametime * 8.0f;

		SpringFunction(m_vPunchAngle, m_vPunchAngleVelocity, PUNCH_SPRING_CONSTANT, PUNCH_DAMPING, gpGlobals->frametime);

		// don't wrap around
		m_vPunchAngle = Vector3D( 
			clamp(m_vPunchAngle.x, -89, 89 ), 
			clamp(m_vPunchAngle.y, -179, 179 ),
			clamp(m_vPunchAngle.z, -89, 89 ) );
	}
}

void CWhiteCagePlayer::ShakeView()
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

void CWhiteCagePlayer::GiveWeapon(const char *classname)
{
	CBaseWeapon* pWeapon = (CBaseWeapon*)entityfactory->CreateEntityByName( classname );

	if(!pWeapon)
		return;

	if(pWeapon->EntType() == ENTTYPE_ITEM)
	{
		pWeapon->Precache();
		pWeapon->Spawn();
		pWeapon->Activate();

		if(!PickupWeapon(pWeapon))
		{
			pWeapon->SetState(ENTITY_REMOVE);
			return;
		}

		if(GetActiveWeapon() == NULL)
		{
			pWeapon->SetActivity(ACT_VM_DEPLOY);
			SetActiveWeapon(pWeapon);
		}
	}
	else
	{
		pWeapon->SetState(ENTITY_REMOVE);
	}
}

void CWhiteCagePlayer::SetActiveWeapon(CBaseWeapon* pWpn)
{
	if(!pWpn)
		GetViewmodel()->SetModel((IEqModel*)NULL);
	else
		GetViewmodel()->SetModel( (const char*)pWpn->GetViewmodel() );

	BaseClass::SetActiveWeapon(pWpn);
}

// thinking functions
void CWhiteCagePlayer::AliveThink()
{
	if(nImpulseCheat != -1)
	{
		switch(nImpulseCheat)
		{
			case 99: // get all weapons
			{
				GiveWeapon("weapon_deagle");
				GiveWeapon("weapon_ak74");
				GiveWeapon("weapon_wiremine");
				break;
			}
			case 69: // revive
			{
				SetHealth( GetHealth() + (m_nMaxHealth/6.0f));
				break;
			}
		}

		nImpulseCheat = -1;
	}

	// TODO: Must be as control sender for active player
	m_nActorButtons = nClientButtons;

	if((m_nActorButtons & IN_FLASHLIGHT) && !(m_nOldActorButtons & IN_FLASHLIGHT))
	{
		EmitSound("items.flashlightswitch");
		m_bFlashlight = !m_bFlashlight;
	}

	// try to pickup weapons
	if(gpGlobals->curtime > m_fNextPickupCheckTime)
	{
		DkList<BaseEntity*> pEnts;
		UTIL_EntitiesInSphere(m_pPhysicsObject->GetPosition()-Vector3D(0,42,0), 60, (DkList<BaseEntity*>*)&pEnts);

		for(int i = 0; i < pEnts.numElem(); i++)
		{
			if(pEnts[i]->EntType() == ENTTYPE_ITEM)
			{
				CBaseWeapon* pWeapon = dynamic_cast<CBaseWeapon*>(pEnts[i]);
				if(pWeapon && pWeapon->GetOwner() == NULL && !IsEquippedWithWeapon(pWeapon))
				{
					if(PickupWeapon( pWeapon ))
					{
						if(GetActiveWeapon() == NULL)
						{
							pWeapon->SetActivity(ACT_VM_DEPLOY);
							SetActiveWeapon(pWeapon);
						}
					}
				}
			}
		}

		m_fNextPickupCheckTime = gpGlobals->curtime + 0.5f;
	}

	// select primary
	if((m_nActorButtons & IN_SLOT_PRIMARY) && !(m_nOldActorButtons & IN_SLOT_PRIMARY))
	{
		if(!IsWeaponSlotEmpty(WEAPONSLOT_PRIMARY))
		{
			SetActiveWeapon(m_pEquipmentSlots[WEAPONSLOT_PRIMARY]);
		}
	}

	// select pistol
	if((m_nActorButtons & IN_SLOT_SECONDARY) && !(m_nOldActorButtons & IN_SLOT_SECONDARY))
	{
		if(!IsWeaponSlotEmpty(WEAPONSLOT_SECONDARY))
		{
			SetActiveWeapon(m_pEquipmentSlots[WEAPONSLOT_SECONDARY]);
		}
	}

	// select grenade slot
	if((m_nActorButtons & IN_SLOT_GRENADE) && !(m_nOldActorButtons & IN_SLOT_GRENADE))
	{
		if(!IsWeaponSlotEmpty(WEAPONSLOT_GRENADE))
		{
			SetActiveWeapon(m_pEquipmentSlots[WEAPONSLOT_GRENADE]);
		}
	}

	// hide weapon
	if((m_nActorButtons & IN_HOLSTER) && !(m_nOldActorButtons & IN_HOLSTER))
	{
		SetActiveWeapon(NULL);
	}

	// check drop weapon button
	if((m_nActorButtons & IN_DROPWEAPON) && !(m_nOldActorButtons & IN_DROPWEAPON))
	{
		CBaseWeapon* pActWeapon = GetActiveWeapon();

		if(pActWeapon != NULL)
		{
			DropWeapon(pActWeapon);

			pActWeapon->PhysicsGetObject()->SetPosition(m_pPhysicsObject->GetPosition() + Vector3D(0,25,0));

			Vector3D f,r,u;
			AngleVectors(GetEyeAngles(), &f,&r,&u);

			pActWeapon->PhysicsGetObject()->SetVelocity(f*WEAPONDROP_VELOCITY);
			pActWeapon->PhysicsGetObject()->SetAngles( Vector3D(0, GetEyeAngles().y, 0) );
			pActWeapon->PhysicsGetObject()->SetAngularVelocity(Vector3D(2,0,2), 1.0f);
			m_fNextPickupCheckTime = gpGlobals->curtime + 1.0f;
		}
	}

	if(IsDead())
	{
		engine->SetCenterMouseCursor(false);
	}

	int nGamePlayFlags = nGPFlags;

	if(this != g_pGameRules->GetLocalPlayer())
		nGamePlayFlags &= ~(GP_FLAG_NOCLIP | GP_FLAG_GOD);

	movementdata_t movedata;
	movedata.forward = 0.0f;
	movedata.right = 0.0f;

	if(m_nActorButtons & IN_FORWARD)
		movedata.forward += 1.0f;

	if(m_nActorButtons & IN_BACKWARD)
		movedata.forward -= 1.0f;

	if(m_nActorButtons & IN_STRAFELEFT)
		movedata.right -= 1.0f;

	if(m_nActorButtons & IN_STRAFERIGHT)
		movedata.right += 1.0f;

	if(m_nCurPose == POSE_CROUCH)
	{
		m_fMaxSpeed = CROUCH_SPEED;
	}
	else if(m_nCurPose == POSE_STAND)
	{
		m_fMaxSpeed = WALK_SPEED;

		if(m_nActorButtons & IN_FORCERUN)
			m_fMaxSpeed = MAX_SPEED;
	}

	if(m_nCurPose < POSE_CINEMATIC)
	{
		if(m_nActorButtons & IN_DUCK)
			SetPose( POSE_CROUCH, 0.3f);
	}

	ProcessMovement( movedata );

	BaseClass::AliveThink();
}

Vector3D flashlight_pos = vec3_zero;
Vector3D flashlight_rot = vec3_zero;

ConVar r_freezeflashlight("r_freezeflashlight", "0");
ConVar r_nohud("r_nohud", "0");

void UTIL_DrawScreenRect(Vector2D &min, Vector2D &max, float cOfs, ColorRGBA &color, ITexture* pTex = NULL)
{
	BlendStateParam_t blending;
	blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
	blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

	Vertex2D_t tmprect[] = { MAKETEXQUAD(min.x, min.y, max.x, max.y, cOfs) };

	materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_STRIP, tmprect, elementsOf(tmprect), pTex, color, &blending);
}

void CWhiteCagePlayer::OnPostRender()
{
	BaseClass::OnPostRender();

	// TODO: g_pGameHUD->....

	if(m_pLastDamager && IsDead() && !stricmp(m_pLastDamager->GetClassname(), "worldinfo"))
	{
		Vector2D screen_size(g_pEngineHost->GetWindowSize().x,g_pEngineHost->GetWindowSize().y);

		UTIL_DrawScreenRect(Vector2D(0), screen_size, 0, ColorRGBA(0));
	}

	if(!r_nohud.GetBool())
	{
		BlendStateParam_t blending;
		blending.srcFactor = BLENDFACTOR_SRC_ALPHA;
		blending.dstFactor = BLENDFACTOR_ONE_MINUS_SRC_ALPHA;

		float health_perc = (float)m_nHealth/(float)m_nMaxHealth;

		static ITexture* pIndicatorBody = g_pShaderAPI->LoadTexture("ui/hud_6indicator_body", TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP);
		static ITexture* pIndicator = g_pShaderAPI->LoadTexture("ui/hud_6indicator", TEXFILTER_TRILINEAR_ANISO, TEXADDRESS_CLAMP);

		float fHealthBarSize = 90.0f;

		Vector2D health_ind_min = Vector2D(g_pEngineHost->GetWindowSize().x * 0.5f - fHealthBarSize*0.5f, g_pEngineHost->GetWindowSize().y-100.0f - fHealthBarSize*0.5f);
		Vector2D health_ind_max = Vector2D(g_pEngineHost->GetWindowSize().x * 0.5f + fHealthBarSize*0.5f, g_pEngineHost->GetWindowSize().y-100.0f + fHealthBarSize*0.5f);

		ColorRGBA body_color(0.2,0.5,0.7,1);

		if(health_perc <= 0.5f)
		{
			if(health_perc <= 0.2f)
			{
				body_color.x = 1.0f;
				body_color.y = 0.25f;
				body_color.z = 0.25f;

				body_color *= fabs(sin(gpGlobals->curtime*15.0f));
			}
			else
			{
				float fSin = fabs(sin(gpGlobals->curtime*8.0f));
				body_color.x = fSin;
				body_color.y = fSin;
				body_color.z = 0.2;
			}
		}

		if(m_fAddEffect < 0)
		{
			m_fAddEffect = 0.0f;
		}
		else
		{
			body_color = lerp(body_color, ColorRGBA(0.1, 1.0f, 0.1, 1.0), m_fAddEffect);

			m_fAddEffect -= gpGlobals->frametime;
		}

		UTIL_DrawScreenRect(health_ind_min, health_ind_max, 0, body_color, pIndicatorBody);

		int nVerts = 0;
		Vertex2D_t health_fan[8];

		Vector2D hfan_center = (health_ind_min+health_ind_max)*0.5f;
		Vector2D hfan_size = health_ind_min-health_ind_max;

#define HFAN_VERT(vec)\
	health_fan[nVerts] = Vertex2D_t(vec-1.0f, Vector2D(dot(Vector2D(1.0f,0), (vec-hfan_center)/hfan_size)+0.5f, dot(Vector2D(0,1.0f), (vec-hfan_center)/hfan_size)+0.5f));\
	nVerts++;

		// add center vertex
		HFAN_VERT(hfan_center);

		Matrix2x2 mat = rotate2( DEG2RAD( -120.0f ) );
		Vector2D rotVec = hfan_center+normalize(mat*Vector2D(1,0))*fHealthBarSize;

		HFAN_VERT(rotVec);

		for(int i = 0; i < 6; i++)
		{
			float health_tenscale = health_perc*10.0f;

			float seg_perc = (float)i / 6.0f;

			float health_segs = health_perc*6.0f;

			if(health_segs <= i)
				break;

			float health_segment_ratio = 1.0f - clamp((health_segs-i), 0.0f,1.0f);

			float seg_angle = 60.0f*((float)i+1) - health_segment_ratio*60.0f;

			Matrix2x2 mat = rotate2( DEG2RAD( seg_angle - 120.0f ) );
			rotVec = hfan_center+normalize(mat*Vector2D(1,0))*fHealthBarSize;

			HFAN_VERT(rotVec);
		}

		materials->DrawPrimitives2DFFP(PRIM_TRIANGLE_FAN, health_fan, nVerts, pIndicator, ColorRGBA(1.0f, 1.0f - m_fPainPercent,1.0f - m_fPainPercent,1), &blending);
		/*
		UTIL_DrawScreenRect(Vector2D(g_pEngineHost->GetWindowSize().x - 70, g_pEngineHost->GetWindowSize().y - 45 - 210), Vector2D(g_pEngineHost->GetWindowSize().x - 40, g_pEngineHost->GetWindowSize().y - 45), 0, ColorRGBA(0,0,0,0.4));
		UTIL_DrawScreenRect(Vector2D(g_pEngineHost->GetWindowSize().x - 70, g_pEngineHost->GetWindowSize().y - 45 - (health_perc*210.0f)), Vector2D(g_pEngineHost->GetWindowSize().x - 40, g_pEngineHost->GetWindowSize().y - 45), 3, ColorRGBA(1.0-health_perc,health_perc,0,0.5));

		UTIL_DrawScreenRect(Vector2D(g_pEngineHost->GetWindowSize().x - 120, g_pEngineHost->GetWindowSize().y - 45 - 210), Vector2D(g_pEngineHost->GetWindowSize().x - 80, g_pEngineHost->GetWindowSize().y - 45), 0, ColorRGBA(0,0,0,0.4));
		UTIL_DrawScreenRect(Vector2D(g_pEngineHost->GetWindowSize().x - 120, g_pEngineHost->GetWindowSize().y - 45 - (m_fPainPercent*210.0f)), Vector2D(g_pEngineHost->GetWindowSize().x - 80, g_pEngineHost->GetWindowSize().y - 45), 3, ColorRGBA(m_fPainPercent,1.0-m_fPainPercent,0,0.5));
		*/

		// draw weapon stats

		CBaseWeapon* pWeapon = GetActiveWeapon();

		if(pWeapon)
		{
			int nTotalAmmo = GetClipCount( pWeapon->GetAmmoType() );
			int nClipCount = pWeapon->GetClip();

			Vector2D wpn_text_pos(g_pEngineHost->GetWindowSize().x-350, g_pEngineHost->GetWindowSize().y - 150);

			eqFontStyleParam_t style;
			g_pEngineHost->GetDefaultFont()->RenderText(varargs("%d\n%d", nClipCount, nTotalAmmo), wpn_text_pos, style);
		}

		Vector2D center_pos(g_pEngineHost->GetWindowSize().x*0.5f, g_pEngineHost->GetWindowSize().y*0.5f);
		
		Vertex2D_t tmprect[] = 
		{ 
			Vertex2D_t(center_pos+Vector2D(0,-1), vec2_zero),
			Vertex2D_t(center_pos+Vector2D(1,1), vec2_zero),
			Vertex2D_t(center_pos+Vector2D(-1,1), vec2_zero)
		};
		

		// Draw crosshair
		materials->DrawPrimitives2DFFP(PRIM_TRIANGLES, tmprect, elementsOf(tmprect), NULL, ColorRGBA(1,1,1,0.45), &blending);
	}
	
}

extern int nGPFlags;

void CWhiteCagePlayer::ApplyDamage( const damageInfo_t& info )
{
	BaseClass::ApplyDamage( info );

	Vector3D punch_angle = Vector3D(0,0,0);

	Vector3D f,r,u;
	AngleVectors(m_vecEyeAngles, &f,&r,&u);

	if(info.m_nDamagetype == DAMAGE_TYPE_BLAST)
	{
		punch_angle.x = dot(f, info.m_vDirection) * info.m_fImpulse * -0.000025f;
		punch_angle.y = punch_angle.z = dot(r, info.m_vDirection) * info.m_fImpulse * 0.000025f;
	}
	
	ViewPunch(punch_angle);
}

void CWhiteCagePlayer::MakeHit(const damageInfo_t& info, bool sound)
{
	if(!(nGPFlags & GP_FLAG_GOD))
	{
		m_fNextHealthAdd = gpGlobals->curtime + 5.0f;
		BaseClass::MakeHit(info, sound);
	}
}

void CWhiteCagePlayer::OnPreRender()
{
	Vector3D forward,right;

	AngleVectors(GetEyeAngles(), &forward, &right);

	if(m_bFlashlight)
	{
		dlight_t *flashlight = viewrenderer->AllocLight();
		SetLightDefaults(flashlight);
		static ITexture* pSpot = g_pShaderAPI->LoadTexture("lights/flashlight",TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,TEXFLAG_NOQUALITYLOD);

		//static ITexture* pSpot = g_pShaderAPI->LoadTexture("lights/flashlight_dudv",TEXFILTER_TRILINEAR_ANISO,TEXADDRESS_CLAMP,TEXFLAG_NOQUALITYLOD);

		if(!r_freezeflashlight.GetBool())
		{
			flashlight_pos = GetEyeOrigin()+forward*8.0f+right*6.0f;
			flashlight_rot = GetEyeAngles();
		}

		flashlight->nType = DLT_SPOT;
		flashlight->position = flashlight_pos;
		flashlight->angles = flashlight_rot;
		//flashlight->nFlags |= LFLAG_SIMULATE_REFLECTOR;

		flashlight->curfadetime = -1;
		flashlight->fadetime = -1;
		flashlight->dietime = -1;

		flashlight->fovWH.x = 512;
		flashlight->fovWH.y = 512;
		flashlight->fovWH.z = 65;

		flashlight->radius = 620;

		flashlight->pMaskTexture = pSpot;
		flashlight->intensity = 1.0f;
		flashlight->curintensity = 1.0f;
		flashlight->color = ColorRGB(1.0f, 0.77f,0.6f);
		
		/*
		if(flashlight_glitch_time < gpGlobals->curtime + flashlight_glitch_time2)
		{
			flashlight->intensity = 0.9f;
			float fac = sinf(10*sinf(gpGlobals->curtime*5))*0.5f - 0.5f + 1;
			flashlight->color = ColorRGB(1.0f, 0.77f+fac*0.1f,0.6f+fac*0.1f);
			flashlight->radius = 440;

			if(flashlight_glitch_time < gpGlobals->curtime)
			{
				flashlight_glitch_time = gpGlobals->curtime + RandomFloat(1.9f,15.45f);
				flashlight_glitch_time2 = RandomFloat(1.9f,4.45f);
			}
		}*/

		viewrenderer->AddLight(flashlight);
	}

	BaseClass::OnPreRender();
}

/*
// min bbox dimensions
Vector3D CWhiteCagePlayer::GetBBoxMins()
{
	return m_pModel->GetBBoxMins();
}

// max bbox dimensions
Vector3D CWhiteCagePlayer::GetBBoxMaxs()
{
	return m_pModel->GetBBoxMaxs();
}

// returns world transformation of this object
Matrix4x4 CWhiteCagePlayer::GetRenderWorldTransform()
{
	BoundingBox bbox(m_pModel->GetBBoxMins(), m_pModel->GetBBoxMaxs());

	Vector3D offset(0, -bbox.GetSize().y*0.5f, 0);

	return ComputeWorldMatrix(m_vecAbsOrigin+offset, Vector3D(0, m_vecEyeAngles.y, 0), Vector3D(1));
}

void CWhiteCagePlayer::Render(int nViewRenderFlags)
{
	if(viewrenderer->GetDrawMode() != VDM_SHADOW)
		return;

	//m_vecAbsOrigin = PhysicsGetObject()->GetPosition();

	// render if in frustum
	Matrix4x4 wvp;
	Matrix4x4 world = GetRenderWorldTransform();

	materials->SetMatrix(MATRIXMODE_WORLD, GetRenderWorldTransform());

	if(materials->GetLight() && materials->GetLight()->nFlags & LFLAG_MATRIXSET)
		wvp = materials->GetLight()->lightWVP * world;
	else 
		materials->GetWorldViewProjection(wvp);

	Volume frustum;
	frustum.LoadAsFrustum(wvp);

	Vector3D mins = GetBBoxMins();
	Vector3D maxs = GetBBoxMaxs();

	if(!frustum.IsBoxInside(mins.x,maxs.x,mins.y,maxs.y,mins.z,maxs.z))
		return;

	studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
	int nLod = 0;

	viewrenderer->GetNearestEightLightsForPoint(m_vecAbsOrigin, m_pLights);

	for(int i = 0; i < pHdr->numBodyGroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

		while(nLod > 0 && nModDescId != -1)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
		{
			viewrenderer->DrawModelPart(m_pModel, nModDescId, j, nViewRenderFlags, m_pLights, m_BoneMatrixList);
		}
	}
}
*/

void CWhiteCagePlayer::OnRemove()
{
	BaseClass::OnRemove();
}

ConVar v_landingYawFactor("v_landingYawFactor", "15");

void CWhiteCagePlayer::CheckLanding()
{
	if(!m_bLastOnGround && m_bOnGround && m_pCurrentMaterial)
	{
		if(m_fMaxFallingVelocity > 120)
		{
			float perc = clamp(fabs(m_fMaxFallingVelocity-120) / 490,0,1);

			m_fEyeHeight -= perc * 65.0f;

			ViewPunch(Vector3D(-perc*9,0,-perc*2));
		}

		float perc = clamp(fabs(m_fMaxFallingVelocity-120) / 290,0,1);

		if(m_fMaxFallingVelocity > 0)
			m_pViewmodel->AddLandingYaw(perc*v_landingYawFactor.GetFloat());
	}

	BaseClass::CheckLanding();
}

inline float TriangularPeriodicFunction(float min, float max, float val)
{
	// shifting coordinate system so function is above x axis now
	// (and performing inverse shift when result is obtained)
	if (min < 0)
		return TriangularPeriodicFunction(0, max - min, val - min) + min;
 
	// function is symmetric relative to axis of values
	val = fabs(val);
  
	// length of monotonous interval (MI)
	float len = max - min;

	// index of MI
	int d = (int) floor(val/len);

	// index of value within MI
	float m = fmod(val, len); 
 
	if (d >= 0)
	{
		// MI is at the right side (relative to the y axis)
		return d&1 
			? max - m // values within MI are decreasing
			: m; // values within MI are increasing
	}
	else
	{
		// MI is at the left side (relative to the y axis)
		return d&1 
			? m - max // values within MI are increasing
			: m; // values within MI are decreasing
	}
}

float triangleWave( float pos )
{
	float sinVal = sin(pos);
	return pow(sinVal, 2.0f)*sign(sinVal);//TriangularPeriodicFunction(-1, 1, pos / (PI_F*0.5f));
}

float triangleWave2( float pos )
{
	float cosVal = cos(pos);
	return pow(cosVal, 2.0f)*sign(cosVal);//TriangularPeriodicFunction(-1, 1, pos / (PI_F*0.5f));
}

// true stalker bobbing

#define SPEED_REMINDER	5.0f
#define CROUCH_FACTOR	0.75f

bool CWhiteCagePlayer::ProcessBob(Vector3D& p, Vector3D& dangle)
{
	m_fBobTime += gpGlobals->frametime;

	//m_fBobSpeedRun = 0.038f * m_fMaxSpeed;

	if ((length(m_vMoveDir) > 0.05f) && m_bOnGround)
	{
		if (m_fBobReminderFactor<1.0f)
			m_fBobReminderFactor += SPEED_REMINDER*gpGlobals->frametime;
		else
			m_fBobReminderFactor = 1.0f;
	}
	else
	{
		if (m_fBobReminderFactor > 0.0f)
			m_fBobReminderFactor -= SPEED_REMINDER*gpGlobals->frametime;
		else
			m_fBobReminderFactor = 0.0f;
	}

	if (!fsimilar(m_fBobReminderFactor,0))
	{
		// apply footstep bobbing effect
		float k		= (m_nPose == POSE_CROUCH)? CROUCH_FACTOR : 1.0f;

		float A, ST;

		A	= m_fBobAmplitudeRun*k;
		ST	= m_fBobSpeedRun*m_fBobTime*k;
	
		float _sinA	= fabs(sin(ST)*A)*m_fBobReminderFactor;
		float _cosA	= cos(ST)*A*m_fBobReminderFactor;

		p.y			+=	_sinA;

		dangle.y	=	_cosA;
		dangle.z	=	_cosA;
		dangle.x	=	_sinA;
	}

	return true;
}

void CWhiteCagePlayer::UpdateView()
{
	if(!m_pPhysicsObject)
		return;

	BaseClass::UpdateView();

	if(IsDead())
	{
		// TODO: Use ragdoll

		//m_vRealEyeAngles = m_vecEyeAngles - m_vPunchAngle;

		m_vRealEyeOrigin = m_vecEyeOrigin;
		m_vRealEyeAngles = m_vecEyeAngles + Vector3D(0,0,45);
		//m_pCameraParams->SetFOV(g_viewFov.GetFloat());

		return;
	}

	// stalker bob
	Vector3D outPos(0);
	Vector3D outBob(0);
	

	if(gpGlobals->frametime > 0)
		ProcessBob(outPos, outBob);

	// update and decay the punch angle
	DecayPunchAngle();

	ShakeView();

	Vector3D f,r,u;
	AngleVectors(m_vecEyeAngles, &f,&r,&u);

	outBob += m_vPunchAngle - m_vPunchAngle2 + m_vShake;
	outPos += m_vShake;

	m_vRealEyeAngles = m_vecEyeAngles - outBob;
	m_vRealEyeOrigin = m_vecEyeOrigin - outPos;

	if(m_pActiveWeapon)
	{
		float rVelocity = dot(r, m_vPrevPhysicsVelocity);
		float fSpd = rVelocity*-0.02f*(1.0 - (m_vRealEyeAngles.x / 180.0f));

		m_interpSignedSpeed = lerp(m_interpSignedSpeed, fSpd, gpGlobals->frametime * 5.0f);

		Vector3D vmOrigin = m_vRealEyeOrigin - outPos;
		Vector3D vmAngles = m_vRealEyeAngles - Vector3D(0, outBob.y,outBob.z);
		Vector3D vmOrigAngles = m_vRealEyeAngles;

		m_pViewmodel->CalcViewModelLag(vmOrigin, vmAngles, vmOrigAngles);

		m_pViewmodel->SetAbsOrigin( vmOrigin );
		m_pViewmodel->SetAbsAngles( vmAngles );
		m_pViewmodel->UpdateTransform();
	}
}

Vector3D CWhiteCagePlayer::GetEyeAngles()
{
	return m_vRealEyeAngles;
}

Vector3D CWhiteCagePlayer::GetEyeOrigin()
{
	return m_vRealEyeOrigin;
}

ConVar g_thirdPerson("g_thirdPerson", "0", "Enables thirdperson view", CV_CHEAT);
ConVar g_thirdPersonDist("g_thirdPersonDist", "100", "Enables thirdperson view", CV_CHEAT);
ConVar g_thirdPersonHeight("g_thirdPersonHeight", "20", "Enables thirdperson view", CV_CHEAT);

// entity view - computation for rendering
// use SetActiveViewEntity to set view from this entity
void CWhiteCagePlayer::CalcView(Vector3D &origin, Vector3D &angles, float &fov)
{
	float fZoomCoeff = 1.0f/(m_fZoomMultiplier*0.5f+0.5f);

	fov = g_viewFov.GetFloat()*fZoomCoeff;

	if( g_thirdPerson.GetBool() )
	{
		Vector3D f,r,u;
		AngleVectors(m_vRealEyeAngles, &f, &r, &u);

		IPhysicsObject* pIgnore = PhysicsGetObject();

		internaltrace_t tr;
		physics->InternalTraceBox(m_vRealEyeOrigin, m_vRealEyeOrigin- f * g_thirdPersonDist.GetFloat(), Vector3D(16), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr, &pIgnore);

		origin = m_vRealEyeOrigin - (f * g_thirdPersonDist.GetFloat()*tr.fraction) + Vector3D(0, g_thirdPersonHeight.GetFloat(), 0);
		angles = m_vRealEyeAngles;
	}
	else
	{
		origin = m_vRealEyeOrigin;
		angles = m_vRealEyeAngles;
	}
}

void CWhiteCagePlayer::SetZoomMultiplier(float fZoomMultiplier)
{
	m_fZoomMultiplier = fZoomMultiplier;
}

float CWhiteCagePlayer::GetZoomMultiplier()
{
	return m_fZoomMultiplier;
}

void CWhiteCagePlayer::ViewPunch(Vector3D &angleOfs, bool bWeapon)
{
	if(bWeapon)
		m_vPunchAngle2 += angleOfs;
	else
		m_vPunchAngleVelocity += angleOfs * 30.0f;
}

void CWhiteCagePlayer::ViewShake(float fMagnutude, float fTime)
{
	m_fShakeDecayCurTime += fTime;

	// clamp time to 5 seconds max
	if(m_fShakeDecayCurTime > 1.5f)
		m_fShakeDecayCurTime = 1.5f;

	m_fShakeMagnitude += fMagnutude;
}

void CWhiteCagePlayer::MouseMoveInput(float x, float y)
{
	// TODO: allow rotate death camera
	if(IsDead())
		return;

	m_vecEyeAngles.y += x;
	m_vecEyeAngles.x += y;

	m_vecEyeAngles.x = clamp(m_vecEyeAngles.x, MAX_ANGLE_PITCH, MIN_ANGLE_PITCH);
}

DECLARE_ENTITYFACTORY(player, CWhiteCagePlayer)