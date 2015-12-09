//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Realistic bullets manager
//////////////////////////////////////////////////////////////////////////////////

#include "BulletSimulator.h"
#include "IDebugOverlay.h"
#include "EntityQueue.h"
#include "coord.h"
#include "GlobalVarsBase.h"
#include "BaseEntity.h"
#include "Effects.h"
#include "IGameRules.h"

ConVar g_debug_bullets("g_debug_bullets", "0");
ConVar g_bullet_ultraricochet("g_bullet_ultraricochet", "0", "Cheat", CV_CHEAT);

CSimulatedBullet::CSimulatedBullet()
{
}

CSimulatedBullet::CSimulatedBullet(Vector3D &startpos, Vector3D &dir, BaseEntity* pShooter, BaseEntity* pInflictor, int ammo_type)
{
	m_vOrigin = startpos;
	m_fInitialSpeed = g_ammotype[ammo_type].startspeed;

	float inv_mass = 1.0f / g_ammotype[ammo_type].mass;

	// build valid gravitation
	m_vGravity = Vector3D(0, -(m_fInitialSpeed / inv_mass), 0);

	m_vVelocity = dir * m_fInitialSpeed;

	m_pInflictor = pInflictor;

	m_pShooter = pShooter;

	m_nAmmoType = ammo_type;
}

bool CSimulatedBullet::Simulate(float deltaTime)
{
	int ignored_objects = 0;

	if(!UTIL_CheckEntityExist(m_pShooter) && m_pShooter != UTIL_EntByIndex(0))
	{
		m_pShooter = (BaseEntity*)UTIL_EntByIndex(0); // set shooter to worldspawn
		ignored_objects = 0;
	}

	if(!UTIL_CheckEntityExist(m_pInflictor) && m_pInflictor != UTIL_EntByIndex(0))
	{
		m_pInflictor = (BaseEntity*)UTIL_EntByIndex(0); // set inflictor to worldspawn
		ignored_objects = 0;
	}

	Vector3D last_origin = m_vOrigin;

	m_vOrigin += m_vVelocity*deltaTime * 2.0f;

	m_vVelocity += m_vGravity*deltaTime;

	if(length(m_vOrigin) > MAX_COORD_UNITS)
		return false;

	BaseEntity* ignored[] = {m_pInflictor, m_pShooter};

	trace_t bullettrace;
	UTIL_TraceLine(last_origin, m_vOrigin, COLLIDE_PROJECTILES, &bullettrace, ignored, 2);

	if(bullettrace.fraction < 1.0f)
	{
		float power_percent = length(m_vVelocity) / m_fInitialSpeed;

		// apply damage to entity
		damageInfo_t damage;
		damage.trace = bullettrace;
		damage.m_fDamage = g_ammotype[m_nAmmoType].playerdamage * power_percent;
		damage.m_fImpulse = g_ammotype[m_nAmmoType].impulse * power_percent;
		damage.m_nDamagetype = DAMAGE_TYPE_BULLET;
		damage.m_pInflictor = m_pInflictor;
		damage.m_pTarget = bullettrace.hitEnt;
		damage.m_vOrigin = bullettrace.traceEnd;
		damage.m_vDirection = normalize(m_vVelocity);

		// first we process damage info event
		g_pGameRules->ProcessDamageInfo( damage );
		
		if(bullettrace.pt.hitObj)
			bullettrace.pt.hitObj->ApplyImpulse(damage.m_vDirection * damage.m_fImpulse, 
												damage.m_vOrigin - bullettrace.pt.hitObj->GetPosition());

		// make impact
		UTIL_Impact( bullettrace, normalize(m_vVelocity) );
		
		if(!ProcessCollision( &bullettrace ))
			return false;
	}

	float power_percent = length(m_vVelocity) / m_fInitialSpeed;

	if(power_percent < 0.1f)
		return false;

	FX_DrawTracer(m_vOrigin, normalize(m_vVelocity), 0.8f, 45, color4_white);

	if(g_debug_bullets.GetBool())
		debugoverlay->Line3D(last_origin, bullettrace.traceEnd, color4_white, color4_white, 10);

	return true;
}

bool CSimulatedBullet::ProcessCollision(trace_t* collided)
{
	// handle penetration
	if(!(g_ammotype[m_nAmmoType].flags & AMMOFLAG_NO_PENETRATE) && !g_bullet_ultraricochet.GetBool())
	{
		float angle_diff = fabs(dot(normalize(m_vVelocity), collided->normal));

		if(angle_diff > 0.5f)
		{
			Vector3D bullet_pos = collided->traceEnd - collided->normal*0.1f;
			Vector3D old_velocity = m_vVelocity;
	
			float current_speed = length(m_vVelocity);

			Vector3D dir = normalize(m_vVelocity);

			// now our bullet is in (huge) wall.
			// trace forward, find exit or smth.
				
			float exit_dist = 0;

			switch(collided->hitmaterial->surfaceword)
			{
				case 'C':
				{
					exit_dist = 16; // 16 units in concrete
					break;
				}
				case 'W':
				{
					exit_dist = 32; // 32 units in wood
					break;
				}
				case 'M':
				{
					exit_dist = 4; // 4 units in metal
					break;
				}
				case 'F':
				{
					exit_dist = 32; // 32 units in flesh
					break;
				}
				case 'S':
				{
					exit_dist = 64; // 64 units in concrete
					break;
				}
			}

			if(exit_dist <= 0)
				return false;

			trace_t penetrationTest;
			UTIL_TraceLine(bullet_pos, bullet_pos + dir*exit_dist, COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_PROJECTILES  | COLLISION_GROUP_DEBRIS, &penetrationTest);

			if(penetrationTest.fraction < 1.0f)
			{
				m_vOrigin = penetrationTest.traceEnd - penetrationTest.normal*0.15f;

				// apply direction randomization
				Vector3D bullet_angles = VectorAngles(dir);

				float saved_power_percent = (current_speed - (collided->hitmaterial->density * penetrationTest.fraction)) / current_speed;
	
				// randomize angles
				bullet_angles += Vector3D(RandomFloat(-90,90),RandomFloat(-90,90),RandomFloat(-90,90)) * (1.0f - saved_power_percent);
				AngleVectors(bullet_angles, &m_vVelocity);

				// do the power loss
				m_vVelocity *= current_speed*saved_power_percent;

				penetrationTest.normal *= -1;

				// make impact on exit
				UTIL_Impact(penetrationTest, penetrationTest.normal);

				return true;
			}
			else
				return false;
		}
	}

	if(g_bullet_ultraricochet.GetBool())
	{
		float current_speed = length(m_vVelocity);

		Vector3D new_dir = normalize(reflect(m_vVelocity, collided->normal));

		m_vVelocity = new_dir*current_speed*0.99f;

		m_vOrigin = collided->traceEnd + collided->normal*0.15f;

		// set shooter to worldspawn
		m_pShooter = (BaseEntity*)UTIL_EntByIndex(0);
		m_pInflictor = m_pShooter;
		
		return true;
	}

	// handle ricochet
	if(!(g_ammotype[m_nAmmoType].flags & AMMOFLAG_NO_RICOCHET))
	{
		Vector3D old_velocity = m_vVelocity;
	
		float current_speed = length(m_vVelocity);

		Vector3D new_dir = normalize(reflect(m_vVelocity, collided->normal));

		// apply direction randomization
		Vector3D bullet_angles = VectorAngles(new_dir);

		float saved_power_percent = dot(normalize(old_velocity), new_dir);

		// randomize angles
		bullet_angles += Vector3D(RandomFloat(-90,90),RandomFloat(-90,90),RandomFloat(-90,90)) * (1.0f - saved_power_percent);
		AngleVectors(bullet_angles, &m_vVelocity);

		m_vVelocity *= current_speed;

		// don't ricochet!
		if(saved_power_percent < 0.75f)
			return false;

		// do the power loss
		m_vVelocity *= (saved_power_percent*saved_power_percent*saved_power_percent);
		m_vOrigin = collided->traceEnd + collided->normal*0.15f;
	}

	return true;
}

//***************************************************
// Bullet simulator
//***************************************************

void CBulletSimulator::Cleanup()
{
	for(int i = 0; i < m_bullets.numElem(); i++)
	{
		CSimulatedBullet* pBullet = m_bullets[i];

		delete pBullet;
	}

	m_bullets.clear();
}

void CBulletSimulator::FireBullet(Vector3D &origin, Vector3D &dir, BaseEntity* pShooter, BaseEntity* pInflictor, int ammo_type)
{
	if(ammo_type == AMMOTYPE_INVALID)
		return;

	// create new bullet
	CSimulatedBullet* pBullet = new CSimulatedBullet(origin, dir, pShooter, pInflictor, ammo_type);

	// add it to simulation list
	m_bullets.append(pBullet);
}

void CBulletSimulator::SimulateBullets()
{
	for(int i = 0; i < m_bullets.numElem(); i++)
	{
		CSimulatedBullet* pBullet = m_bullets[i];

		// simulate or remove bullet
		if(!pBullet->Simulate(gpGlobals->frametime))
		{
			delete pBullet;
			m_bullets.fastRemoveIndex(i);
			i--;
		}
	}
}

static CBulletSimulator s_bulletSim;
CBulletSimulator* g_pBulletSimulator = &s_bulletSim;