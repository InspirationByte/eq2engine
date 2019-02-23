//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Hybrid City soldier
//////////////////////////////////////////////////////////////////////////////////

#include "BaseActor.h"
#include "BaseWeapon.h"
#include "ragdoll.h"

#include "GameInput.h"

#include "AI/ai_navigator.h"


class CNPCTestSoldier : public CBaseActor
{
public:
	DEFINE_CLASS_BASE(CBaseActor);

	CNPCTestSoldier();

	void Precache();
	void Spawn();
	void OnRemove();

	void AliveThink();
	//void DeathThink();

	void UpdateView();

	void GiveWeapon(const char *classname);

	// min bbox dimensions
	Vector3D GetBBoxMins();

	// max bbox dimensions
	Vector3D GetBBoxMaxs();

	// returns world transformation of this object
	Matrix4x4 GetRenderWorldTransform();

	void Render(int nViewRenderFlags);

	void OnPreRender();

	char* GetScriptFilename() {return "scripts/npcs/soldier.txt";}

	void	DeathSound();

	void	ReleaseRagdoll();

	void	ApplyDamage(Vector3D &origin, Vector3D &direction, BaseEntity* pInflictor, float fHitImpulse, float fDamage, DamageType_e type);

protected:
	//DECLARE_DATAMAP();

	bool m_bHasSeenPlayer;

	CAINavigationPath m_TargetPath;
	// CAINavigationPath m_CoverPath;
	// CAINavigationPath m_EnemyPath;

	int m_numShots;
	float m_fNextShootEndTime;
	float m_fNextShootTime;
	float m_fPlayerSeeReset;
	float m_fRouteCalculateNextTime;

	ragdoll_t* m_pRagdoll;

	slight_t	m_static_lights[8];

	int			m_pRooms[32];
	int			m_nRooms;

	Vector3D	m_vecOldPos;
};

DECLARE_ENTITYFACTORY(npc_soldier, CNPCTestSoldier)

CNPCTestSoldier::CNPCTestSoldier() : CBaseActor()
{
	m_pRagdoll = NULL;
	m_numShots = NULL;
	m_fNextShootEndTime = 0;
	m_fNextShootTime = 0;
	m_bHasSeenPlayer = false;
	m_fPlayerSeeReset = 0.0f;
	m_fRouteCalculateNextTime = 0.0f;
	m_nMaxHealth = 100;
}

void CNPCTestSoldier::Precache()
{
	PrecacheStudioModel("models/characters/devushka.egf");
	PrecacheScriptSound("soldier.enemyspotted");
	PrecacheScriptSound("soldier.enemylost");
	PrecacheScriptSound("soldier.death");
}

void CNPCTestSoldier::Spawn()
{
	SetModel("models/characters/devushka.egf");
	BaseClass::Spawn();
	SetActivity(ACT_IDLE);
//	m_TargetPath.SetTimeToUpdate(1.0f);

	GiveWeapon("weapon_ak74");
}

void CNPCTestSoldier::OnRemove()
{
	if(m_pRagdoll)
		DestroyRagdoll(m_pRagdoll);

	m_pRagdoll = NULL;

	BaseClass::OnRemove();
}

void CNPCTestSoldier::DeathSound()
{
	EmitSound_t ep;
	ep.fPitch = 1.0;
	ep.fRadiusMultiplier = 1.0f;
	ep.fVolume = 1;
	ep.origin = Vector3D(0,0,0);

	ep.name = (char*)"soldier.death";

	// attach this sound to entity
	ep.pEntity = this;

	g_sounds->EmitSound(&ep);
}

void CNPCTestSoldier::GiveWeapon(const char *classname)
{
	CBaseWeapon* pWeapon = (CBaseWeapon*)entityfactory->CreateEntityByName(classname);

	if(!pWeapon)
		return;

	if(pWeapon->Classify() == CLASS_WEAPON)
	{
		pWeapon->Spawn();
		pWeapon->Activate();

		if(!PickupWeapon(pWeapon))
		{
			pWeapon->SetState(IEntity::ENTITY_REMOVE);
			return;
		}

		if(GetActiveWeapon() == NULL)
		{
			pWeapon->SetActivity(ACT_WPN_DEPLOY);
			SetActiveWeapon(pWeapon);
		}
	}
	else
	{
		pWeapon->SetState(IEntity::ENTITY_REMOVE);
	}
}

/*
void CNPCTestSoldier::DeathThink()
{

}
*/


void CNPCTestSoldier::AliveThink()
{
	CBaseActor* pPlayer = (CBaseActor*)UTIL_EntByIndex(1);

	if(m_fRouteCalculateNextTime < gpGlobals->curtime)
	{
		if(pPlayer && pPlayer->IsAlive() && m_bHasSeenPlayer)
		{
			g_pAINavigator->FindPath(&m_TargetPath, GetAbsOrigin(), pPlayer->GetAbsOrigin());
			m_fRouteCalculateNextTime = gpGlobals->curtime + 1.0f;
		}
		else
		{
			g_pAINavigator->FindPath(&m_TargetPath, GetAbsOrigin(), GetAbsOrigin()+Vector3D(RandomFloat(-500, 500), RandomFloat(-500, 500), RandomFloat(-500, 500)));
			m_fRouteCalculateNextTime = gpGlobals->curtime + 4.0f;
		}
	}

	// Turn our camera to the waypoint vector
	Vector3D targetWay = m_TargetPath.GetCurrentWayPoint().position;
	Vector3D targetLook = m_TargetPath.GetCurrentWayPoint().position;

	internaltrace_t tr;
	physics->InternalTraceLine(GetEyeOrigin(), pPlayer->GetAbsOrigin(), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS | COLLISION_GROUP_ACTORS, &tr, &m_pPhysicsObject, 1);

	Matrix4x4 proj = perspectiveMatrixY(DEG2RAD(100), 1,1,1,10000);
	Vector3D vRadianRotation = VDEG2RAD(GetEyeAngles());
	Matrix4x4 view = rotateZXY(-vRadianRotation.x,-vRadianRotation.y,-vRadianRotation.z);
	view.translate(-GetEyeOrigin());

	Matrix4x4 viewproj = proj*view;

	Volume frustumVolume;
	frustumVolume.LoadAsFrustum(viewproj);

	if(tr.fraction != 1.0f && tr.m_pHit == pPlayer->PhysicsGetObject() && frustumVolume.IsPointInside(pPlayer->GetAbsOrigin()) && pPlayer->IsAlive())
	{
		targetLook = pPlayer->GetAbsOrigin();

		m_nActorButtons &= ~IN_RELOAD;

		if(m_fNextShootTime < gpGlobals->curtime)
		{
			if(m_fNextShootEndTime > gpGlobals->curtime)
			{
				m_nActorButtons |= IN_PRIMARY;
			}
			else
			{
				m_nActorButtons &= ~IN_PRIMARY;
				m_fNextShootTime = gpGlobals->curtime + 0.6f;
				m_fNextShootEndTime = m_fNextShootTime+RandomFloat(0.04f, 0.2f);
			}
		}

		if(!m_bHasSeenPlayer)
		{
			EmitSound("soldier.enemyspotted");
			m_bHasSeenPlayer = true;
		}

		m_fPlayerSeeReset = gpGlobals->curtime+10.0f;
	}
	else
	{
		m_nActorButtons &= ~IN_PRIMARY;

		if(m_fPlayerSeeReset < gpGlobals->curtime && m_bHasSeenPlayer)
		{
			m_bHasSeenPlayer = false;
			EmitSound("soldier.enemylost");
		}
	}

	/*
	if(m_bHasSeenPlayer)
		m_TargetPath.SetTimeToUpdate(1.0f);
	else
		m_TargetPath.SetTimeToUpdate(4.0f);
		*/

	if(GetActiveWeapon() && GetActiveWeapon()->GetClip() == 0)
	{
		m_nActorButtons &= ~IN_PRIMARY;
		m_nActorButtons |= IN_RELOAD;
	}

	Vector3D unNormDir = targetLook-GetAbsOrigin();

	Vector3D dir = normalize(unNormDir);
	Vector3D angles = VectorAngles(dir);

	Vector3D eye_right;
	AngleVectors(angles, NULL, &eye_right);

	SnapEyeAngles(angles);

	BoundingBox box;
	m_pPhysicsObject->GetAABB(box.minPoint, box.maxPoint);

	Vector3D pos = m_vecAbsOrigin-Vector3D(0, box.GetSize().y*0.5f, 0);

	if(m_TargetPath.GetNumWayPoints() > 0/* && (GetActiveWeapon() && GetActiveWeapon()->GetClip() == 0)*/)
	{
		
		Vector3D move_dir = m_TargetPath.GetCurrentWayPoint().plane.normal; //normalize(targetWay - pos);

		debugoverlay->Line3D(GetAbsOrigin(), GetAbsOrigin() + move_dir*100.0f, ColorRGBA(1,0,0,1), ColorRGBA(1,0,0,1), 0.0f);

		debugoverlay->Box3D(targetWay-Vector3D(16,0,16), targetWay+Vector3D(16,32,16), ColorRGBA(1,0,0,1), 0.0f);
		
		m_nActorButtons |= IN_FORCERUN | IN_FORWARD;
		m_vMoveDir = Vector3D(move_dir.x,0,move_dir.z);
		m_bMovementPressed = true;
		
	}
	else
	{
		m_vMoveDir = vec3_zero;
		m_bMovementPressed = false;
	}

	if(m_TargetPath.GetCurrentWayPoint().plane.Distance(pos) > -1.0f /*length(pos - targetWay) < 128.0f*/)
		m_TargetPath.AdvanceWayPoint();

	BaseClass::AliveThink();
}

void CNPCTestSoldier::ApplyDamage(Vector3D &origin, Vector3D &direction, BaseEntity* pInflictor, float fHitImpulse, float fDamage, DamageType_e type)
{
	BaseClass::ApplyDamage( origin, direction, pInflictor, fHitImpulse, fDamage, type);

	if(IsDead())
	{
		/*
		m_vMoveDir = vec3_zero;
		m_bMovementPressed = false;

		// remove me
		SetState(IEntity::ENTITY_REMOVE);
		*/

		ReleaseRagdoll();
	}
}

void CNPCTestSoldier::ReleaseRagdoll()
{
	if(!m_pRagdoll)
	{
		m_pRagdoll = CreateRagdoll(m_pModel);

		Matrix4x4 object_transform = GetRenderWorldTransform();

		for(int i = 0; i < m_numBones; i++)
			m_boneTransforms[i] = m_boneTransforms[i] * object_transform;

		m_pRagdoll->SetBoneTransform(m_boneTransforms);

		//m_pRagdoll->m_pJoints[0]->GetPhysicsObjectA()->SetLinearFactor(vec3_zero);
		//m_pRagdoll->m_pJoints[0]->GetPhysicsObjectA()->SetAngularFactor(vec3_zero);
		
		m_pRagdoll->Translate(m_vecAbsOrigin);
	}
}

void CNPCTestSoldier::OnPreRender()
{
	if(IsDead())
	{
		m_pRagdoll->GetVisualBonesTransforms( m_boneTransforms );
	}
	else
		UpdateBones();

	g_modelList->AddRenderable(this);

	m_TargetPath.DebugRender();

	// update looking
	int pitchContorol	= FindPoseController("aim_pitch");
	int moveContorol	= FindPoseController("move_yaw");

	SetPoseControllerValue(pitchContorol, -GetEyeAngles().x);

	Vector3D right;
	AngleVectors(Vector3D(0, GetEyeAngles().y, 0), NULL, &right);

	float fDot = dot(normalize(PhysicsGetObject()->GetVelocity()), right);

	if(length(PhysicsGetObject()->GetVelocity().xz()) > 5)
	{
		SetPoseControllerValue(moveContorol, fDot * 90);

		if(GetCurrentActivity() != ACT_RUN)
			SetActivity(ACT_RUN);
	}
	else
	{
		if(GetCurrentActivity() != ACT_IDLE)
			SetActivity(ACT_IDLE);
	}

	float speedfactor = (180 - length(PhysicsGetObject()->GetVelocity().xz())) / 180.0f;

	SetPlaybackSpeedScale(clamp(1.0f - speedfactor, 0, 1));

	BaseClass::OnPreRender();
	
	//SetAbsOrigin(PhysicsGetObject()->GetPosition() - Vector3D(0,ACTOR_HEIGHT / 2,0));
	//SetAbsAngles(Vector3D(0,GetEyeAngles().y,0));
}

void CNPCTestSoldier::UpdateView()
{

}

// min bbox dimensions
Vector3D CNPCTestSoldier::GetBBoxMins()
{
	return m_pModel->GetBBoxMins();
}

// max bbox dimensions
Vector3D CNPCTestSoldier::GetBBoxMaxs()
{
	return m_pModel->GetBBoxMaxs();
}

// returns world transformation of this object
Matrix4x4 CNPCTestSoldier::GetRenderWorldTransform()
{
	if(IsDead())
		return translate(m_pRagdoll->GetPosition());

	BoundingBox box;
	m_pPhysicsObject->GetAABB(box.minPoint, box.maxPoint);

	Vector3D center = m_vecAbsOrigin-Vector3D(0, box.GetSize().y*0.5f, 0);

	return ComputeWorldMatrix(center, Vector3D(0,m_vecEyeAngles.y,0), Vector3D(1));
}

void CNPCTestSoldier::Render(int nViewRenderFlags)
{
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

	if( viewrenderer->GetAreaList() && viewrenderer->GetView() )
	{
		Vector3D bbox_center = (mins+maxs)*0.5f;
		float bbox_size = length(mins-maxs);

		Vector3D checkVec = m_vecOldPos-GetAbsOrigin();

		if(dot(checkVec, checkVec) > 40)
		{
			m_vecOldPos = GetAbsOrigin();
			m_nRooms = eqlevel->GetRoomsForSphere( GetAbsOrigin()+bbox_center, bbox_size, m_pRooms );

			viewrenderer->GetNearestEightLightsForPoint(GetAbsOrigin(), m_static_lights);
		}

		int view_rooms[2] = {-1, -1,};
		viewrenderer->GetViewRooms(view_rooms);

		int cntr = 0;

		for(int i = 0; i < m_nRooms; i++)
		{
			if(m_pRooms[i] != -1)
			{
				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].doRender)
				{
					cntr++;
					continue;
				}

				if(!viewrenderer->GetAreaList()->area_list[ m_pRooms[i] ].IsSphereInside(m_vecAbsOrigin, bbox_size))
				{
					cntr++;
					continue;
				}
			}
		}

		if(cntr == m_nRooms)
			return;
	}

	studiohdr_t* pHdr = m_pModel->GetHWData()->studio;
	int nLod = 0;

	for(int i = 0; i < pHdr->numBodyGroups; i++)
	{
		int nLodableModelIndex = pHdr->pBodyGroups(i)->lodModelIndex;
		int nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];

		while(nModDescId == -1 && nLod > 0)
		{
			nLod--;
			nModDescId = pHdr->pLodModel(nLodableModelIndex)->modelsIndexes[nLod];
		}

		if(nModDescId == -1)
			continue;

		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numGroups; j++)
		{
			materials->SetSkinningEnabled(true);
			viewrenderer->DrawModelPart(m_pModel, nModDescId, j, nViewRenderFlags, m_static_lights, m_boneTransforms);
		}
	}
}