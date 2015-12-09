//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game state objects and handling
//
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "GameRenderer.h"
#include "EngineEntities.h"
#include "IGameRules.h"
#include "ParticleRenderer.h"

#define FRAMES_TO_START_GAME 1

// some globals
sndEffect_t* g_pSoundEffect = NULL;
int first_game_update_frames = 0;

CWorldInfo* g_pWorldInfo = NULL;

// sets world info entity
void SetWorldSpawn(BaseEntity* pEntity)
{
	g_pWorldInfo = dynamic_cast<CWorldInfo*>(pEntity);
}

void GAME_STATE_InitGame()
{
	first_game_update_frames = 0;

	GR_Init();

	soundsystem->SetListener(vec3_zero,Vector3D(0,0,1),Vector3D(0,1,0), vec3_zero);
}

void GAME_STATE_GameStartSession( bool bFromSavedGame )
{
	g_pSoundEffect = NULL;

	g_pEngineHost->SetCursorPosition(g_pEngineHost->GetWindowSize()/2);

	if(!bFromSavedGame)
		g_pGameRules->RespawnPlayers();

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(!pEnt)
			return;

		// if we're not from saved game, call OnGameStart
		if(!bFromSavedGame)
			pEnt->OnGameStart();

		pEnt->OnMapSpawn(); // Call OnMapSpawn event
	}
	
	// init dynamics after game start
	GR_InitDynamics();

	g_pAINavigator->InitNavigationMesh();

	//MsgInfo("GameStartSession\n");

	engine->Reset();

	materials->Wait();
}

void GAME_STATE_UnloadGameObjects()
{
	g_EventQueue.Clear();

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt)
		{
			pEnt->UpdateOnRemove();
			delete pEnt;
		}
	}

	// clean ents
	ents->clear();

	g_pWorldInfo = NULL;

	effectrenderer->RemoveAllEffects();

	g_pAINavigator->Shutdown();

	GR_Cleanup();
}


void GAME_STATE_GameEndSession()
{
	//Msg("GameEndSession\n");

	ses->StopAllSounds();
	ses->Update();

	g_pSoundEffect = NULL;
	soundsystem->SetListener(vec3_zero, Vector3D(0,0,1), Vector3D(0,1,0), NULL);
	soundsystem->Update();

	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		pEnt->SetState( BaseEntity::ENTITY_REMOVE );

		pEnt->OnGameEnd();
	}

	DestroyGameRules();
}

// caches all scripted sounds of physics surface params
void PrecachePhysicsSounds()
{
	DkList<phySurfaceMaterial_t*> *pMatList = physics->GetMaterialList();

	for(int i = 0; i < pMatList->numElem(); i++)
	{
		PrecacheScriptSound(pMatList->ptr()[i]->footStepSound);
		PrecacheScriptSound(pMatList->ptr()[i]->lightImpactSound);
		PrecacheScriptSound(pMatList->ptr()[i]->heavyImpactSound);
		PrecacheScriptSound(pMatList->ptr()[i]->bulletImpactSound);
		PrecacheScriptSound(pMatList->ptr()[i]->scrapeSound);
	}
}

void FillEntityParameters(BaseEntity* pEnt, kvkeybase_t* pEntSection)
{
	for(int j = 0; j < pEntSection->keys.numElem(); j++)
	{
		kvkeybase_t* pKey = pEntSection->keys[j];

		pEnt->SetKey(pKey->name, pKey->values[0]);
	}
}


void GAME_STATE_SpawnEntities( KeyValues* inputKeyValues )
{
	// Initialize sound emitter system
	ses->Init();

	PrecachePhysicsSounds();

	// create gamerules with worldspawn
	IGameRules* pGameRules = CreateGameRules( GAME_SINGLEPLAYER );

	gpGlobals->curtime = 0.0f;
	gpGlobals->frametime = 0.0f;
	gpGlobals->timescale = 1.0f;

	DkList<BaseEntity*> pEnts;

	if(inputKeyValues)
	{
		pEnts.resize(inputKeyValues->GetRootSection()->keys.numElem());

		for(int i = 0; i < inputKeyValues->GetRootSection()->keys.numElem(); i++)
		{
			kvkeybase_t* pEntSec = inputKeyValues->GetRootSection()->keys[i];

			if(!pEntSec->keys.numElem())
				continue;

			BaseEntity* pEntity = NULL;
			EqString sClassName = "null";

			// search for classname first
			for(int j = 0; j < pEntSec->keys.numElem(); j++)
			{
				if(!stricmp(pEntSec->keys[j]->name,"classname"))
				{
					sClassName = pEntSec->keys[j]->values[0];
					pEntity = (BaseEntity*)entityfactory->CreateEntityByName(sClassName.GetData());

					if(!stricmp(pEntSec->keys[j]->values[0], "worldinfo" ))
						g_pWorldInfo = (CWorldInfo*)pEntity;

					break;
				}
			}

			if(pEntity)
			{
				// begin from it and go recursively from this func
				FillEntityParameters( pEntity, pEntSec );

				UniqueRegisterEntityForUpdate( pEntity );

				pEnts.append(pEntity);
			}
		}
	}

	for(int i = 0; i < pEnts.numElem(); i++)
	{
		pEnts[i]->Precache();

		pEnts[i]->Spawn();
		pEnts[i]->Activate();
	}

	pGameRules->Activate();
}

float GAME_STATE_FrameUpdate(float frametime)
{
	if( first_game_update_frames < FRAMES_TO_START_GAME )
		return 0;

	return frametime;
}

void GAME_STATE_Prerender()
{
	// setup scene
	GR_SetupScene();

	// call entity prerender event
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt->GetState() != BaseEntity::ENTITY_REMOVE && pEnt->GetState() != BaseEntity::ENTITY_NO_UPDATE)
			pEnt->OnPreRender();
	}

	// setup sound listener and
	if(g_pViewEntity)
	{
		Vector3D origin, angles;
		float fFOV;

		g_pViewEntity->CalcView(origin, angles, fFOV);

		// setup sound
		Vector3D vecForward, vecUp;
		AngleVectors(angles,&vecForward,NULL,&vecUp);

		soundsystem->SetListener(origin, vecForward, vecUp, vec3_zero, g_pSoundEffect);
	}
}

bool	g_bChangeTimescale = false;
ConVar	g_timescale_speed("g_timescale_speed", "20");
float	g_fTimescale_FadeTo = 1.0f;

DECLARE_CMD(timescale, "Sets timescale", CV_CHEAT)
{
	g_bChangeTimescale = true;

	if(args && args->numElem() > 0)
	{
		g_fTimescale_FadeTo = atof(args->ptr()[0].GetData());
	}
}

void GAME_STATE_Postrender()
{
	if(g_bChangeTimescale)
	{
		float actual_timescale = gpGlobals->timescale;

		actual_timescale = lerp(actual_timescale, g_fTimescale_FadeTo, g_timescale_speed.GetFloat()*g_pEngineHost->GetFrameTime());

		gpGlobals->timescale = actual_timescale;

		if(actual_timescale == g_fTimescale_FadeTo)
			g_bChangeTimescale = false;
	}

	//g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	materials->Setup2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);

	// Will update entities of the game (Post-Render)
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(pEnt->GetState() != BaseEntity::ENTITY_REMOVE && pEnt->GetState() != BaseEntity::ENTITY_NO_UPDATE)
			pEnt->OnPostRender();
	}

	if(engine->GetGameState() == IEngineGame::GAME_PAUSE)
	{
		static const char* pausetext = "pause";

		Rectangle_t winRect(20,20,720,150);

		g_pEngineHost->GetDefaultFont()->DrawSetColor(ColorRGBA(1,1,1,1.0f));
		g_pEngineHost->GetDefaultFont()->DrawTextInRect(pausetext,winRect,35,35);
	}

	if(first_game_update_frames == FRAMES_TO_START_GAME)
	{
		for(int i = 0; i < ents->numElem(); i++)
		{
			BaseEntity *pEnt = ents->ptr()[i];

			if(!pEnt)
				continue;

			if(pEnt->GetState() != BaseEntity::ENTITY_NO_UPDATE)
				pEnt->FirstUpdate();
		}
	}

	first_game_update_frames++;

	if(first_game_update_frames > FRAMES_TO_START_GAME+5)
		first_game_update_frames = FRAMES_TO_START_GAME+5;
}