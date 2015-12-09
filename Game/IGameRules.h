//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Camera for Engine
//////////////////////////////////////////////////////////////////////////////////

#ifndef IGAMERULES_H
#define IGAMERULES_H

#include "BaseActor.h"
#include "DamageInfo.h"

// developer stuff
enum GamePlayFlags_e
{
	GP_FLAG_GOD			= (1 << 0),
	GP_FLAG_NOCLIP		= (1 << 1),
	GP_FLAG_NOTARGET	= (1 << 2),
};

enum GameType_e
{
	GAME_UNKNOWN		= -1,
	GAME_SINGLEPLAYER,
};

enum RelationShipState_e
{
	RLS_NEUTRAL = 0,
	RLS_LIKE,
	RLS_HATE,
	RLS_FEAR,
};

class CInfoPlayerStart;

class IGameRules : public BaseEntity
{
public:
	virtual GameType_e				GetGameType() = 0;

	virtual void					RespawnPlayers() = 0;
	virtual void					RespawnItems() = 0;
	virtual void					UpdateGameRules() = 0;

	virtual bool					IsSingleplayer() = 0;
	//virtual bool					IsMultiplayer() = 0;

	virtual BaseEntity*				GetPlayerActorById(int id = 0) = 0;
	virtual	BaseEntity*				GetLocalPlayer() = 0;

	virtual void					AddPlayerStart(CInfoPlayerStart* pStart) = 0;
	virtual void					SetLocalPlayer(BaseEntity* pPlayer) = 0;

	virtual int						GetMaxPlayersCount() = 0;

	virtual bool					IsMenuMode() = 0;

	virtual void					Update(float dt) {UpdateGameRules();}

	virtual void					KillActor(int id = -1) = 0;

	virtual void					ProcessDamageInfo( damageInfo_t& damage ) = 0;

	//virtual float					GetDamageMultiplier(CBaseActor* pAct1, CBaseActor* pAct2) = 0;

	virtual RelationShipState_e		GetRelationshipState(EntRelClass_e nClass, EntRelClass_e nRelatedClass) = 0;
};

IGameRules* CreateGameRules(GameType_e type);
void DestroyGameRules( void );

extern IGameRules* g_pGameRules;

#endif // IGAMERULES_H