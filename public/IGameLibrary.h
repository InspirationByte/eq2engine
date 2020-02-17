//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game library abstract
//	Те самые интерфейсы, которые делают движок движком
//		IEngine
//		IEngineGame
//		IAudio
//		IPhysics
//		IMaterialSystem
//		IRenderer
//		ISceneRenderer
//		IDebugOverlay
//		IEntityFactory
//////////////////////////////////////////////////////////////////////////////////

#ifndef IGAMELIB_H
#define IGAMELIB_H

#include <stdio.h>
#include "Platform/Platform.h"
#include "GlobalVarsBase.h"

class IEngine;
class IViewRenderer;
class IDebugOverlay;
class IEngineGame;
class IPhysics;
class IMaterialSystem;
class IShaderAPI;
class ISoundSystem;
class IModelCache;
class KeyValues;

#define LOADINGSCREEN_SHOW		1
#define LOADINGSCREEN_DESTROY	2

class IGameLibrary
{
public:
								~IGameLibrary() {}

	virtual bool					Init(	ISoundSystem*			pSoundSystem,
											IPhysics*				pPhysics,
											IDebugOverlay*			pDebugOverlay,
											IViewRenderer*			pViewRenderer,
											IStudioModelCache*		pModelCache) = 0;

	virtual bool				GameLoad() = 0;
	virtual void				GameUnload() = 0;

	virtual void				GameStart() = 0;
	virtual void				GameEnd() = 0;
		
	virtual void				SpawnEntities(kvkeybase_t* inputKeyValues) = 0;

	virtual void				LoadingScreenFunction(int function, float fPercentage) = 0;

	virtual void				PreRender() = 0;
	virtual void				PostRender() = 0;
	virtual void				OnRenderScene() = 0;

	// key/mouse events goes here now
	virtual bool				ProcessKeyChar( ubyte ch ) = 0;
	virtual bool				Key_Event( int key, bool down ) = 0;
	virtual bool				Mouse_Event( float x, float y, int buttons, bool down ) = 0;
	virtual bool				MouseMove_Event( int x, int y, float deltaX, float deltaY ) = 0;
	virtual bool				MouseWheel_Event( int x, int y, int scroll) = 0;

	virtual void				Update(float decaytime) = 0;
	virtual float 				StateUpdate( int nGamestate, float frametime ) = 0;

	virtual GlobalVarsBase*		GetGlobalVars() = 0;
};

extern IGameLibrary* gamedll;


#endif //IGAMELIB_H
