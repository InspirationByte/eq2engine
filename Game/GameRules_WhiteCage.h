//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: White Cage game rules
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMERULES_WHITECAGE_H
#define GAMERULES_WHITECAGE_H

#include "BaseEngineHeader.h"
#include "IGameRules.h"

class CInfoPlayerStart;

class CWhiteCageGameRules : public IGameRules
{
public:
	CWhiteCageGameRules();

	void						Spawn();
	void						InitRelationShips();

	GameType_e					GetGameType() {return GAME_SINGLEPLAYER;}

	void						RespawnPlayers();
	void						RespawnItems();
	void						UpdateGameRules();

	bool						IsSingleplayer() {return true;}
	bool						IsMultiplayer() {return false;}

	CBaseActor*					GetPlayerActorById(int id = 0);
	CBaseActor*					GetLocalPlayer();

	void						AddPlayerStart(CInfoPlayerStart* pStart);
	void						SetLocalPlayer(BaseEntity* pPlayer);

	bool						IsMenuMode() {return engine->GetGameState() == IEngineGame::GAME_RUNNING_MENU;}

	int							GetMaxPlayersCount() {return 1;}

	void						KillActor(int id = -1);

	void						ProcessDamageInfo( damageInfo_t& damage );

	RelationShipState_e			GetRelationshipState(EntRelClass_e nClass, EntRelClass_e nRelatedClass);
	void						SetRelationshipState(EntRelClass_e nClass, EntRelClass_e nRelatedClass, RelationShipState_e state);

protected:
	ubyte						m_nRelationShipStates[ENTCLASS_COUNT][ENTCLASS_COUNT];

	DkList<CBaseActor*>			m_pActorList;
	DkList<CInfoPlayerStart*>	m_pPlayerStarts;

	int							m_nLocalPlayerID;	// unused for now
};

#endif // GAMERULES_WHITECAGE_H