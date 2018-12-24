//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game library
//
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

#include "BaseEngineHeader.h"
#include "GameRenderer.h"
#include "IGameLibrary.h"

#include "utils\eqtimer.h"
//#include "utils\eqjobs.h"
#include "Coord.h"

#include "GameState.h"

#include "EngineEntities.h"

#include "GameInput.h"
#include "ParticleRenderer.h"

#include "EqUI/EqUI_Manager.h"
#include "EqUI/EqUI_Panel.h"

using namespace Threading;

extern void SaveRestoreInitDefaults();


ONLY_EXPORTS IGameLibrary* GameLibFactory( void ) {return gamedll;}

// Important interfaces
IEngineHost*			g_pEngineHost	= NULL;
IEngineGame*			engine			= NULL;
IEntityFactory*			entityfactory	= NULL;
IMaterialSystem*		materials		= NULL;
IViewRenderer*			viewrenderer	= NULL;
IEqLevel*				eqlevel			= NULL;

IShaderAPI*				g_pShaderAPI	= NULL;

IStudioModelCache*		g_studioModelCache = NULL;

// Common
ISoundSystem*			soundsystem		= NULL;
IPhysics*				physics			= NULL;
IEngineSceneRenderer*	scenerenderer	= NULL;
IDebugOverlay*			debugoverlay	= NULL;
IEqFontCache*			g_fontCache		= NULL;

// non-engine globals

void ImportEngineInterfaces()
{
	if(!g_pEngineHost)
		g_pEngineHost = (IEngineHost*)GetCore()->GetInterface( IENGINEHOST_INTERFACE_VERSION );

	if(!engine)
		engine = (IEngineGame*)GetCore()->GetInterface( IENGINEGAME_INTERFACE_VERSION );

	if(!entityfactory)
		entityfactory = (IEntityFactory*)GetCore()->GetInterface( IENTFACTORY_INTERFACE_VERSION );

	if(!materials)
		materials = (IMaterialSystem*)GetCore()->GetInterface( MATSYSTEM_INTERFACE_VERSION );

	if(!eqlevel)
		eqlevel	= (IEqLevel*)GetCore()->GetInterface( IEQLEVEL_INTERFACE_VERSION );

	if(!g_fontCache)
		g_fontCache = (IEqFontCache*)GetCore()->GetInterface(FONTCACHE_INTERFACE_VERSION);
}

class CGameLibrary : public IGameLibrary
{
public:
	friend class		BaseEntity;

						CGameLibrary();
						~CGameLibrary();

	bool					Init(	ISoundSystem*			pSoundSystem,
									IPhysics*				pPhysics,
									IDebugOverlay*			pDebugOverlay,
									IViewRenderer*			pViewRenderer,
									IStudioModelCache*		pModelCache);


	void				LoadingScreenFunction(int function, float fPercentage);

	bool				GameLoad();
	void				GameUnload();

	void				GameStart();
	void				GameEnd();

	void				PreRender();
	void				PostRender();
	void				OnRenderScene();

	void				SpawnEntities(kvkeybase_t* inputKeyValues);

	// key/mouse events goes here now
	bool				ProcessKeyChar( ubyte ch );
	bool				Key_Event( int key, bool down );
	bool				Mouse_Event( float x, float y, int buttons, bool down );
	bool				MouseMove_Event( int x, int y, float deltaX, float deltaY );
	bool				MouseWheel_Event( int x, int y, int scroll);

	void				Update(float decaytime);
	float 				StateUpdate( int nGamestate, float frametime );

	GlobalVarsBase*		GetGlobalVars();
private:
	CEqMutex			m_Mutex;

	float				m_loadingPercentage;
	float				m_fApproachPerc;
	float				m_fPercentage;
	float				m_fApprSpeed;
};

static CGameLibrary g_gameLib;
IGameLibrary *gamedll = (IGameLibrary*)&g_gameLib;

CGameLibrary::CGameLibrary()
{

}

CGameLibrary::~CGameLibrary()
{
	
}

GlobalVarsBase*	CGameLibrary::GetGlobalVars()
{
	return gpGlobals;
}

bool CGameLibrary::Init(ISoundSystem*			pSoundSystem,
						IPhysics*				pPhysics,
						IDebugOverlay*			pDebugOverlay,
						IViewRenderer*			pViewRenderer,
						IStudioModelCache*		pModelCache)
{
	// Initialize shared pointers
	soundsystem		= pSoundSystem;
	physics			= pPhysics;
	debugoverlay	= pDebugOverlay;
	viewrenderer	= pViewRenderer;
	g_studioModelCache = pModelCache;

	g_pShaderAPI = materials->GetShaderAPI();

	// СИСЬКИ! :3

	g_localizer->AddTokensFile("game");

	SaveRestoreInitDefaults();

	materials->AddDestroyLostCallbacks( ShutdownParticleBuffers, InitParticleBuffers );

	equi::Manager->Init();


	// init scripting system
	//GetScriptSys()->Init();

	m_loadingPercentage = 0.0f;
	m_fPercentage = 0;
	m_fApproachPerc = 0;

	m_fApprSpeed = 0.0f;

	return true;
}
// when game loads
bool CGameLibrary::GameLoad()
{
	GAME_STATE_InitGame();

	return true;
}

void CGameLibrary::GameUnload()
{
	GR_Shutdown();

	g_pBulletSimulator->Cleanup();

	ses->Shutdown();

	g_pAINavigator->Shutdown();

	GAME_STATE_UnloadGameObjects();
}

extern CWorldInfo* g_pWorldInfo;

void CGameLibrary::GameStart()
{
	// Don't forget to set game state
	if( strlen(g_pWorldInfo->GetMenuName()) > 0 )
	{
		engine->SetGameState( IEngineGame::GAME_RUNNING_MENU );
	}
	else
	{
		g_pEngineHost->SetCursorShow(false);

		engine->SetGameState( IEngineGame::GAME_IDLE );
	}

	// start a session
	GAME_STATE_GameStartSession( false );
}

void CGameLibrary::GameEnd()
{
	GAME_STATE_GameEndSession();
}

void CGameLibrary::SpawnEntities(kvkeybase_t* inputKeyValues)
{
	GAME_STATE_SpawnEntities(inputKeyValues);
}

/*
class CLoadingRenderThread : public CEqThread
{
public:
	CLoadingRenderThread()
	{
		m_nFunction = LOADINGSCREEN_SHOW;
		m_fPercentage = 0;
		m_fApproachPerc = 0;

		m_fApprSpeed = 0.0f;
	}

	int	Run()
	{
		Vector2D vSize = g_pEngineHost->GetWindowSize();

		float fBorderOffset = -32.0f;

		float fFullLineSize = vSize.x + fBorderOffset*2;

		float curTime = Platform_GetCurrentTime();
		float prevTime = Platform_GetCurrentTime();

		while( m_nFunction != LOADINGSCREEN_DESTROY )
		{
			curTime = Platform_GetCurrentTime();

			float fFrameTime = curTime - prevTime;

			CScopedMutex m(m_Mutex);

			materials->BeginFrame();
			g_pShaderAPI->Reset();

			materials->Setup2D( vSize.x, vSize.y );

			g_pShaderAPI->Clear( true, true, false );

			if(m_nFunction == LOADINGSCREEN_DESTROY)
				m_fApproachPerc = 1.0f;

			if(m_fApproachPerc < 0)
				m_fApproachPerc = 0.0f;

			if(m_fApproachPerc > 1.0f)
				m_fApproachPerc = 1.0f;

			Vertex2D_t verts[] = {MAKETEXQUAD(-fBorderOffset, vSize.y - 5 + fBorderOffset, -fBorderOffset + fFullLineSize*m_fApproachPerc, vSize.y + fBorderOffset, 0)};

			materials->DrawPrimitives2DFFP( PRIM_TRIANGLE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(0.25,0.35,0.28,1));

			if(m_fApproachPerc < m_fPercentage)
				m_fApproachPerc += fFrameTime * m_fApprSpeed;


			materials->EndFrame();

			prevTime = curTime;
		}

		return 0;
	}

	void SetFunction(int nFunction, float fPercentage)
	{
		m_Mutex.Lock();

		m_fApproachPerc = m_fPercentage;

		m_nFunction = nFunction;
		m_fPercentage = fPercentage;

		m_fApprSpeed = (fPercentage-m_fApproachPerc)*2.0f;
		m_fApprSpeed *= m_fApprSpeed;

		if(nFunction == 0)
		{
			m_fApproachPerc = 0.0f;
			m_fApprSpeed = 1.0f;
			m_fPercentage = 0.0f;
		}

		m_Mutex.Unlock();
	}

protected:

	int			m_nFunction;
	float		m_fApproachPerc;
	float		m_fPercentage;
	float		m_fApprSpeed;

	CEqMutex	m_Mutex;
};

CLoadingRenderThread s_loadingThread;
CLoadingRenderThread* g_pLoadingRender = &s_loadingThread;
*/

void CGameLibrary::LoadingScreenFunction(int function, float fPercentage)
{
	m_Mutex.Lock();

	m_loadingPercentage = fPercentage;
	m_fApproachPerc = m_fPercentage;

	m_fPercentage = fPercentage;

	m_fApprSpeed = (fPercentage-m_fApproachPerc)*2.0f;
	m_fApprSpeed *= m_fApprSpeed;

	if(function == 0)
	{
		m_fApproachPerc = 0.0f;
		m_fApprSpeed = 1.0f;
		m_fPercentage = 0.0f;
	}

	m_Mutex.Unlock();
}

void CGameLibrary::PreRender()
{
	GAME_STATE_Prerender();
}

ConVar r_showcamerainfo("r_showcamerainfo","0","Shows camera position", CV_ARCHIVE);
ConVar r_showfps("r_showfps","1","Shows current fps", CV_ARCHIVE);

void CGameLibrary::PostRender()
{
	// Post-rendering of post-processing effects, GUI, others
	GR_Postrender();

	g_pAINavigator->DebugRender();

	// draw all debug
	viewrenderer->DrawDebug();

	GAME_STATE_Postrender();

	materials->Setup2D(g_pEngineHost->GetWindowSize().x, g_pEngineHost->GetWindowSize().y);

	// Here draw all 2D stuff
	equi::Manager->SetViewFrame(IRectangle(IVector2D(0), g_pEngineHost->GetWindowSize()));
	equi::Manager->Render();

	if(r_showcamerainfo.GetBool() && g_pViewEntity)
	{
		Vector3D origin, angles;
		float fFOV;

		g_pViewEntity->CalcView(origin, angles, fFOV);

		eqFontStyleParam_t fstyle;
		fstyle.textColor = ColorRGBA(0,1,0,1.0f);
		g_pEngineHost->GetDefaultFont()->RenderText(varargs("--- Camera origin: %.1f %.1f %.1f ---",origin.x,origin.y,origin.z),Vector2D(25,55),fstyle);
	}

	if(r_showfps.GetBool())
	{
		// FPS status
		static float accTime = 0.1f;
		static int fps = 0;
		static int nFrames = 0;

		if (accTime > 0.1f)
		{
			fps = (int) (nFrames / accTime + 0.5f);
			nFrames = 0;
			accTime = 0;
		}

		accTime += engine->GetFrameTime();
		nFrames++;

		ColorRGBA col(1,0,0,1);

		if(fps > 25)
		{
			col = ColorRGBA(1,1,0,1);
			if(fps > 45)
				col = ColorRGBA(0,1,0,1);
		}

		eqFontStyleParam_t fstyle;
		fstyle.textColor = col;
		g_pEngineHost->GetDefaultFont()->RenderText(varargs("FPS: %d on world '%s'\nCurrent game time: %f\n", fps, gpGlobals->worldname, gpGlobals->curtime), Vector2D(0, 16), fstyle);
	}
}

extern IWaterInfo*	g_pCurrentWaterInfo;

extern ConVar		r_debugwaterreflection;

void CGameLibrary::OnRenderScene()
{
	if(engine->GetGameState() == IEngineGame::GAME_LEVEL_LOAD)
	{
		// draw loading screens

		IVector2D vSize = g_pEngineHost->GetWindowSize();

		float fBorderOffset = -32.0f;

		float fFullLineSize = vSize.x + fBorderOffset*2;

		float fFrameTime = engine->GetFrameTime();

		g_pShaderAPI->Reset();

		materials->Setup2D( vSize.x, vSize.y );

		g_pShaderAPI->Clear( true, true, false, ColorRGBA(0.0,0.0,0.0,1.0) );

		if(m_fApproachPerc < 0)
			m_fApproachPerc = 0.0f;

		if(m_fApproachPerc > 1.0f)
			m_fApproachPerc = 1.0f;

		Vertex2D_t verts[] = {MAKETEXQUAD(-fBorderOffset, vSize.y - 5 + fBorderOffset, -fBorderOffset + fFullLineSize*m_fApproachPerc, vSize.y + fBorderOffset, 0)};

		materials->DrawPrimitives2DFFP( PRIM_TRIANGLE_STRIP, verts, elementsOf(verts), NULL, ColorRGBA(0.55,0.0,0.0,1));

		if(m_fApproachPerc < m_loadingPercentage)
			m_fApproachPerc += fFrameTime * m_fApprSpeed;

		return;
	}

	// no level loaded?
	if(!eqlevel->IsLoaded())
		return;

	// wait for SimulateEnts
	//g_pJobList->Wait();

	// recognize water info 
	// TODO: GetBestVisibleWaterInfo
	if(g_pViewEntity)
		g_pCurrentWaterInfo = eqlevel->GetNearestWaterInfo( g_pViewEntity->GetAbsOrigin() );

	if( g_pCurrentWaterInfo)
	{
		Vector3D mins, maxs;
		mins = g_pCurrentWaterInfo->GetBounds().minPoint;
		maxs = g_pCurrentWaterInfo->GetBounds().maxPoint;

		// draw reflections
		if(viewrenderer->GetViewFrustum()->IsBoxInside(	mins.x, maxs.x, 
														mins.y,maxs.y,
														mins.z,maxs.z ))
		{
			sceneinfo_t origSceneInfo = viewrenderer->GetSceneInfo();

			// if waterinfo present, draw reflection first
			GR_DrawScene( VR_FLAG_WATERREFLECTION );

			// clear depth or you'll get broken skybox
			g_pShaderAPI->ChangeRenderTargetToBackBuffer();
			g_pShaderAPI->Clear( false, true, false );

			// reset draw mode
			viewrenderer->SetDrawMode( VDM_AMBIENT );
			viewrenderer->SetupRenderTargetToBackbuffer();
			
			//g_pShaderAPI->Clear(true, true, false);
			g_pShaderAPI->Reset();
			g_pShaderAPI->SetDepthRange(0.0f,1.0f);

			viewrenderer->SetSceneInfo( origSceneInfo );
		}
	}

	// draw scene
	if(r_debugwaterreflection.GetInt() != 2)
		GR_DrawScene();

	GR_PostDraw();
}

void Game_Update(float decaytime)
{
	// Will update entities of the game
	for(int i = 0; i < ents->numElem(); i++)
	{
		BaseEntity *pEnt = ents->ptr()[i];

		if(!pEnt)
			continue;

		// entity will be removed on next frame
		if(pEnt->GetState() == BaseEntity::ENTITY_REMOVE)
		{
			pEnt->UpdateOnRemove();

			if(pEnt)
				delete pEnt;

			ents->removeIndex(i);
			i--;

			continue;
		}

		if(pEnt->GetState() != BaseEntity::ENTITY_NO_UPDATE)
		{
			if(	pEnt->GetAbsOrigin().x > MAX_COORD_UNITS || pEnt->GetAbsOrigin().x < MIN_COORD_UNITS ||
				pEnt->GetAbsOrigin().y > MAX_COORD_UNITS || pEnt->GetAbsOrigin().y < MIN_COORD_UNITS ||
				pEnt->GetAbsOrigin().z > MAX_COORD_UNITS || pEnt->GetAbsOrigin().z < MIN_COORD_UNITS)
			{
				MsgError("Error! entity '%s' outside the WORLD_SIZE limits (> %d units)\n",pEnt->GetClassname(),MAX_COORD_UNITS);
					
				pEnt->SetState(BaseEntity::ENTITY_NO_UPDATE);
			}
			
			pEnt->Simulate( decaytime );
		}
	}
}

void CGameLibrary::Update(float decaytime)
{
	float measureses = MEASURE_TIME_BEGIN();

	//GetScriptSys()->Update(decaytime);

	ses->Update();

	debugoverlay->Text(Vector4D(1,1,1,1), "  soundemittersystem: %.2f ms", MEASURE_TIME_STATS(measureses));

	debugoverlay->Text(Vector4D(1,1,1,1), " Num entities: %i", ents->numElem());

	float measure = MEASURE_TIME_BEGIN();

	Game_Update(decaytime);

	//g_pJobList->AddJob((jobFunction_t)Game_Update, *(void**)&decaytime);
	//g_pJobList->Submit();

	// service our output events
	g_EventQueue.ServiceEvents();

	// simulate bullets
	g_pBulletSimulator->SimulateBullets();

	debugoverlay->Text(Vector4D(1,1,1,1), "  entities: %.2f ms", MEASURE_TIME_STATS(measure));
}

float CGameLibrary::StateUpdate( int nGamestate, float frametime )
{
	// Game state (running\paused, etc)
	return GAME_STATE_FrameUpdate(frametime);
}

// key/mouse events goes here now
bool CGameLibrary::ProcessKeyChar( ubyte ch )
{
	// TODO: GUI ENTRY
	// if( eqgui->GetMainPanel()->IsVisible() )

	return true;
}

bool CGameLibrary::Key_Event( int key, bool down )
{
	// TODO: GUI KEYS AND SOME GAME KEYS

	// skip cutscenes for controls
	//if(g_pGameRules->IsCutScenePlaying())
	//	return false;
	//

	//if( engine->GetGameState() != IEngineGame::GAME_RUNNING_MENU )
	//	return true;

	if(equi::Manager->ProcessKeyboardEvents( key, down))
		return false;

	return true;
}

bool CGameLibrary::Mouse_Event( float x, float y, int buttons, bool down )
{
	// TODO: GUI MOUSE BUTTONS

	//if( engine->GetGameState() != IEngineGame::GAME_RUNNING_MENU )
	//	return true;

	if(equi::Manager->ProcessMouseEvents( x, y, buttons, down))
		return false;

	return true;
}

bool CGameLibrary::MouseMove_Event( int x, int y, float deltaX, float deltaY )
{
	// TODO: GUI/GAME MOUSE MOVEMENT

	if(engine->GetGameState() == IEngineGame::GAME_IDLE)
	{
		IN_MouseMove( deltaX, deltaY );
	}

	return true;
}

bool CGameLibrary::MouseWheel_Event( int x, int y, int scroll)
{
	// TODO: GUI MOUSE WHEEL EVENT

	return true;
}