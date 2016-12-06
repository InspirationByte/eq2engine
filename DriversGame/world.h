//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers world renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef WORLD_H
#define WORLD_H

#include "luabinding/LuaBinding.h"

#include "materialsystem/IMaterialSystem.h"
#include "input.h"
#include "level.h"
#include "GameObject.h"

#include "predictablerandom.h"

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
	RFLAG_INSTANCE			= (1 << 26),
	RFLAG_TRANSLUCENCY		= (1 << 27)
};

// 2 regs, 52 lights, 128 regs (PS/VS), can be reduced to 48
#define MAX_LIGHTS				16
#define MAX_LIGHTS_INST			6
#define MAX_LIGHTS_QUEUE		128
#define MAX_LIGHTS_TEXTURE		32

#define MAX_RENDERABLE_LIST		256		// maximum objects drawn

struct worldinfo_t
{
	ColorRGBA	sunColor;
	ColorRGBA	ambientColor;

	Vector3D	sunDir;
	float		rainBrightness;

	FogInfo_t	fogInfo;
};

//----------------------------------------------------------

enum EWorldLights
{
	WLIGHTS_NONE = 0,

	WLIGHTS_CARS	= (1 << 0),
	WLIGHTS_LAMPS	= (1 << 1),
	WLIGHTS_CITY	= (1 << 2),
};

enum EWeatherType
{
	WEATHER_TYPE_CLEAR = 0,		// fair weather
	WEATHER_TYPE_RAIN,			// slight raining
	WEATHER_TYPE_THUNDERSTORM,	// heavy raining along with thunder

	WEATHER_COUNT,
};

struct worldEnvConfig_t
{
	worldEnvConfig_t()
	{
		skyboxMaterial = NULL;
	}

	EqString		name;

	ColorRGB		sunColor;
	ColorRGB		ambientColor;
	ColorRGBA		shadowColor;

	float			lensIntensity;

	float			brightnessModFactor;
	float			streetLightIntensity;

	float			rainBrightness;
	float			rainDensity;

	Vector3D		sunAngles;
	Vector3D		sunLensDirection;

	int				lightsType;
	EWeatherType	weatherType;

	bool			fogEnable;
	float			fogNear;
	float			fogFar;

	Vector3D		fogColor;

	EqString		skyboxPath;
	IMaterial*		skyboxMaterial;
};

//----------------------------------------------------------------------

struct lensFlareTable_t
{
	float		scale;
	int			atlasIdx;
	ColorRGB	color;
};

#define LENSFLARE_TABLE_SIZE 12

class CGameWorld : public IMaterialRenderParamCallbacks, public CBaseNetworkedObject
{
	friend class CGameObject;
	friend class CGameSession;
	friend class CNetGameSession;
	friend class CLevelRegion;

public:
	DECLARE_NETWORK_TABLE()
	DECLARE_CLASS( CGameWorld, CBaseNetworkedObject )

	CGameWorld();
	virtual ~CGameWorld() {}

	void							Init();
	void							Cleanup( bool unloadLevel = true );

	//-------------------------------------------------------------------------
	
	void							SetLevelName(const char* name);
	const char*						GetLevelName() const;

	bool							LoadLevel();
#ifdef EDITOR
	bool							SaveLevel();
#endif // EDITOR

	//-------------------------------------------------------------------------
	// objects

	void							AddObject(CGameObject* pObject, bool spawned = true);
	void							RemoveObject(CGameObject* pObject);
	bool							IsValidObject(CGameObject* pObject) const;

	CGameObject*					CreateGameObject( const char* typeName, kvkeybase_t* kvdata ) const;
	CGameObject*					CreateObject( const char* objectDefName ) const;
	CGameObject*					FindObjectByName( const char* objectName ) const;

	//-------------------------------------------------------------------------
	// world simulation

	void							UpdateWorld( float fDt );
	void							UpdateTrafficLightState( float fDt );

	// queries nearest regions and loads them in separate thread
	// you can wait for load if needed
	void							QueryNearestRegions(const Vector3D& pos, bool waitLoad = false);

	//-------------------------------------------------------------------------
	// world rendering

	void							BuildViewMatrices(int width, int height, int nRenderFlags);
	void							UpdateOccludingFrustum();

	void							Draw( int nRenderFlags );

	bool							AddLight(const wlight_t& light);
	int								GetLightIndexList(const BoundingBox& bbox, int* lights, int maxLights = MAX_LIGHTS_INST) const;
	void							GetLightList(const BoundingBox& bbox, wlight_t list[MAX_LIGHTS], int& numLights) const;

	void							SetView(const CViewParams& params);
	CViewParams*					GetView();

	//-------------------------------------------------------------------------
	// visual settings presets

	void							SetEnvironmentName(const char* name);
	const char*						GetEnvironmentName() const;
	void							InitEnvironment();

	void							FillEnviromentList(DkList<EqString>& list);
	CBillboardList*					FindBillboardList(const char* name) const;

public:
	CViewParams						m_view;

#ifdef EDITOR
	CEditorLevel					m_level;
#else
	CGameLevel						m_level;
#endif // EDITOR

	occludingFrustum_t				m_occludingFrustum;

	worldinfo_t						m_info;
	worldEnvConfig_t				m_envConfig;

	IVertexFormat*					m_vehicleVertexFormat;
	IVertexFormat*					m_objectInstVertexFormat;
	IVertexBuffer*					m_objectInstVertexBuffer;

	CNetworkVar(float,				m_globalTrafficLightTime);
	CNetworkVar(int,				m_globalTrafficLightDirection);

protected:
	// matsystem callback
	void							OnPreApplyMaterial( IMaterial* pMaterial );

	void							SimulateObjects( float fDt );
	void							DrawFakeReflections();
	void							DrawLensFlare( const Vector2D& screenSize, const Vector2D& screenPos, float intensity );

	void							UpdateRenderables( const occludingFrustum_t& frustum );
	void							UpdateLightTexture();

	void							DrawSkyBox(int nRenderFlags);

	void							GenerateEnvmapAndFogTextures();

	//--------------------------------------

	DkList<CBillboardList*>			m_billboardModels;

	lensFlareTable_t				m_lensTable[LENSFLARE_TABLE_SIZE];
	float							m_lensIntensityTiming;
	IOcclusionQuery*				m_sunGlowOccQuery;	// sun glow
	IMaterial*						m_depthTestMat;

	ITexture*						m_lightsTex;		// vertex texture fetch lights
	ITexture*						m_reflectionTex;	// reflection texture
	ITexture*						m_tempReflTex;		// temporary reflection buffer
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

	IMatVar*						m_skyColor;
	IEqModel*						m_skyModel;

	EqString						m_envName;

	wlight_t						m_lights[MAX_LIGHTS_QUEUE];
	int								m_numLights;

	IShaderProgram*					m_prevProgram;

	CNetworkVar(float,				m_fNextThunderTime);
	CNetworkVar(float,				m_fThunderTime);

	float							m_frameTime;
	float							m_curTime;

	sceneinfo_t						m_sceneinfo;

public:

	bool							m_levelLoaded;

	Matrix4x4						m_viewprojection;
	Matrix4x4						m_matrices[4];

	Volume							m_frustum;

#ifndef EDITOR
	CShadowRenderer					m_shadowRenderer;
#endif // EDITOR
};

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

	OOLUA_MFUNC_CONST(CreateObject)
	OOLUA_MFUNC(RemoveObject)
	OOLUA_MFUNC_CONST(IsValidObject)
	
	OOLUA_MFUNC_CONST(FindObjectByName)

	OOLUA_MFUNC(QueryNearestRegions)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // WORLD_H
