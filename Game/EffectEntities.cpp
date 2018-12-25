//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Effect entities
//////////////////////////////////////////////////////////////////////////////////

#include "BaseEngineHeader.h"
#include "effects.h"
#include "env_explode.h"

#include "Rain.h"
#include "Snow.h"

//ConVar *r_fulllighting = (ConVar*)g_sysConsole->FindCvar("r_fulllighting");

struct ThunderInfo_t
{
	float time;
	Vector3D pos;
};

//ConVar r_rainmap("r_rainmap", "0", "Rainmap technology (Deferred lighting model only)", CV_ARCHIVE);

//-----------------------------------------------------
// ENV_RAIN - rain simulation entity
//-----------------------------------------------------

class CEnvRain : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvRain()
	{
		m_fEmissionRate = 10.0f;
		m_fSpeed = 10000.0f;
		m_bThunder = false;
		m_pEmitter = NULL;
		lastSunPos = Vector3D(0);
	}

	void Precache()
	{
		BaseClass::Precache();

		ses->PrecacheSound("rain.rain");
		ses->PrecacheSound("rain.thunder");
	}

	void Spawn()
	{
		BaseClass::Spawn();

		EmitSound_t rainsound;
		rainsound.nFlags |= EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_START_ON_UPDATE;

		rainsound.fPitch = 1.0f;
		rainsound.fRadiusMultiplier = 1.0f;
		rainsound.fVolume = 1.0f;
		rainsound.pObject = this;

		rainsound.origin = Vector3D(0);

		rainsound.name = "rain.rain";

		ses->EmitSound(&rainsound);
	}

	void OnRemove()
	{
		if(m_pEmitter)
			delete m_pEmitter;

		m_pEmitter = NULL;
	}

	void OnPostRender()
	{
		if(!m_pEmitter)
		{
			m_pEmitter = new RainEmitter();
			m_pEmitter->Init();
		}

		float randomize_time = 1.0 / (1+gpGlobals->frametime);

		int random_lighting = RandomInt(0,800*randomize_time);
		if(random_lighting == 0 && m_bThunder && gpGlobals->frametime > 0) // if not paused
		{
			dlight_t *sunlight = viewrenderer->AllocLight();

			SetLightDefaults(sunlight);

			sunlight->position = g_pViewEntity->GetEyeOrigin();
			sunlight->angles = Vector3D(-60, RandomFloat(0,180), 0);
			sunlight->radius = 1.0f;
			sunlight->color = ColorRGB(0.9f,0.9f,1.0f);
			sunlight->nType = DLT_SUN;

			//sunlight->nFlags |= (!r_fulllighting->GetBool() ? LFLAG_NOSHADOWS : 0);

			sunlight->fovWH.z = 90;
			sunlight->dietime = RandomFloat(0.10f,0.15f);
			sunlight->fadetime = sunlight->dietime;

			viewrenderer->AddLight(sunlight);

			ThunderInfo_t info;
			info.time = gpGlobals->curtime + RandomFloat(0.0f, 10.0f);

			Vector3D sun_dir;
			AngleVectors((sunlight->angles - Vector3D(0,10,0)) * Vector3D(1,-1,1), &sun_dir, NULL, NULL);

			info.pos = g_pViewEntity->GetEyeOrigin() - sun_dir*info.time*200.0f;

			m_thunderTimes.append(info);
		}


		for(int i = 0; i < m_thunderTimes.numElem(); i++)
		{
			if(m_thunderTimes[i].time < gpGlobals->curtime)
			{
				EmitSound_t thunder;
				thunder.nFlags |= EMITSOUND_FLAG_FORCE_CACHED;

				thunder.fPitch = RandomFloat(0.9f, 1.05f);
				thunder.fRadiusMultiplier = 1.0f;
				thunder.fVolume = 1.0f;

				thunder.origin = m_thunderTimes[i].pos;

				thunder.name = "rain.thunder";

				ses->EmitSound(&thunder);

				m_thunderTimes.removeIndex(i);
				i--;
			}
		}
		
		/*
		// Only deferred shading supports rainmap feature
		if(materials->GetLightingModel() == MATERIAL_LIGHT_DEFERRED && r_rainmap.GetBool())
		{
			dlight_t *rainmap = viewrenderer->AllocLight();

			SetLightDefaults(rainmap);

			Vector3D diff = lastSunPos - viewrenderer->GetView()->GetOrigin();

			#define SUNDIFF_EPS 100

			if(length(diff) > SUNDIFF_EPS)
				lastSunPos = viewrenderer->GetView()->GetOrigin();

			rainmap->position = lastSunPos;
			rainmap->angles = Vector3D(90,0,0);
			rainmap->radius = 1000.0f;
			rainmap->color = ColorRGB(0);
			rainmap->nFlags = LFLAG_RAINMAP;
			rainmap->fovWH.z = 90;

			//if(scenerenderer->GetView())
			//	rainmap->leaf = scenerenderer->GetView()->GetLeafIndex();

			viewrenderer->AddLight(rainmap);
		}
		*/

		m_pEmitter->Update_Draw(gpGlobals->frametime, m_fEmissionRate, m_fSpeed);
	}

private:

	float					m_fEmissionRate;
	float					m_fSpeed;
	bool					m_bThunder;

	RainEmitter*			m_pEmitter;

	Vector3D				lastSunPos;

	DkList<ThunderInfo_t>	m_thunderTimes;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CEnvRain)

	DEFINE_KEYFIELD( m_bThunder,		"thunder", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_fEmissionRate,	"emissionrate", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fSpeed,			"speed", VTYPE_FLOAT),

END_DATAMAP()

DECLARE_ENTITYFACTORY(env_rain,CEnvRain)

//-----------------------------------------------------
// ENV_SNOW - snow simulation entity
//-----------------------------------------------------

class CEnvSnow : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSnow()
	{
		m_fEmissionRate = 10.0f;
		m_fSpeed = 1000.0f;
		m_pEmitter = NULL;
		lastSunPos = Vector3D(0);
	}

	void OnRemove()
	{
		if(m_pEmitter)
			delete m_pEmitter;

		m_pEmitter = NULL;
	}

	void OnPostRender()
	{
		if(!m_pEmitter)
		{
			m_pEmitter = new SnowEmitter();
			m_pEmitter->Init();
		}

		m_pEmitter->Update_Draw(gpGlobals->frametime, m_fEmissionRate, m_fSpeed, GetAbsAngles());
	}

private:

	float					m_fEmissionRate;
	float					m_fSpeed;

	SnowEmitter*			m_pEmitter;

	Vector3D				lastSunPos;

	DkList<ThunderInfo_t>	m_thunderTimes;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CEnvSnow)

	DEFINE_KEYFIELD( m_fEmissionRate,	"emissionrate", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fSpeed,		"speed", VTYPE_FLOAT),

END_DATAMAP()

DECLARE_ENTITYFACTORY(env_snow,CEnvSnow)

ConVar r_drawsprites("r_drawsprites", "1");

//-----------------------------------------------------
// ENV_SPRITE - single sprite or glow
//-----------------------------------------------------

class CEnvSprite : public BaseEntity
{
public:
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSprite()
	{
		color = color3_white;
		size = Vector2D(10,10);
		glowMaterial = "effects/glow";
		itsGroup = 0;
		m_bLockX = false;
	}

	void Precache()
	{
		BaseClass::Precache();

		itsGroup = PrecacheParticleMaterial(glowMaterial.GetData());
	}

	void OnPostRender()
	{
		if(!r_drawsprites.GetBool())
			return;

		PFXBillboard_t effect;
		effect.vOrigin = GetAbsOrigin();
		effect.vColor = Vector4D(color,1);
		effect.nGroupIndex = itsGroup;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;
		
		if(m_bLockX)
			effect.nFlags |= EFFECT_FLAG_LOCK_X;

		effect.fZAngle = 0;
		effect.fWide = size.x;
		effect.fTall = size.y;

		GR_DrawBillboard(&effect);
	}

private:
	DECLARE_DATAMAP();

	EqString		glowMaterial;
	Vector3D		color;
	Vector2D		size;

	int				itsGroup;
	bool			m_bLockX;
};

DECLARE_ENTITYFACTORY(env_sprite,CEnvSprite)

// declare save data
BEGIN_DATAMAP(CEnvSprite)

	DEFINE_KEYFIELD( glowMaterial,	"effect", VTYPE_STRING),
	DEFINE_KEYFIELD( size,			"size", VTYPE_VECTOR2D),
	DEFINE_KEYFIELD( color,			"color", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_bLockX,		"lockx", VTYPE_BOOLEAN),
	

END_DATAMAP()

//-----------------------------------------------------
// ENV_GLOW - light glow effect
//-----------------------------------------------------

class CEnvGlow : public BaseEntity
{
public:
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvGlow()
	{
		color = color3_white;
		size = Vector2D(10,10);
		glowMaterial = "effects/glow";
		itsGroup = 0;

		m_bLockX = false;

		fFadeLevel = 0.0f;

		fMinDistance = 50.0f;
		fMaxDistance = 1800.0f;
		fMinSizeScale = 1.0f;
		fMaxSizeScale = 3.0f;
	}

	void Precache()
	{
		BaseClass::Precache();

		itsGroup = PrecacheParticleMaterial(glowMaterial.GetData());
	}

	void OnPostRender()
	{
		BaseEntity* pActor = (BaseEntity*)UTIL_EntByIndex(1);

		if(!r_drawsprites.GetBool() || !g_pViewEntity || !pActor)
			return;

		IPhysicsObject* pObject = pActor->PhysicsGetObject();

		float view_dist = length(g_pViewEntity->GetEyeOrigin() - GetAbsOrigin());

		internaltrace_t tr;
		physics->InternalTraceLine(GetAbsOrigin(), g_pViewEntity->GetEyeOrigin(), COLLISION_GROUP_WORLD, &tr, &pObject, 1);

		// fade level timing
		if(tr.fraction >= 1.0f)
			fFadeLevel += gpGlobals->frametime*4.0f;
		else
			fFadeLevel -= gpGlobals->frametime*8.0f;

		bool bDraw = (fFadeLevel > 0.01f);

		fFadeLevel = clamp(fFadeLevel,0,1);

		float dist_fac = clamp((view_dist - fMinDistance) / fMaxDistance, 0, 1);

		// don't render invisible
		if(!bDraw || (dist_fac >= 0.99f))
			return;

		float fSizeScale = fMinSizeScale + dist_fac * fMaxSizeScale;

		PFXBillboard_t effect;
		effect.vOrigin = GetAbsOrigin();
		effect.vColor = Vector4D(color,1.0f) * (1.0f - (dist_fac*dist_fac*dist_fac)) * fFadeLevel;
		effect.nGroupIndex = itsGroup;
		effect.nFlags = EFFECT_FLAG_ALWAYS_VISIBLE;

		if(m_bLockX)
			effect.nFlags |= EFFECT_FLAG_LOCK_X;

		effect.fZAngle = 0;

		effect.fWide = size.x*fSizeScale;
		effect.fTall = size.y*fSizeScale;

		GR_DrawBillboard(&effect);
	}

private:
	DECLARE_DATAMAP();

	EqString		glowMaterial;
	Vector3D		color;
	Vector2D		size;

	float			fMinDistance;
	float			fMaxDistance;
	float			fMinSizeScale;
	float			fMaxSizeScale;

	float			fFadeLevel;

	int				itsGroup;

	bool			m_bLockX;
};

DECLARE_ENTITYFACTORY(env_glow,CEnvGlow)

// declare save data
BEGIN_DATAMAP(CEnvGlow)

	DEFINE_KEYFIELD( glowMaterial,	"effect", VTYPE_STRING),
	DEFINE_KEYFIELD( size,			"size", VTYPE_VECTOR2D),
	DEFINE_KEYFIELD( color,			"color", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( fMinDistance,	"mindistance", VTYPE_FLOAT),
	DEFINE_KEYFIELD( fMaxDistance,	"maxdistance", VTYPE_FLOAT),
	DEFINE_KEYFIELD( fMinSizeScale,	"minsizescale", VTYPE_FLOAT),
	DEFINE_KEYFIELD( fMaxSizeScale,	"maxsizescale", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_bLockX,		"lockx", VTYPE_BOOLEAN),

END_DATAMAP()

//-----------------------------------------------------
// ENV_SMOKE - smoke emitter
//-----------------------------------------------------

class CEnvSmoke : public BaseEntity
{
public:
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSmoke()
	{
		startcolor = color3_white;
		endcolor =  color3_white;

		startsize = 10;
		endsize = 15;

		glowMaterial = "dev/particle";
		itsGroup = 0;

		m_fParticlesEmitTime = 0.5f;
		m_nParticlesPerTime = 4;

		m_fParticleLifeTime = 0.15f;

		m_fNextParticleEmissionTime = 0.0f;

		m_fParticleMaxRotation = 45;
		m_fParticleMaxSpeed = 45;

		m_fParticleAlpha = 1.0f;

		m_vGravity = vec3_zero;

		m_fSpreadingAngle = 0.0f;

		m_bEnabled = false;
	}

	void Precache()
	{
		BaseClass::Precache();

		itsGroup = PrecacheParticleMaterial(glowMaterial.GetData());
	}

	void Input_Enable(inputdata_t &var)
	{
		m_bEnabled = true;
	}

	void Input_Disable(inputdata_t &var)
	{
		m_bEnabled = false;
	}

	void Input_Toggle(inputdata_t &var)
	{
		m_bEnabled = !m_bEnabled;
	}

	void OnPostRender()
	{
		bool isVisibleToCamera = true;//viewrenderer->PointIsVisible( GetAbsOrigin() );

		if(m_fNextParticleEmissionTime < gpGlobals->curtime && m_bEnabled)
		{
			for(int i = 0; i < m_nParticlesPerTime && isVisibleToCamera; i++)
			{
				Vector3D forw(0,0,1);

				Vector3D angles = GetAbsAngles()+Vector3D(RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle),RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle),RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle));

				Matrix4x4 rotMatrix = !rotateXYZ4(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z));

				forw = rotMatrix.rows[2].xyz();

				//AngleVectors(,&forw);
				/* REWRITE_GAME_EFFECTS*/
				CSmokeEffect* pEffect = new CSmokeEffect(GetAbsOrigin(),
														forw*m_fParticleMaxSpeed+RandomFloat(-0.1f, 0.1f),
														startsize+RandomFloat(-0.9f, 0.9f), 
														endsize+RandomFloat(-0.9f, 0.9f), 
														m_fParticleLifeTime+RandomFloat(-0.12f, 0.12f), 
														itsGroup,m_fParticleMaxRotation+RandomFloat(-40, 40), 
														m_vGravity*RandomFloat(0.9f, 1.1f), 
														startcolor*RandomFloat(0.9f, 1.1f), 
														endcolor*RandomFloat(0.9f, 1.1f), 
														m_fParticleAlpha);
				
				effectrenderer->RegisterEffectForRender(pEffect);
				
			}

			m_fNextParticleEmissionTime = gpGlobals->curtime+m_fParticlesEmitTime;
		}
	}

private:
	DECLARE_DATAMAP();

	EqString		glowMaterial;

	Vector3D		startcolor;
	Vector3D		endcolor;

	float			startsize;
	float			endsize;

	float			m_fParticlesEmitTime;
	int				m_nParticlesPerTime;
	float			m_fParticleLifeTime;
	float			m_fParticleAlpha;
	float			m_fSpreadingAngle;

	float			m_fParticleMaxSpeed;
	Vector3D		m_vGravity;

	int				itsGroup;

	float			m_fNextParticleEmissionTime;

	float			m_fParticleMaxRotation;

	bool			m_bEnabled;
};

DECLARE_ENTITYFACTORY(env_smoke,CEnvSmoke)

// declare save data
BEGIN_DATAMAP(CEnvSmoke)

	DEFINE_KEYFIELD( glowMaterial,	"effect", VTYPE_STRING),
	DEFINE_KEYFIELD( startsize,		"startsize", VTYPE_FLOAT),
	DEFINE_KEYFIELD( endsize,		"endsize", VTYPE_FLOAT),

	DEFINE_KEYFIELD( startcolor,		"startcolor", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( endcolor,		"endcolor", VTYPE_VECTOR3D),

	DEFINE_KEYFIELD( m_vGravity,		"gravity", VTYPE_VECTOR3D),

	DEFINE_KEYFIELD( m_fParticleLifeTime, "lifetime", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fParticleMaxRotation, "maxrotation", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fParticleMaxSpeed, "maxspeed", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_fParticleAlpha, "alpha", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_fParticlesEmitTime, "emittime", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_nParticlesPerTime, "emitcount", VTYPE_INTEGER),

	DEFINE_KEYFIELD( m_fSpreadingAngle, "spreadangle", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_bEnabled,			"enabled", VTYPE_BOOLEAN),

	DEFINE_INPUTFUNC( "Enable",			Input_Enable),
	DEFINE_INPUTFUNC( "Disable",		Input_Disable),
	DEFINE_INPUTFUNC( "Toggle",			Input_Toggle),

END_DATAMAP()

//-----------------------------------------------------
// ENV_SPARKS - spark emitter
//-----------------------------------------------------

class CEnvSparks : public BaseEntity
{
public:
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvSparks()
	{
		startcolor = color3_white;
		endcolor =  color3_white;

		startsize = 0.085f;
		endsize = 0.085f;

		startlength = 90;

		glowMaterial = "effects/tracer";
		itsGroup = 0;

		m_fParticlesEmitTime = 0.15f;
		m_nParticlesPerTime = 15;

		m_fParticleLifeTime = 1.75f;

		m_fNextParticleEmissionTime = 0.0f;

		m_fParticleMaxSpeed = 45;

		m_fParticleAlpha = 1.0f;

		m_vGravity = Vector3D(0,-300,0);

		m_fSpreadingAngle = 65.0f;
		
		m_bEnabled = false;
	}

	void Precache()
	{
		BaseClass::Precache();

		itsGroup = PrecacheParticleMaterial(glowMaterial.GetData());
	}

	void Input_Enable(inputdata_t &var)
	{
		m_bEnabled = true;
	}

	void Input_Disable(inputdata_t &var)
	{
		m_bEnabled = false;
	}

	void Input_Toggle(inputdata_t &var)
	{
		m_bEnabled = !m_bEnabled;
	}
	void OnPostRender()
	{
		bool isVisibleToCamera = true;

		if(m_fNextParticleEmissionTime < gpGlobals->curtime && m_bEnabled)
		{
			for(int i = 0; i < m_nParticlesPerTime && isVisibleToCamera; i++)
			{
				Vector3D forw(0,0,1);

				Vector3D angles = GetAbsAngles()+Vector3D(RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle),RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle),RandomFloat(-m_fSpreadingAngle, m_fSpreadingAngle));

				Matrix4x4 rotMatrix = !rotateXYZ4(DEG2RAD(angles.x), DEG2RAD(angles.y), DEG2RAD(angles.z));

				forw = rotMatrix.rows[2].xyz();
				/* REWRITE_GAME_EFFECTS*/
				CSparkLine* pEffect = new CSparkLine(	GetAbsOrigin(),
														forw*m_fParticleMaxSpeed+RandomFloat(-0.1f, 0.1f),
														m_vGravity*RandomFloat(0.9f, 1.1f),
														startlength+RandomFloat(-0.09f, 0.09f),
														startsize+RandomFloat(-0.09f, 0.09f), 
														endsize+RandomFloat(-0.09f, 0.09f), 
														m_fParticleLifeTime+RandomFloat(-0.12f, 0.12f), 
														itsGroup,
														true);
				
				effectrenderer->RegisterEffectForRender(pEffect);
				
			}

			m_fNextParticleEmissionTime = gpGlobals->curtime+m_fParticlesEmitTime;
		}
	}

private:
	DECLARE_DATAMAP();

	bool			m_bEnabled;

	EqString		glowMaterial;

	Vector3D		startcolor;
	Vector3D		endcolor;

	float			startsize;
	float			endsize;

	float			startlength;

	float			m_fParticlesEmitTime;
	int				m_nParticlesPerTime;
	float			m_fParticleLifeTime;
	float			m_fParticleAlpha;
	float			m_fSpreadingAngle;

	float			m_fParticleMaxSpeed;
	Vector3D		m_vGravity;

	int				itsGroup;

	float			m_fNextParticleEmissionTime;
};

DECLARE_ENTITYFACTORY(env_sparks,CEnvSparks)

// declare save data
BEGIN_DATAMAP(CEnvSparks)

	DEFINE_KEYFIELD( glowMaterial,			"effect", VTYPE_STRING),
	DEFINE_KEYFIELD( startsize,				"startsize", VTYPE_FLOAT),
	DEFINE_KEYFIELD( endsize,				"endsize", VTYPE_FLOAT),
	DEFINE_KEYFIELD( startlength,			"startlength", VTYPE_FLOAT),

	DEFINE_KEYFIELD( startcolor,			"startcolor", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( endcolor,				"endcolor", VTYPE_VECTOR3D),

	DEFINE_KEYFIELD( m_vGravity,			"gravity", VTYPE_VECTOR3D),

	DEFINE_KEYFIELD( m_fParticleLifeTime,	"lifetime", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fParticleMaxSpeed,	"maxspeed", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_fParticleAlpha,		"alpha", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_fParticlesEmitTime,	"emittime", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_nParticlesPerTime,	"emitcount", VTYPE_INTEGER),

	DEFINE_KEYFIELD( m_fSpreadingAngle,		"spreadangle", VTYPE_FLOAT),

	DEFINE_KEYFIELD( m_bEnabled,			"enabled", VTYPE_BOOLEAN),

	DEFINE_INPUTFUNC( "Enable",			Input_Enable),
	DEFINE_INPUTFUNC( "Disable",		Input_Disable),
	DEFINE_INPUTFUNC( "Toggle",			Input_Toggle),

END_DATAMAP()

//-----------------------------------------------------
// ENV_FOG - fog entity for level
//-----------------------------------------------------

class CEnvFog : public BaseEntity
{
public:
	
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CEnvFog()
	{
		m_FogColor = ColorRGB(1,1,1);

		m_fFogFar = 3500;
		m_fFogNear = 1000;
		m_fFogIntensity = 1.0f;
		n_bFogEnable = false;
	}

	void OnPreRender()
	{
		FogInfo_t fog;

		fog.fognear = m_fFogNear;
		fog.fogfar = m_fFogFar;
		fog.fogdensity = m_fFogIntensity;
		fog.fogColor = m_FogColor;
		fog.enableFog = n_bFogEnable;

		materials->SetFogInfo(fog);

//		SetFogInfo(fog);
	}

private:

	ColorRGB	m_FogColor;

	float		m_fFogFar;
	float		m_fFogNear;
	float		m_fFogIntensity;
	bool		n_bFogEnable;
	bool		m_bRoomControlled;

	DECLARE_DATAMAP();
};



// declare save data
BEGIN_DATAMAP(CEnvFog)

	DEFINE_KEYFIELD( n_bFogEnable,		"enablefog", VTYPE_BOOLEAN),
	DEFINE_KEYFIELD( m_fFogFar,			"far", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fFogNear,		"near", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fFogIntensity,	"intensity", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_FogColor,		"_color", VTYPE_VECTOR3D),
	DEFINE_KEYFIELD( m_bRoomControlled, "roomcontrol", VTYPE_BOOLEAN)

END_DATAMAP()

DECLARE_ENTITYFACTORY(env_fog,CEnvFog)

class CPhysExplode : public BaseEntity
{
public:
	// always do this type conversion
	typedef BaseEntity BaseClass;

	CPhysExplode()
	{
		m_fMagnitude = 1000.0f;
		m_fRadius = 128;
		m_fDamage = 0.0f;
	}

	void InputExplode(inputdata_t &var)
	{
		// we doesn't produce damage for that explosion

		explodedata_t exp;
		exp.magnitude = m_fMagnitude;
		exp.radius = m_fRadius;
		exp.damage = m_fDamage;
		exp.origin = GetAbsOrigin();
		exp.pInflictor = this;
		//exp.fast = false;

		physics->DoForEachObject(ExplodeAddImpulseToEachObject, &exp);
	}

private:
	float		m_fMagnitude;
	float		m_fRadius;
	float		m_fDamage;

	DECLARE_DATAMAP();
};

// declare save data
BEGIN_DATAMAP(CPhysExplode)

	DEFINE_KEYFIELD( m_fMagnitude,		"magnitude", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fRadius,			"radius", VTYPE_FLOAT),
	DEFINE_KEYFIELD( m_fDamage,			"damage", VTYPE_FLOAT),
	DEFINE_INPUTFUNC( "Explode",		InputExplode),

END_DATAMAP()

DECLARE_ENTITYFACTORY(phys_explosion,CPhysExplode)
