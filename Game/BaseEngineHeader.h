//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base game header
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEENGINEHEADER_H
#define BASEENGINEHEADER_H

// core base header
#include "core_base_header.h"
#include "ILocalize.h"

// engine
#include "IEngineHost.h"
#include "IEngineGame.h"

#include "GameTrace.h"		// with idkphysics

// scene managing, rendering
#include "BaseRenderableObject.h"
#include "IViewRenderer.h"
#include "IEngineWorld.h"
#include "materialsystem/IMaterialSystem.h"
#include "IDebugOverlay.h"
#include "IFontCache.h"

// sound system
#include "ISoundSystem.h"

// entity-specific
#include "IEntityFactory.h"
#include "EntityQueue.h"
#include "BaseEntity.h"
#include "BulletSimulator.h"

// scripts
//#include "EqGMS.h"

// math
#include "Math/Random.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "math/math_util.h"

// Sound
#include "GameSoundEmitterSystem.h"

// AI
#include "AI/ai_navigator.h"

#define EQ_AUDIO_MAX_DISTANCE	 (10000.0f)

extern BaseEntity* g_pViewEntity;

#define PrecacheScriptSound(snd)	g_sounds->PrecacheSound(snd)

#define PrecacheStudioModel(mod)	g_studioModelCache->PrecacheModel(mod)

#define PrecacheEntity(ent)			\
{	BaseEntity* pCacheEnt = (BaseEntity*)entityfactory->CreateEntityByName(#ent);\
	if(pCacheEnt != NULL)											\
	{																\
		pCacheEnt->Precache();										\
		delete pCacheEnt;											\
	}																\
}																	\

#endif //BASEENGINEHEADER_H