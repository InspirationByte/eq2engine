//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers world renderer
//////////////////////////////////////////////////////////////////////////////////

#include "world.h"
#include "IDebugOverlay.h"

#include "eqGlobalMutex.h"

#include "materialsystem/MeshBuilder.h"

#include "EqParticles.h"

#include "Rain.h"
#include "BillboardList.h"

#include "imaging/ImageLoader.h"

#include "Shiny.h"

#ifndef NO_GAME
#include "system.h"
#else
#include "eqParallelJobs.h"
#endif // NO_GAME

CPredictableRandomGenerator	g_replayRandom;

CPFXAtlasGroup* g_vehicleEffects = NULL;
CPFXAtlasGroup* g_vehicleShadows = NULL;

CPFXAtlasGroup* g_vehicleLights = NULL;

CPFXAtlasGroup* g_translParticles = NULL;
CPFXAtlasGroup* g_additPartcles = NULL;
CPFXAtlasGroup* g_treeAtlas = NULL;

//-----------------------------------------------------------------------------------

ConVar r_zfar("r_zfar", "350", NULL, CV_ARCHIVE);

ConVar fog_override("fog_override","0",NULL, CV_CHEAT);
ConVar fog_enable("fog_enable","0",NULL,CV_ARCHIVE);

ConVar fog_color_r("fog_color_r","0.5",NULL,CV_ARCHIVE);
ConVar fog_color_g("fog_color_g","0.5",NULL,CV_ARCHIVE);
ConVar fog_color_b("fog_color_b","0.5",NULL,CV_ARCHIVE);
ConVar fog_far("fog_far","400",NULL,CV_ARCHIVE);
ConVar fog_near("fog_near","20",NULL,CV_ARCHIVE);

ConVar r_no3D("r_no3D", "0", "Disable whole 3D rendering", CV_CHEAT);

ConVar r_drawWorld("r_drawWorld", "1", NULL, CV_CHEAT);
ConVar r_drawObjects("r_drawObjects", "1", NULL, CV_CHEAT);
ConVar r_drawFakeReflections("r_drawFakeReflections", "1", NULL, CV_ARCHIVE);

ConVar r_nightBrightness("r_nightBrightness", "1.0", NULL, CV_ARCHIVE);
ConVar r_drawLensFlare("r_drawLensFlare", "2", "Draw lens flare\n\t1 - query by physics\n\t2 - query by occlusion", CV_ARCHIVE);

ConVar r_lightDistance("r_lightDistance", "150", "Light distance scale cutoff", CV_ARCHIVE);

ConVar r_ambientScale("r_ambientScale", "1.0f", NULL, CV_ARCHIVE);

ConVar r_freezeFrustum("r_freezeFrustum", "0", NULL, CV_CHEAT);

ConVar r_glowsProjOffset("r_glowsProjOffset", "0.015", NULL, CV_CHEAT);
ConVar r_glowsProjOffsetB("r_glowsProjOffsetB", "0.0", NULL, CV_CHEAT);

ConVar r_drawsky("r_drawsky", "1", NULL, CV_CHEAT);

ConVar r_ortho("r_ortho", "0", NULL, CV_CHEAT);
ConVar r_ortho_size("r_ortho_size", "0.5", NULL, CV_ARCHIVE);

#define TRAFFICLIGHT_TIME 15.0f

DkList<EqString> g_envList;
void cmd_environment_variants(DkList<EqString>& list, const char* query)
{
	if(g_envList.numElem() == 0)
	{
		g_pGameWorld->FillEnviromentList(g_envList);
	}

	list.append(g_envList);
}

DECLARE_CMD_VARIANTS(w_environment, "Loads new environment parameters", cmd_environment_variants, CV_CHEAT)
{
	if(CMD_ARGC == 0)
	{
		Msg("Current: %s\n", g_pGameWorld->GetEnvironmentName());
	}
	else
	{
		g_pGameWorld->SetEnvironmentName( CMD_ARGV(0).c_str() );
		g_pGameWorld->InitEnvironment();
	}
}

DECLARE_CMD(w_respawn, "Respawn all level objects", CV_CHEAT)
{
	g_pGameWorld->m_level.RespawnAllObjects();
}

int SortGameObjectsByDistance(CGameObject* const& a, CGameObject* const& b)
{
	Vector3D cam_pos = g_pGameWorld->m_view.GetOrigin();
	Vector3D objPosA = a->GetOrigin();
	Vector3D objPosB = b->GetOrigin();

	Vector3D vecObjA = objPosA-cam_pos;
	Vector3D vecObjB = objPosB-cam_pos;

	float distA = length(vecObjA);
	float distB = length(vecObjB);

	if(fsimilar(distA, distB, 0.05f))
		return 0;

	return (int)sign(distB - distA);
}

BEGIN_NETWORK_TABLE_NO_BASE( CGameWorld )
	DEFINE_SENDPROP_FLOAT(m_fNextThunderTime),
	DEFINE_SENDPROP_FLOAT(m_fThunderTime),
	DEFINE_SENDPROP_FLOAT(m_globalTrafficLightTime),
	DEFINE_SENDPROP_INT(m_globalTrafficLightDirection),
END_NETWORK_TABLE()

CGameWorld::CGameWorld()
{
	m_rainSound = NULL;
	m_vehicleVertexFormat = NULL;

	m_skyColor = NULL;
	m_skyModel = NULL;

	m_sceneinfo.m_fZNear = 0.25f;
#ifdef EDITOR
	m_sceneinfo.m_fZFar = 8000.0f;
#else
	m_sceneinfo.m_fZFar = r_zfar.GetFloat();
#endif
	m_sceneinfo.m_bFogEnable = false;
	m_sceneinfo.m_cAmbientColor = color3_white;

	m_levelname = "unnamed";
	m_envName = "day_clear";

	m_prevProgram = nullptr;
	m_lightsTex = nullptr;
	m_reflectionTex = nullptr;
	m_tempReflTex = nullptr;
	m_blurYMaterial = nullptr;
	m_envMap = nullptr;
	m_fogEnvMap = nullptr;
	m_skyMaterial = nullptr;

	m_globalTrafficLightTime = 0.0f;
	m_globalTrafficLightDirection = 0;

	m_levelLoaded = false;

	m_objectInstVertexFormat = nullptr;
	m_objectInstVertexBuffer = nullptr;
	m_envMapsDirty = true;
}

void CGameWorld::SetEnvironmentName(const char* name)
{
	m_envName = name;
}

const char* CGameWorld::GetEnvironmentName() const
{
	return m_envName.c_str();
}

void CGameWorld::FillEnviromentList(DkList<EqString>& list)
{
	list.clear();

	KeyValues envKvs;

	bool status = envKvs.LoadFromFile(varargs("scripts/environments/%s_environment.txt", m_levelname.c_str()), SP_MOD);

	if( !status )
	{
		MsgWarning("No environment file loaded level '%s' (scripts/environments/<levelname>_environment.txt)!\n", m_levelname.c_str());

		status = envKvs.LoadFromFile("scripts/environments/default_environment.txt", SP_MOD);
	}

	if( !status )
		return;

	for(int i = 0; i < envKvs.GetRootSection()->keys.numElem(); i++)
	{
		kvkeybase_t* env = envKvs.GetRootSection()->keys[i];

		list.append( env->name );
	}
}

void SetupDefaultEnvConfig(worldEnvConfig_t& env)
{
	// set defaults
	env.ambientColor = ColorRGB(0.45f, 0.45f, 0.51f);
	env.sunColor = ColorRGB(0.79f, 0.71f, 0.61f);
	env.shadowColor = 0.0f;
	env.streetLightIntensity = 0.0f;

	env.sunAngles = Vector3D(-45.0f, -35.0f, 0.0f);
	env.skyboxPath = "sky/sky_day";
	env.lensIntensity = 1.0f;
	env.headLightIntensity = 1.0f;
	env.rainDensity = 0.0f;
	env.rainBrightness = 0.4f;
	env.brightnessModFactor = 0.0f;

	env.fogEnable = false;
	env.fogNear = DrvSynUnits::MaxCoordInUnits;
	env.fogFar = DrvSynUnits::MaxCoordInUnits;
	env.fogColor = ColorRGB(0.5f);

	env.weatherType = WEATHER_TYPE_CLEAR;
	env.thunder = false;

	env.lightsType = 0;
}

void ParseEnvConfig(worldEnvConfig_t& env, kvkeybase_t* kvs)
{
	if(kvs == nullptr)
		return;

	env.ambientColor = KV_GetVector3D(kvs->FindKeyBase("ambientColor"), 0, env.ambientColor);
	env.sunColor = KV_GetVector3D(kvs->FindKeyBase("sunColor"), 0, env.sunColor);
	env.shadowColor = KV_GetVector4D(kvs->FindKeyBase("shadowColor"), 0, env.shadowColor);

	env.sunAngles = KV_GetVector3D(kvs->FindKeyBase("sunAngles"), 0, env.sunAngles);
	env.brightnessModFactor = KV_GetValueFloat(kvs->FindKeyBase("brightModFactor"), 0, env.brightnessModFactor);
	env.streetLightIntensity = KV_GetValueFloat(kvs->FindKeyBase("streetLightIntensity"), 0, env.streetLightIntensity);
	env.lensIntensity = KV_GetValueFloat(kvs->FindKeyBase("sunLensItensity"), 0, env.lensIntensity);
	env.skyboxPath = KV_GetValueString(kvs->FindKeyBase("sky"), 0, env.skyboxPath.c_str());
	env.headLightIntensity = KV_GetValueFloat(kvs->FindKeyBase("headlightIntensity"), 0, env.headLightIntensity);

	kvkeybase_t* sunLensAngles = kvs->FindKeyBase("sunLensAngles");

	if(sunLensAngles)
		env.sunLensAngles = KV_GetVector3D(sunLensAngles);
	else
		env.sunLensAngles = env.sunAngles;

	env.rainBrightness = KV_GetValueFloat(kvs->FindKeyBase("rainBrightness"), 0, env.rainBrightness);
	env.rainDensity = KV_GetValueFloat(kvs->FindKeyBase("rainDensity"), 0, env.rainDensity);

	kvkeybase_t* lightsSec = kvs->FindKeyBase("lights");
	if(lightsSec)
	{
		env.lightsType = 0;

		for(int i = 0; i < lightsSec->ValueCount(); i++)
		{
			const char* valueStr = KV_GetValueString(lightsSec, i);

			if(!stricmp(valueStr, "cars"))
				env.lightsType |= WLIGHTS_CARS;
			else if(!stricmp(valueStr, "city"))
				env.lightsType |= WLIGHTS_CITY;
			else if(!stricmp(valueStr, "lamps"))
				env.lightsType |= WLIGHTS_LAMPS;
		}
	}

	const char* weatherName = KV_GetValueString(kvs->FindKeyBase("weather"), 0, s_weatherNamesStr[env.weatherType]);

	// resolve weather type
	for(int i = 0; i < WEATHER_COUNT; i++)
	{
		if(!stricmp(s_weatherNamesStr[i], weatherName))
			env.weatherType = (EWeatherType)i;
	}

	// thunder enabled automatically
	if(env.weatherType == WEATHER_TYPE_STORM)
		env.thunder = true;

	// thunder?
	env.thunder = KV_GetValueBool(kvs->FindKeyBase("thunder"), 0, env.thunder);

	// get the fog
	kvkeybase_t* fogSec = kvs->FindKeyBase("fog");

	env.fogEnable = KV_GetValueBool(fogSec, 0, env.fogEnable);

	if(fogSec)
	{
		env.fogColor = KV_GetVector3D(fogSec->FindKeyBase("fog_color"), 0, env.fogColor);
		env.fogNear = KV_GetValueFloat(fogSec->FindKeyBase("fog_near"), 0, env.fogNear);
		env.fogFar = KV_GetValueFloat(fogSec->FindKeyBase("fog_far"), 0, env.fogFar);
	}
}

void InterpolateEnv(worldEnvConfig_t& result, const worldEnvConfig_t& from, const worldEnvConfig_t& to, float factor)
{
	result.ambientColor = lerp(from.ambientColor, to.ambientColor, factor);
	result.sunColor = lerp(from.sunColor, to.sunColor, factor);
	result.brightnessModFactor = lerp(from.brightnessModFactor, to.brightnessModFactor, factor);

	result.sunAngles = lerp(from.sunAngles, to.sunAngles, factor);
	result.sunLensAngles = lerp(from.sunLensAngles,to.sunLensAngles,factor);
	result.lensIntensity = lerp(from.lensIntensity, to.lensIntensity, factor);
	result.rainBrightness = lerp(from.rainBrightness, to.rainBrightness, factor);
	result.rainDensity = lerp(from.rainDensity, to.rainDensity, factor);

	result.headLightIntensity = lerp(from.headLightIntensity, to.headLightIntensity, factor);
	//result.streetLightIntensity = lerp(from.streetLightIntensity, to.streetLightIntensity, factor);

	result.shadowColor = lerp(from.shadowColor, to.shadowColor, factor);

	result.lightsType = 0;

	bool carLights = lerp((from.lightsType & WLIGHTS_CARS) > 0 ? 1.0f : 0.0f, (to.lightsType & WLIGHTS_CARS) > 0 ? 1.0f : 0.0f, factor) >= 0.5f;
	bool lampLights = lerp((from.lightsType & WLIGHTS_LAMPS) > 0 ? 1.0f : 0.0f, (to.lightsType & WLIGHTS_LAMPS) > 0 ? 1.0f : 0.0f, factor) >= 0.5f;
	bool cityLights = lerp((from.lightsType & WLIGHTS_CITY) > 0 ? 1.0f : 0.0f, (to.lightsType & WLIGHTS_CITY) > 0 ? 1.0f : 0.0f, factor) >= 0.5f;

	if(carLights)
		result.lightsType |= WLIGHTS_CARS;

	if(cityLights)
		result.lightsType |= WLIGHTS_CITY;


	if(lampLights)
	{
		result.lightsType |= WLIGHTS_LAMPS;

		if((from.lightsType & WLIGHTS_LAMPS) == 0)
			result.streetLightIntensity = lerp(from.streetLightIntensity, to.streetLightIntensity, (factor-0.5f) * 2.0f);
		else
			result.streetLightIntensity = lerp(from.streetLightIntensity, to.streetLightIntensity, factor);
		
	}

	result.weatherType = (EWeatherType)(int)lerp((float)from.weatherType, (float)to.weatherType, factor);

	result.thunder = lerp(from.thunder ? 1.0f : 0.0f, to.thunder ? 1.0f : 0.0f, factor) >= 0.5f;

	result.fogEnable = from.fogEnable || to.fogEnable;

	if(result.fogEnable)
	{
		result.fogColor = lerp(from.fogColor, to.fogColor, factor);
		result.fogFar = lerp(from.fogFar, to.fogFar, factor);
		result.fogNear = lerp(from.fogNear, to.fogNear, factor);
	}
}

void CGameWorld::InitEnvironment()
{
	// TODO: IsLoaded
#ifndef EDITOR
	if(m_level.m_wide == 0 || m_level.m_tall == 0)
		return;
#endif // EDITOR

	for(int i = 0; i < m_envTransitions.numElem(); i++)
		g_pShaderAPI->FreeTexture(m_envTransitions[i].skyTexture);

	m_envTransitions.clear();

	g_pShaderAPI->FreeTexture(m_envConfig.skyTexture);
	m_envConfig.skyTexture = nullptr;

	m_envMapsDirty = true;
	materials->SetEnvironmentMapTexture(NULL);

	KeyValues envKvs;

	bool status = envKvs.LoadFromFile(varargs("scripts/environments/%s_environment.txt", m_levelname.c_str()), SP_MOD);

	if( !status )
	{
		MsgWarning("No environment file loaded level '%s' (scripts/environments/<levelname>_environment.txt)!\n", m_levelname.c_str());

		status = envKvs.LoadFromFile("scripts/environments/default_environment.txt", SP_MOD);
	}

	m_fNextThunderTime = 6.0f;
	m_fThunderTime = 0.2f;

	// find section
	kvkeybase_t* envSection = envKvs.GetRootSection()->FindKeyBase( m_envName.c_str(), KV_FLAG_SECTION );

	// first setup default env config
	SetupDefaultEnvConfig(m_envConfig);

	if(envSection)
	{
		kvkeybase_t* transitionData = envSection->FindKeyBase("transition", KV_FLAG_ARRAY);
		bool isTransition = transitionData != nullptr;

		if(isTransition)
		{
			m_envTransitions.setNum(transitionData->ValueCount());

			for(int i = 0; i < transitionData->ValueCount(); i++)
			{
				// make all params default
				SetupDefaultEnvConfig(m_envTransitions[i]);

				// find environment section
				kvkeybase_t* transitionEnv = envKvs.GetRootSection()->FindKeyBase( KV_GetValueString(transitionData, i), KV_FLAG_SECTION);

				const char* baseEnvName = KV_GetValueString(transitionEnv->FindKeyBase("base"), 0, nullptr);

				if(baseEnvName) // parse base env into this first
				{
					kvkeybase_t* baseEnvKvs = envKvs.GetRootSection()->FindKeyBase( baseEnvName, KV_FLAG_SECTION);
					ParseEnvConfig(m_envTransitions[i], baseEnvKvs);
				}

				// parse into new item
				ParseEnvConfig(m_envTransitions[i], transitionEnv);

				// load it's sky textre
				m_envTransitions[i].skyTexture = g_pShaderAPI->LoadTexture( m_envTransitions[i].skyboxPath.c_str(), TEXFILTER_TRILINEAR_ANISO);
				m_envTransitions[i].skyTexture->Ref_Grab();
			}

			m_envTransitionTotalTime = KV_GetValueFloat(envSection->FindKeyBase("transition_time"), 0, 100.0f );
			m_envTransitionTime = 0.0f;

			m_envConfig = m_envTransitions[0];
			m_envConfig.skyTexture = nullptr;
		}
		else
		{
			const char* baseEnvName = KV_GetValueString(envSection->FindKeyBase("base"), 0, nullptr);

			if(baseEnvName) // parse base env into this first
			{
				kvkeybase_t* baseEnvKvs = envKvs.GetRootSection()->FindKeyBase( baseEnvName, KV_FLAG_SECTION);
				ParseEnvConfig(m_envConfig, baseEnvKvs);
			}

			ParseEnvConfig(m_envConfig, envSection);

			// load sky texture
			m_envConfig.skyTexture = g_pShaderAPI->LoadTexture( m_envConfig.skyboxPath.c_str(), TEXFILTER_TRILINEAR_ANISO );
			m_envConfig.skyTexture->Ref_Grab();
		}
	}

	m_envWetness = (m_envConfig.rainDensity * 0.01f);
	m_envWetness = min(m_envWetness, 1.0f);

	// adjust sun dir
	AngleVectors(m_envConfig.sunAngles, &m_info.sunDir);

	// prepare
	m_skyTexture1->AssignTexture(m_envConfig.skyTexture);
	m_skyTexture2->AssignTexture(m_envConfig.skyTexture);
	m_skyInterp->SetFloat(0.0f);

	m_envMapRegenTime = 0.0f;

	if(m_reflectionTex)
	{
		g_pShaderAPI->ChangeRenderTarget(m_reflectionTex);
		g_pShaderAPI->Clear(true, false, false);
		g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	}
}

void CGameWorld::Init()
{
	g_replayRandom.SetSeed(0);

	m_frameTime = 0.0f;
	m_curTime = 0.0f;

	m_globalTrafficLightTime = 0.0f;
	m_globalTrafficLightDirection = 0;

	m_lensIntensityTiming = 0.0f;

	PrecacheScriptSound("rain.rain");
	PrecacheScriptSound("rain.thunder");

	EmitSound_t rainSound;
	rainSound.name = "rain.rain";
	rainSound.fPitch = 1.0;
	rainSound.fVolume = 1.0f;

	m_rainSound = ses->CreateSoundController(&rainSound);

	//----------------------------------------------------

	//
	// Create vertex format for vehicle EGF models (EGF + EGF)
	//
	VertexFormatDesc_t pFormat[] = {
		// model at stream 0
		{ 0, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position 0
		{ 0, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // texcoord 0

		{ 0, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF }, // Tangent UNUSED
		{ 0, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF }, // Binormal UNUSED
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Normal (TC1)

		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Bone indices (hw skinning), (TC2)
		{ 0, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF },  // Bone weights (hw skinning), (TC3)

		// model at stream 1
		{ 1, 3, VERTEXATTRIB_POSITION, ATTRIBUTEFORMAT_FLOAT },	  // position 0
		{ 1, 2, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // texcoord 4

		{ 1, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF }, // Tangent UNUSED
		{ 1, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF }, // Binormal UNUSED
		{ 1, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Normal (TC5)

		{ 1, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF }, // Bone indices (hw skinning), (TC4)
		{ 1, 4, VERTEXATTRIB_UNUSED, ATTRIBUTEFORMAT_HALF },  // Bone weights (hw skinning), (TC5)
	};

	if(!m_vehicleVertexFormat)
		m_vehicleVertexFormat = g_pShaderAPI->CreateVertexFormat(pFormat, elementsOf(pFormat));

	//-------------------------

	g_pPFXRenderer->Init();

#ifndef EDITOR
	m_shadowRenderer.Init();
#endif // EDITOR

	if(!m_skyModel)
	{
		int cacheIdx = g_pModelCache->PrecacheModel("models/engine/sky.egf");
		m_skyModel = g_pModelCache->GetModel(cacheIdx);
	}

	if(!g_vehicleLights)
	{
		g_vehicleLights = new CPFXAtlasGroup();
		g_vehicleLights->Init("scripts/effects_lights.atlas", false);

		// triangle list mode
		g_vehicleLights->SetTriangleListMode(true);
		g_vehicleLights->SetCullInverted(true);

		g_pPFXRenderer->AddRenderGroup( g_vehicleLights );
	}

	if(!g_vehicleShadows)
	{
		g_vehicleShadows = new CPFXAtlasGroup();
		g_vehicleShadows->Init("scripts/effects_vehicles_shadows.atlas", false);

		// triangle list mode
		g_vehicleShadows->SetTriangleListMode(true);
		g_vehicleLights->SetCullInverted(true);

		g_pPFXRenderer->AddRenderGroup( g_vehicleShadows );
	}

	if(!g_vehicleEffects)
	{
		g_vehicleEffects = new CPFXAtlasGroup();
		g_vehicleEffects->Init("scripts/effects_vehicles.atlas", false, 8192);
		g_pPFXRenderer->AddRenderGroup( g_vehicleEffects );
	}

	if(!g_treeAtlas)
	{
		g_treeAtlas = new CPFXAtlasGroup();
		g_treeAtlas->Init("scripts/billboard_trees.atlas", false);
		g_pPFXRenderer->AddRenderGroup( g_treeAtlas );
	}

	if(!g_translParticles)
	{
		g_translParticles = new CPFXAtlasGroup();
		g_translParticles->Init("scripts/effects_translucent.atlas", false, 10000);
		g_pPFXRenderer->AddRenderGroup( g_translParticles );
	}

	if(!g_additPartcles)
	{
		g_additPartcles = new CPFXAtlasGroup();
		g_additPartcles->Init("scripts/effects_additive.atlas", false, 4096);
		g_pPFXRenderer->AddRenderGroup( g_additPartcles );

		int lightId = g_additPartcles->FindEntryIndex("glow1");
		int lens1Id = g_additPartcles->FindEntryIndex("lens1");
		int lens2Id = g_additPartcles->FindEntryIndex("lens2");
		int lens3Id = g_additPartcles->FindEntryIndex("lens3");
		int lens4Id = g_additPartcles->FindEntryIndex("lens4");

		// two lens textures are reserved
		m_lensTable[0] = {1250.0f, lightId, ColorRGB(0.68f)};
		m_lensTable[1] = {150.0f, lightId, ColorRGB(2.0f)};

		m_lensTable[2] = {120.0f, lens4Id, ColorRGB(0.2f,0.3f,0.8f)};
		m_lensTable[3] = {100.0f, lens3Id, ColorRGB(0.35f)};
		m_lensTable[4] = {70.0f, lens2Id, ColorRGB(0.22f,0.9f,0.2f)};
		m_lensTable[5] = {60.0f, lens1Id, ColorRGB(1,1,1)};

		m_lensTable[6] = {50.0f, lens4Id, ColorRGB(0.65f)};
		m_lensTable[7] = {50.0f, lens4Id, ColorRGB(0.65f)};

		m_lensTable[8] = {60.0f, lens1Id, ColorRGB(1,1,1)};
		m_lensTable[9] = {70.0f, lens2Id, ColorRGB(0.22f,0.9f,0.2f)};
		m_lensTable[10] = {120.0f, lens3Id, ColorRGB(0.35f)};
		m_lensTable[11] = {120.0f, lens4Id, ColorRGB(0.2f,0.3f,0.8f)};

	}

	g_pPFXRenderer->PreloadMaterials();

	g_pRainEmitter->Init();

	// instancing
	if(g_pShaderAPI->GetCaps().isInstancingSupported)
	{
		if(!m_objectInstVertexFormat)
			m_objectInstVertexFormat = g_pShaderAPI->CreateVertexFormat(s_gameObjectInstanceFmtDesc, elementsOf(s_gameObjectInstanceFmtDesc));

		if(!m_objectInstVertexBuffer)
		{
			m_objectInstVertexBuffer = g_pShaderAPI->CreateVertexBuffer(BUFFER_DYNAMIC, MAX_EGF_INSTANCES, sizeof(gameObjectInstance_t));
			m_objectInstVertexBuffer->SetFlags( VERTBUFFER_FLAG_INSTANCEDATA );
		}
	}

	// init render stuff
	if(!m_lightsTex)
	{
		m_lightsTex = g_pShaderAPI->CreateProceduralTexture("_vlights",
														FORMAT_RGBA16F, MAX_LIGHTS_TEXTURE, 2, 1,1,
														TEXFILTER_NEAREST, TEXADDRESS_CLAMP, TEXFLAG_NOQUALITYLOD);
		if(m_lightsTex)
			m_lightsTex->Ref_Grab();
	}

	if(!m_sunGlowOccQuery)
		m_sunGlowOccQuery = g_pShaderAPI->CreateOcclusionQuery();

	if(!m_pointQueryMat)
	{
		kvkeybase_t pointQueryParams;
		pointQueryParams.SetName("BaseUnlit");
		pointQueryParams.SetKey("basetexture", "_matsys_white");
		pointQueryParams.SetKey("ztest", true);
		pointQueryParams.SetKey("zwrite", true);
		pointQueryParams.SetKey("translucent", true);
		pointQueryParams.SetKey("color", "[1 1 1 0]");
		pointQueryParams.SetKey("noMSAA", true);

		m_pointQueryMat = materials->CreateMaterial("_occquery", &pointQueryParams);
		m_pointQueryMat->Ref_Grab();

		materials->PutMaterialToLoadingQueue(m_pointQueryMat);
	}

	if(!m_reflectionTex)
	{
		m_reflectionTex = g_pShaderAPI->CreateNamedRenderTarget("_reflection", 512, 256, FORMAT_RGBA8, TEXFILTER_LINEAR, TEXADDRESS_CLAMP);
		m_reflectionTex->Ref_Grab();
	}

	if(!m_tempReflTex)
	{
		m_tempReflTex = g_pShaderAPI->CreateNamedRenderTarget("_tempTexture", 512, 256, FORMAT_RGBA8, TEXFILTER_NEAREST, TEXADDRESS_CLAMP);
		m_tempReflTex->Ref_Grab();
	}

	if(!m_reflDepth)
	{
		m_reflDepth = g_pShaderAPI->CreateNamedRenderTarget("_reflDepth", 512, 256, FORMAT_D16, TEXFILTER_NEAREST);
		m_reflDepth->Ref_Grab();
	}
	
	
	if(!m_blurYMaterial)
	{
		kvkeybase_t blurParams;
		blurParams.SetName("BlurFilter");
		blurParams.SetKey("BaseTexture", "_tempTexture");
		blurParams.SetKey("BlurY", true); // blur by Y
		blurParams.SetKey("YLow", true); // upper kernel
		blurParams.SetKey("YHigh", true); // lower kernel

		m_blurYMaterial = materials->CreateMaterial("_reflBlur", &blurParams);
		m_blurYMaterial->Ref_Grab();

		materials->PutMaterialToLoadingQueue(m_blurYMaterial);
	}

	if(!m_skyMaterial)
	{
		kvkeybase_t skyParams;
		skyParams.SetName("DrvSynSky");
		skyParams.SetKey("ztest", true);
		skyParams.SetKey("zwrite", false);

		m_skyMaterial = materials->CreateMaterial("_sky", &skyParams);
		m_skyMaterial->Ref_Grab();

		m_skyMaterial->LoadShaderAndTextures();
	}

	// get material vars we gonna control
	m_skyColor = m_skyMaterial->GetMaterialVar("color", "[1 1 1 1]");
	m_skyTexture1 = m_skyMaterial->GetMaterialVar("Base1", "$base1");
	m_skyTexture2 = m_skyMaterial->GetMaterialVar("Base2", "$base1");
	m_skyInterp = m_skyMaterial->GetMaterialVar("Interp", "0.0");

	ASSERT(m_skyColor);
	ASSERT(m_skyTexture1);
	ASSERT(m_skyTexture2);
	ASSERT(m_skyInterp);

	m_envTransitionTime = 0.0f;
	m_envMapRegenTime = 0.0f;
}

void CGameWorld::AddObject(CGameObject* pObject, bool spawned)
{
	if(!spawned)
	{
		m_nonSpawnedObjects.append(pObject);
	}
	else
	{
		m_gameObjects.append( pObject );
		pObject->Ref_Grab();
	}
}

void CGameWorld::RemoveObject(CGameObject* pObject)
{
	pObject->m_state = GO_STATE_REMOVE;
}

bool CGameWorld::IsValidObject(CGameObject* pObject) const
{
	if(!pObject)
		return false;

	for (int i = 0; i < m_gameObjects.numElem(); i++)
	{
		if (m_gameObjects[i] == pObject)
			return true;
	}

	return false;
}

void CGameWorld::Cleanup( bool unloadLevel )
{
#ifndef NO_LUA
	{
		OOLUA::Script& state = GetLuaState();
		EqLua::LuaStackGuard g(state);

		OOLUA::Table tab = OOLUA::new_table(state);
		EqLua::LuaCallUserdataCallback(g_pGameWorld, "OnLevelUnload", tab);
	}
#endif // EDITOR

	// wait until it load all before unload
	m_level.WaitForThread();

	m_renderingObjects.clear();

	for(int i = 0; i < m_gameObjects.numElem(); i++)
	{
		CGameObject* obj = m_gameObjects[i];

		if(m_gameObjects.fastRemove(obj))
		{
			obj->OnRemove();

			if (obj->m_state != GO_STATE_REMOVED)
				ASSERTMSG(false, "PROGRAMMER ERROR - your object doesn't fully called OnRemove (not GO_STATE_REMOVED)");

			obj->Ref_Drop();
			i--;
		}
	}
	m_gameObjects.clear();

	// cvar stuff
	g_envList.clear();

	m_occludingFrustum.Clear();

	g_pRainEmitter->Clear();

	if(m_rainSound != nullptr)
		ses->RemoveSoundController(m_rainSound);

	m_rainSound = nullptr;

	ses->StopAllSounds();

	// clear transition environments
	for(int i = 0; i < m_envTransitions.numElem(); i++)
		g_pShaderAPI->FreeTexture(m_envTransitions[i].skyTexture);

	m_envTransitions.clear();

	g_pShaderAPI->FreeTexture(m_envConfig.skyTexture);
	m_envConfig.skyTexture = nullptr;
	
	// non-spawned objects are deleted in UpdateRegions()

	if(unloadLevel)
	{
		for(int i = 0; i < m_nonSpawnedObjects.numElem(); i++)
		{
			m_nonSpawnedObjects[i]->OnRemove();
			delete m_nonSpawnedObjects[i];
		}

		m_nonSpawnedObjects.clear();
	}
	else
	{
		// regions must be unloaded
		m_level.UnloadRegions();
	}

	if(unloadLevel)
	{
		Msg("Unloading level...\n");

		m_levelLoaded = false;
		m_level.Cleanup();

		for(int i = 0; i < m_billboardModels.numElem(); i++)
		{
			m_billboardModels[i]->DestroyBlb();
			delete m_billboardModels[i];
		}
		m_billboardModels.clear();

		g_pPFXRenderer->RemoveRenderGroup(g_vehicleEffects);
		g_pPFXRenderer->RemoveRenderGroup(g_additPartcles);
		g_pPFXRenderer->RemoveRenderGroup(g_translParticles);
		g_pPFXRenderer->RemoveRenderGroup(g_treeAtlas);

		g_pPFXRenderer->RemoveRenderGroup(g_vehicleLights);
		g_pPFXRenderer->RemoveRenderGroup(g_vehicleShadows);

		if(g_vehicleEffects)
			delete g_vehicleEffects;
		g_vehicleEffects = NULL;

		if(g_translParticles)
			delete g_translParticles;
		g_translParticles = NULL;

		if(g_additPartcles)
			delete g_additPartcles;
		g_additPartcles = NULL;

		if(g_treeAtlas)
			delete g_treeAtlas;
		g_treeAtlas = NULL;

		if(g_vehicleLights)
			delete g_vehicleLights;
		g_vehicleLights = NULL;

		if(g_vehicleShadows)
			delete g_vehicleShadows;
		g_vehicleShadows = NULL;

#ifndef EDITOR
		m_shadowRenderer.Shutdown();
#endif // EDITOR

		g_pPFXRenderer->Shutdown();

		g_pShaderAPI->DestroyVertexFormat(m_vehicleVertexFormat);
		m_vehicleVertexFormat = nullptr;

		g_pShaderAPI->FreeTexture(m_lightsTex);
		m_lightsTex = nullptr;

		g_pShaderAPI->FreeTexture(m_reflectionTex);
		m_reflectionTex = nullptr;

		g_pShaderAPI->FreeTexture(m_tempReflTex);
		m_tempReflTex = nullptr;

		g_pShaderAPI->FreeTexture(m_reflDepth);
		m_reflDepth = nullptr;

		materials->FreeMaterial(m_blurYMaterial);
		m_blurYMaterial = nullptr;

		materials->FreeMaterial(m_skyMaterial);
		m_skyMaterial = nullptr;

		m_skyColor = nullptr;
		m_skyInterp = nullptr;
		m_skyTexture1 = nullptr;
		m_skyTexture2 = nullptr;

		materials->SetEnvironmentMapTexture(nullptr);
		g_pShaderAPI->FreeTexture(m_envMap);
		m_envMap = nullptr;

		g_pShaderAPI->FreeTexture(m_fogEnvMap);
		m_fogEnvMap = nullptr;

		g_pShaderAPI->DestroyVertexFormat(m_objectInstVertexFormat);
		m_objectInstVertexFormat = nullptr;

		g_pShaderAPI->DestroyVertexBuffer(m_objectInstVertexBuffer);
		m_objectInstVertexBuffer = nullptr;

		g_pShaderAPI->DestroyOcclusionQuery(m_sunGlowOccQuery);
		m_sunGlowOccQuery = nullptr;

		materials->FreeMaterial(m_pointQueryMat);
		m_pointQueryMat = nullptr;

		m_skyModel = nullptr;
	}
}

void GWJob_UpdateWorldAndEffects(void* data, int i)
{
	float fDt = *(float*)data;
	g_pRainEmitter->Update_Draw(fDt, g_pGameWorld->m_envConfig.rainDensity, 200.0f);
}

void CGameWorld::UpdateTrafficLightState(float fDt)
{
	if( fDt <= 0.0f )
		return;

	m_globalTrafficLightTime -= fDt;

	if(m_globalTrafficLightTime <= 0.0f)
	{
		m_globalTrafficLightTime = TRAFFICLIGHT_TIME;
		m_globalTrafficLightDirection = !m_globalTrafficLightDirection;
	}
}

void RegionCallbackFunc(CLevelRegion* reg, int regIdx)
{
#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	OOLUA::Table tab = OOLUA::new_table(state);
	tab.set("regionId", regIdx);

	if (reg->m_isLoaded)
		EqLua::LuaCallUserdataCallback(g_pGameWorld, "OnRegionLoaded", tab);
	else
		EqLua::LuaCallUserdataCallback(g_pGameWorld, "OnRegionUnloaded", tab);
#endif // NO_LUA
}

void CGameWorld::SimulateObjects( float fDt )
{
	for(int i = 0; i < m_gameObjects.numElem(); i++)
	{
		CGameObject* obj = m_gameObjects[i];

		if(obj->m_state == GO_STATE_IDLE)
			obj->Simulate( fDt );
	}
}

void CGameWorld::UpdateWorld(float fDt)
{
	PROFILE_FUNC();

	static float jobFrametime = fDt;
	jobFrametime = fDt;

	g_parallelJobs->AddJob(GWJob_UpdateWorldAndEffects, &jobFrametime);
	g_parallelJobs->Submit();

	if( fDt > 0 )
	{
		if(m_envTransitions.numElem() > 0)
		{
			// total percentage between first and last transition environments
			float timePercentage = m_envTransitionTime / m_envTransitionTotalTime;
			
			if(m_envTransitionTime >= m_envTransitionTotalTime)
			{
				m_envTransitionTime = m_envTransitionTotalTime;
				timePercentage = 1.0f;
			}
			else
			{
				m_envTransitionTime += fDt;

				// every 15 seconds we should regenerate cubemaps
				m_envMapRegenTime -= fDt;
				if(m_envMapRegenTime <= 0.0f)
				{
					m_envMapsDirty = true;
					m_envMapRegenTime = 15.0f;
				}
			}

			float percentageScaled = timePercentage*(m_envTransitions.numElem()-1);

			int weatherIdx = ceil(percentageScaled);

			float transitionPercent = percentageScaled - (weatherIdx-1);

			if(m_envTransitionTime <= 0.1f)
			{
				transitionPercent = 0.0f;
				weatherIdx = 1;
			}

			if(weatherIdx == 0)
				weatherIdx = 1;

			if(weatherIdx >= m_envTransitions.numElem())
				weatherIdx = m_envTransitions.numElem()-1;

			// interpolate
			InterpolateEnv(m_envConfig, m_envTransitions[weatherIdx-1], m_envTransitions[weatherIdx], transitionPercent);

			float fromWetness = m_envTransitions[weatherIdx-1].rainDensity*0.01f;
			float toWetness = m_envTransitions[weatherIdx].rainDensity*0.01f;

			// prepare
			m_skyTexture1->AssignTexture(m_envTransitions[weatherIdx-1].skyTexture);
			m_skyTexture2->AssignTexture(m_envTransitions[weatherIdx].skyTexture);
			m_skyInterp->SetFloat(transitionPercent);

			m_envWetness = lerp(fromWetness, toWetness, transitionPercent);
			m_envWetness = min(m_envWetness, 1.0f);

			// adjust sun dir
			AngleVectors(m_envConfig.sunAngles, &m_info.sunDir);

			debugoverlay->Text(ColorRGBA(1,1,0,1), "------ weather transition -------");
			debugoverlay->Text(ColorRGBA(1,1,0,1), "     time: %g / %g sec", m_envTransitionTime, m_envTransitionTotalTime);
			debugoverlay->Text(ColorRGBA(1,1,0,1), "     env: %d / %d", weatherIdx, m_envTransitions.numElem()-1);
			debugoverlay->Text(ColorRGBA(1,1,0,1), "     trans: %g", transitionPercent);
		}

		if(	m_envConfig.thunder )
		{
			m_fNextThunderTime -= fDt;

			if(m_fNextThunderTime <= 0.0f)
			{
				// do sound and restore the time
				m_fNextThunderTime = 10.0f + RandomFloat(-2.0f, 10.0f);
				m_fThunderTime = RandomFloat(0.12f, 0.3f);

				EmitSound_t thunderSnd;
				thunderSnd.name = "rain.thunder";
				thunderSnd.origin = m_view.GetOrigin();
				thunderSnd.fRadiusMultiplier = 10.0f;

				ses->EmitSound(&thunderSnd);

				m_info.ambientColor = ColorRGBA(m_envConfig.ambientColor, m_envConfig.brightnessModFactor);
			}
		}

		float rainVolume = powf(m_envConfig.rainDensity/100.0f, 0.5f);

		if(m_rainSound && rainVolume > 0.01f)
		{
			// calc rain sound volume
			m_rainSound->SetVolume(rainVolume);

			if(m_rainSound->IsStopped())
				m_rainSound->Play();

			CollisionData_t coll;
			g_pPhysics->TestLine(m_view.GetOrigin()+Vector3D(0, 100, 0), m_view.GetOrigin()-Vector3D(0, 40, 0), coll, (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS));

			m_rainSound->SetOrigin( coll.position );
		}
		else if(!m_rainSound->IsStopped())
			m_rainSound->Stop();
	}

	// reset light count, 'Simulate' will add new lights
	m_numLights = 0;

	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		m_lights[i].position = vec4_zero;
		m_lights[i].color = vec4_zero;
	}

	PROFILE_BEGIN(UpdateWorldObjectStates);

	if( fDt > 0 )
	{
		m_level.UpdateRegions( RegionCallbackFunc );

		m_level.m_mutex.Lock();
		for(int i = 0; i < m_nonSpawnedObjects.numElem(); i++)
		{
			m_nonSpawnedObjects[i]->Spawn();
			m_nonSpawnedObjects[i]->Ref_Grab();

			m_gameObjects.append(m_nonSpawnedObjects[i]);
			m_nonSpawnedObjects.fastRemoveIndex(i);
			i--;
		}
		m_level.m_mutex.Unlock();
	}

	PROFILE_END();

	// simulate objects of world
	PROFILE_CODE( SimulateObjects(fDt) );

	// remove marked objects after simulation
    if( fDt > 0 )
	{
		// remove objects
		for(int i = 0; i < m_gameObjects.numElem(); i++)
		{
			CGameObject* obj = m_gameObjects[i];

			if(!IsRemoveState(obj->m_state))
                continue;

            m_level.m_mutex.Lock();

            if( m_gameObjects.fastRemove(obj) )
            {
                obj->OnRemove();

                if (obj->m_state != GO_STATE_REMOVED)
                    ASSERTMSG(false, "PROGRAMMER ERROR - your object doesn't fully called OnRemove (not GO_STATE_REMOVED)");

                obj->Ref_Drop();
                i--;
            }

            m_level.m_mutex.Unlock();
		}
	}

	m_frameTime = fDt;
	m_curTime += m_frameTime;
}

void CGameWorld::UpdateRenderables( const occludingFrustum_t& frustum )
{
	m_renderingObjects.clear();

	// simulate objects of world
	for(int i = 0; i < g_pGameWorld->m_gameObjects.numElem(); i++)
	{
		CGameObject* obj = g_pGameWorld->m_gameObjects[i];

		if(obj->m_state != GO_STATE_IDLE)
			continue;

		// sorted insert into render list
		if( obj->CheckVisibility( frustum ) )
			m_renderingObjects.insertSorted( obj, SortGameObjectsByDistance );
	}
}

bool CGameWorld::AddLight(const wlight_t& light)
{
	if(m_numLights >= MAX_LIGHTS_QUEUE-1)
	{
		// TOO many lights!
		return true;
	}

	if( !m_occludingFrustum.IsSphereVisible(light.position.xyz(), light.position.w) )
		return false;

	float fDistance = length(light.position.xyz() - m_view.GetOrigin());

	if(fDistance > r_lightDistance.GetFloat())
		return true;

	const float fadeDistVal = 1.0f / r_lightDistance.GetFloat();

	float fLightBrightness = 1.0f - pow(clamp(fDistance*fadeDistVal, 0.0f, 1.0f), 4.0f);

	m_lights[m_numLights] = light;

	m_lights[m_numLights].color *= m_lights[m_numLights].color.w*fLightBrightness;
	m_numLights++;

	return true;
}

void CGameWorld::BuildViewMatrices(int width, int height, int nRenderFlags)
{
	m_view.GetMatrices(m_matrices[MATRIXMODE_PROJECTION], m_matrices[MATRIXMODE_VIEW], width, height, m_sceneinfo.m_fZNear, m_sceneinfo.m_fZFar);

	m_matrices[MATRIXMODE_WORLD] = identity4();

	// store the viewprojection matrix for some purposes
	m_viewprojection = m_matrices[MATRIXMODE_PROJECTION] * m_matrices[MATRIXMODE_VIEW];

	materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
	materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);

	// calculate custom projection matrix for additive particles
	Matrix4x4 customGlowsProj = perspectiveMatrixY(DEG2RAD(m_view.GetFOV()), width, height, m_sceneinfo.m_fZNear+r_glowsProjOffset.GetFloat(), m_sceneinfo.m_fZFar+r_glowsProjOffsetB.GetFloat());
	g_additPartcles->SetCustomProjectionMatrix( customGlowsProj );

	if(!r_freezeFrustum.GetBool())
		m_frustum.LoadAsFrustum(m_viewprojection);

	FogInfo_t fog;
	materials->GetFogInfo(fog);
	fog.viewPos = m_view.GetOrigin();
	materials->SetFogInfo(fog);
}

void CGameWorld::DrawSkyBox(int renderFlags)
{
	if(!m_skyModel)
		return;

	FogInfo_t noFog;
	noFog.enableFog = false;

	materials->SetFogInfo(noFog);

	materials->SetCullMode(CULL_BACK);
	materials->BindMaterial( m_skyMaterial );

	studiohdr_t* pHdr = m_skyModel->GetHWData()->studio;
	int nLOD = 0;

	// draw skydome body groups
	for(int i = 0; i < pHdr->numBodyGroups; i++)
	{
		//if(!(m_bodyGroupFlags & (1 << i)))
		//	continue;

		int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
		int nModDescId = pHdr->pLodModel(nLodModelIdx)->modelsIndexes[ nLOD ];

		if(nModDescId == -1)
			continue;

		studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

		// render model groups that in this body group
		for(int j = 0; j < modDesc->numGroups; j++)
			m_skyModel->DrawGroup( nModDescId, j );
	}
}

void CopyPixels(int* src, int* dest, int w1, int h1, int w2, int h2)
{
    int x_ratio = (int)((w1<<16)/w2) +1;
    int y_ratio = (int)((h1<<16)/h2) +1;

    int x2, y2 ;
    for (int i=0;i<h2;i++)
	{
        for (int j=0;j<w2;j++)
		{
            x2 = ((j*x_ratio)>>16);
            y2 = ((i*y_ratio)>>16);

            dest[(i*w2)+j] = src[(y2*w1)+x2] ;
        }
    }
}

ConVar r_skyToCubemap("r_skyToCubemap", "1");

void CGameWorld::GenerateEnvmapAndFogTextures()
{
	if(!m_envMapsDirty)
		return;

	if(!r_skyToCubemap.GetBool())
		return;

	materials->Wait();

	m_envMapsDirty = false;

	// TODO: more efficient way

	ITexture* tempRenderTarget = g_pShaderAPI->CreateNamedRenderTarget("_tempSkyboxRender", 256, 256, FORMAT_RGBA8,
										TEXFILTER_NEAREST, TEXADDRESS_CLAMP, COMP_NEVER, TEXFLAG_CUBEMAP);

	tempRenderTarget->Ref_Grab();

	materials->SetMaterialRenderParamCallback(this);

	// render the skybox into cubemap
	for(int i = 0; i < 6; i++)
	{
		g_pShaderAPI->ChangeRenderTarget(tempRenderTarget, i);
		g_pShaderAPI->Clear(true,true,false);

		// Draw sky
		materials->SetMatrix(MATRIXMODE_PROJECTION, cubeProjectionMatrixD3D(0.1f, 1000.0f));
		materials->SetMatrix(MATRIXMODE_VIEW, cubeViewMatrix(i));
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		m_skyColor->SetVector4( 1.0f );
		DrawSkyBox(0);
	}

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
	//g_pShaderAPI->Finish();

	texlockdata_t tempLock;

	CImage envMap, fogEnvMap;
	envMap.Create(FORMAT_RGBA8, 256, 256, 0, 1);
	fogEnvMap.Create(FORMAT_RGBA8, 64, 64, 0, 1);

	ubyte* envMapData = envMap.GetPixels(0);
	ubyte* fogEnvMapData = fogEnvMap.GetPixels(0);

	int envMapFace = envMap.GetMipMappedSize(0,1) / 6;
	int fogEnvMapFace = fogEnvMap.GetMipMappedSize(0,1) / 6;

	// do resize by software
	for(int i = 0; i < 6; i++)
	{
		tempRenderTarget->Lock(&tempLock, NULL, false, true, 0, i );
		if(tempLock.pData)
		{
			CopyPixels((int*)tempLock.pData, (int*)envMapData, tempRenderTarget->GetWidth(), tempRenderTarget->GetHeight(), envMap.GetWidth(), envMap.GetHeight());
			envMapData += envMapFace;

			CopyPixels((int*)tempLock.pData, (int*)fogEnvMapData, tempRenderTarget->GetWidth(), tempRenderTarget->GetHeight(), fogEnvMap.GetWidth(), fogEnvMap.GetHeight());
			fogEnvMapData += fogEnvMapFace;

			tempRenderTarget->Unlock();
		}
	}

	//g_pShaderAPI->Finish();

	envMap.SwapChannels(0, 2);
	fogEnvMap.SwapChannels(0, 2);

	envMap.CreateMipMaps(7);

	DkList<CImage*> envMapImg;
	envMapImg.append(&envMap);

	DkList<CImage*> fogEnvMapImg;
	fogEnvMapImg.append(&fogEnvMap);

	envMap.SetName("_skyEnvMap");
	fogEnvMap.SetName("_fogEnvMap");

	SamplerStateParam_t sampler = g_pShaderAPI->MakeSamplerState(TEXFILTER_LINEAR, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP, TEXADDRESS_CLAMP);

	// set cubemap to none before we freeing the texture
	materials->SetEnvironmentMapTexture(nullptr);
	g_pShaderAPI->FreeTexture(m_envMap);

	m_envMap = g_pShaderAPI->CreateTexture(envMapImg, sampler);
	m_envMap->Ref_Grab();

	g_pShaderAPI->FreeTexture(m_fogEnvMap);
	m_fogEnvMap = g_pShaderAPI->CreateTexture(fogEnvMapImg, sampler);
	m_fogEnvMap->Ref_Grab();

	g_pShaderAPI->FreeTexture( tempRenderTarget );
}

void CGameWorld::OnPreApplyMaterial( IMaterial* pMaterial )
{
	HOOK_TO_CVAR(r_lightscale)

	if( pMaterial->GetState() != MATERIAL_LOAD_OK )
		return;
	
	ColorRGBA sunColor = m_info.sunColor*r_lightscale->GetFloat();

	// detect render to texture mode
	if(!m_reflectionStage)
	{
		// update material proxy
		pMaterial->UpdateProxy( m_frameTime );

		g_pShaderAPI->SetTexture(m_reflectionTex, "Reflection", 4);
		g_pShaderAPI->SetTexture(m_fogEnvMap, "FogEnvMap", 6);
	}
	else
	{
		g_pShaderAPI->SetTexture(nullptr, "Reflection", 4);
		g_pShaderAPI->SetTexture(nullptr, "FogEnvMap", 6);

		sunColor *= 0.25f;
	} 

	g_pShaderAPI->SetTexture(m_lightsTex, "vlights", VERTEX_TEXTURE_INDEX(0));
	g_pShaderAPI->SetTexture(m_lightsTex, "vlights", 7);

	g_pShaderAPI->SetShaderConstantVector4D("SunColor", sunColor);
	g_pShaderAPI->SetShaderConstantVector3D("SunDir", m_info.sunDir);

	g_pShaderAPI->SetShaderConstantFloat("GameTime", m_curTime);

	Vector2D envParams;
	envParams.x = m_envWetness;		// wetness
	envParams.y = (m_envConfig.lightsType & WLIGHTS_CITY) > 0 ? 1.0f : 0.0f; // texture lights

	g_pShaderAPI->SetShaderConstantVector2D("ENVPARAMS", envParams);
}

int CGameWorld::GetLightIndexList(const BoundingBox& bbox, int* lights, int maxLights) const
{
	for(int i = 0; i < maxLights; i++)
		lights[i] = -1;

	int numLights = 0;

	for(int i = 0; i < m_numLights; i++)
	{
		if(numLights >= maxLights-1)
			break;

		if(!bbox.IntersectsSphere(m_lights[i].position.xyz(), m_lights[i].position.w))
			continue;

		lights[numLights++] = i;
	}

	return numLights;
}

void CGameWorld::GetLightList(const BoundingBox& bbox, wlight_t lights[MAX_LIGHTS], int& numLights) const
{
	for(int i = 0; i < MAX_LIGHTS; i++)
	{
		lights[i].position = vec4_zero;
		lights[i].color.w = 0;
	}

	int appliedLights = 0;

	for(int i = 0; i < m_numLights; i++)
	{
		if(appliedLights >= MAX_LIGHTS-1)
			break;

		if(!bbox.IntersectsSphere(m_lights[i].position.xyz(), m_lights[i].position.w))
			continue;

		lights[appliedLights] = m_lights[i];
		lights[appliedLights].position = Vector4D(m_lights[i].position.xyz(), 1.0f / m_lights[i].position.w);

		appliedLights++;
	}

	numLights = appliedLights;
}

ConVar r_disableLightUpdates("r_disableLightUpdates", "0", "Light updates disabled (test only)", CV_CHEAT);

void CGameWorld::UpdateLightTexture()
{
	if(!m_lightsTex)
		return;

	if(r_disableLightUpdates.GetBool())
		return;

	texlockdata_t lockdata;

	m_lightsTex->Lock(&lockdata, NULL, true);
	if(lockdata.pData)
	{
		memset(lockdata.pData, 0, MAX_LIGHTS_TEXTURE*2*sizeof(TVec4D<half>));

		TVec4D<half>* lightData = (TVec4D<half>*)lockdata.pData;

		for(int i = 0; i < m_numLights && i < MAX_LIGHTS_TEXTURE; i++)
		{
			lightData[i] = Vector4D(m_view.GetOrigin()-m_lights[i].position.xyz(), 1.0f / m_lights[i].position.w);
			lightData[MAX_LIGHTS_TEXTURE+i] = m_lights[i].color;
		}
		m_lightsTex->Unlock();
	}

	debugoverlay->Text(ColorRGBA(1,1,0,1),"lights in view: %d/%d", m_numLights,MAX_LIGHTS_TEXTURE);
}

void CGameWorld::UpdateOccludingFrustum()
{
	m_occludingFrustum.Clear();
	m_level.CollectVisibleOccluders( m_occludingFrustum, m_view.GetOrigin() );
}

void CGameWorld::DrawLensFlare( const Vector2D& screenSize, const Vector2D& screenPos, float intensity )
{
	Vector2D halfScreen(screenSize * 0.5f);

	Vector2D lensDir = halfScreen-screenPos;

	float lensDist = length(lensDir);
	float lensScale = lensDist*3.0f;

	lensDir = normalize(lensDir);

	float invTableSize = 1.0f / float(LENSFLARE_TABLE_SIZE-2);
	const int halfTable = (LENSFLARE_TABLE_SIZE-2)/2;

	for(int i = 0; i < LENSFLARE_TABLE_SIZE; i++)
	{
		PFXVertex_t* verts;
		if(g_additPartcles->AllocateGeom(4, 4, &verts, NULL, true) < 0)
			break;

		Rectangle_t texCoords(0,0,1,1);

		TexAtlasEntry_t* entry = g_additPartcles->GetEntry( m_lensTable[i].atlasIdx );

		if(entry)
			texCoords = entry->rect;

		int cnt = i-2;
		cnt = max(0,cnt);

		Vector2D lensPos;

		//float sizeScale = (2000.0f-lensDist)*0.0007f;
		float fScale = m_lensTable[i].scale;//*sizeScale;

		if(i < 2)
		{
			lensPos = screenPos + lensDir*(float(cnt)*invTableSize)*lensScale;
			fScale *= intensity;
		}
		else
			lensPos = halfScreen + lensDir*((0.5f+float(cnt-halfTable))*invTableSize)*lensScale;

		ColorRGB lensColor = m_lensTable[i].color * m_envConfig.sunColor * intensity;

		if(i > 1)
			lensColor *= 0.35f;

		verts[0].point = Vector3D(lensPos + Vector2D(fScale, fScale), 0.0f);
		verts[0].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vrightBottom.y);
		verts[0].color = TVec4D<half>(lensColor, 1.0f);

		verts[1].point =  Vector3D(lensPos + Vector2D(fScale, -fScale), 0.0f);
		verts[1].texcoord = Vector2D(texCoords.vrightBottom.x, texCoords.vleftTop.y);
		verts[1].color = TVec4D<half>(lensColor, 1.0f);

		verts[2].point =  Vector3D(lensPos + Vector2D(-fScale, fScale), 0.0f);
		verts[2].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vrightBottom.y);
		verts[2].color = TVec4D<half>(lensColor, 1.0f);

		verts[3].point = Vector3D(lensPos + Vector2D(-fScale, -fScale), 0.0f);
		verts[3].texcoord = Vector2D(texCoords.vleftTop.x, texCoords.vleftTop.y);
		verts[3].color = TVec4D<half>(lensColor, 1.0f);
	}
}

void CGameWorld::DrawFakeReflections()
{
#ifndef EDITOR
	bool draw = r_drawFakeReflections.GetBool() && (m_envConfig.lightsType != 0 || m_envWetness > 0.01f);
	if (!draw)
	{
		// just clear the reflection texture
		g_pShaderAPI->ChangeRenderTarget(m_reflectionTex);
		g_pShaderAPI->Clear(true,false,false);

		g_pShaderAPI->ChangeRenderTargetToBackBuffer();

		m_reflectionStage = false;

		return;
	}

	m_reflectionStage = true;

	// trace down view
	Matrix4x4 proj, view, skyView;
	CollisionData_t coll;
	g_pPhysics->TestLine(m_view.GetOrigin(), m_view.GetOrigin() - Vector3D(0, 100, 0), coll, (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS));

	float traceResultDist = coll.fract*100.0f;
	Vector3D newViewPos = -(coll.position - Vector3D(0, traceResultDist,0));

	// flip view
	{
		// quickly produce matrices
		Vector3D radians = m_view.GetAngles();
		radians = VDEG2RAD(radians);

		view = rotateZXY4(radians.x, -radians.y, radians.z);
		skyView = view;

		view.translate(newViewPos);

		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());
	}

	// draw into temporary buffer
	g_pShaderAPI->ChangeRenderTarget(m_tempReflTex, 0, m_reflDepth, 0);
	g_pShaderAPI->Clear(true,true,false);
	/*
	if(r_drawsky.GetBool())
	{
		materials->SetMatrix(MATRIXMODE_VIEW, skyView);
		m_skyColor->SetVector3(ColorRGB(0.25f));
		DrawSkyBox(0);
	}*/
	
	materials->SetMatrix(MATRIXMODE_VIEW, view);

	Matrix4x4 viewProj = proj*view;

	if(r_drawFakeReflections.GetInt() > 1)
	{
		FogInfo_t fogInfo = m_info.fogInfo;
		fogInfo.fogColor *= 0.0f;
		fogInfo.fogfar *= 0.8f;
		materials->SetFogInfo(fogInfo);

		materials->SetAmbientColor(ColorRGBA(0, 0, 0, 1.0f));

		materials->SetMaterialRenderParamCallback(this);

		m_level.Render(newViewPos, viewProj, m_occludingFrustum, 0);

		// restore
		materials->SetAmbientColor(ColorRGBA(1.0f));
		materials->SetMaterialRenderParamCallback(NULL);
	}

	// draw only additive particles
	g_additPartcles->Render( EPRFLAG_DONT_FLUSHBUFFERS );
	g_pShaderAPI->ChangeRenderTargetToBackBuffer();

	// Apply the vertical blur on texture
	{
		g_pShaderAPI->ChangeRenderTarget( m_reflectionTex );

		// setup 2D here
		materials->Setup2D(512, 512);
		materials->SetCullMode(CULL_FRONT);

		materials->BindMaterial(m_blurYMaterial);

		const float bufHalfTexel = (1.0f / 512.0f)*0.5f;
		Vertex2D_t screenQuad[] = {MAKETEXQUAD(0,0,512,512,0)};

		CMeshBuilder meshBuilder(materials->GetDynamicMesh());

		meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
			for(int i = 0; i < 4; i++)
			{
				meshBuilder.Position3fv(Vector3D(screenQuad[i].position - Vector2D(bufHalfTexel),0));
				meshBuilder.TexCoord2fv( screenQuad[i].texCoord );
				meshBuilder.AdvanceVertex();
			}
		meshBuilder.End();
	}

	g_pShaderAPI->ChangeRenderTargetToBackBuffer();
#endif // EDITOR

	m_reflectionStage = false;
}

void CGameWorld::Draw( int nRenderFlags )
{
	if(r_no3D.GetBool())
	{
		g_parallelJobs->Wait();
		g_pPFXRenderer->ClearBuffers();

		return;
	}

	// we need to regenerate cubemap for reflections and pretty texture for the fog
	GenerateEnvmapAndFogTextures();

#ifndef EDITOR
	const Vector2D& screenSize = g_pHost->GetWindowSize();
#else
	Vector2D screenSize(512, 512);
#endif // EDITOR

#ifdef EDITOR
	m_level.Ed_Prerender(m_view.GetOrigin());
#endif // EDITOR

	// below operations started asynchronously
	UpdateLightTexture();
	DrawFakeReflections();

	ColorRGB ambColor = lerp(m_envConfig.ambientColor, m_envConfig.ambientColor*r_nightBrightness.GetFloat(), m_envConfig.brightnessModFactor);

	m_info.ambientColor = ColorRGBA(ambColor, 1.0f);
	m_info.sunColor = ColorRGBA(m_envConfig.sunColor, 1.0f);
	m_info.rainBrightness = m_envConfig.rainBrightness;

	float fSkyBrightness = 1.0f;

	// calculate ambient, sun and sky colors
	if( m_envConfig.thunder && m_fNextThunderTime > 0 && m_fNextThunderTime < m_fThunderTime)
	{
		float fThunderLight = saturate(sinf((m_fThunderTime - m_fNextThunderTime)*30.0f))*0.5f;
		fThunderLight += saturate(sinf((m_fThunderTime - m_fNextThunderTime)*70.0f));

		fSkyBrightness = 1.0f + (1.0 - fThunderLight)*0.85f;
		m_info.rainBrightness += fThunderLight*0.5f;

		m_info.sunColor = ColorRGBA(m_envConfig.sunColor + 0.9f * fThunderLight, 1.0f);
	}

	if(r_drawsky.GetBool())
	{
		// Draw sky
		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);
		materials->SetMatrix(MATRIXMODE_WORLD, translate(m_view.GetOrigin()));

		m_skyColor->SetVector4( fSkyBrightness );
		DrawSkyBox(nRenderFlags);
	}

	// calculate fog parameters
	if (fog_override.GetBool())
	{
		FogInfo_t overrridefog;

		overrridefog.viewPos = m_view.GetOrigin();

		overrridefog.enableFog = fog_enable.GetBool();
		overrridefog.fogColor = Vector3D(fog_color_r.GetFloat(), fog_color_g.GetFloat(), fog_color_b.GetFloat());
		overrridefog.fogfar = fog_far.GetFloat();
		overrridefog.fognear = fog_near.GetFloat();

		materials->SetFogInfo(overrridefog);
	}
	else
	{
		m_info.fogInfo.viewPos = m_view.GetOrigin();

		m_info.fogInfo.enableFog = m_envConfig.fogEnable;
		m_info.fogInfo.fogColor = m_envConfig.fogColor;
		m_info.fogInfo.fogfar = m_envConfig.fogFar;
		m_info.fogInfo.fognear = m_envConfig.fogNear;

		materials->SetFogInfo(m_info.fogInfo);
	}

	// setup scene distance
#ifdef EDITOR
	m_sceneinfo.m_fZFar = 10000.0f;
	m_info.fogInfo.fogfar = m_sceneinfo.m_fZFar;
	materials->SetFogInfo(m_info.fogInfo);
#else
	m_sceneinfo.m_fZFar = r_zfar.GetFloat();	// TODO: define by environment info
#endif

	materials->SetAmbientColor(m_info.ambientColor * r_ambientScale.GetFloat());

	// draw axe
	debugoverlay->Line3D(vec3_zero, Vector3D(1, 0, 0), ColorRGBA(0, 0, 0, 1), ColorRGBA(1, 0, 0, 1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0, 1, 0), ColorRGBA(0, 0, 0, 1), ColorRGBA(0, 1, 0, 1));
	debugoverlay->Line3D(vec3_zero, Vector3D(0, 0, 1), ColorRGBA(0, 0, 0, 1), ColorRGBA(0, 0, 1, 1));

	materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
	materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);
	materials->SetMatrix(MATRIXMODE_WORLD, identity4());

	// set debugoverlay transformation
	debugoverlay->SetMatrices(m_matrices[MATRIXMODE_PROJECTION], m_matrices[MATRIXMODE_VIEW]);

	// set global pre-apply callback
	materials->SetMaterialRenderParamCallback(this);

	materials->SetEnvironmentMapTexture( m_envMap );

	// world rendering
	if(r_drawWorld.GetBool())
	{
		// DRAW ONLY OPAQUE OBJECTS
		m_level.Render(m_view.GetOrigin(), m_viewprojection, m_occludingFrustum, nRenderFlags);
	}

	if(r_drawObjects.GetBool())
		UpdateRenderables( m_occludingFrustum );

#ifndef EDITOR
	// cleanup of casters
	m_shadowRenderer.Clear();
#endif // EDITOR

	// setup culling
	materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

	// rendering of instanced objects
	if( m_renderingObjects.goToFirst() )
	{
		// this will render models without instances or add instances
		do
		{
			CGameObject* obj = m_renderingObjects.getCurrent();
			obj->Draw( nRenderFlags );

#ifndef EDITOR
			m_shadowRenderer.AddShadowCaster( obj );
#endif // EDITOR

		}while(m_renderingObjects.goToNext());

		// draw instanced models
		int numCachedModels = g_pModelCache->GetCachedModelCount();

		// draw model instances
		for(int i = 0; i < numCachedModels; i++)
		{
			IEqModel* model = g_pModelCache->GetModel(i);

			if(model && model->GetInstancer())
			{
				model->GetInstancer()->Draw( nRenderFlags, model );
			}
		}

	}

	/*
	if(r_drawWorld.GetBool())
	{
		// UpdateUnderwaterBuffer();

		// DRAW ONLY TRANSPARENT OBJECTS
		m_level.Render(m_CameraParams.GetOrigin(), m_viewprojection, m_occludingFrustum, nRenderFlags | RFLAG_TRANSLUCENCY);
	}
	*/

	//
	// Draw particles
	//

#ifndef EDITOR
	if(m_envConfig.shadowColor.w > 0.01f) // shadow opacity
	{
		materials->SetMaterialRenderParamCallback( NULL );

		// render shadows
		m_shadowRenderer.SetShadowAngles(m_envConfig.sunAngles);
		m_shadowRenderer.RenderShadowCasters();

		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		materials->SetAmbientColor(m_envConfig.shadowColor);
		m_shadowRenderer.Draw();

		materials->SetMaterialRenderParamCallback( this );
	}
#endif // EDITOR

	materials->SetAmbientColor(ColorRGBA(1, 1, 1, 1.0f));

	// restore projection and draw the particles
	materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);

	// wait scheduled PFX render
	g_parallelJobs->Wait();

	// draw particle effects
	g_pPFXRenderer->Render( nRenderFlags );

	// now we done our world render, drop this callback for a while
	materials->SetMaterialRenderParamCallback( NULL );

#ifndef EDITOR
	Vector3D sunLensDirection;
	AngleVectors(m_envConfig.sunLensAngles, &sunLensDirection);

	Vector3D virtualSunPos = m_view.GetOrigin() + sunLensDirection*1000.0f;

	Vector2D lensScreenPos;
	PointToScreen(virtualSunPos, lensScreenPos, m_viewprojection, screenSize);

	// lens flare rendering on screen by using particle engine
	if(m_envConfig.lensIntensity > 0.0f && r_drawLensFlare.GetBool())
	{
		float fIntensity = dot(sunLensDirection, m_matrices[MATRIXMODE_VIEW].rows[2].xyz());

		// Occlusion query begin
		if(m_sunGlowOccQuery &&
			r_drawLensFlare.GetInt() == 2)
		{
			// get previous result

			g_pShaderAPI->Reset( STATE_RESET_VBO );
			materials->Setup2D(screenSize.x, screenSize.y);

			const int LENS_PIXELS_HALFSIZE = 8;
			const int LENS_TOTAL_PIXELS = LENS_PIXELS_HALFSIZE*LENS_PIXELS_HALFSIZE * 4;

			const float LENS_PIXEL_TO_INTENSITY = 1.0f / (float)LENS_TOTAL_PIXELS;

			uint32 pixels = LENS_TOTAL_PIXELS;

			if(m_sunGlowOccQuery->IsReady())
				pixels = m_sunGlowOccQuery->GetVisiblePixels();

			m_lensIntensityTiming = (float)pixels * LENS_PIXEL_TO_INTENSITY * fIntensity;

			m_sunGlowOccQuery->Begin();
			{
				materials->BindMaterial(m_pointQueryMat);
				Vector2D screenQuad[] = {MAKEQUAD(
											lensScreenPos.x,
											lensScreenPos.y,
											lensScreenPos.x,
											lensScreenPos.y, -LENS_PIXELS_HALFSIZE)};

				CMeshBuilder meshBuilder(materials->GetDynamicMesh());

				meshBuilder.Begin(PRIM_TRIANGLE_STRIP);
					for(int i = 0; i < 4; i++)
					{
						meshBuilder.Position3fv(Vector3D(screenQuad[i],1.0f));
						meshBuilder.AdvanceVertex();
					}
				meshBuilder.End();
			}
			m_sunGlowOccQuery->End();
		}
		else
		{
			CollisionData_t coll;
			int collMask = OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_SOLID_GROUND;
			if( g_pPhysics->TestLine(m_view.GetOrigin(), virtualSunPos, coll, collMask))
			{
				m_lensIntensityTiming -= g_pHost->GetFrameTime()*10.0f;
				m_lensIntensityTiming = max(0.0f, m_lensIntensityTiming*fIntensity);
			}
			else
			{
				m_lensIntensityTiming += g_pHost->GetFrameTime()*10.0f;
				m_lensIntensityTiming = min(1.0f, m_lensIntensityTiming*fIntensity);
			}
		}
	}
#endif // EDITOR

#ifndef EDITOR
	// Draw lensflare
	if(m_lensIntensityTiming > 0 && r_drawLensFlare.GetBool())
	{
		FogInfo_t fogDisabled;
		fogDisabled.enableFog = false;
		materials->SetFogInfo(fogDisabled);

		DrawLensFlare(screenSize, lensScreenPos, m_envConfig.lensIntensity*m_lensIntensityTiming );

		materials->SetMatrix(MATRIXMODE_VIEW, identity4());
		materials->SetMatrix(MATRIXMODE_WORLD, identity4());

		g_additPartcles->SetCustomProjectionMatrix( projection2DScreen(screenSize.x,screenSize.y) );
		g_additPartcles->Render( nRenderFlags );

		// restore matrix
		materials->SetMatrix(MATRIXMODE_PROJECTION, m_matrices[MATRIXMODE_PROJECTION]);
		materials->SetMatrix(MATRIXMODE_VIEW, m_matrices[MATRIXMODE_VIEW]);
	}
#endif // EDITOR

	// render all the rest
	//g_pShaderAPI->Flush();
}

void CGameWorld::SetView(const CViewParams& params)
{
	m_view = params;
}

CViewParams* CGameWorld::GetView()
{
	return &m_view;
}

CBillboardList* CGameWorld::FindBillboardList(const char* name) const
{
	for(int i = 0; i < m_billboardModels.numElem(); i++)
	{
		if(!stricmp(m_billboardModels[i]->m_name.c_str(), name))
			return m_billboardModels[i];
	}

	return NULL;
}

void CGameWorld::QueryNearestRegions(const Vector3D& pos, bool waitLoad )
{
	m_level.QueryNearestRegions(pos, waitLoad);
}

bool CGameWorld::LoadLevel()
{
	bool result = true;

	if(!m_levelLoaded)
	{
		// load object definition file
		KeyValues objectDefsKV;
		if( !objectDefsKV.LoadFromFile(varargs("scripts/levels/%s_objects.def", m_levelname.GetData())) )
		{
			MsgWarning("Object definition file for '%s' cannot be loaded or not found\n", m_levelname.GetData());

			if( !objectDefsKV.LoadFromFile(varargs("scripts/levels/default_objects.def", m_levelname.GetData())) )
			{
				MsgError("DEFAULT Object definition file for '%s' cannot be loaded or not found\n");
			}
		}

		// load billboard lists
		for(int i = 0; i < objectDefsKV.GetRootSection()->keys.numElem(); i++)
		{
			kvkeybase_t* kvk = objectDefsKV.GetRootSection()->keys[i];

			if(!stricmp(kvk->name, "billboardlist"))
			{
				KeyValues blbKV;
				if(blbKV.LoadFromFile(KV_GetValueString(kvk, 1)))
				{
					CBillboardList* pBillboardList = new CBillboardList();

					// set the name for searching
					pBillboardList->m_name = KV_GetValueString(kvk, 0);

					// load our file
					pBillboardList->LoadBlb( blbKV.GetRootSection() );

					m_billboardModels.append(pBillboardList);
				}
			}
		}

		result = m_level.Load( m_levelname.GetData(), objectDefsKV.GetRootSection() );
	}

	if(result)
	{
		m_levelLoaded = true;
		InitEnvironment();
	}

#ifndef NO_LUA
	{
		OOLUA::Script& state = GetLuaState();
		EqLua::LuaStackGuard g(state);

		OOLUA::Table tab = OOLUA::new_table(state);
		EqLua::LuaCallUserdataCallback(g_pGameWorld, "OnLevelLoad", tab);
	}
#endif // NO_LUA

	return result;
}

#ifdef EDITOR
bool CGameWorld::LoadPrefabLevel()
{
	bool result = true;

	if(!m_levelLoaded)
	{
		result = m_level.LoadPrefab( m_levelname.GetData() );
	}

	if(result)
	{
		m_levelLoaded = true;
		InitEnvironment();
	}

	return result;
}
#endif // EDITOR

void CGameWorld::SetLevelName(const char* name)
{
	m_levelname = name;
}

const char*	CGameWorld::GetLevelName() const
{
	return m_levelname.GetData();
}

// object factory
#ifndef EDITOR

#include "object_debris.h"
#include "object_physics.h"
#include "object_static.h"
#include "object_tree.h"
#include "object_trafficlight.h"
#include "object_sheets.h"
#include "object_scripted.h"

#endif // EDITOR

// creates non-spawned pre-defined object
// it's a little object factory
CGameObject* CGameWorld::CreateGameObject( const char* typeName, kvkeybase_t* kvdata ) const
{
#ifndef EDITOR
	if(!stricmp(typeName, "dummy"))
	{
		return NULL;
	}
	else if(!stricmp(typeName, "debris"))
	{
		return new CObject_Debris(kvdata);
	}
	else if(!stricmp(typeName, "physics"))
	{
		return new CObject_Physics(kvdata);
	}
	else if(!stricmp(typeName, "static"))
	{
		return new CObject_Static(kvdata);
	}
	else if(!stricmp(typeName, "tree"))
	{
		return new CObject_Tree(kvdata);
	}
	else if(!stricmp(typeName, "trafficlight"))
	{
		return new CObject_TrafficLight(kvdata);
	}
	else if(!stricmp(typeName, "sheets"))
	{
		return new CObject_Sheets(kvdata);
	}
	else if(!stricmp(typeName, "scripted"))
	{
		return new CObject_Scripted(kvdata);
	}

	MsgError("CGameWorld::CreateGameObject error: Invalid type '%s'\n", typeName);
#endif // EDITOR
	return NULL;
}

CGameObject* CGameWorld::CreateObject( const char* objectDefName ) const
{
	if(!stricmp(objectDefName, "dummy"))
	{
		// return clean CGameObject
		return new CGameObject();
	}

	for(int i = 0; i < m_level.m_objectDefs.numElem(); i++)
	{
		CLevObjectDef* def = m_level.m_objectDefs[i];

		if(def->m_info.type != LOBJ_TYPE_OBJECT_CFG)
			continue;

		if(def->m_name == objectDefName)
		{
			return CreateGameObject(def->m_defType.c_str(), def->m_defKeyvalues);
		}
	}

	MsgError("CGameWorld::CreateObject error: Invalid def '%s', check <levelname>_objects.def\n", objectDefName);

	return NULL;
}

CGameObject* CGameWorld::FindObjectByName( const char* objectName ) const
{
	const DkList<CGameObject*>& objList = m_gameObjects;

	for(int i = 0; i < objList.numElem(); i++)
	{
		if(objList[i]->m_name.Length() == 0)
			continue;

		if( !objList[i]->m_name.Compare(objectName) )
			return objList[i];
	}

	return NULL;
}

#ifndef EDITOR
OOLUA::Table CGameWorld::L_FindObjectOnLevel( const char* name ) const
{
	OOLUA::Table newTable;
	newTable = OOLUA::new_table(GetLuaState());

	if(!newTable.valid())
		return newTable;

	levCellObject_t cellObj;
	if( m_level.FindObjectOnLevel(cellObj, name, nullptr) )
	{
		newTable.set("name", cellObj.name);
		newTable.set("position", Vector3D(cellObj.position));
		newTable.set("rotation", cellObj.rotation);
	}

	return newTable;
}
#endif // EDITOR
static CGameWorld s_GameWorld;
CGameWorld*	g_pGameWorld = &s_GameWorld;

#ifndef NO_LUA

OOLUA_EXPORT_FUNCTIONS(CGameWorld, SetEnvironmentName, SetLevelName, GetView, QueryNearestRegions, RemoveObject)
OOLUA_EXPORT_FUNCTIONS_CONST(CGameWorld, FindObjectByName, FindObjectOnLevel, CreateObject, IsValidObject, GetEnvironmentName, GetLevelName)

OOLUA_EXPORT_FUNCTIONS(CViewParams, SetOrigin, SetAngles, SetFOV)
OOLUA_EXPORT_FUNCTIONS_CONST(CViewParams, GetOrigin, GetAngles, GetFOV)

#endif // NO_LUA
