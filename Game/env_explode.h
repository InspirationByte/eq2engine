//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: env_explode
//////////////////////////////////////////////////////////////////////////////////**

#ifndef ENV_EXPLODE_H
#define ENV_EXPLODE_H

#include "BaseEngineHeader.h"
#include "BaseActor.h"

struct explodedata_t
{
	Vector3D	origin;
	float		magnitude;
	float		radius;
	bool		fast;
	float		damage;

	DkList<IPhysicsObject*> pIgnore;
	BaseEntity* pInflictor;
};

#define MAX_EXPLODE_RAYS 64 // random rays of explosions, if test fails

inline void ExplodeAddImpulseToEachObject(IPhysicsObject* pObject, void* data)
{
	explodedata_t* exp = (explodedata_t*)data;

	for(int i = 0; i < exp->pIgnore.numElem(); i++)
	{
		if(exp->pIgnore[i] == pObject)
			return;
	}

	Vector3D direction = pObject->GetPosition() - exp->origin;
	float dirLen = length(direction);

	// shake player views on triple distance

	if(dirLen < exp->radius*2)
	{
		float distToHit = length(pObject->GetPosition() - exp->origin);
		float finalmagnitude = exp->magnitude * (1.0f-(distToHit / (exp->radius*2)));
		BaseEntity* pDamaged = (BaseEntity*)pObject->GetEntityObjectPtr();

		if(pDamaged && pDamaged->Classify() == ENTCLASS_PLAYER)
		{
			CBaseActor* pActor = (CBaseActor*)pDamaged;

			pActor->ViewShake(finalmagnitude*4, 0.8f);
		}
	}
	
	if(dirLen < exp->radius)
	{
		trace_t tr;
		UTIL_TraceLine(exp->origin, pObject->GetPosition(), COLLIDE_PROJECTILES, &tr);

		if(tr.pt.hitObj == pObject && tr.fraction < 1)
		{
			float distToHit = length(tr.traceEnd - exp->origin);
			float finalmagnitude = exp->magnitude * (1.0f-(distToHit / exp->radius));
			BaseEntity* pDamaged = (BaseEntity*)pObject->GetEntityObjectPtr();

			if(pDamaged)
			{
				float final_damage = exp->damage * (1.0f-(distToHit / exp->radius));
				// apply damage to entity
				damageInfo_t damage;
				damage.trace = tr;
				damage.m_fDamage = final_damage;
				damage.m_fImpulse = finalmagnitude;
				damage.m_nDamagetype = DAMAGE_TYPE_BLAST;
				damage.m_pInflictor = exp->pInflictor;
				damage.m_pTarget = pDamaged;
				damage.m_vOrigin = tr.traceEnd;
				damage.m_vDirection = direction;

				g_pGameRules->ProcessDamageInfo( damage );
			}

			pObject->ApplyImpulse(normalize(direction)*finalmagnitude, tr.traceEnd - pObject->GetPosition());
		}
		else if(!exp->fast)
		{
			BoundingBox aabb;

			int numHits = 0;

			for(int i = 0; i < MAX_EXPLODE_RAYS; i++)
			{
				Vector3D randomized_angle(RandomFloat(-360,360),RandomFloat(-360,360),RandomFloat(-360,360));
				Vector3D forward;
				AngleVectors(randomized_angle, &forward);

				UTIL_TraceLine(exp->origin, exp->origin + forward * exp->radius, COLLIDE_PROJECTILES, &tr);

				if(tr.pt.hitObj == pObject && tr.fraction < 1)
				{
					numHits++;

					aabb.AddVertex(tr.traceEnd);
				}
			}

			if(numHits > 0)
			{
				Vector3D centerOfHits = aabb.GetCenter();
				Vector3D dir = centerOfHits - exp->origin;
				float distToHit = length(dir);

				float finalmagnitude = exp->magnitude * (1.0f-(distToHit / exp->radius));

				BaseEntity* pDamaged = (BaseEntity*)pObject->GetEntityObjectPtr();
				if(pDamaged)
				{
					float final_damage = exp->damage * (1.0f-(distToHit / exp->radius));

					// apply damage to entity
					damageInfo_t damage;
					damage.trace = tr;
					damage.m_fDamage = final_damage;
					damage.m_fImpulse = finalmagnitude;
					damage.m_nDamagetype = DAMAGE_TYPE_BLAST;
					damage.m_pInflictor = exp->pInflictor;
					damage.m_pTarget = pDamaged;
					damage.m_vOrigin = tr.traceEnd;
					damage.m_vDirection = dir;

					g_pGameRules->ProcessDamageInfo( damage );

					//pDamaged->ApplyDamage(pObject->GetPosition(), normalize(dir), exp->pInflictor, finalmagnitude, final_damage, DAMAGE_TYPE_BLAST);
				}

				pObject->ApplyImpulse(normalize(dir)*finalmagnitude, centerOfHits - pObject->GetPosition());
			}
		}
	}
}

#endif // ENV_EXPLODE_H