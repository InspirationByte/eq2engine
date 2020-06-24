//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers world renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef WORLD_H
#define WORLD_H

#include "luabinding/LuaBinding.h"

#include "materialsystem/IMaterialSystem.h"

#include "level.h"
#include "worldenv.h"

#include "predictablerandom.h"

#include "ViewParams.h"
#include "scene_def.h"

#include "GameObject.h"

#ifdef EDITOR
#include "../DriversEditor/level_generator.h"
#include "../DriversEditor/EditorLevel.h"
#else
#include "ShadowRenderer.h"
#endif // EDITOR

class CBillboardList;

enum ERenderFlags
{
	RFLAG_FLIP_VIEWPORT_X	= (1 << 25),
	RFLAG_NOINSTANCE		= (1 << 26),
	RFLAG_SHADOW			= (1 << 27),
	RFLAG_TRANSLUCENCY		= (1 << 28)
};

// 2 regs, 52 lights, 128 regs (PS/VS), can be reduced to 48
#define MAX_LIGHTS				16
#define MAX_LIGHTS_INST			6
#define MAX_LIGHTS_QUEUE		128
#define MAX_LIGHTS_TEXTURE		32

#define MAX_RENDERABLE_LIST		512		// maximum objects drawn

struct worldinfo_t
{
	ColorRGBA	sunColor;
	ColorRGBA	ambientColor;

	Vector3D	sunDir;
	float		rainBrightness;

	FogInfo_t	fogInfo;
};

//----------------------------------------------------------------------

struct lensFlareTable_t
{
	float		scale;
	int			atlasIdx;
	ColorRGB	color;
};

struct WorldGlobals_t
{
	struct mtstruct_t {
		mtstruct_t();

		CEqSignal				decalsCompleted;
		CEqSignal				sheetsCompleted;
		CEqSignal				treesCompleted;

		CEqSignal				effectsUpdateCompleted;
		CEqSignal				rainUpdateCompleted;

		CEqInterlockedInteger	decalsQueue;
		CEqInterlockedInteger	sheetsQueue;
		CEqInterlockedInteger	treeQueue;
	} mt;

	IVertexFormat*			vehicleVF;
	IVertexFormat*			gameObjectVF;
	IVertexFormat*			levelObjectVF;

	IMaterial*				licPlatesMat;

	IVertexBuffer*			objectInstBuffer;

	TexAtlasEntry_t*		trans_grasspart;
	TexAtlasEntry_t*		trans_smoke2;
	TexAtlasEntry_t*		trans_raindrops;
	TexAtlasEntry_t*		trans_rainripple;
	TexAtlasEntry_t*		trans_fleck;

	TexAtlasEntry_t*		veh_skidmark_asphalt;
	TexAtlasEntry_t*		veh_raintrail;
	TexAtlasEntry_t*		veh_shadow;
};

#define LENSFLARE_TABLE_SIZE 12

class CGameWorld : public IMaterialRenderParamCallbacks, public CBaseNetworkedObject
{
	friend class CGameObject;
	friend class CGameSessionBase;
	friend class CNetGameSession;
	friend class CLevelRegion;
	friend class CState_Game;
	friend class CShadowRenderer;

public:
	DECLARE_NETWORK_TABLE()
	DECLARE_CLASS( CGameWorld, CBaseNetworkedObject )

	CGameWorld();
	virtual ~CGameWorld() {}

	void							Init();
	void							Cleanup( bool unloadLevel = true );

	void							RemoveAllObjects();

	//-------------------------------------------------------------------------

	void							SetLevelName(const char* name);
	const char*						GetLevelName() const;

	bool							LoadLevel();

#ifdef EDITOR
	bool							LoadPrefabLevel();
#endif // EDITOR

	//-------------------------------------------------------------------------
	// objects

	// adds object to world, returns ID
	int								AddObject(CGameObject* pObject);

	void							RemoveObject(CGameObject* pObject);
	void							RemoveObjectById(int objectId);
	bool							IsValidObject(CGameObject* pObject) const;

	CGameObject*					CreateGameObject( const char* typeName, kvkeybase_t* kvdata ) const;
	CGameObject*					CreateObject( const char* objectDefName ) const;
	CGameObject*					FindObjectByName( const char* objectName ) const;

	void							QueryObjects(DkList<CGameObject*>& list, float radius, const Vector3D& position, void* caller, bool(*comparator)(CGameObject* obj, void* caller)) const;

	int								AddObjectDef(const char* type, const char* name, kvkeybase_t* kvs);

#ifndef EDITOR
	OOLUA::Table					L_FindObjectOnLevel( const char* name ) const;
	OOLUA::Table					L_QueryObjects(float radius, const Vector3D& position, OOLUA::Lua_func_ref compFunc) const;
#endif // EDITOR

	//-------------------------------------------------------------------------
	// world simulation

	void							UpdateEnvironmentTransition( float fDt );

	void							ForceUpdateObjects();
	void							SpawnPendingObjects();

	void							UpdateWorld( float fDt );
	void							UpdateTrafficLightState( float fDt );

	// queries nearest regions and loads them in separate thread
	// you can wait for load if needed
	void							QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);

	bool							IsQueriedRegionLoaded() const;

	//-------------------------------------------------------------------------
	// world rendering

	float							GetFrameTime() const { return m_frameTime; }

	void							BuildViewMatrices(int width, int height, int nRenderFlags);
	void							UpdateOccludingFrustum();

	void							Draw( int nRenderFlags );

	bool							AddLight(const wlight_t& light);
	int								GetLightIndexList(const BoundingBox& bbox, int* lights, int maxLights = MAX_LIGHTS_INST) const;
	int								GetLightList(const BoundingBox& bbox, wlight_t list[MAX_LIGHTS]) const;

	void							SetView(const CViewParams& params);
	CViewParams*					GetView();

	//-------------------------------------------------------------------------
	// visual settings presets

	void							SetEnvironmentName(const char* name);
	const char*						GetEnvironmentName() const;
	void							InitEnvironment();

	void							FillEnviromentList(DkList<EqString>& list);

	CBillboardList*					GetBillboardList(const char* name);

public:
	CViewParams						m_view;

#ifdef EDITOR
	CEditorLevel					m_level;
#else
	CGameLevel						m_level;
#endif // EDITOR

	occludingFrustum_t				m_occludingFrustum;

	worldinfo_t						m_info;
	bool							m_reflectionStage;

	worldEnvConfig_t				m_envConfig;
	DkList<worldEnvConfig_t>		m_envTransitions;

	Vector2D						m_envSkyProps;

	float							m_envWetness;
	float							m_envTransitionTime;
	float							m_envTransitionTotalTime;

	float							m_envMapRegenTime;

	float							m_trafficLightTime[2];
	int								m_trafficLightPhase[2];

protected:

	static void						UpdateWorldAndEffectsJob(void* data, int i);

	void							OnObjectSpawnedEvent(CGameObject* obj);
	void							OnObjectRemovedEvent(CGameObject* obj);

	// matsystem callback
	void							OnPreApplyMaterial( IMaterial* pMaterial );

	void							SimulateObjects( float fDt );
	void							DrawReflections();

	void							DrawLensFlare( const Vector2D& screenSize, const Vector2D& screenPos, float intensity );
	void							DrawMoon();
	void							DrawMoonGlow( const Vector2D& screenSize, const Vector2D& screenPos, float intensity );

	void							UpdateRenderables( const occludingFrustum_t& frustum );
	void							UpdateLightTexture();

	void							DrawSkyBox(int nRenderFlags);

	void							GenerateEnvmapAndFogTextures();


	//--------------------------------------

	DkList<CBillboardList*>			m_billboardModels;

	lensFlareTable_t				m_lensTable[LENSFLARE_TABLE_SIZE];
	float							m_lensIntensityTiming;

	IOcclusionQuery*				m_sunGlowOccQuery;	// sun glow
	IMaterial*						m_pointQueryMat;

	ITexture*						m_lightsTex;		// vertex texture fetch lights

	ITexture*						m_reflectionTex;	// reflection texture
	ITexture*						m_tempReflTex;		// temporary reflection buffer
	ITexture*						m_reflDepth;

	ITexture*						m_noiseTex;			// noise for shaders

	IMaterial*						m_blurYMaterial;	// y blur shader

	ISoundController*				m_rainSound;

	DkList<CGameObject*>			m_gameObjects;
	DkList<CGameObject*>			m_nonSpawnedObjects;

	DkFixedLinkedList<CGameObject*, MAX_RENDERABLE_LIST>
									m_renderingObjects;

	EqString						m_levelname;

	ITexture*						m_envMap;
	ITexture*						m_fogEnvMap;
	bool							m_envMapsDirty;

	IEqModel*						m_skyModel;

	IMaterial*						m_skyMaterial;
	IMatVar*						m_skyColor;
	IMatVar*						m_skyTexture1;
	IMatVar*						m_skyTexture2;
	IMatVar*						m_skyInterp;

	EqString						m_envName;

	wlight_t						m_lights[MAX_LIGHTS_QUEUE];
	int								m_numLights;

	CNetworkVar(float,				m_fNextThunderTime);
	CNetworkVar(float,				m_fThunderTime);

	float							m_frameTime;
	float							m_waterWavesTime;

	sceneinfo_t						m_sceneinfo;

	CLevelRegion*					m_lastQueriedRegion;

public:

	bool							m_levelLoaded;

	Matrix4x4						m_viewprojection;
	Matrix4x4						m_matrices[4];

	Volume							m_frustum;

#ifndef EDITOR
	CShadowRenderer					m_shadowRenderer;
#endif // EDITOR

	int								m_objectIndexCounter;
};

extern WorldGlobals_t				g_worldGlobals;

extern CPredictableRandomGenerator	g_replayRandom;

extern CGameWorld*					g_pGameWorld;

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CViewParams)
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC_CONST(GetOrigin)
	OOLUA_MFUNC_CONST(GetAngles)
	OOLUA_MFUNC_CONST(GetFOV)

	OOLUA_MFUNC( SetOrigin )
	OOLUA_MFUNC( SetAngles )
	OOLUA_MFUNC( SetFOV )
OOLUA_PROXY_END

OOLUA_PROXY(CGameWorld)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(SetEnvironmentName)
	OOLUA_MFUNC_CONST(GetEnvironmentName)

	OOLUA_MFUNC(SetLevelName)
	OOLUA_MFUNC_CONST(GetLevelName)

	OOLUA_MFUNC(GetView)

	OOLUA_MFUNC(AddObjectDef)
	OOLUA_MFUNC_CONST(CreateGameObject)
	OOLUA_MFUNC_CONST(CreateObject)
	OOLUA_MFUNC(RemoveObject)
	OOLUA_MFUNC_CONST(IsValidObject)

	OOLUA_MEM_FUNC_CONST(maybe_null<CGameObject*>, FindObjectByName, const char*)

	OOLUA_MEM_FUNC_CONST_RENAME(FindObjectOnLevel, OOLUA::Table, L_FindObjectOnLevel, const char*)

	OOLUA_MEM_FUNC_CONST_RENAME(QueryObjects, OOLUA::Table, L_QueryObjects, float, const Vector3D&, OOLUA::Lua_func_ref)

	OOLUA_MFUNC(QueryNearestRegions)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // WORLD_H
