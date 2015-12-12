//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers world renderer
//////////////////////////////////////////////////////////////////////////////////

#ifndef WORLD_H
#define WORLD_H

#include "materialsystem/IMaterialSystem.h"
#include "input.h"
#include "level.h"
#include "GameObject.h"

#include "predictablerandom.h"

#include "luabinding/LuaBinding.h"
class CBillboardList;

enum ERenderFlags
{
	RFLAG_FLIP_VIEWPORT_X	= (1 << 25),
	RFLAG_INSTANCE			= (1 << 26),
};

// COLLISION_MASK_ALL defined in eqCollision_Object.h

// here we assembly the custom collision mask to use
enum EPhysCollisionContents
{
	OBJECTCONTENTS_SOLID_GROUND		= (1 << 0),	// ground objects
	OBJECTCONTENTS_SOLID_OBJECTS	= (1 << 1),	// other world objects

	OBJECTCONTENTS_WATER			= (1 << 2), // water surface

	OBJECTCONTENTS_DEBRIS			= (1 << 3),
	OBJECTCONTENTS_OBJECT			= (1 << 4),
	OBJECTCONTENTS_VEHICLE			= (1 << 5),

	OBJECTCONTENTS_CUSTOM_START		= (1 << 8),
};

#define COLLIDEMASK_VEHICLE (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_DEBRIS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE | OBJECTCONTENTS_WATER)
#define COLLIDEMASK_DEBRIS (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE)
#define COLLIDEMASK_OBJECT (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT)

// 2 regs, 52 lights, 128 regs (PS/VS), can be reduced to 48
#define MAX_LIGHTS			16
#define MAX_LIGHTS_INST		6
#define MAX_LIGHTS_QUEUE	128
#define MAX_LIGHTS_TEXTURE	32

struct worldinfo_t
{
	ColorRGBA	sunColor;
	ColorRGBA	ambientColor;

	Vector3D	sunDir;

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

#define LENSFLARE_TABLE_SIZE 8

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

	void							Init();
	void							Cleanup( bool unloadLevel = true );

	//-------------------------------------------------------------------------


	void							SetLevelName(const char* name);
	const char*						GetLevelName() const;
	bool							LoadLevel();

	void							SetEnvironmentName(const char* name);
	const char*						GetEnvironmentName() const;
	void							InitEnvironment();

#ifdef EDITOR
	void							Ed_FillEnviromentList(DkList<EqString>& list);
#endif // EDITOR

	CBillboardList*					FindBillboardList(const char* name) const;

	//-------------------------------------------------------------------------
	// objects

	void							AddObject(CGameObject* pObject, bool spawned = true);
	void							RemoveObject(CGameObject* pObject);
	bool							IsValidObject(CGameObject* pObject) const;
	void							UpdateWorld(float fDt);
	void							UpdateTrafficLightState(float fDt);

	//-------------------------------------------------------------------------

	CGameObject*					CreateGameObject( const char* typeName, kvkeybase_t* kvdata ) const;
	CGameObject*					CreateObject( const char* objectDefName ) const;
	CGameObject*					FindObjectByName( const char* objectName ) const;

	//-------------------------------------------------------------------------
	// render stuff

	void							BuildViewMatrices(int width, int height, int nRenderFlags);
	void							UpdateOccludingFrustum();

	void							Draw( int nRenderFlags );
	void							DrawFakeReflections();
	void							DrawLensFlare( const Vector2D& screenSize, const Vector2D& screenPos, float intensity );

	void							UpdateRenderables( const occludingFrustum_t& frustum );
	void							UpdateLightTexture();

	bool							AddLight(const wlight_t& light);
	void							ApplyLighting(const BoundingBox& bbox);
	int								GetLightIndexList(const BoundingBox& bbox, int* lights, int maxLights = MAX_LIGHTS_INST) const;
	void							GetLightList(const BoundingBox& bbox, wlight_t list[MAX_LIGHTS], int& numLights) const;

	void							SetCameraParams(CViewParams& params);
	CViewParams*					GetCameraParams();

	//--------------------------------------------------------------------------

	void							OnPreApplyMaterial( IMaterial* pMaterial );

public:
	CViewParams						m_CameraParams;
	CGameLevel						m_level;

	occludingFrustum_t				m_occludingFrustum;

	worldinfo_t						m_info;
	worldEnvConfig_t				m_envConfig;

	DkList<CBillboardList*>			m_billboardModels;

	IVertexFormat*					m_vehicleVertexFormat;
	IVertexFormat*					m_objectInstVertexFormat;
	IVertexBuffer*					m_objectInstVertexBuffer;

	CPredictableRandomGenerator		m_random;

	CNetworkVar(float,				m_globalTrafficLightTime);
	CNetworkVar(int,				m_globalTrafficLightDirection);

protected:

	lensFlareTable_t				m_lensTable[LENSFLARE_TABLE_SIZE];
	float							m_lensIntensityTiming;
	IOcclusionQuery*				m_sunGlowOccQuery;	// sun glow
	IMaterial*						m_depthTestMat;

	ITexture*						m_lightsTex;		// vertex texture fetch lights
	ITexture*						m_reflectionTex;	// reflection texture
	ITexture*						m_tempReflTex;		// temporary reflection buffer
	IMaterial*						m_blurYMaterial;	// y blur shader

	ISoundController*				m_rainSound;

	DkList<CGameObject*>			m_pGameObjects;
	DkList<CGameObject*>			m_nonSpawnedObjects;

	DkLinkedList<CGameObject*>		m_renderingObjects;
	
	EqString						m_levelname;

	IMaterial*						m_skyMaterial;

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
};

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
	OOLUA_MFUNC(SetEnvironmentName)
	OOLUA_MFUNC(SetLevelName)

	OOLUA_MFUNC_CONST(IsValidObject)

	OOLUA_MFUNC_CONST(CreateObject)
	OOLUA_MFUNC(GetCameraParams)

	OOLUA_MFUNC_CONST(FindObjectByName)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // WORLD_H