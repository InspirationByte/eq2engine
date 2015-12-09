//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Wire mine, that attaches surfaces when in air
//////////////////////////////////////////////////////////////////////////////////

#include "BaseWeapon.h"
#include "BaseEngineHeader.h"
#include "Effects.h"
#include "env_explode.h"

#define VIEWMODEL_NAME "models/weapons/v_grenade.egf"
#define WORLDMODEL_NAME "models/weapons/w_grenade.egf"

#define MINE_JUMP_VELOCITY		(200.0f)
#define MINE_JUMP_MINVELOCITY	(260.0f) // if it slow enough, it will jump
#define MINE_MAX_WIRES			32
#define MINE_WIRE_MAXLENGTH		(400.0f)

class CWireMineProjectile : public BaseEntity
{
	DEFINE_CLASS_BASE(BaseEntity);
public:
	CWireMineProjectile()
	{
		m_bAttachFailed = false;
		m_bDeployed = false;
		m_fJumpTime = 1.0f;
		m_pAttacker = NULL;
		m_bShouldAttach = false;
	}

	void Precache()
	{
		PrecacheStudioModel(WORLDMODEL_NAME);
		PrecacheScriptSound("grenade.explode");
	}

	void SetAttacker(BaseEntity* pAttacker)
	{
		m_pAttacker = pAttacker;
	}

	void PhysicsCreateObjects()
	{
		if(m_pPhysicsObject)
			return;

		PhysicsInitNormal(COLLISION_GROUP_PROJECTILES, false);

		// collide only with world
		m_pPhysicsObject->SetCollisionMask( COLLIDE_PROJECTILES );
	}

	void OnRemove()
	{
		for(int i = 0; i < m_pRopes.numElem(); i++)
			physics->DestroyPhysicsRope(m_pRopes[i]);

		BaseClass::OnRemove();
	}

	void Spawn()
	{
		BaseClass::Spawn();

		SetModel( WORLDMODEL_NAME );

		PhysicsCreateObjects();

		SetHealth(15);

		m_pPhysicsObject->WakeUp();
		SetThink(&CWireMineProjectile::IdleThink);
	}

	void OnPreRender()
	{
		g_modelList->AddRenderable(this);
	}

	bool IsSomeObjectIntersectedWire()
	{
		return false;
	}

	void IdleThink()
	{
		// we can't jump from players
		int traceProps = COLLIDE_PROJECTILES;

		BaseEntity* pIgnored = this;

		// test for ground
		bool bOnGround = false;
		trace_t groundTrace;
		trace_t jumpTrace;

		UTIL_TraceBox(GetAbsOrigin(), GetAbsOrigin() + Vector3D(0, -5, 0), Vector3D(8), traceProps, &groundTrace, &pIgnored, 1);
		UTIL_TraceBox(GetAbsOrigin(), GetAbsOrigin() + Vector3D(0, -5, 0), Vector3D(64) , traceProps, &jumpTrace, &pIgnored, 1);

		bOnGround = (groundTrace.fraction < 1.0f);

		if( m_bShouldAttach && !bOnGround )
		{
			m_bShouldAttach = false;

			// generate and test ray directions
			Vector3D	tracePositions[MINE_MAX_WIRES];
			Vector3D	traceDirections[MINE_MAX_WIRES];
			bool		rayNeedsRope[MINE_MAX_WIRES];

			int numWiresCanBeAttached = 0;
			int numWiresCanBeInAir = 0;

			for(int i = 0; i < MINE_MAX_WIRES; i++)
			{
				// test with random angles
				// FIXME: read from bones?
				//Vector3D randomAngles(RandomFloat(-90, 90), RandomFloat(-180, 180), 0);

				float fPerc = float(i) / float(MINE_MAX_WIRES);

				Vector3D randomAngles(fPerc * 180.0f - 90.0f, fPerc * 1440.0f, 0);

				Vector3D fwDir;
				AngleVectors(randomAngles, &fwDir);

				trace_t tr;
				UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + fwDir*MINE_WIRE_MAXLENGTH, traceProps, &tr, &pIgnored, 1);

				rayNeedsRope[i] = false;

				if( tr.fraction < 1.0f )
				{
					if( !tr.pt.hitObj->IsStatic() )
					{
						rayNeedsRope[i] = false; // don't attach to non-generic physics object
						continue;
					}

					rayNeedsRope[i] = true;
					tracePositions[i] = tr.traceEnd;
					traceDirections[i] = fwDir;
					numWiresCanBeAttached++;

					if( dot(fwDir, Vector3D(0, 1, 0)) > 0.5 )
						numWiresCanBeInAir++;
				}
			}

			if( (numWiresCanBeInAir == 0) || (float(numWiresCanBeAttached) / float(MINE_MAX_WIRES)) < 0.25f )
			{
				m_bAttachFailed = true;
				SetNextThink( gpGlobals->curtime + 0.1f );
				return;
			}

			PhysicsGetObject()->SetActivationState( PS_INACTIVE );

			// make ropes now
			for(int i = 0; i < MINE_MAX_WIRES; i++)
			{
				if(!rayNeedsRope[i])
					continue;
				
				Vector3D posA = PhysicsGetObject()->GetPosition()+traceDirections[i]*5.0f;
				Vector3D posB = tracePositions[i];

				debugoverlay->Line3D(posA, posB, ColorRGBA(1,0,0,1), ColorRGBA(0,0,1,1), 5.0f);
				/*
				IPhysicsRope* pRope = physics->CreateRope(posA, posB, 2);
				pRope->SetFullMass( 5 );
				pRope->SetNodeMass(0, 0);
				pRope->SetNodeMass( pRope->GetNodeCount()-1, 1);
				pRope->SetIterations(1);
				pRope->SetLinearStiffness(0.5f);

				pRope->AttachObject(PhysicsGetObject(), pRope->GetNodeCount()-1, true);
				m_pRopes.append( pRope );
				*/
			}

			m_bDeployed = true;
		}

		if(!m_bDeployed && !m_bShouldAttach && !m_bAttachFailed)
		{
			m_fJumpTime -= gpGlobals->frametime;

			if(m_fJumpTime > 0.0f)
			{
				SetNextThink( gpGlobals->curtime );
				return;
			}

			// try to jump if me on ground and speed lower than MINE_JUMP_MINVELOCITY
			float fVelocity = length( PhysicsGetObject()->GetVelocity() );
			if( fVelocity < MINE_JUMP_MINVELOCITY )
			{
				if(bOnGround)
				{
					// jump by normal (if we near wall or something non-flat...)
					PhysicsGetObject()->ApplyImpulse(jumpTrace.normal*MINE_JUMP_VELOCITY, Vector3D(0,-5, 0));

					if(dot( jumpTrace.normal, groundTrace.normal ) > 0.7f)
						m_bShouldAttach = true;

					SetNextThink( gpGlobals->curtime + 0.5f );
					return;
				}
			}
			else
			{
				m_fJumpTime = 0.2f;
			}
		}

		if( IsDead() || m_bAttachFailed || IsSomeObjectIntersectedWire())
		{
			explodedata_t exp;
			exp.magnitude = 8000;
			exp.radius = 400;
			exp.damage = 300;
			exp.origin = GetAbsOrigin();
			exp.pInflictor = m_pAttacker;

			exp.pIgnore.append(m_pPhysicsObject);

			physics->DoForEachObject(ExplodeAddImpulseToEachObject, &exp);
			exp.fast = false;

			EmitSound("grenade.explode");

			SetState(ENTITY_REMOVE);

			UTIL_ExplosionEffect(GetAbsOrigin(), Vector3D(0,1,0), false);

			// create a light
			dlight_t *light = viewrenderer->AllocLight();
			SetLightDefaults(light);

			light->position = GetAbsOrigin() + Vector3D(0,15,0);
			light->color = Vector3D(0.9,0.4,0.25);
			light->intensity = 2;
			light->radius = 530;
			light->dietime = 0.1f;
			light->fadetime = 0.1f;

			viewrenderer->AddLight(light);
		}

		SetNextThink( gpGlobals->curtime );
	}

	void Update(float dt)
	{
		ObjectSoundEvents(m_pPhysicsObject, this);

		BaseEntity::SetAbsAngles(m_pPhysicsObject->GetAngles());
		BaseEntity::SetAbsOrigin(m_pPhysicsObject->GetPosition());

		BaseClass::Update(dt);
	}

private:
	float		m_fJumpTime;
	bool		m_bDeployed;
	bool		m_bShouldAttach;
	bool		m_bAttachFailed;

	BaseEntity*	m_pAttacker;

	DkList<IPhysicsRope*> m_pRopes;

	DECLARE_DATAMAP();
};

BEGIN_DATAMAP(CWireMineProjectile)
	DEFINE_FIELD(m_fJumpTime, VTYPE_FLOAT),
END_DATAMAP()

DECLARE_ENTITYFACTORY(projectile_wiremine, CWireMineProjectile)

class CWeaponWireMine : public CBaseWeapon
{
	DEFINE_CLASS_BASE(CBaseWeapon);

public:
	void Spawn();
	void Precache();

	void IdleThink();

	void OnPreRender();

	const char*	GetViewmodel();
	const char*	GetWorldmodel();

	const char*	GetShootSound()				{return "de.shoot";} // shoot sound script

	const char*	GetReloadSound()			{return "de.reload";} // reload sound script

	const char*	GetFastReloadSound()		{return "de.fastreload";} // reload sound script

	int			GetMaxClip() {return 1;}

	int			GetBulletsPerShot() {return 1;}

	float		GetSpreadingMultiplier()	{return 5.0f;}

	void		HandleEvent(AnimationEvent nEvent, char* options);

	void		Deploy();

// get it's slot
	int			GetSlot() {return WEAPONSLOT_GRENADE;}

protected:
	float		m_fLastShootTime;

	bool		m_bDeployed;

};

DECLARE_ENTITYFACTORY(weapon_wiremine, CWeaponWireMine)

void CWeaponWireMine::Spawn()
{
	m_nAmmoType = GetAmmoTypeDef("wiremine");

	m_fLastShootTime = 0.0f;

	m_bDeployed = false;

	BaseClass::Spawn();

	SetHealth(15);
}

void CWeaponWireMine::Precache()
{
	PrecacheEntity(projectile_wiremine);

	BaseClass::Precache();
}

const char*	CWeaponWireMine::GetViewmodel()
{
	return VIEWMODEL_NAME;
}

const char*	CWeaponWireMine::GetWorldmodel()
{
	return WORLDMODEL_NAME;
}

void CWeaponWireMine::Deploy()
{
	BaseClass::Deploy();

	BeginReload();
}

void CWeaponWireMine::OnPreRender()
{
	BaseClass::OnPreRender();

	if(GetOwner())
		return;

	if(IsDead())
	{
		explodedata_t exp;
		exp.magnitude = 7000;
		exp.radius = 350;
		exp.damage = 500;
		exp.origin = GetAbsOrigin();
		exp.pInflictor = NULL;

		exp.pIgnore.append(m_pPhysicsObject);

		physics->DoForEachObject(ExplodeAddImpulseToEachObject, &exp);
		exp.fast = false;

		EmitSound("grenade.explode");

		SetState(ENTITY_REMOVE);

		UTIL_ExplosionEffect(GetAbsOrigin(), Vector3D(0,1,0), false);

		// create a light
		dlight_t *light = viewrenderer->AllocLight();
		SetLightDefaults(light);

		light->position = GetAbsOrigin() + Vector3D(0,15,0);
		light->color = Vector3D(0.9,0.4,0.25);
		light->intensity = 2;
		light->radius = 530;
		light->dietime = 0.1f;
		light->fadetime = 0.1f;

		viewrenderer->AddLight(light);
	}
}

void CWeaponWireMine::HandleEvent(AnimationEvent nEvent, char* options)
{
	if((int)nEvent == 5)
	{
		m_bDeployed = true;
	}
	if((int)nEvent == 6)
	{
		m_bDeployed = false;

		CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

		if(!pActor)
			return;

		CWireMineProjectile* pProjectile = (CWireMineProjectile*)entityfactory->CreateEntityByName("projectile_wiremine");
		if(pProjectile)
		{
			pProjectile->Spawn();
			pProjectile->Activate();

			SetClip(GetClip() - 1);

			pProjectile->PhysicsGetObject()->AddObjectToIgnore( GetOwner()->PhysicsGetObject() );

			Vector3D fw,rt,up;
			AngleVectors(pActor->GetEyeAngles(), &fw, &rt, &up);

			// set owner of this owner
			pProjectile->SetAttacker(GetOwner());

			pProjectile->PhysicsGetObject()->SetPosition(pActor->GetEyeOrigin() + fw*10.0f+rt*2.0f+up*6.0f);
			pProjectile->PhysicsGetObject()->SetVelocity(fw * 980.0f + up*280.0f);
			pProjectile->PhysicsGetObject()->SetAngularVelocity(Vector3D(RandomFloat(-10,10),RandomFloat(-10,10),RandomFloat(-10,10)), 1.0f);

			// try to reload
			BeginReload();
		}
	}

	BaseClass::HandleEvent(nEvent, options);
}

void CWeaponWireMine::IdleThink()
{
	BaseClass::IdleThink();

	CBaseActor* pActor = dynamic_cast<CBaseActor*>(m_pOwner);

	if(!pActor)
		return;

	SetButtons(pActor->GetButtons());

	if(m_fLastShootTime < gpGlobals->curtime)
		m_nShotsFired = 0;

	if(m_fPrimaryAttackTime < gpGlobals->curtime && GetClip() > 0 && (m_nButtons & IN_PRIMARY) && !(m_nOldButtons & IN_PRIMARY) && !m_bDeployed)
	{
		BeginReload();

		SetActivity( ACT_GRN_PULLPIN );
	}

	if(!(m_nButtons & IN_PRIMARY) && m_bDeployed)
	{
		m_bDeployed = false;

		SetActivity( ACT_GRN_THROW );
	}

	SetOldButtons(pActor->GetButtons());
}