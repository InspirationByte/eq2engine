//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: White Cage game rules
//
// TODO: the game stats computed here//////////////////////////////////////////////////////////////////////////////////

#include "GameRules_WhiteCage.h"
#include "PlayerSpawn.h"

int nGPFlags = 0;
int nGPLastFlags = 0;


DECLARE_CMD( whereisamy ,"toggle player collision", CV_CHEAT | CV_CLIENTCONTROLS)
{
	nGPLastFlags = nGPFlags;

	if(nGPFlags & GP_FLAG_NOCLIP)
	{
		nGPFlags &= ~GP_FLAG_NOCLIP;
		Msg("Noclip DISABLED\n");
	}
	else
	{
		nGPFlags |= GP_FLAG_NOCLIP;
		Msg("Noclip ENABLED\n");
	}
}

DECLARE_CMD( imacyborg, "toggle invicibility mode", CV_CHEAT | CV_CLIENTCONTROLS)
{
	nGPLastFlags = nGPFlags;

	if(nGPFlags & GP_FLAG_GOD)
	{
		nGPFlags &= ~GP_FLAG_GOD;
		
		Msg("Invicibility mode DISABLED\n");
	}
	else
	{
		nGPFlags |= GP_FLAG_GOD;

		Msg("Invicibility mode ENABLED\n");
	}
}


DECLARE_CMD( friendzone, "toggles notarget", CV_CHEAT | CV_CLIENTCONTROLS)
{
	nGPLastFlags = nGPFlags;

	if(nGPFlags & GP_FLAG_NOTARGET)
	{
		nGPFlags &= ~GP_FLAG_NOTARGET;
		Msg("NoTarget DISABLED\n");
	}
	else
	{
		nGPFlags |= GP_FLAG_NOTARGET;
		Msg("NoTarget ENABLED\n");
	}
}

DECLARE_CMD(iddqd, "",CV_CHEAT | CV_CLIENTCONTROLS)
{
	Msg("old times, you know that!\n");
}

DECLARE_CMD(kill, "Kills player", 0)
{
	g_pGameRules->KillActor(0);
}

DECLARE_CMD(respawnplayers, "Respawns players", CV_CHEAT)
{
	g_pGameRules->RespawnPlayers();
}

DECLARE_CMD(give, "gives an item to player", CV_CHEAT)
{
	if(!g_pGameRules->GetLocalPlayer() || !args)
		return;

	if(args->numElem() <= 0)
		return;

	CBaseActor* pActor = (CBaseActor*)g_pGameRules->GetLocalPlayer();
	pActor->GiveWeapon(args->ptr()[0].GetData());
}

IGameRules* g_pGameRules = NULL;

IGameRules* CreateGameRules(GameType_e type)
{
	g_pGameRules = (IGameRules*)entityfactory->CreateEntityByName("whitecage_gamerules");
	g_pGameRules->Spawn();
	g_pGameRules->Activate();

	return g_pGameRules;
}

void DestroyGameRules( void )
{
	if(g_pGameRules)
	{
		g_pGameRules->SetState(BaseEntity::ENTITY_REMOVE);
		g_pGameRules = NULL;
	}
}

CWhiteCageGameRules::CWhiteCageGameRules()
{
	for(int i = 0; i < ENTCLASS_COUNT; i++)
		memset(m_nRelationShipStates[i], 0, ENTCLASS_COUNT);
}

void CWhiteCageGameRules::RespawnPlayers()
{
	if(IsMenuMode())
		return;

	for(int i = 0; i < m_pActorList.numElem(); i++)
	{
		m_pActorList[i]->SetState(ENTITY_REMOVE);
	}
	m_pActorList.clear();

	if(m_pPlayerStarts.numElem())
	{
		CBaseActor* pActor = (CBaseActor*)entityfactory->CreateEntityByName("player");
		if(pActor)
		{
			pActor->Precache();

			pActor->SnapEyeAngles(Vector3D(0,m_pPlayerStarts[0]->GetYawAngle(),0));
			pActor->SetAbsOrigin(m_pPlayerStarts[0]->GetAbsOrigin());

			// bind script
			//if(strlen(m_pPlayerStarts[0]->GetPlayerScriptName()) > 0)
			//	OBJ_BINDSCRIPT( m_pPlayerStarts[0]->GetPlayerScriptName(), pActor );

			pActor->Spawn();
			pActor->Activate();

			pActor->UpdateView();

			// setup entity indexes
			pActor->SetIndex(m_pActorList.numElem()+1);

			SetLocalPlayer(pActor);
		}
	}
	else
	{
		CBaseActor* pActor = (CBaseActor*)entityfactory->CreateEntityByName("player");
		if(pActor)
		{
			pActor->Precache();
			pActor->Spawn();
			pActor->Activate();

			// setup entity indexes
			pActor->SetIndex(m_pActorList.numElem()+1);

			SetLocalPlayer(pActor);
		}
	}
}

void CWhiteCageGameRules::Spawn()
{
	// check for the player if we have loaded the game

	// init basic relationships
	InitRelationShips();
}

void CWhiteCageGameRules::SetLocalPlayer(BaseEntity* pPlayer)
{
	m_pActorList.insert((CBaseActor*)pPlayer, 0);
	GR_SetViewEntity( pPlayer );
}

void CWhiteCageGameRules::RespawnItems()
{

}

void CWhiteCageGameRules::UpdateGameRules()
{

}

void CWhiteCageGameRules::KillActor(int id)
{
	if(m_pActorList.numElem() == 0)
		return;

	if(id == -1)
	{
		for(int i = 0; i < m_pActorList.numElem(); i++)
		{
			// kill players
			m_pActorList[i]->SetHealth(-1);
		}
	}
	else
	{
		if(id < m_pActorList.numElem())
		{
			m_pActorList[id]->SetHealth(-1);
		}
	}
}

CBaseActor* CWhiteCageGameRules::GetPlayerActorById(int id)
{
	if(m_pActorList.numElem() == 0)
		return NULL;

	return m_pActorList[id];
}

CBaseActor* CWhiteCageGameRules::GetLocalPlayer()
{
	if(m_pActorList.numElem() == 0)
		return NULL;

	return m_pActorList[0];
}

void CWhiteCageGameRules::AddPlayerStart(CInfoPlayerStart* pStart)
{
	m_pPlayerStarts.append(pStart);
}

void CWhiteCageGameRules::ProcessDamageInfo( damageInfo_t& damage )
{
	if(damage.m_pTarget)
	{
		damage.m_pTarget->ApplyDamage( damage );
	}
}

void CWhiteCageGameRules::InitRelationShips()
{
	SetRelationshipState( ENTCLASS_CYBORG, ENTCLASS_PLAYER, RLS_HATE );
	SetRelationshipState( ENTCLASS_CYBORG, ENTCLASS_CYBORG_FRIENDLY, RLS_HATE );
	SetRelationshipState( ENTCLASS_CYBORG, ENTCLASS_PLAYER_ALLY, RLS_HATE );
	SetRelationshipState( ENTCLASS_CYBORG, ENTCLASS_REBEL, RLS_HATE );
	SetRelationshipState( ENTCLASS_CYBORG, ENTCLASS_CITIZEN, RLS_NEUTRAL );

	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_PLAYER, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_CYBORG_FRIENDLY, RLS_FEAR );
	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_CYBORG, RLS_FEAR );
	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_CLEANINGBOT, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_REBEL, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CITIZEN, ENTCLASS_PLAYER_ALLY, RLS_LIKE );

	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_PLAYER, RLS_LIKE );
	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_CYBORG_FRIENDLY, RLS_FEAR );
	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_CYBORG, RLS_FEAR );
	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_CLEANINGBOT, RLS_LIKE );
	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_PLAYER_ALLY, RLS_LIKE );
	SetRelationshipState( ENTCLASS_REBEL, ENTCLASS_CITIZEN, RLS_LIKE );

	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_PLAYER, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_CITIZEN, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_CYBORG, RLS_HATE );
	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_CLEANINGBOT, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_REBEL, RLS_LIKE );
	SetRelationshipState( ENTCLASS_CYBORG_FRIENDLY, ENTCLASS_PLAYER_ALLY, RLS_LIKE );
}

RelationShipState_e	CWhiteCageGameRules::GetRelationshipState(EntRelClass_e nClass, EntRelClass_e nRelatedClass)
{
	// if NoTarget enabled and AI checks player...
	if((nGPFlags & GP_FLAG_NOTARGET) && nRelatedClass == ENTCLASS_PLAYER)
		return RLS_NEUTRAL;

	return (RelationShipState_e)m_nRelationShipStates[nClass][nRelatedClass];
}

void CWhiteCageGameRules::SetRelationshipState(EntRelClass_e nClass, EntRelClass_e nRelatedClass, RelationShipState_e state)
{
	m_nRelationShipStates[nClass][nRelatedClass] = state;
}

DECLARE_ENTITYFACTORY(whitecage_gamerules,CWhiteCageGameRules)