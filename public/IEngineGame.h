//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine interface for game
//				Contains base instuments for game
//////////////////////////////////////////////////////////////////////////////////

#ifndef IENGINEGAME_H
#define IENGINEGAME_H

// Keys
#include "in_keys_ident.h"
#include "IEqModel.h"
#include "GlobalVarsBase.h"
#include "ISoundSystem.h"

#define IENGINEGAME_INTERFACE_VERSION "IEngineGame_002"

typedef bool (*pfnSavedGameLoader)( void* pData );

class IEngineGame : public ICoreModuleInterface
{
public:
	enum
	{
		GAME_NOTRUNNING = 0,
		GAME_RUNNING_MENU,
		GAME_LEVEL_LOAD,
		GAME_LEVEL_UNLOAD,
		GAME_PAUSE,
		GAME_IDLE
	};

	virtual						~IEngineGame( void ) { }
	
	virtual bool				Init(void) = 0;
	virtual void				Shutdown(void) = 0;
	
	virtual void				ProcessKeyChar		( ubyte ch ) = 0;
	virtual void				Key_Event			( int key, bool down ) = 0;
	virtual void				Mouse_Event			( float x, float y, int buttons, bool down ) = 0;
	virtual void				MouseMove_Event		( int x, int y, float deltaX, float deltaY ) = 0;
	virtual void				MouseWheel_Event	( int x, int y, int scroll) = 0;

	virtual int					GetGameState( void ) = 0;
	virtual void				SetGameState( int gamestateType ) = 0;

	virtual double				GetFrameTime( void ) = 0;

	virtual bool				LoadSaveGame( pfnSavedGameLoader func, void* pData ) = 0;

	virtual bool				SetLevelName( char const* pszLevelName ) = 0;
	virtual	bool				LoadGameCycle( bool loadSaved = false ) = 0;
	virtual void				UnloadGame(bool freeCache, bool force = false) = 0;
	virtual bool				IsLevelChanged() = 0;

	virtual bool				EngineRunFrame( float dTime ) = 0;
	virtual bool				FilterTime( float dTime ) = 0;
	virtual double				StateUpdate( void ) = 0;
	virtual void				Reset( bool frameTimeOnly = false ) = 0;

	//virtual void				SetUpdatesEnabled(bool enabled);

	virtual void				SetCenterMouseCursor(bool enable) = 0;
};

extern IEngineGame *engine;

#endif //IENGINEGAME_H