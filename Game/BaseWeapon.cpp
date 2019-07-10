//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game basic weapon class
//////////////////////////////////////////////////////////////////////////////////

#include "BaseWeapon.h"
#include "ParticleRenderer.h"
#include "Effects.h"
#include "Player.h"
#include "viewmodel.h"

DECLARE_EFFECT_GROUP( Muzzle )
{
	PRECACHE_PARTICLE_MATERIAL_POST( "dev/particle");
	//PRECACHE_PARTICLE_MATERIAL_POST( "effects/muzzleflash1_heat");
	PRECACHE_PARTICLE_MATERIAL_POST( "effects/muzzleflash1");
}

#define WORLD_WEAPON_COLLIDE (COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS)

// declare data table for actor
BEGIN_DATAMAP( CBaseWeapon )

	DEFINE_FIELD( m_nClipCurrent,			VTYPE_INTEGER),
	DEFINE_FIELD( m_nAmmoType,				VTYPE_INTEGER),
	
	DEFINE_FIELD(m_nShotsFired,				VTYPE_INTEGER),
	DEFINE_FIELD(m_fNextIdleTime,			VTYPE_TIME),
	DEFINE_FIELD(m_fIdleTime,				VTYPE_FLOAT),

	DEFINE_FIELD( m_fPrimaryAttackTime,		VTYPE_TIME),
	DEFINE_FIELD( m_fSecondaryAttackTime,	VTYPE_TIME),
	DEFINE_FIELD( m_bReload,				VTYPE_BOOLEAN),
	DEFINE_FIELD( m_bAiming,				VTYPE_BOOLEAN),
	//DEFINE_FIELD( m_vecLastFacing,			VTYPE_VECTOR3D),

	DEFINE_THINKFUNC(IdleThink),
	DEFINE_THINKFUNC(InactiveThink),

END_DATAMAP()

CBaseWeapon::CBaseWeapon()
{
	m_nClipCurrent			= 0;
	m_nAmmoType				= AMMOTYPE_INVALID;

	m_nShotsFired			= 0;

	m_fIdleTime				= 0.5f;
	m_fNextIdleTime			= 0.0f;

	m_fPrimaryAttackTime	= 0.0f;
	m_fSecondaryAttackTime	= 0.0f;

	m_nButtons				= 0;
	m_nOldButtons			= 0;

	m_bReload				= false;

	m_bAiming				= false;
	// this must be initialized in constructor, not at spawn
	//SetClip(GetMaxClip());
}

CBaseWeapon::~CBaseWeapon()
{

}

void CBaseWeapon::PhysicsCreateObjects()
{
	if(m_physObj)
		return;

	PhysicsInitNormal(COLLISION_GROUP_OBJECTS, true);

	// collide only with world
	m_physObj->SetCollisionMask(WORLD_WEAPON_COLLIDE);

	m_physObj->SetPosition(BaseClass::GetAbsOrigin());
	m_physObj->SetAngles(BaseClass::GetAbsAngles());
}

void CBaseWeapon::Spawn()
{
	SetModel( GetWorldmodel() );

	PhysicsCreateObjects();

	BaseClass::Spawn();

	SetThink(&CBaseWeapon::IdleThink);
	SetNextThink(gpGlobals->curtime);

	SetClip(GetMaxClip());
}

void CBaseWeapon::OnPreRender()
{
	if(m_pOwner)
		return;

	g_modelList->AddRenderable(this);
}

void CBaseWeapon::Activate()
{
	BaseClass::Activate();
}

void CBaseWeapon::Reload()
{
	if(!m_bReload && m_fPrimaryAttackTime < gpGlobals->curtime)
	{
		BeginReload();
	}
}

void CBaseWeapon::IdleThink()
{
	CBaseActor* pActor = (CBaseActor*)m_pOwner;

	if(!pActor)
	{
		SetThink(&CBaseWeapon::InactiveThink);

		SetNextThink(gpGlobals->curtime);
		return;
	}

	if(pActor->GetActiveWeapon() != this)
	{
		SetThink(&CBaseWeapon::InactiveThink);
		SetNextThink(gpGlobals->curtime);
		return;
	}

	if(pActor->GetButtons() & IN_RELOAD)
		Reload();

	if(m_bReload && m_fPrimaryAttackTime < gpGlobals->curtime)
		FinishReload();

	if(m_fNextIdleTime < gpGlobals->curtime)
	{
		m_nShotsFired = 0;
	}

	//if(GetCurrentActivity(1) != ACT_VM_SPRINT)
	//	SetActivity(ACT_VM_SPRINT, 1);

	/*
	if(length(pActor->GetAbsVelocity().xz()) > 190)
	{
		float fFactor = (length(pActor->GetAbsVelocity().xz()) - 190.0f)*0.01f;
		if(fFactor > 1.0f)
			fFactor = 1.0f;

		SetSequenceBlending(1, fFactor);
	}
	else
		SetSequenceBlending(1, 0.0f);
		*/

	SetNextThink(gpGlobals->curtime);
}

void CBaseWeapon::InactiveThink()
{
	CBaseActor* pActor = (CBaseActor*)m_pOwner;

	if(pActor && pActor->GetActiveWeapon() == this)
		SetThink(&CBaseWeapon::IdleThink);

	m_nShotsFired = 0;

	SetNextThink(gpGlobals->curtime);
}

void CBaseWeapon::Deploy()
{
	SetActivity(ACT_VM_DEPLOY);

	m_fPrimaryAttackTime = gpGlobals->curtime + GetCurrentAnimationDuration();
	m_fSecondaryAttackTime = gpGlobals->curtime + GetCurrentAnimationDuration();
}

void CBaseWeapon::Holster()
{

}

void CBaseWeapon::Drop()
{

}

void CBaseWeapon::Precache()
{
	PrecacheStudioModel(GetViewmodel());
	PrecacheStudioModel(GetWorldmodel());


	PrecacheScriptSound(GetShootSound());
	PrecacheScriptSound(GetReloadSound());
	PrecacheScriptSound(GetFastReloadSound());
	PrecacheScriptSound(GetEmptySound());

	PrecacheScriptSound("mg.pickup");

	BaseClass::Precache();
}

void CBaseWeapon::OnRemove()
{
	BaseClass::OnRemove();
}

void CBaseWeapon::Update(float decaytime)
{
	if(!m_pOwner)
	{
		// collide only with world
		m_physObj->SetContents(COLLISION_GROUP_OBJECTS);
		m_physObj->SetCollisionMask(WORLD_WEAPON_COLLIDE);

		BaseEntity::SetAbsAngles(m_physObj->GetAngles());
		BaseEntity::SetAbsOrigin(m_physObj->GetPosition());

		m_physObj->SetAngularFactor(Vector3D(1));

		ObjectSoundEvents(PhysicsGetObject(), this);
	}
	else
	{
		// use bone merge with player model

		// Don't collide with anything
		m_physObj->SetCollisionMask( 0 );
		m_physObj->SetContents(0);
		m_physObj->SetActivationState(PS_INACTIVE);

		m_physObj->SetPosition(m_pOwner->GetAbsOrigin()-Vector3D(0,25,0));

		SetLocalOrigin(vec3_zero);
		SetLocalAngles(vec3_zero);
	}

	BaseClass::Update(decaytime);
}

// weapon-specific stuff

void CBaseWeapon::SetClip(int nBullets)
{
	m_nClipCurrent = nBullets;
}

void CBaseWeapon::SetAmmoType(int nAmmoType)
{
	// TODO: make me functional again
}

int CBaseWeapon::GetClip()
{
	return m_nClipCurrent;
}

// default for now
//int CBaseWeapon::GetAmmoType();

void CBaseWeapon::PrimaryAttack()
{
	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(GetClip() > 0)
	{
		SetClip(GetClip() - 1);

		Vector3D forward;
		pActor->EyeVectors( &forward );

		Vector3D shootPosition = pActor->GetEyeOrigin() + forward*5.0f; // TODO: GetWeaponShootPosition

		Vector3D real_eye_pos, ang;
		float fov;
		pActor->CalcView(real_eye_pos, ang, fov);

		internaltrace_t tr;
		physics->InternalTraceLine(real_eye_pos, real_eye_pos + forward * MAX_COORD_UNITS, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_ACTORS, &tr);
		Vector3D shootAngle = VectorAngles(normalize(tr.traceEnd-shootPosition));

		StartShootSound( shootPosition );
		FireBullets(GetBulletsPerShot(), shootPosition, shootAngle, 4096);

		SetActivity( ACT_VM_PRIMARYATTACK );

		Vector3D devation = GetDevationVector();
		devation = Vector3D(-devation.x, RandomFloat(-devation.y,devation.y), RandomFloat(-devation.z,devation.z));

		pActor->ViewPunch(devation + devation*((m_nShotsFired+1)*0.1f), true);
		pActor->SnapEyeAngles(pActor->GetEyeAngles() + Vector3D(RandomFloat(0.1,0.5),0,0)*((m_nShotsFired+1)*0.1f));

		m_nShotsFired++;
		m_fNextIdleTime = gpGlobals->curtime + m_fIdleTime;
	}
	else
	{
		SetActivity(ACT_VM_PRIMARYATTACK_EMPTY);
		EmitSound(GetEmptySound());
	}
}

Activity CBaseWeapon::TranslateActorActivity(Activity act)
{
	return act;
}

void CBaseWeapon::SetActivity(Activity act, int slot)
{
	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(!pActor)
		return;

	if(pActor->Classify() == ENTCLASS_PLAYER)
	{
		CWhiteCagePlayer* pPlayer = dynamic_cast<CWhiteCagePlayer*>(m_pOwner);

		if(pPlayer)
			pPlayer->GetViewmodel()->SetActivity(act, slot);
	}
	else
	{
		// translate viewmodel activities
		pActor->SetActivity( act, 2 );
	}
}

void CBaseWeapon::SecondaryAttack()
{

}
/*
class CMuzzleEffect : public IEffect
{
public:
	CMuzzleEffect(Vector3D &position, Vector3D &velocity, float StartSize, float EndSize, float lifetime, int nMaterial, float rotation)
	{
		InternalInit(position, lifetime, nMaterial);

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		rotate = rotation;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime;

		SetSortOrigin(m_vOrigin);

		float lifeTimePerc = GetLifetimePercent();

		PFXBillboard_t effect;

		effect.vOrigin = m_vOrigin;
		effect.vColor = color4_white;//*lifeTimePerc;
		effect.nGroupIndex = m_nMaterialGroup;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
		effect.fZAngle = rotate;

		effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
		effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		GR_DrawBillboard(&effect);

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_vVelocity;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		rotate;
};

class CMuzzleSmokeEffect : public IEffect
{
public:
	CMuzzleSmokeEffect(Vector3D &position, Vector3D &velocity, float StartSize, float EndSize, float lifetime, int nMaterial, float rotation)
	{
		InternalInit(position, lifetime, nMaterial);

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		rotate = rotation;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime;

		SetSortOrigin(m_vOrigin);

		float lifeTimePerc = GetLifetimePercent();

		PFXBillboard_t effect;

		effect.vOrigin = m_vOrigin;
		effect.vColor = color4_white*lifeTimePerc;
		effect.nGroupIndex = m_nMaterialGroup;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
		effect.fZAngle = lifeTimePerc*rotate;

		effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
		effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		GR_DrawBillboard(&effect);

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_vVelocity;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		rotate;
};
*/
void CBaseWeapon::FireBullets(int numBullets, Vector3D &origin, Vector3D &angles, float fMaxDist)
{
	Vector3D forward, up, right;

	dlight_t *light = viewrenderer->AllocLight();

	light->position = origin;
	light->color = Vector3D(0.35f,0.4f,0.95f);
	light->intensity = 1.0f;
	light->curintensity = 2.0f;
	light->radius = 320.0f;
	light->dietime = 0.1f;
	light->fadetime = 0.1f;
	light->curfadetime = 0.1f;
	light->nFlags |= LFLAG_NOSHADOWS;

	viewrenderer->AddLight(light);

	Vector3D randomizedAngle = Vector3D(RandomFloat(-1,1),RandomFloat(-1,1),RandomFloat(-1,1))*GetSpreadingMultiplier()*((m_nShotsFired+1)*0.5f);

	for(int bullet = 0; bullet < numBullets; bullet++)
	{
		AngleVectors(angles+Vector3D(RandomFloat(-1,1),RandomFloat(-1,1),RandomFloat(-1,1))*GetSpreadingMultiplier()+randomizedAngle, &forward, &right, &up);

		g_pBulletSimulator->FireBullet(origin, forward, this, GetOwner(), GetAmmoType());
	}
}

void CBaseWeapon::StartShootSound(Vector3D& origin)
{
	EmitSound_t emit;

	emit.fPitch = 1.0f;
	emit.fRadiusMultiplier = 1.0f;
	emit.fVolume = 1.0f;
	emit.origin = origin;
	emit.pObject = this;
	emit.origin -= GetAbsOrigin();

	emit.name = (char*)GetShootSound();

	EmitSoundWithParams(&emit);
}

void CBaseWeapon::MakeViewKick(Vector3D &kick)
{
	// default isn't kickable
}

void CBaseWeapon::BeginReload()
{
	if(GetClip() >= GetMaxClip())
		return;

	CBaseActor* pActor = (CBaseActor*)m_pOwner;

	if( pActor->GetClipCount(GetAmmoType()) == 0)
		return;

	m_bReload = true;

	if(GetClip() > 0)
	{
		SetActivity(ACT_VM_RELOAD_SHORT);
		EmitSound( GetFastReloadSound() );
	}
	else
	{
		SetActivity(ACT_VM_RELOAD_FULL);
		EmitSound( GetReloadSound() );
	}

	m_fPrimaryAttackTime = gpGlobals->curtime + GetCurrentAnimationDuration();
	m_fSecondaryAttackTime = gpGlobals->curtime + GetCurrentAnimationDuration();
}

float CBaseWeapon::GetCurrentAnimationDuration()
{
	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(!pActor)
		return 0.0f;

	if(pActor->Classify() == ENTCLASS_PLAYER)
	{
		CWhiteCagePlayer* pPlayer = (CWhiteCagePlayer*)m_pOwner;

		return pPlayer->GetViewmodel()->GetCurrentAnimationDuration();
	}
	else
	{
		return pActor->GetCurrentAnimationDuration();
	}

	return 0.0f;
}

void CBaseWeapon::FinishReload()
{
	CBaseActor* pActor = (CBaseActor*)m_pOwner;

	if(!pActor)
		return;

	if(GetClip() > 0)
		SetClip( GetMaxClip()+1 );
	else
		SetClip( GetMaxClip() );

	int nClipCount = pActor->GetClipCount(GetAmmoType());

	pActor->SetClipCount( GetAmmoType(), nClipCount-1 );

	m_bReload = false;
}

void CBaseWeapon::SetAiming(bool bAiming)
{
	m_bAiming = bAiming;
}

bool CBaseWeapon::IsAiming()
{
	return m_bAiming;
}