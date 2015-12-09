//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game state objects and handling
//
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAME_STATE_H
#define GAME_STATE_H

class BaseEntity;

void	PrecachePhysicsSounds();

void	SetWorldSpawn(BaseEntity* pEntity);
void	GAME_STATE_InitGame();
void	GAME_STATE_GameStartSession( bool bFromSavedGame );
void	GAME_STATE_UnloadGameObjects();
void	GAME_STATE_GameEndSession();
void	GAME_STATE_SpawnEntities( KeyValues* inputKeyValues );

float	GAME_STATE_FrameUpdate(float frametime);
void	GAME_STATE_Prerender();
void	GAME_STATE_Postrender();

#endif // GAME_STATE_H
