//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#include "car.h"

#include "world.h"
#include "input.h"

#include "DebugOverlay.h"
#include "session_stuff.h"
#include "EqParticles.h"
#include "replay.h"
#include "object_debris.h"
#include "heightfield.h"
#include "ParticleEffects.h"

#include "director.h"

#include "Shiny.h"

#pragma todo("optimize vehicle code")
#pragma TODO("use torque curve based on integer numbers? Heavy vehicles will be proud")

static const char* s_pBodyPartsNames[] =
{
	"front.left",
	"front.right",

	"side.left",
	"side.right",

	"back.left",
	"back.right",

	"top.roof",

	"window.front",
	"window.back",

	"window.left",
	"window.right"
};

static Vector3D s_BodyPartDirections[] =
{
	Vector3D(-1.0f,0,1),
	Vector3D(1.0f,0,1),

	Vector3D(-1.0f,0,0),
	Vector3D(1.0f,0,0),

	Vector3D(-1.0f,0,-1.0f),
	Vector3D(1.0f,0,-1.0f),

	Vector3D(0,1.0f,0),
};

#ifndef EDITOR
DECLARE_CMD(towfun, "", CV_CHEAT)
{
	if (!g_pGameSession)
		return;

	CCar* pcar = g_pGameSession->GetPlayerCar();

	if (pcar->GetHingedVehicle())
	{
		pcar->ReleaseHingedVehicle();
		return;
	}

	DkList<CGameObject*> nearestCars;
	g_pGameWorld->QueryObjects(nearestCars, length(pcar->m_conf->physics.body_size)*2.0f, pcar->GetOrigin(), [](CGameObject* x) {
		return (x->ObjType() == GO_CAR || x->ObjType() == GO_CAR_AI);
	});

	CCar* nearestCar = nullptr;
	float maxDist = DrvSynUnits::MaxCoordInUnits;

	for (int i = 0; i < nearestCars.numElem(); i++)
	{
		if (nearestCars[i] == pcar)
			continue;

		float distBetweenCars = distance(pcar->GetOrigin(), nearestCars[i]->GetOrigin());

		if (distBetweenCars < maxDist)
		{
			nearestCar = (CCar*)nearestCars[i];
			maxDist = distBetweenCars;
		}
	}

	if (nearestCar)
		pcar->HingeVehicle(1, nearestCar, 0);
}
#endif // EDITOR

//
// Some default parameters for handling
//

const float DEFAULT_CAMERA_FOV			= 60.0f;
const float CAMERA_MIN_FOV				= 50.0f;
const float CAMERA_MAX_FOV				= 90.0f;

const float CAMERA_DISTANCE_BIAS		= 0.25f;

const float ACCELERATION_CONST			= 2.0f;
const float	ACCELERATION_SOUND_CONST	= 10.0f;

const float STEERING_CONST				= 1.5f;

const float STEERING_SPEED_REDUCE_FACTOR= 0.7f;
const float STEERING_SPEED_REDUCE_CURVE = 1.25f;
const float STEERING_REDUCE_SPEED_MIN	= 80.0f;
const float STEERING_REDUCE_SPEED_MAX	= 160.0f;

const float EXTEND_STEER_SPEED_MULTIPLIER = 1.75f;

const float STEERING_HELP_START			= 0.25f;
const float STEERING_HELP_CONST			= 0.75f;

const float ANTIROLL_FACTOR_DEADZONE	= 0.01f;
const float ANTIROLL_FACTOR_MAX			= 1.0f;
const float ANTIROLL_SCALE				= 4.0f;

const float DEFAULT_SHIFT_ACCEL_FACTOR	= 0.25f;
const float	GEARBOX_SHIFT_DELAY			= 0.35f;

const float DEFAULT_CAR_INERTIA_SCALE	= 1.5f;

const float FLIPPED_TOLERANCE_COSINE	= 0.15f;

const float MIN_VISUAL_BODYPART_DAMAGE	= 0.32f;

const float DAMAGE_MINIMPACT			= 0.45f;
const float DAMAGE_SCALE				= 0.12f;
const float	DAMAGE_VISUAL_SCALE			= 0.75f;
const float DAMAGE_WHEEL_SCALE			= 0.5f;

const float DEFAULT_MAX_SPEED			= 150.0f;
const float BURNOUT_MAX_SPEED			= 70.0f;

const float DAMAGE_SOUND_SCALE			= 0.25f;
const float SIREN_SOUND_DEATHTIME		= 3.5f;

const float	DAMAGE_WATERDAMAGE_RATE		= 5.5f;

const float CAR_DEFAULT_MAX_DAMAGE		= 16.0f;

const int SKIDMARK_MAX_LENGTH			= 256;
const float SKIDMARK_MIN_INTERVAL		= 0.25f;

const float GEARBOX_DECEL_SHIFTDOWN_FACTOR	= 0.8f;

const float WHELL_ROLL_RESISTANCE_CONST		= 700.0f;

const float WHEEL_ROLL_RESISTANCE_FREE	= 8.0f;
const float ENGINE_ROLL_RESISTANCE		= 38.0f;

const float WHEEL_MIN_SKIDTIME			= 2.5f;
const float WHEEL_SKIDTIME_EFFICIENCY	= 0.25f;
const float WHEEL_SKID_COOLDOWNTIME		= 10.0f;
const float WHEEL_SKID_WETSCALING		= 0.7f;	// env wetness scale

const float WHEEL_VISUAL_DAMAGE_FACTOR_Y	= 0.025f;
const float WHEEL_VISUAL_DAMAGE_FACTOR_Z	= 0.05f;

const float WHEEL_MIN_DAMAGE_LOOSE_HUBCAP	= 0.5f;

const float HINGE_INIT_DISTANCE_THRESH		= 4.0f;
const float HINGE_DISCONNECT_COS_ANGLE		= 0.2f;

const float SKID_SMOKE_MAX_WETNESS			= 0.5f; // wetness level before skid sound disappear
const float SKID_WATERTRAIL_MIN_WETNESS		= 0.25f; // wetness level before skid sound disappear

const float CAR_PHYSICS_RESTITUTION			= 0.5f;
const float CAR_PHYSICS_FRICTION			= 0.25f;


bool ParseVehicleConfig( vehicleConfig_t* conf, const kvkeybase_t* kvs )
{
	conf->visual.cleanModelName = KV_GetValueString(kvs->FindKeyBase("cleanModel"), 0, "");
	conf->visual.damModelName = KV_GetValueString(kvs->FindKeyBase("damagedModel"), 0, conf->visual.cleanModelName.c_str());

	conf->flags.isCar = KV_GetValueBool(kvs->FindKeyBase("isCar"), 0, true);
	conf->flags.allowParked = KV_GetValueBool(kvs->FindKeyBase("allowParked"), 0, true);
	conf->flags.isCop = KV_GetValueBool(kvs->FindKeyBase("isCop"), 0, false);
	conf->flags.reverseWhine = KV_GetValueBool(kvs->FindKeyBase("reverseWhine"), 0, true);

	ASSERTMSG(conf->visual.cleanModelName.Length(), "ParseVehicleConfig - missing cleanModel!\n");

	kvkeybase_t* wheelModelType = kvs->FindKeyBase("wheelBodyGroup");

	const char* defaultWheelName = KV_GetValueString(wheelModelType, 0, "wheel1");
	const char* defaultHubcapWheelName = KV_GetValueString(wheelModelType, 1, "");
	const char* defaultHubcapName = KV_GetValueString(wheelModelType, 2, "");

	conf->physics.body_size = KV_GetVector3D(kvs->FindKeyBase("bodySize"));
	conf->physics.body_center = KV_GetVector3D(kvs->FindKeyBase("center"));
	conf->physics.virtualMassCenter = KV_GetVector3D(kvs->FindKeyBase("gravityCenter"));

	conf->physics.engineType = (EEngineType)KV_GetValueInt(kvs->FindKeyBase("engineType"), 0, CAR_ENGINE_PETROL);
	conf->physics.differentialRatio = KV_GetValueFloat(kvs->FindKeyBase("differential"));
	conf->physics.torqueMult = KV_GetValueFloat(kvs->FindKeyBase("torqueMultipler"));
	conf->physics.transmissionRate = KV_GetValueFloat(kvs->FindKeyBase("transmissionRate"));
	conf->physics.shiftAccelFactor = KV_GetValueFloat(kvs->FindKeyBase("shiftAccelFactor"), 0, DEFAULT_SHIFT_ACCEL_FACTOR);
	conf->physics.mass = KV_GetValueFloat(kvs->FindKeyBase("mass"));
	conf->physics.inertiaScale = KV_GetValueFloat(kvs->FindKeyBase("inertiaScale"), 0, DEFAULT_CAR_INERTIA_SCALE);
	conf->physics.antiRoll = KV_GetValueFloat(kvs->FindKeyBase("antiRoll"));

	conf->physics.maxSpeed = KV_GetValueFloat(kvs->FindKeyBase("maxSpeed"), 0, DEFAULT_MAX_SPEED);
	conf->physics.burnoutMaxSpeed = KV_GetValueFloat(kvs->FindKeyBase("burnoutMaxSpeed"), 0, BURNOUT_MAX_SPEED);

	conf->physics.steeringSpeed = KV_GetValueFloat(kvs->FindKeyBase("steerSpeed"), 0, STEERING_CONST);
	conf->physics.handbrakeScale = KV_GetValueFloat(kvs->FindKeyBase("handbrakeScale"), 0, 1.0f);

	kvkeybase_t* hingePoints = kvs->FindKeyBase("hingePoints");
	conf->physics.hingePoints[0] = 0.0f;
	conf->physics.hingePoints[1] = 0.0f;

	if(hingePoints)
	{
		conf->physics.hingePoints[0] = KV_GetVector3D(hingePoints->FindKeyBase("front"), 0, vec3_zero);
		conf->physics.hingePoints[1] = KV_GetVector3D(hingePoints->FindKeyBase("back"), 0, vec3_zero);
	}

	float suspensionLift = KV_GetValueFloat(kvs->FindKeyBase("suspensionLift"));

	kvkeybase_t* pGears = kvs->FindKeyBase("gears");

	if(pGears)
	{
		conf->physics.numGears = pGears->values.numElem();
		conf->physics.gears = new float[conf->physics.numGears];

		for(int i = 0; i < conf->physics.numGears; i++)
			conf->physics.gears[i] = KV_GetValueFloat( pGears, i );
	}
	else if(conf->flags.isCar)
		MsgError("no gears found in config for '%s'!", conf->carName.c_str());

	kvkeybase_t* pWheels = kvs->FindKeyBase("wheels");

	if( pWheels )
	{
		conf->physics.numWheels = 0;

		for(int i = 0; i < pWheels->keys.numElem(); i++)
		{
			if(!pWheels->keys[i]->IsSection())
				continue;

			conf->physics.numWheels++;
		}

		conf->physics.wheels = new carWheelConfig_t[conf->physics.numWheels];

		// time for wheels
		for(int i = 0; i < pWheels->keys.numElem(); i++)
		{
			if(!pWheels->keys[i]->IsSection())
				continue;

			carWheelConfig_t& wconf = conf->physics.wheels[i];

			kvkeybase_t* pWheelKv = pWheels->keys[i];

			Vector3D suspensiontop = KV_GetVector3D(pWheelKv->FindKeyBase("SuspensionTop"));
			Vector3D suspensionbottom = KV_GetVector3D(pWheelKv->FindKeyBase("SuspensionBottom"));

			float wheel_width = KV_GetValueFloat(pWheelKv->FindKeyBase("width"), 0.0f, 1.0f);

			float rot_sign = (suspensiontop.x > 0) ? 1 : -1;
			float widthAdd = (wheel_width * 0.5f) * rot_sign;

			suspensiontop.x += widthAdd;
			suspensionbottom.x += widthAdd;
			suspensionbottom.y += suspensionLift;

			bool bDriveWheel = KV_GetValueBool(pWheelKv->FindKeyBase("drive"));
			bool bHandbrakeWheel = KV_GetValueBool(pWheelKv->FindKeyBase("handbrake"));
			bool bSteerWheel = fabs( KV_GetValueFloat(pWheelKv->FindKeyBase("steer")) ) > 0.0f;

			wconf.radius = KV_GetValueFloat(pWheelKv->FindKeyBase("radius"), 0, 1.0f);
			wconf.width = wheel_width;
			wconf.woffset = widthAdd;

			kvkeybase_t* wheelModels = kvs->FindKeyBase("bodyGroup");

			const char* wheelName = KV_GetValueString(wheelModels, 0, NULL);
			const char* wheelHubcapName = KV_GetValueString(wheelModels, 1, NULL);
			const char* hubcapName = KV_GetValueString(wheelModels, 2, NULL);

			if(wheelName)
				strncpy(wconf.wheelName, wheelName, 16 );
			else
				strncpy(wconf.wheelName, defaultWheelName, 16 );

			if(wheelHubcapName)
				strncpy(wconf.hubcapWheelName, wheelHubcapName, 16 );
			else
				strncpy(wconf.hubcapWheelName, defaultHubcapWheelName, 16 );

			if(hubcapName)
				strncpy(wconf.hubcapName, hubcapName, 16 );
			else
				strncpy(wconf.hubcapName, defaultHubcapName, 16 );

			wconf.flags = (bDriveWheel ? WHEEL_FLAG_DRIVE : 0) | (bHandbrakeWheel ? WHEEL_FLAG_HANDBRAKE : 0) | (bSteerWheel ? WHEEL_FLAG_STEER : 0);
			wconf.springConst = KV_GetValueFloat(pWheelKv->FindKeyBase("SuspensionSpringConstant"), 0, 100.0f);
			wconf.dampingConst = KV_GetValueFloat(pWheelKv->FindKeyBase("SuspensionDampingConstant"), 0, 50.0f);
			wconf.camber = KV_GetValueFloat(pWheelKv->FindKeyBase("CamberAngle"), 0, 10.0f);
			wconf.suspensionTop = suspensiontop;
			wconf.suspensionBottom = suspensionbottom;
			wconf.brakeTorque = KV_GetValueFloat(pWheelKv->FindKeyBase("BrakeTorque"), 0, 100.0f);
			wconf.visualTop = KV_GetValueFloat(pWheelKv->FindKeyBase("visualTop"), 0, 0.7);
			wconf.steerMultipler = KV_GetValueFloat(pWheelKv->FindKeyBase("steer"));
		}
	}

	// parse visuals
	kvkeybase_t* visuals = kvs->FindKeyBase("visuals");

	if(visuals)
	{
		conf->visual.sirenType = KV_GetValueInt(visuals->FindKeyBase("siren_lights"), 0, SERVICE_LIGHTS_NONE);
		conf->visual.sirenPositionWidth = KV_GetVector4D(visuals->FindKeyBase("siren_lights"), 1, vec4_zero);

		conf->visual.headlightType = KV_GetValueInt(visuals->FindKeyBase("headlights"), 0, LIGHTS_SINGLE);
		conf->visual.headlightPosition = KV_GetVector4D(visuals->FindKeyBase("headlights"), 1, vec4_zero);

		conf->visual.backlightPosition = KV_GetVector4D(visuals->FindKeyBase("backlights"), 0, vec4_zero);

		conf->visual.brakelightType = KV_GetValueInt(visuals->FindKeyBase("brakelights"), 0, LIGHTS_SINGLE);
		conf->visual.brakelightPosition = KV_GetVector4D(visuals->FindKeyBase("brakelights"), 1, vec4_zero);

		conf->visual.frontDimLights = KV_GetVector4D(visuals->FindKeyBase("frontdimlights"), 0, vec4_zero);
		conf->visual.backDimLights = KV_GetVector4D(visuals->FindKeyBase("backdimlights"), 0, vec4_zero);

		conf->visual.enginePosition = KV_GetVector3D(visuals->FindKeyBase("engine"), 0, vec3_zero);

		conf->visual.exhaustPosition = KV_GetVector3D(visuals->FindKeyBase("exhaust"), 0, vec3_zero);
		conf->visual.exhaustDir = KV_GetValueInt(visuals->FindKeyBase("exhaust"), 3, -1);

		conf->visual.driverPosition = KV_GetVector3D(visuals->FindKeyBase("driver"), 0, vec3_zero);
	}

	kvkeybase_t* colors = kvs->FindKeyBase("colors");

	if(colors)
	{
		conf->numColors = colors->keys.numElem();
		conf->colors = new carColorScheme_t[conf->numColors];

		for(int i = 0; i < colors->keys.numElem(); i++)
		{
			carColorScheme_t& colScheme = conf->colors[i];

			kvkeybase_t* colKey = colors->keys[i];

			if(colKey->IsSection())
			{	// multiple colors
				ColorRGBA c1 = KV_GetVector4D(colKey->FindKeyBase("1"), 0, color4_white);
				ColorRGBA c2 = KV_GetVector4D(colKey->FindKeyBase("2"), 0, colScheme.col1);

				if( c1.w > 1.0f )
					c1 /= 255.0f;

				if( c2.w > 1.0f )
					c2 /= 255.0f;

				colScheme.col1 = c1;
				colScheme.col2 = c2;
			}
			else
			{	// simple colors
				ColorRGBA c1 = KV_GetVector4D(colKey, 0, color4_white);

				if( c1.w > 1.0f )
					c1 /= 255.0f;

				colScheme.col1 = c1;
				colScheme.col2 = colScheme.col1;
			}
		}
	}

	conf->useBodyColor = KV_GetValueBool(kvs->FindKeyBase("useBodyColor"), 0, true) && (conf->numColors > 0);

	kvkeybase_t* pSoundSec = kvs->FindKeyBase("sounds", KV_FLAG_SECTION);

	if(pSoundSec)
	{
		conf->sounds[CAR_SOUND_ENGINE_IDLE] = KV_GetValueString(pSoundSec->FindKeyBase("engine_idle"), 0, "defaultcar.engine_idle");
		conf->sounds[CAR_SOUND_ENGINE_LOW] = KV_GetValueString(pSoundSec->FindKeyBase("engine_low"), 0, "defaultcar.engine_low");
		conf->sounds[CAR_SOUND_ENGINE] = KV_GetValueString(pSoundSec->FindKeyBase("engine"), 0, "defaultcar.engine");
		conf->sounds[CAR_SOUND_HORN] = KV_GetValueString(pSoundSec->FindKeyBase("horn"), 0, "generic.horn1");
		conf->sounds[CAR_SOUND_SIREN] = KV_GetValueString(pSoundSec->FindKeyBase("siren"), 0, "generic.copsiren1");
		conf->sounds[CAR_SOUND_BRAKERELEASE] = KV_GetValueString(pSoundSec->FindKeyBase("brake_release"), 0, "");

		for(int i = 0; i < CAR_SOUND_CONFIGURABLE; i++)
			PrecacheScriptSound( conf->sounds[i].c_str() );
	}

	conf->cameraConf.dist = KV_GetValueFloat(kvs->FindKeyBase("camera_distance"), 0, 7.0f);
	conf->cameraConf.distInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_distance_in"), 0, conf->physics.body_size.z - CAMERA_DISTANCE_BIAS);
	conf->cameraConf.height = KV_GetValueFloat(kvs->FindKeyBase("camera_height"), 0, 1.3f);
	conf->cameraConf.heightInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_height_in"), 0, conf->physics.body_center.y);
	conf->cameraConf.widthInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_side_in"), 0, conf->physics.body_size.x - CAMERA_DISTANCE_BIAS);
	conf->cameraConf.fov = KV_GetValueFloat(kvs->FindKeyBase("camera_fov"), 0, DEFAULT_CAMERA_FOV);

	return true;
}

float vehicleConfig_t::GetMixedBrakeTorque() const
{
	return GetFullBrakeTorque() / (float)physics.numWheels;
}

float vehicleConfig_t::GetFullBrakeTorque() const
{
	float brakeTorque = 0.0f;

	for(int i = 0; i < physics.numWheels; i++)
		brakeTorque += physics.wheels[i].brakeTorque;

	return brakeTorque;
}

float vehicleConfig_t::GetBrakeIneffectiveness() const
{
	return physics.mass / GetFullBrakeTorque();
}

float vehicleConfig_t::GetBrakeEffectPerSecond() const
{
	return GetFullBrakeTorque() / physics.mass;
}

#pragma todo("make better lateral sliding on steering wheels when going backwards")

// wheel friction modifier on diferrent weathers
static float s_weatherTireFrictionMod[WEATHER_COUNT] =
{
	1.0f,
	0.9f,
	0.8f
};

ConVar g_debug_car_lights("g_debug_car_lights", "0");

ConVar r_drawSkidMarks("r_drawSkidMarks", "1", "Draw skidmarks, 1 - player, 2 - all cars", CV_ARCHIVE);
ConVar r_carShadowDetailDistance("r_carShadowDetailDistance", "50.0", "Detailed shadow distance", CV_ARCHIVE);

extern CPFXAtlasGroup* g_vehicleEffects;
extern CPFXAtlasGroup* g_translParticles;
extern CPFXAtlasGroup* g_additPartcles;
extern CPFXAtlasGroup* g_vehicleLights;
extern CPFXAtlasGroup* g_vehicleShadows;

#define PETROL_ENGINE_CURVE				(1.4f)
#define PETROL_MINIMAL_TORQUE_FACTOR	(6.4f)

#define DIESEL_ENGINE_CURVE				(0.8f)
#define DIESEL_MINIMAL_TORQUE_FACTOR	(6.0f)

#define PEAK_TORQUE_FACTOR				(90.0f)

float PetrolEngineTorqueByRPM( float rpm )
{
	float fTorque = ( rpm * ( 0.245f * 0.001f ) );

	fTorque *= fTorque;
	fTorque -= PETROL_ENGINE_CURVE;
	fTorque *= fTorque;

	return ( PETROL_MINIMAL_TORQUE_FACTOR - fTorque ) * PEAK_TORQUE_FACTOR;
}

float DieselEngineTorqueByRPM( float rpm )
{
	float fTorque = ( rpm * ( 0.225f * 0.001f ) );

	fTorque *= fTorque;
	fTorque -= DIESEL_ENGINE_CURVE;
	fTorque *= fTorque;

	return ( DIESEL_MINIMAL_TORQUE_FACTOR - fTorque ) * PEAK_TORQUE_FACTOR;
}

static TORQUECURVEFUNC s_torqueFuncs[] =
{
	PetrolEngineTorqueByRPM,
	DieselEngineTorqueByRPM,
};

static float CalcTorqueCurve( float radsPerSec, int type )
{
	float rpm = radsPerSec * 60.0f / ( 2.0f * PI_F );
	return s_torqueFuncs[type]( rpm );
}

float DBSlipAngleToLateralForce(float fSlipAngle, float fLongitudinalForce, eqPhysSurfParam_t* surfParam, const slipAngleCurveParams_t& params)
{
	const float fSegmentEndAOut = params.fInitialGradient * params.fSegmentEndA;
	const float fSegmentEndBOut = params.fEndGradient * params.fSegmentEndB + params.fEndOffset;

	const float fInvSegBLength = 1.0f / (params.fSegmentEndB - params.fSegmentEndA);
	const float fCubicGradA = params.fInitialGradient * (params.fSegmentEndB - params.fSegmentEndA);
	const float fCubicGradB = params.fEndGradient * (params.fSegmentEndB - params.fSegmentEndA);

	float fSign = sign(fSlipAngle);
	fSlipAngle *= fSign;

	float fResult;
	if (fSlipAngle < params.fSegmentEndA)
	{
		fResult = fSign * params.fInitialGradient * fSlipAngle;
	}
	else if (fSlipAngle < params.fSegmentEndB)
	{
		fResult = fSign * cerp(
			fCubicGradB, fCubicGradA, fSegmentEndBOut, fSegmentEndAOut,
			(fSlipAngle - params.fSegmentEndA) * fInvSegBLength);
	}
	else
	{
		float fValue = params.fEndGradient * fSlipAngle + params.fEndOffset;
		if (fValue < 0.0f)
			fValue = 0.0f;
		fResult = fSign * fValue;
	}

	fResult *= 2.5f;
	fResult /= (fabs(fLongitudinalForce) * 2.5f + 1.0f);

	if(surfParam)
		fResult *= surfParam->tirefriction;
	else
		fResult *= 0.2f;

	return fResult * s_weatherTireFrictionMod[g_pGameWorld->m_envConfig.weatherType];
}

//-------------------------------------------------------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------------------------------------------------------

CCarWheel::CCarWheel()
{
	m_pitch = 0.0f;
	m_pitchVel = 0.0f;

	m_flags.isBurningOut = false;
	m_flags.onGround = true;
	m_flags.lastOnGround = true;
	m_flags.lostHubcap = false;
	m_flags.doSkidmarks = false;
	m_flags.lastDoSkidmarks = false;

	m_smokeTime = 0.0f;
	m_skidTime = 0.0f;
	m_surfparam = NULL;
	m_damage = 0.0f;
	m_hubcapLoose = 0.0f;
	m_defaultBodyGroup = 0;
	m_hubcapBodygroup = -1;
	m_damagedBodygroup = -1;
	m_velocityVec = vec3_zero;
}

void CCarWheel::SetModelPtr(IEqModel* modelPtr)
{
	BaseClass::SetModelPtr( modelPtr );

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init(g_worldGlobals.objectInstFormat, g_worldGlobals.objectInstBuffer );

		m_pModel->SetInstancer( instancer );
	}
}

extern ConVar r_enableObjectsInstancing;

void CCarWheel::Draw( int nRenderFlags )
{
	if(r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_view.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
		{
			if(!(m_bodyGroupFlags & (1 << i)))
				continue;

			gameObjectInstance_t& inst = instancer->NewInstance( i , nLOD );
			inst.world = m_worldMatrix;
		}
	}
	else
		BaseClass::Draw( nRenderFlags );
}

void CCarWheel::CalculateTransform(Matrix4x4& out, const carWheelConfig_t& conf, bool applyScale)
{
	float wheelVisualPosFactor = m_collisionInfo.fract;

	if (wheelVisualPosFactor < conf.visualTop)
		wheelVisualPosFactor = conf.visualTop;

	bool leftWheel = conf.suspensionTop.x < 0.0f;

	Vector3D wheelSuspDir = fastNormalize(conf.suspensionTop - conf.suspensionBottom);
	Vector3D wheelCenterPos = lerp(conf.suspensionTop, conf.suspensionBottom, wheelVisualPosFactor) + wheelSuspDir * conf.radius;

	Quaternion wheelRotation = Quaternion(m_wheelOrient) * Quaternion(-m_pitch, leftWheel ? PI_F : 0.0f, 0.0f);

	wheelRotation = wheelRotation * Quaternion(0, m_damage*WHEEL_VISUAL_DAMAGE_FACTOR_Y, m_damage*WHEEL_VISUAL_DAMAGE_FACTOR_Z);

	if(applyScale)
	{
		Matrix4x4 wheelScale = scale4(conf.width, conf.radius * 2, conf.radius * 2);
		out = translate(Vector3D(wheelCenterPos - Vector3D(conf.woffset, 0, 0))) * Matrix4x4(wheelRotation) * wheelScale;
	}
	else
		out = translate(wheelCenterPos) * Matrix4x4(wheelRotation);
}

BEGIN_NETWORK_TABLE_NO_BASE(carColorScheme_t)
	DEFINE_SENDPROP_VECTOR4D(col1),
	DEFINE_SENDPROP_VECTOR4D(col2)
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE_NO_BASE(PursuerData_t)
	DEFINE_SENDPROP_FLOAT(felonyRating),
	DEFINE_SENDPROP_INT(pursuedByCount)
END_NETWORK_TABLE()

BEGIN_NETWORK_TABLE( CCar )
	DEFINE_SENDPROP_BYTE(m_sirenEnabled),
	DEFINE_SENDPROP_FLOAT(m_maxSpeed),
	DEFINE_SENDPROP_FLOAT(m_torqueScale),
	DEFINE_SENDPROP_FREAL(m_gearboxShiftThreshold),
	DEFINE_SENDPROP_FLOAT(m_gameDamage),
	DEFINE_SENDPROP_FLOAT(m_gameMaxDamage),
	DEFINE_SENDPROP_BYTE(m_lightsEnabled),
	DEFINE_SENDPROP_BYTE(m_locked),
	DEFINE_SENDPROP_BYTE(m_enabled),
	DEFINE_SENDPROP_BYTE(m_autohandbrake),
	DEFINE_SENDPROP_BYTE(m_autogearswitch),
	DEFINE_SENDPROP_BYTE(m_inWater),
	DEFINE_SENDPROP_BYTE(m_hasDriver),
	DEFINE_SENDPROP_EMBEDDED(m_carColor),
	DEFINE_SENDPROP_EMBEDDED(m_pursuerData)
END_NETWORK_TABLE()

CCar::CCar()
{

}

CCar::CCar( vehicleConfig_t* config ) :
	m_physObj(NULL),
	m_trailerHinge(NULL),
	m_fEngineRPM(0.0f),
	m_nPrevGear(-1),
	m_steering(0.0f),
	m_steeringHelper(0.0f),
	m_fAcceleration(0.0f),
	m_fBreakage(0.0f),
	m_fAccelEffect(0.0f),
	m_engineIdleFactor(1.0f),
	m_curTime(0),
	m_sirenEnabled(false),
	m_oldSirenState(false),
	m_isLocalCar(false),
	m_lightsEnabled(false),
	m_nGear(1),
	m_radsPerSec(0),
	m_effectTime(0.0f),
	m_pDamagedModel(NULL),
	m_engineSmokeTime(0.0f),
	m_enablePhysics(true),
	m_gameDamage(0.0f),
	m_gameMaxDamage(CAR_DEFAULT_MAX_DAMAGE),
	m_locked(false),
	m_enabled(true),
	m_inWater(false),
	m_hasDriver(false),
	m_autohandbrake(true),
	m_autogearswitch(true),
	m_torqueScale(1.0f),
	m_maxSpeed(125.0f),
	m_gearboxShiftThreshold(1.0f),
	m_shiftDelay(0.0f),
	m_assignedRoadblock(nullptr)
{
	m_conf = config;
	memset(m_bodyParts, 0,sizeof(m_bodyParts));
	memset(m_sounds, 0,sizeof(m_sounds));
	memset(&m_pursuerData, 0, sizeof(m_pursuerData));

	m_drawFlags |= GO_DRAW_FLAG_SHADOW;

	m_slipParams = &(slipAngleCurveParams_t&)GetDefaultSlipCurveParams();

	m_carColor.col1 = m_carColor.col2 = ColorRGBA(0.9, 0.25, 0.25, 1.0);

	if(m_conf)
	{
		SetColorScheme(0);
		m_maxSpeed = m_conf->physics.maxSpeed;
	}
}

void CCar::DebugReloadCar()
{
	Vector3D vel = GetVelocity();
	Vector3D pos = GetOrigin();
	Quaternion ang = GetOrientation();
	Vector3D angVel = GetAngularVelocity();

	OnRemove();

	Spawn();

	Repair();

	SetVelocity(vel);
	SetOrigin(pos);
	SetOrientation(ang);
	SetAngularVelocity(angVel);
}

void CCar::Precache()
{
	PrecacheScriptSound("tarmac.skid");
	PrecacheScriptSound("tarmac.wetnoise");
	PrecacheScriptSound("defaultcar.engine");
	PrecacheScriptSound("generic.horn1");
	PrecacheScriptSound("generic.copsiren1");

	PrecacheScriptSound("generic.hit");
	PrecacheScriptSound("generic.impact");
	PrecacheScriptSound("generic.crash");
	PrecacheScriptSound("generic.wheelOnGround");
	PrecacheScriptSound("generic.waterHit");
	PrecacheScriptSound("generic.whine");

	PrecacheStudioModel("models/characters/driver_bust.egf");
}

void CCar::CreateCarPhysics()
{
	CEqRigidBody* body = new CEqRigidBody();

	int wheelCacheId = g_studioModelCache->PrecacheModel( "models/vehicles/wheel.egf" );
	IEqModel* wheelModel = g_studioModelCache->GetModel( wheelCacheId );

	m_wheels = new CCarWheel[m_conf->physics.numWheels]();

	for(int i = 0; i < m_conf->physics.numWheels; i++)
	{
		carWheelConfig_t& wconf = m_conf->physics.wheels[i];

		CCarWheel& winfo = m_wheels[i];

		winfo.SetModelPtr( wheelModel );

		winfo.m_defaultBodyGroup = Studio_FindBodyGroupId(wheelModel->GetHWData()->studio, wconf.hubcapWheelName);
		winfo.m_damagedBodygroup = Studio_FindBodyGroupId(wheelModel->GetHWData()->studio, wconf.wheelName);
		winfo.m_hubcapBodygroup = Studio_FindBodyGroupId(wheelModel->GetHWData()->studio, wconf.hubcapName);

		if(winfo.m_defaultBodyGroup == -1)
		{
			winfo.m_defaultBodyGroup = winfo.m_damagedBodygroup;

			// if hubcapped model not available
			winfo.m_flags.lostHubcap = true;
		}

		if(winfo.m_defaultBodyGroup == -1)
		{
			MsgWarning("Wheel submodel (%s) not found!!!\n", wconf.wheelName);
		}

		winfo.m_bodyGroupFlags = (1 << winfo.m_defaultBodyGroup);
	}

	body->m_flags |= BODY_ISCAR;

	// CREATE BODY
	Vector3D body_mins = m_conf->physics.body_center - m_conf->physics.body_size;
	Vector3D body_maxs = m_conf->physics.body_center + m_conf->physics.body_size;


	if(m_pModel->GetHWData()->physModel.numObjects)
	{
		body->Initialize(&m_pModel->GetHWData()->physModel, 0);
	}
	else
		body->Initialize(body_mins, body_maxs);

	// CREATE BODY
	body->m_aabb.minPoint = m_conf->physics.body_center - m_conf->physics.body_size;
	body->m_aabb.maxPoint = m_conf->physics.body_center + m_conf->physics.body_size;

	// TODO: custom friction and restitution
	body->SetFriction( CAR_PHYSICS_FRICTION );
	body->SetRestitution( CAR_PHYSICS_RESTITUTION );

	body->SetMass( m_conf->physics.mass, m_conf->physics.inertiaScale);
	body->SetCenterOfMass( m_conf->physics.virtualMassCenter );

	// don't forget about contents!
	body->SetContents( OBJECTCONTENTS_VEHICLE );
	body->SetCollideMask( COLLIDEMASK_VEHICLE );

	body->SetUserData(this);
	body->SetDebugName(m_conf->carName.c_str());

	body->SetPosition(m_vecOrigin);
	body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

	m_physObj = new CPhysicsHFObject( body, this );
	g_pPhysics->AddObject( m_physObj );

	UpdateVehiclePhysics(0.0f);
}

ISoundController* CCar::CreateCarSound(const char* name, float radiusMult)
{
	EmitSound_t soundEp;

	soundEp.name = name;
	soundEp.fPitch = 1.0f;
	soundEp.fVolume = 1.0f;
	soundEp.fRadiusMultiplier = radiusMult;
	soundEp.origin = m_vecOrigin;
	soundEp.pObject = this;
	soundEp.nFlags |= EMITSOUND_FLAG_STARTSILENT;

	return g_sounds->CreateSoundController(&soundEp);
}

void CCar::InitCarSound()
{
	m_sounds[CAR_SOUND_SKID] = CreateCarSound("tarmac.skid", 0.55f);
	m_sounds[CAR_SOUND_SURFACE] = CreateCarSound("tarmac.wetnoise", 0.55f);

	//
	// Don't initialize sounds if this is not a car
	//
	if(!m_conf->flags.isCar)
		return;

	if(m_conf->flags.reverseWhine)
		m_sounds[CAR_SOUND_WHINE] = CreateCarSound("generic.whine", 0.45f);

	m_sounds[CAR_SOUND_ENGINE_IDLE] = CreateCarSound(m_conf->sounds[CAR_SOUND_ENGINE_IDLE].c_str(), 0.45f);
	m_sounds[CAR_SOUND_ENGINE] = CreateCarSound(m_conf->sounds[CAR_SOUND_ENGINE].c_str(), 0.45f);
	m_sounds[CAR_SOUND_ENGINE_LOW] = CreateCarSound(m_conf->sounds[CAR_SOUND_ENGINE_LOW].c_str(), 0.45f);

	m_sounds[CAR_SOUND_HORN] = CreateCarSound(m_conf->sounds[CAR_SOUND_HORN].c_str(), 1.0f);

	if(m_conf->sounds[CAR_SOUND_BRAKERELEASE].Length() > 0)
		m_sounds[CAR_SOUND_BRAKERELEASE] = CreateCarSound(m_conf->sounds[CAR_SOUND_BRAKERELEASE].c_str(), 1.0f);

	if(m_conf->visual.sirenType != SERVICE_LIGHTS_NONE)
	{
		EmitSound_t siren_ep;

		siren_ep.name = m_conf->sounds[CAR_SOUND_SIREN].c_str();

		siren_ep.fPitch = 1.0f;
		siren_ep.fVolume = 1.0f;

		siren_ep.fRadiusMultiplier = 1.0f;
		siren_ep.origin = m_vecOrigin;
		siren_ep.sampleId = 0;
		siren_ep.pObject = this;

		m_sounds[CAR_SOUND_SIREN] = g_sounds->CreateSoundController(&siren_ep);
	}
}

void CCar::Spawn()
{
	// init default vehicle
	// load visuals
	SetModel( m_conf->visual.cleanModelName.c_str() );
	/*
	// create instancer for lod models only
	if(!(nRenderFlags & RFLAG_NOINSTANCE) && g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_vehicleInstVertexFormat, g_pGameWorld->m_vehicleInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}*/

	int nDamModelIndex = g_studioModelCache->PrecacheModel( m_conf->visual.damModelName.c_str() );

	if(nDamModelIndex != -1)
		m_pDamagedModel = g_studioModelCache->GetModel( nDamModelIndex );
	else
		m_pDamagedModel = NULL;

	// Init car body parts
	for(int i = 0; i < CB_PART_WINDOW_PARTS; i++)
	{
		m_bodyParts[i].boneIndex = Studio_FindBoneId(m_pModel->GetHWData()->studio, s_pBodyPartsNames[i]);

		//if(m_bodyParts[i].boneIndex == -1)
		//	MsgError("Error in model '%s' - can't find bone '%s' which is required\n", m_pModel->GetHWData()->studio->modelName, s_pBodyPartsNames[i]);
	}

	CreateCarPhysics();

	InitCarSound();

	// baseclass spawn
	CGameObject::Spawn();

#ifndef EDITOR
	if(m_replayID == REPLAY_NOT_TRACKED)
		g_replayData->PushSpawnOrRemoveEvent( REPLAY_EVENT_SPAWN, this, (m_state == GO_STATE_REMOVE_BY_SCRIPT) ? REPLAY_FLAG_SCRIPT_CALL : 0  );
#endif // EDITOR

	UpdateLightsState();

	// init driver model and instancer for it
	m_driverModel.SetModel("models/characters/driver_bust.egf");

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_driverModel.GetModel() && m_driverModel.GetModel()->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init(g_worldGlobals.objectInstFormat, g_worldGlobals.objectInstBuffer );

		m_driverModel.GetModel()->SetInstancer( instancer );
	}

	// spawn cars with enabled headlights

}

void CCar::AlignToGround()
{
	IVector2D hfieldCell;
	CLevelRegion* reg = NULL;
	if( !g_pGameWorld->m_level.GetRegionAndTile( m_vecOrigin, &reg, hfieldCell ) )
		return;

	Vector3D pos = reg->CellToPosition(hfieldCell.x, hfieldCell.y);

	/*
	Vector3D t,b,n;
	reg->GetHField()->GetTileTBN(hfieldCell.x, hfieldCell.y, t,b,n);
	Matrix3x3 cellAngle(b,n,t);
	Matrix3x3 finalAngle = !cellAngle * Matrix3x3( GetOrientation() );
	if(m_physObj)
		SetOrientation( Quaternion( transpose(finalAngle) ) );
	*/

	float suspLowPos = lerp(m_conf->physics.wheels[0].suspensionTop.y, m_conf->physics.wheels[0].suspensionBottom.y, 0.85f);

	pos = Vector3D(m_vecOrigin.x, pos.y - suspLowPos, m_vecOrigin.z);

	SetOrigin( pos );
}

void CCar::OnRemove()
{
	CGameObject::OnRemove();

	ReleaseHingedVehicle();

#ifndef EDITOR
	if(m_replayID != REPLAY_NOT_TRACKED)
		g_replayData->PushSpawnOrRemoveEvent( REPLAY_EVENT_REMOVE, this, (m_state == GO_STATE_REMOVE_BY_SCRIPT) ? REPLAY_FLAG_SCRIPT_CALL : 0 );
#endif // EDITOR

	for(int i = 0; i < CAR_SOUND_COUNT; i++)
	{
		g_sounds->RemoveSoundController(m_sounds[i]);
		m_sounds[i] = nullptr;
	}
	
	delete [] m_wheels;
	m_wheels = NULL;

	g_pPhysics->RemoveObject(m_physObj);
	m_physObj = nullptr;

	if (m_assignedRoadblock)
	{
		m_assignedRoadblock->activeCars.fastRemove(this);
		m_assignedRoadblock = nullptr;
	}

}

void CCar::PlaceOnRoadCell(CLevelRegion* reg, levroadcell_t* cell)
{
	if (!reg || !cell)
		return;

	Vector3D t,b,n;
	reg->GetHField()->GetTileTBN(cell->posX, cell->posY, t,b,n);
	Vector3D pos = reg->CellToPosition(cell->posX, cell->posY);

	float roadCellAngle = cell->direction*-90.0f + 180.0f;

	Matrix3x3 cellAngle(b,n,t);
	Matrix3x3 finalAngle = !cellAngle*rotateY3(DEG2RAD(roadCellAngle) );

	SetOrigin(pos - Vector3D(0.0f, m_conf->physics.wheels[0].suspensionBottom.y, 0.0f));

	Quaternion rotation( finalAngle );
	renormalize(rotation);
	m_physObj->GetBody()->SetOrientation(rotation);
}

void CCar::SetOrigin(const Vector3D& origin)
{
	if(m_physObj)
		m_physObj->GetBody()->SetPosition(origin);

	m_vecOrigin = origin;
}

void CCar::SetAngles(const Vector3D& angles)
{
	if(m_physObj)
		m_physObj->GetBody()->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CCar::SetOrientation(const Quaternion& q)
{
	m_physObj->GetBody()->SetOrientation(q);
}

void CCar::SetVelocity(const Vector3D& vel)
{
	m_physObj->GetBody()->SetLinearVelocity( vel  );
	m_physObj->GetBody()->TryWake();
}

void CCar::SetAngularVelocity(const Vector3D& vel)
{
	m_physObj->GetBody()->SetAngularVelocity( vel );
	m_physObj->GetBody()->TryWake();
}

ConVar cam_custom("cam_custom", "0", NULL, CV_CHEAT);
ConVar cam_custom_height("cam_custom_height", "1.3", NULL, CV_ARCHIVE);
ConVar cam_custom_dist("cam_custom_dist", "7", NULL, CV_ARCHIVE);
ConVar cam_custom_fov("cam_custom_fov", "52", NULL, CV_ARCHIVE);

void CCar::ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const
{
	CCar* hingedVehicle = GetHingedVehicle();

	filter.AddObject(GetPhysicsBody());

	if (hingedVehicle)
	{
		conf = m_conf->cameraConf;
		cameraConfig_t& hingeConf = hingedVehicle->m_conf->cameraConf;

		conf.dist += hingeConf.dist;
		conf.height += hingeConf.height;

		// add hinged vehicle to collision filter
		filter.AddObject(hingedVehicle->GetPhysicsBody());
	}
	else
	{
		conf = m_conf->cameraConf;

		if (cam_custom.GetBool())
		{
			conf.dist = cam_custom_dist.GetFloat();
			conf.height = cam_custom_height.GetFloat();
			conf.fov = cam_custom_fov.GetFloat();
		}
	}
}

const Quaternion& CCar::GetOrientation() const
{
	if(!m_physObj)
	{
		static Quaternion _zeroQuaternion;
		return _zeroQuaternion;
	}

	return m_physObj->GetBody()->GetOrientation();
}

const Vector3D CCar::GetForwardVector() const
{
	return rotateVector(vec3_forward,GetOrientation());
}

const Vector3D CCar::GetUpVector() const
{
	return rotateVector(vec3_up,GetOrientation());
}

const Vector3D CCar::GetRightVector() const
{
	return rotateVector(vec3_right,GetOrientation());
}

const Vector3D& CCar::GetVelocity() const
{
	return m_physObj->GetBody()->GetLinearVelocity();
}

const Vector3D& CCar::GetAngularVelocity() const
{
	return m_physObj->GetBody()->GetAngularVelocity();
}

CEqRigidBody* CCar::GetPhysicsBody() const
{
	if(!m_physObj)
		return NULL;

	return m_physObj->GetBody();
}

void CCar::L_SetContents(int contents)
{
	if (!m_physObj)
		return;

	m_physObj->GetBody()->SetContents(contents);
}

void CCar::L_SetCollideMask(int contents)
{
	if (!m_physObj)
		return;

	m_physObj->GetBody()->SetCollideMask(contents);
}

int	CCar::L_GetContents() const
{
	if (!m_physObj)
		return 0;

	return m_physObj->GetBody()->GetContents();
}

int	CCar::L_GetCollideMask() const
{
	if (!m_physObj)
		return 0;

	return m_physObj->GetBody()->GetCollideMask();
}

void CCar::SetControlButtons(int flags)
{
	CControllableGameObject::SetControlButtons(flags);

	// make car automatically has driver
	if(IsAlive() && IsEnabled() && m_conf->flags.isCar)
		m_hasDriver = true;
}

void CCar::AnalogSetControls(float accel_brake, float steering, bool extendSteer, bool handbrake, bool burnout)
{
#define CONTROL_TOLERANCE	(0.05f)

	int buttons = IN_ANALOGSTEER | (extendSteer ? IN_EXTENDTURN : 0) | (burnout ? IN_BURNOUT : 0) | (handbrake ? IN_HANDBRAKE : 0);

	float accelRatio = accel_brake;
	float brakeRatio = -accel_brake;

	if(accelRatio < 0.0f) accelRatio = 0.0f;
	if(brakeRatio < 0.0f) brakeRatio = 0.0f;

	if(accelRatio > CONTROL_TOLERANCE)
		buttons |= IN_ACCELERATE;

	if(brakeRatio > CONTROL_TOLERANCE)
		buttons |= IN_BRAKE;

	SetControlVars(accelRatio, brakeRatio, steering);
	SetControlButtons(buttons);
}

void CCar::HingeVehicle(int thisHingePoint, CCar* otherVehicle, int otherHingePoint)
{
#ifndef EDITOR
	// get the global positions of hinge points and check

	// don't hinge if too far (because it messes physics somehow)
	// if(dist < HINGE_INIT_DISTANCE_THRESH)
	//	return;

	if(m_trailerHinge)
		ReleaseHingedVehicle();

	otherVehicle->m_isLocalCar = m_isLocalCar;

	otherVehicle->SetAngles(GetAngles());
	otherVehicle->UpdateTransform();

	Vector3D offsetA = transpose(!m_worldMatrix.getRotationComponent()) * m_conf->physics.hingePoints[thisHingePoint];
	Vector3D offsetB = transpose(!otherVehicle->m_worldMatrix.getRotationComponent()) * otherVehicle->m_conf->physics.hingePoints[otherHingePoint];

	// teleport the hinging object
	otherVehicle->SetOrigin(GetOrigin() + offsetA - offsetB);

	// FIXME: rotate the hinged vehicle same way

	m_trailerHinge = new CEqPhysicsHingeJoint();
	m_trailerHinge->Init(GetPhysicsBody(), otherVehicle->GetPhysicsBody(), vec3_up,
											m_conf->physics.hingePoints[thisHingePoint],
											0.2f, DEG2RAD(65.0f), DEG2RAD(65.0f), DEG2RAD(90), 0.005f);

	m_trailerHinge->SetEnabled(true);

	g_pPhysics->m_physics.AddController( m_trailerHinge );
#endif // EDITOR
}

void CCar::ReleaseHingedVehicle()
{
#ifndef EDITOR
	if(m_trailerHinge)
	{
		CCar* hingedCar = GetHingedVehicle();

		if(hingedCar)	// kill lights
			hingedCar->m_lightsEnabled = 0;

		g_pPhysics->m_physics.DestroyController(m_trailerHinge);
	}

	m_trailerHinge = nullptr;
#endif // EDITOR
}

CCar* CCar::GetHingedVehicle() const
{
#ifndef EDITOR
	if(!m_trailerHinge)
		return NULL;

	CEqRigidBody* body = m_trailerHinge->GetBodyB();
	if(body)
		return (CCar*)body->GetUserData();

#endif // EDITOR
	return NULL;
}

CEqRigidBody* CCar::GetHingedBody() const
{
#ifndef EDITOR
	if(m_trailerHinge)
		m_trailerHinge->GetBodyB();
#endif // EDITOR

	return NULL;
}

int	CCar::GetControlButtons() const
{
	if (m_locked) // TODO: cvar option to lock or not
		return IN_HANDBRAKE;

	return CControllableGameObject::GetControlButtons();
}

void CCar::UpdateVehiclePhysics(float delta)
{
	PROFILE_FUNC();

	CEqRigidBody* carBody = m_physObj->GetBody();

	// retrieve body matrix
	Matrix4x4 worldMatrix;
	carBody->ConstructRenderMatrix(worldMatrix);
	m_worldMatrix = worldMatrix;

	int controlButtons = GetControlButtons();

	bool isCar = m_conf->flags.isCar;

	//
	// Update controls first
	//

	FReal fSteerAngle = 0;
	FReal fAccel = 0;
	FReal fBrake = 0;
	FReal fHandbrake = 0;

	bool bBurnout = false;
	bool bExtendTurn = false;

	bool needsWake = false;

	if( m_enabled )
	{
		if( isCar )
		{
			if(controlButtons & IN_ACCELERATE)
				fAccel = (float)((float)m_accelRatio*_oneBy1024);

			if(controlButtons & IN_BURNOUT )
				bBurnout = true;

			if(controlButtons & IN_HANDBRAKE )
				fHandbrake = 1;
		}

		// non-car vehicles still can brake
		if(controlButtons & IN_BRAKE )
			fBrake = (float)((float)m_brakeRatio*_oneBy1024);
	}
	else
		fHandbrake = 1;

	int controlsChanged = controlButtons & m_oldControlButtons;

	if (controlButtons != m_oldControlButtons && !((controlsChanged & IN_HORN) || (controlsChanged & IN_SIREN)))
		needsWake = true;

	if(controlButtons & IN_EXTENDTURN )
		bExtendTurn = true;

	if(controlButtons & IN_TURNLEFT )
		fSteerAngle -= (float)((float)m_steerRatio*_oneBy1024);
	else if(controlButtons & IN_TURNRIGHT )
		fSteerAngle += (float)((float)m_steerRatio*_oneBy1024);
	else if(controlButtons & IN_ANALOGSTEER)
	{
		fSteerAngle = (float)((float)m_steerRatio*_oneBy1024);
		m_steering = fSteerAngle;
	}

	//------------------------------------------------------------------------------------------

	// do acceleration and burnout
	if(bBurnout)
		fAccel = 1.0f;

	bool bDoBurnout = bBurnout && (GetSpeed() < m_conf->physics.burnoutMaxSpeed);

	if(fAccel > m_fAcceleration)
	{
		if(m_nGear > 1)
			m_fAcceleration += delta * ACCELERATION_CONST;
		else
			m_fAcceleration = fAccel;
	}
	else
		m_fAcceleration -= delta * ACCELERATION_CONST;

	if(m_fAcceleration < 0)
		m_fAcceleration = 0;

	//--------------------------------------------------------

	float steerSpeedMultiplier = 1.0f;

	{
		FReal steer_diff = fSteerAngle-m_steering;

		float speed = GetSpeed();

		steerSpeedMultiplier = RemapValClamp(STEERING_REDUCE_SPEED_MAX-speed, STEERING_REDUCE_SPEED_MIN, STEERING_REDUCE_SPEED_MAX, STEERING_SPEED_REDUCE_FACTOR, 1.0f);
		steerSpeedMultiplier = powf(steerSpeedMultiplier, STEERING_SPEED_REDUCE_CURVE);

		if (bExtendTurn)
			steerSpeedMultiplier *= EXTEND_STEER_SPEED_MULTIPLIER;

		if(FPmath::abs(steer_diff) > 0.01f)
			m_steering += FPmath::sign(steer_diff) * m_conf->physics.steeringSpeed * steerSpeedMultiplier * delta;
		else
			m_steering = fSteerAngle;
	}

	if (FPmath::abs(fSteerAngle) > STEERING_HELP_START)
	{
		FReal steerHelp_diff = fSteerAngle -m_steeringHelper;
		if(FPmath::abs(steerHelp_diff) > 0.1f)
			m_steeringHelper += FPmath::sign(steerHelp_diff) * STEERING_HELP_CONST * steerSpeedMultiplier * delta;
		else
			m_steeringHelper = fSteerAngle;
	}

	if (fsimilar(fSteerAngle, 0.0f, 0.1f))
		m_steeringHelper = 0;

	const FReal MAX_STEERING_ANGLE = 1.0f;

	FReal steer_clamp(0.6f);

	if(bExtendTurn)
		steer_clamp = MAX_STEERING_ANGLE;

	m_steering = clamp(m_steering, -steer_clamp, steer_clamp);

	// kick in wake
	if (needsWake)
		carBody->TryWake(false);

	int wheelCount = GetWheelCount();

	for (int i = 0; i < wheelCount; i++)
	{
		CCarWheel& wheel = m_wheels[i];
		carWheelConfig_t& wheelConf = m_conf->physics.wheels[i];

		float camberAngle = sign(wheelConf.suspensionTop.x) * ((wheel.m_collisionInfo.fract - 0.75f) * wheelConf.camber);

		wheel.m_wheelOrient = rotateZ3(DEG2RAD(camberAngle));

		if (wheelConf.flags & WHEEL_FLAG_STEER)
		{
			float fWheelSteerAngle = DEG2RAD(m_steering*40) * wheelConf.steerMultipler;
			wheel.m_wheelOrient = rotateY3(fWheelSteerAngle) * wheel.m_wheelOrient;
		}
	}

	if (carBody->IsFrozen() && !bBurnout)
		return;

	int numDriveWheels = 0;
	int numHandbrakeWheelsOnGround = 0;
	int numDriveWheelsOnGround = 0;
	int numWheelsOnGround = 0;

	float wheelsSpeed = 0.0f;

	// final: trace new straight for blocking cars and deny if have any
	CollisionData_t coll_line;

	eqPhysCollisionFilter collFilter;
	collFilter.flags = EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC;
	collFilter.AddObject(carBody);

	

	// Raycast the wheels and do some simple calculations
	for(int i = 0; i < wheelCount; i++)
	{
		CCarWheel& wheel = m_wheels[i];
		carWheelConfig_t& wheelConf = m_conf->physics.wheels[i];

		float suspensionLength = length(wheelConf.suspensionTop-wheelConf.suspensionBottom);

		// transform suspension trace line by car origin
		Vector3D line_start = (worldMatrix*Vector4D(wheelConf.suspensionTop, 1)).xyz();
		Vector3D line_end = (worldMatrix*Vector4D(wheelConf.suspensionBottom, 1)).xyz();

		float fractionOld = wheel.m_collisionInfo.fract;
		float fractionOldDist = suspensionLength * fractionOld;

		// trace solid ground only
		g_pPhysics->TestLine(line_start, line_end, wheel.m_collisionInfo, OBJECTCONTENTS_SOLID_GROUND, &collFilter);

		float fractionNewDist = suspensionLength * wheel.m_collisionInfo.fract;

		// play wheel hit sound if wheel suddenly goes up by HFIELD_HEIGHT_STEP
		if(m_isLocalCar && fractionOld < 1.0f && (fractionOldDist - fractionNewDist) >= HFIELD_HEIGHT_STEP)
		{
			// add damage to hubcap
			wheel.m_hubcapLoose += (fractionOldDist - fractionNewDist) * 0.01f * length(wheel.m_velocityVec);

			EmitSound_t ep;

			ep.name = "generic.wheelOnGround";

			ep.origin = GetOrigin();
			EmitSoundWithParams(&ep);
		}

		if(wheelConf.flags & WHEEL_FLAG_DRIVE)
		{
			numDriveWheels++;
			wheelsSpeed += wheel.m_pitchVel;
		}

		if(wheel.m_collisionInfo.fract < 1.0f)
		{
			numWheelsOnGround++;

			if(wheelConf.flags & WHEEL_FLAG_DRIVE)
				numDriveWheelsOnGround++;

			if(wheelConf.flags & WHEEL_FLAG_HANDBRAKE)
				numHandbrakeWheelsOnGround++;

			// find surface parameter
			wheel.m_surfparam = g_pPhysics->GetSurfaceParamByID( wheel.m_collisionInfo.materialIndex );
		}

		// update effects
		UpdateWheelEffect(i, delta);
	}

	float wheelMod = 1.0f / numDriveWheels; //(numDriveWheelsOnGround > 0) ? (1.0f / (float)numDriveWheelsOnGround) : 1.0f;
	float driveGroundWheelMod = (numDriveWheelsOnGround > 0) ? (1.0f / (float)numDriveWheelsOnGround) : 1.0f;

	wheelsSpeed *= wheelMod;
	
	// disable damping when the handbrake is on
	if(numHandbrakeWheelsOnGround > 0 && fHandbrake > 0.0f)
		carBody->m_flags |= BODY_DISABLE_DAMPING;
	else
		carBody->m_flags &= ~BODY_DISABLE_DAMPING;

	// Sound effect update
	{
		//
		// Calculate fake engine load effect with clutch
		//
		float accelEffectFactor = m_fAcceleration+fBrake * -sign(wheelsSpeed);
		float accelEffect = m_fAccelEffect;

		accelEffect += (accelEffectFactor -m_fAccelEffect) * delta * ACCELERATION_SOUND_CONST;
		accelEffect = clamp(accelEffect, -1.0f, 1.0f);

		float fRPM = GetRPM();

		const float CLUTCH_GRIP_MIN_SPEED = 3.7f;
		const float CLUTCH_GRIP_RPM = 3500.0f;
		const float CLUTCH_GRIP_RPM_LOAD = 1100.0f;

		if(bDoBurnout)
		{
			m_engineIdleFactor = 0.0f;
			fRPM = 5800.0f;
		}
		else
		{
			float absWheelsSpeed = fabs(wheelsSpeed);
			if(absWheelsSpeed < CLUTCH_GRIP_MIN_SPEED && accelEffect > 0.0f)
			{
				float optimalRpmLoad = RemapValClamp(accelEffect*absWheelsSpeed, 0.0f, 1.25f, 0.0f, 1.0f);
				fRPM = lerp(fRPM, CLUTCH_GRIP_RPM, optimalRpmLoad) - CLUTCH_GRIP_RPM_LOAD * accelEffect * RemapValClamp(absWheelsSpeed,0.0f, CLUTCH_GRIP_MIN_SPEED, 0.0f, 1.0f);
			}
		}

		if (numDriveWheelsOnGround == 0)
			fRPM = 8500.0f * accelEffect;

		m_fAccelEffect = accelEffect;

		//
		// make RPM changes smooth
		//
		#define RPM_REFRESH_TIMES 16

		float fDeltaRpm = delta / RPM_REFRESH_TIMES;

		float engineRPM = m_fEngineRPM;

		for(int i = 0; i < RPM_REFRESH_TIMES; i++)
		{
			if(fabs(fRPM - engineRPM) > 0.02f)
				engineRPM += sign(fRPM - engineRPM) * 10000.0f * fDeltaRpm;
		}

		m_fEngineRPM = clamp(engineRPM, 0.0f, 10000.0f);
	}

	//-------------------------------------------------------

	//
	// Some steering stuff from Driver 1
	//

	float autoHandbrakeHelper = 0.0f;

	if( m_autohandbrake && GetSpeedWheels() > 0.0f && !bDoBurnout )
	{
		const float AUTOHANDBRAKE_MIN_SPEED		= 8.0f;
		const float AUTOHANDBRAKE_MAX_SPEED		= 40.0f;
		const float AUTOHANDBRAKE_MAX_FACTOR	= 5.0f;
		const float AUTOHANDBRAKE_SCALE			= 2.0f;
		const float AUTOHANDBRAKE_START_MIN		= 0.1f;

		float carForwardSpeed = dot(GetForwardVector().xz(), GetVelocity().xz()) * MPS_TO_KPH;

		float handbrakeFactor = RemapVal(carForwardSpeed, AUTOHANDBRAKE_MIN_SPEED, AUTOHANDBRAKE_MAX_SPEED, 0.0f, 1.0f);

		float fLateral = GetLateralSlidingAtBody();
		float fLateralSign = sign(GetLateralSlidingAtBody())*-0.01f;

		if( fLateralSign > 0 && (m_steeringHelper > fLateralSign)
			|| fLateralSign < 0 && (m_steeringHelper < fLateralSign) || fabs(fLateral) < 5.0f)
		{
			if(fabs(m_steeringHelper) > AUTOHANDBRAKE_START_MIN)
				autoHandbrakeHelper = (FPmath::abs(m_steeringHelper)-AUTOHANDBRAKE_START_MIN)*AUTOHANDBRAKE_SCALE * handbrakeFactor;
		}

		autoHandbrakeHelper = clamp(autoHandbrakeHelper,0.0f,AUTOHANDBRAKE_MAX_FACTOR);
	}

	float fAcceleration = m_fAcceleration;
	float fBreakage = fBrake;

	float torque = 0.0f;

	//
	// Update gearbox
	//
	if( isCar )
	{
		int engineType = m_conf->physics.engineType;

		float differentialRatio = m_conf->physics.differentialRatio;
		float transmissionRate = m_conf->physics.transmissionRate;
		float torqueMultiplier = m_conf->physics.torqueMult;
		
		if(bDoBurnout)
		{
			if(m_nGear == 0)
				m_nGear = 1;
		}
		else if(m_autogearswitch)
		{
			// neutral zone
			// check for backwards gear
			if(fsimilar(wheelsSpeed, 0.0f, 0.1f))
			{
				if(fBrake > 0)
					m_nGear = 0;
				else if(fAccel > 0)
					m_nGear = 1;
			}
			else // switch gears quickly if we move in wrong direction
			{
				if(wheelsSpeed < 0.0f && m_nGear > 0)
					m_nGear = 0;
				else if(wheelsSpeed > 0.0f && m_nGear == 0)
					m_nGear = 1;
			}
		}

		// setup current gear
		float torqueConvert = differentialRatio * m_conf->physics.gears[m_nGear];
		
		torque = CalcTorqueCurve(m_radsPerSec, engineType) * torqueMultiplier;
		torque *= torqueConvert * transmissionRate;

		m_radsPerSec = wheelsSpeed * torqueConvert;

		float gbxDecelRate = max(float(fAccel), GEARBOX_DECEL_SHIFTDOWN_FACTOR);

		if(m_nGear == 0)
		{
			// swap brake and acceleration
			swap(fAcceleration, fBreakage);
			fBreakage *= -1.0f;
		}
		else
		{
			//
			// update forward automatic transmission
			//

			int newGear = m_nGear;

			for( int gear = 1; gear < m_conf->physics.numGears; gear++ )
			{
				// find gear to diffential
				torqueConvert = differentialRatio * m_conf->physics.gears[gear];
				float gearRadsPerSecond = wheelsSpeed * torqueConvert;

				float gearTorque = CalcTorqueCurve(gearRadsPerSecond, engineType) * torqueMultiplier;
				gearTorque *= torqueConvert * transmissionRate;

				bool shouldShift =	(gear < m_nGear) ? 
									(gearTorque*gbxDecelRate*m_gearboxShiftThreshold > torque) :	// shift down logic
									(gearTorque > torque*m_gearboxShiftThreshold);					// shift up logic

				if(!shouldShift)
					continue;

				// setup new gear
				newGear = gear;
				torque = gearTorque;

				m_radsPerSec = gearRadsPerSecond;
			}

			float shiftDelay = m_shiftDelay;

			shiftDelay -= delta;
			shiftDelay = max(shiftDelay, 0.0f);
			
			// if shifted up, reduce gas since we pressed clutch
			if(	!bDoBurnout && newGear > m_nPrevGear && m_fAcceleration >= 0.9f && numDriveWheelsOnGround && shiftDelay <= 0.0f)
			{
				shiftDelay = GEARBOX_SHIFT_DELAY;

				// lol, how I can calculate it like that?
				float engineLoadFactor = fabs(dot(vec3_up, GetUpVector()));
				float shiftAccelFactor = engineLoadFactor - m_conf->physics.shiftAccelFactor;
				m_fAcceleration -= max(shiftAccelFactor, 0.0f);
			}

			m_shiftDelay = shiftDelay;
			m_nPrevGear = m_nGear;
			m_nGear = newGear;
		}

		SetLight(CAR_LIGHT_REVERSELIGHT, (m_nGear == 0) && wheelsSpeed < -0.25f);
	}

	if(FPmath::abs(fBreakage) > m_fBreakage)
	{
		m_fBreakage += delta * ACCELERATION_CONST;
	}
	else
	{
		if(m_sounds[CAR_SOUND_BRAKERELEASE] && m_fBreakage > 0.4f && fabs(fBreakage) < 0.01f)
		{
			// m_fBreakage is volume
			EmitSound_t ep;
			ep.name = m_conf->sounds[CAR_SOUND_BRAKERELEASE].c_str();
			ep.fVolume = m_fBreakage;
			ep.fPitch = 1.0f+(1.0f-m_fBreakage);
			ep.origin = GetOrigin();

			EmitSoundWithParams(&ep);
			m_fBreakage = 0.0f;
		}

		m_fBreakage -= delta * ACCELERATION_CONST;
	}

	bool brakeLightsActive = !bDoBurnout && FPmath::abs(fBreakage) > 0.0f;
	SetLight(CAR_LIGHT_BRAKE, brakeLightsActive);

	if(m_fBreakage < 0)
		m_fBreakage = 0;

	if(fHandbrake > 0)
		fAcceleration = 0.0f;

	float fAccelerator = fAcceleration * torque * m_torqueScale;
	float fBraker = fBreakage * pow(m_torqueScale, 0.5f);

	float acceleratorAbs = fabs(fAccelerator);

	// Limit the speed
	if(GetSpeed() > m_maxSpeed)
		fAccelerator = 0.0f;

	Matrix3x3 transposedRotation(transpose(worldMatrix.getRotationComponent()));

	// springs for wheels
	for(int i = 0; i < wheelCount; i++)
	{
		CCarWheel& wheel = m_wheels[i];
		carWheelConfig_t& wheelConf = m_conf->physics.wheels[i];

		bool isDriveWheel = (wheelConf.flags & WHEEL_FLAG_DRIVE) > 0;
		bool isSteerWheel = (wheelConf.flags & WHEEL_FLAG_STEER) > 0;
		bool isHandbrakeWheel = (wheelConf.flags & WHEEL_FLAG_HANDBRAKE) > 0;

		// apply car rays
		Vector3D line_start = (worldMatrix*Vector4D(wheelConf.suspensionTop, 1)).xyz();
		Vector3D line_end = (worldMatrix*Vector4D(wheelConf.suspensionBottom, 1)).xyz();

		// compute wheel world rotation (turn + body rotation)
		Matrix3x3 wrotation = wheel.m_wheelOrient*transposedRotation;

		// get wheel vectors
		Vector3D wheel_right = wrotation.rows[0];
		Vector3D wheel_forward = wrotation.rows[2];

		float fPitchFac = RemapVal(m_conf->physics.burnoutMaxSpeed - GetSpeed(), 0.0f, 70.0f, 0.0f, 1.0f);

		if(wheel.m_collisionInfo.fract < 1.0f)
		{
			// calculate wheel traction friction
			float wheelTractionFrictionScale = 0.2f;

			if(wheel.m_surfparam)
				wheelTractionFrictionScale = wheel.m_surfparam->tirefriction;

			wheelTractionFrictionScale *= s_weatherTireFrictionMod[g_pGameWorld->m_envConfig.weatherType];

			// recalculate wheel velocity
			Vector3D wheelVelAtPoint = carBody->GetVelocityAtWorldPoint(wheel.m_collisionInfo.position);
			wheel.m_velocityVec = wheelVelAtPoint;

			//
			// Apply waving for dirt
			//
			if(wheel.m_surfparam && wheel.m_surfparam->word == 'G') // wheel on grass
			{
				float fac1 = wheel.m_collisionInfo.position.x*0.65f;
				float fac2 = wheel.m_collisionInfo.position.z*0.65f;

				wheel.m_collisionInfo.fract += ((0.5f-sin(fac1))+(0.5f-sin(fac2+0.5f)))*wheelConf.radius*0.11f;
			}

			//
			// Calculate spring force
			//
			float springPowerFac = 1.0f-wheel.m_collisionInfo.fract;

			float springPower = springPowerFac*wheelConf.springConst;
			float dampingFactor = dot(wheel.m_collisionInfo.normal, wheelVelAtPoint);

			if ( fabs( dampingFactor ) > 1.0f )
				dampingFactor = sign( dampingFactor ) * 1.0f;

			// apply spring damping now
			springPower -= wheelConf.dampingConst*dampingFactor;

			if(springPower <= 0.0f)
				continue;

			Vector3D springForce = wheel.m_collisionInfo.normal*springPower;

			//
			// processing slip
			//
			Vector3D wheelForward = wheel_forward;

			// apply velocity
			wheelForward -= wheel.m_collisionInfo.normal * dot( wheel.m_collisionInfo.normal, wheelVelAtPoint );

			// normalize
			float fwdLength = length(wheelForward);
			if ( fwdLength < 0.001f )
				continue;

			wheelForward /= fwdLength;

			//---------------------------------------------------------------------------------

			// get direction vectors based on surface normal
			Vector3D wheelTractionForceDir = cross(wheel_right, wheel.m_collisionInfo.normal);

			// calculate wheel forward speed
			float wheelPitchSpeed = dot(wheelVelAtPoint, wheelTractionForceDir);

			float fLongitudinalForce = 0.0f;

			{
				float wheelDirVelocity = dot(wheelForward, wheelVelAtPoint );
				float longitudial = 0.0f ;

				if ( fabs( wheelDirVelocity ) > 0.01f )
				{
					longitudial = ( wheelConf.radius) / fabs ( wheelDirVelocity ) ;

					const float lOwheels = 0.12f ;

					if ( fabs ( longitudial ) > lOwheels * 1.1f )
					{
						longitudial = 1.0f * sign( longitudial ) * lOwheels + ( 1.0f - 1.0f ) * longitudial ;
					}
				}

				fLongitudinalForce = longitudial;

				if( isDriveWheel && bDoBurnout )
				{
					fLongitudinalForce += wheelTractionFrictionScale * (1.0f-fPitchFac);
				}

				float handbrakes = autoHandbrakeHelper+fHandbrake;

				if(isHandbrakeWheel && handbrakes > 0.0f)
				{
					fLongitudinalForce += handbrakes*m_conf->physics.handbrakeScale;
				}
			}


			Vector3D wheelSlipForceDir	= cross(wheel.m_collisionInfo.normal, wheelForward);

			// get wheel slip force based on
			//float wheelSlipVelocity = dot(wheelSlipForceDir, wheelVelAtPoint);

			float wheelSlipOppositeForce = 0.0f;
			float wheelTractionForce = 0.0f;

			float velocityMagnitude = dot(wheelVelAtPoint, wheelVelAtPoint);

			//
			// Bilateral force
			//
			{
				//
				// compute slip angle and lateral force
				//
				if ( velocityMagnitude > 1.0f )
				{
					float fSlipAngle = -atan2f( dot(wheelSlipForceDir,  wheelVelAtPoint ), fabs( dot(wheelForward, wheelVelAtPoint ) ) ) ;

					if(isSteerWheel && wheelPitchSpeed > 0.1f)
						fSlipAngle += m_steering * wheelConf.steerMultipler * 0.25f;

					wheelSlipOppositeForce = DBSlipAngleToLateralForce( fSlipAngle, fLongitudinalForce, wheel.m_surfparam, *m_slipParams);
				}
				else
				{
					// supress slip force on low speeds
					wheelSlipOppositeForce = dot(wheelSlipForceDir, wheelVelAtPoint ) * -4.0f ;
				}
				
				// contact surface modifier (perpendicularness to ground)
				{
					const float SURFACE_GRIP_SCALE = 1.0f;
					const float SURFACE_GRIP_DEADZONE = 0.1f;

					float surfaceForceMod = dot( wheel_right, wheel.m_collisionInfo.normal );
					surfaceForceMod = 1.0f - fabs ( surfaceForceMod );

					float fGrip = surfaceForceMod;
					fGrip = clamp( (fGrip * SURFACE_GRIP_SCALE) + SURFACE_GRIP_DEADZONE, 0.0f , 1.0f );

					wheelSlipOppositeForce *= fGrip;
				}
			}

			//
			// apply engine torque to drive wheel
			//
			if(isDriveWheel)
			{
				if (fAcceleration > 0.01f)
				{
					if (bDoBurnout)
					{
						wheelTractionForce += acceleratorAbs * driveGroundWheelMod * wheelTractionFrictionScale;
						wheelSlipOppositeForce *= wheelTractionFrictionScale * (1.0f - fPitchFac); // BY DIFFERENCE
					}

					wheelTractionForce += fAccelerator * driveGroundWheelMod;
				}
			}

			//
			// apply roll resistance
			//
			if(m_conf->flags.isCar)
			{
				float engineBrakeModifier = 1.0f - clamp((float)fAccel + (float)fabs(fBrake) + fabs(m_steering), 0.0f, 1.0f);

				if (!isDriveWheel)
					engineBrakeModifier = 0.0f;

				float velocityDamp = wheelPitchSpeed * engineBrakeModifier * 0.25f + sign(wheelPitchSpeed)*clamp((float)fabs(wheelPitchSpeed), 0.0f, 1.0f);
				wheelTractionForce -= velocityDamp * (1.0f - dampingFactor) * WHELL_ROLL_RESISTANCE_CONST;
			}

			//
			// apply brake
			//
			if(fabs(fBraker) > 0 && fabs(wheelPitchSpeed) > 0)
			{
				wheelTractionForce -= sign(wheelPitchSpeed) * fabs(fBraker * wheelConf.brakeTorque) * wheelTractionFrictionScale * 4.0f * (1.0f + springPowerFac);

				if (fBraker >= 0.95f)
					wheelPitchSpeed -= wheelPitchSpeed * 3.0f * wheelTractionFrictionScale;
			}

			//
			// apply handbrake
			//
			if(isHandbrakeWheel && fHandbrake > 0.0f)
			{
				const float HANDBRAKE_TORQUE = 8500.0f;

				wheelTractionForce -= sign(wheelPitchSpeed) * HANDBRAKE_TORQUE * wheelTractionFrictionScale * 3.0f;

				if(fHandbrake == 1.0f)
					wheelPitchSpeed = 0.0f;
			}
			
			{
				// set wheel velocity, add pitch radians
				wheel.m_pitchVel = wheelPitchSpeed;
			}

			wheelSlipOppositeForce *= springPower;

			springForce += wheelSlipForceDir * wheelSlipOppositeForce;
			springForce += wheelTractionForceDir * wheelTractionForce;

			// spring force position in a couple with antiroll
			Vector3D springForcePos = wheel.m_collisionInfo.position;

			// TODO: dependency on other wheels
			float fAntiRollFac = springPowerFac+ANTIROLL_FACTOR_DEADZONE;

			if(fAntiRollFac > ANTIROLL_FACTOR_MAX)
				fAntiRollFac = ANTIROLL_FACTOR_MAX;

			springForcePos += worldMatrix.rows[1].xyz() * fAntiRollFac * m_conf->physics.antiRoll * ANTIROLL_SCALE;

			// apply force of wheel to the car body so it can "stand" on the wheels
			carBody->ApplyWorldForce(springForcePos, springForce);
		}
		else
		{
			// wheel is in air
			float rollResistance = isDriveWheel ? ENGINE_ROLL_RESISTANCE : WHEEL_ROLL_RESISTANCE_FREE;

			if(isDriveWheel && acceleratorAbs > 0.0f)
			{
				wheel.m_pitchVel += acceleratorAbs*sign(fAccelerator) * carBody->GetInvMass() * delta * 8.0f;
			}
			else
			{
				wheel.m_pitchVel -= sign(wheel.m_pitchVel) * delta * rollResistance;
			}
		}

		bool burnout = isDriveWheel && bDoBurnout;
		if(burnout)
		{
			if(wheel.m_pitchVel < 0 && bDoBurnout)
				fPitchFac = 1.0f;

			wheel.m_pitch += (15.0f/wheelConf.radius) * fPitchFac * delta;
		}

		wheel.m_flags.isBurningOut = burnout;

		wheel.m_pitch += (wheel.m_pitchVel/wheelConf.radius)*delta;

		// normalize
		wheel.m_pitch = DEG2RAD(ConstrainAngle180(RAD2DEG(wheel.m_pitch)));
	}

	CCar* hingedCar = GetHingedVehicle();

	if(hingedCar)
	{
		// if it was seriously damaged or it flips over it will be disconnected
		if(!hingedCar->IsAlive() || dot(hingedCar->GetUpVector(), GetUpVector()) < HINGE_DISCONNECT_COS_ANGLE)
		{
			ReleaseHingedVehicle();
		}
	}
}

void CCar::RefreshWindowDamageEffects()
{
	if(m_bodyParts[CB_FRONT_LEFT].damage + m_bodyParts[CB_FRONT_RIGHT].damage > 1.25f)
		m_bodyParts[CB_WINDOW_FRONT].damage = 1.0f;
	else
		m_bodyParts[CB_WINDOW_FRONT].damage = 0.0f;

	if(m_bodyParts[CB_BACK_LEFT].damage + m_bodyParts[CB_BACK_RIGHT].damage > 1.25f)
		m_bodyParts[CB_WINDOW_BACK].damage = 1.0f;
	else
		m_bodyParts[CB_WINDOW_BACK].damage = 0.0f;

	if(m_bodyParts[CB_SIDE_LEFT].damage > 0.65f)
		m_bodyParts[CB_WINDOW_LEFT].damage = 1.0f;
	else
		m_bodyParts[CB_WINDOW_LEFT].damage = 0.0f;

	if(m_bodyParts[CB_SIDE_RIGHT].damage > 0.65f)
		m_bodyParts[CB_WINDOW_RIGHT].damage = 1.0f;
	else
		m_bodyParts[CB_WINDOW_RIGHT].damage = 0.0f;
}

void CCar::EmitCollisionParticles(const Vector3D& position, const Vector3D& velocity, const Vector3D& normal, int numDamageParticles, float fCollImpulse)
{
	//CEqRigidBody* carBody = m_physObj->GetBody();

	float wlen = length(velocity);

	if(m_effectTime <= 0.0f)
	{
		if(wlen > 5.0)
		{
			Vector3D reflDir = reflect(velocity, normal);
			MakeSparks(position+normal*0.05f, reflDir, Vector3D(10.0f), 1.0f, 2);
		}

		if(wlen > 10.5f && fCollImpulse > 150)
		{
			const float FLECK_SCALE = 0.8f;
			const int FLECK_COUNT = 2;

			int partCount = numDamageParticles*FLECK_COUNT;

			for(int j = 0; j < partCount; j++)
			{
				ColorRGB randColor = m_carColor.col1.Get().xyz() * RandomFloat(0.7f, 1.0f);

				Vector3D fleckPos = Vector3D(position) + Vector3D(RandomFloat(-0.05,0.05),RandomFloat(-0.05,0.05),RandomFloat(-0.05,0.05));

				CPFXAtlasGroup* feffgroup = g_translParticles;
				TexAtlasEntry_t* fentry = g_worldGlobals.trans_fleck;

				Vector3D norm(normal);

				if(j > partCount/2)
					norm *= -1;

				Vector3D rnd_ang = VectorAngles(fastNormalize(norm)) + Vector3D(RandomFloat(-55,55),RandomFloat(-55,55),RandomFloat(-55,55));

				Vector3D n;
				AngleVectors(rnd_ang, &n);

				float rwlen = wlen + RandomFloat(-wlen*0.15f, wlen*0.8f);

				float fleckPower = clamp(rwlen, 3.0f, 5.0f);

				CFleckEffect* fleck = new CFleckEffect(fleckPos, n*fleckPower + Vector3D(0.0f,2.0f,0.0f),
														Vector3D(0,-16.0f,0),
														RandomFloat(0.04, 0.07)*FLECK_SCALE, RandomFloat(0.4, 0.8), RandomFloat(120, 300),
														randColor, feffgroup, fentry);

				effectrenderer->RegisterEffectForRender(fleck);
			}
		}

		m_effectTime = 0.05f;
	}
}

void CCar::OnPrePhysicsFrame(float fDt)
{
	// update pursuit stuff
	if (m_pursuerData.pursuedByCount)
	{
		m_pursuerData.lastSeenTimer += fDt;
		m_pursuerData.lastDirectionTimer += fDt;
	}
	else
	{
		m_pursuerData.lastSeenTimer = 0.0f;
		m_pursuerData.lastDirectionTimer = 0.0f;
	}

#ifndef EDITOR
	// record it
	g_replayData->UpdateReplayObject( m_replayID );
#endif // EDITOR

	if( m_enablePhysics )
	{
		// update vehicle wheels, suspension, engine
		UpdateVehiclePhysics(fDt);
	}
}

bool CCar::UpdateWaterState( float fDt, bool hasCollidedWater )
{
	CEqRigidBody* carBody = m_physObj->GetBody();
	
	// if body frozen, return previous state
	if(carBody->IsFrozen())
		return m_inWater;

	Vector3D carPos = carBody->GetPosition();

	Vector3D waterNormal = vec3_up; // FIXME: calculate like I do in shader

	float waterLevel = g_pGameWorld->m_level.GetWaterLevel( carPos );

	if((carBody->m_aabb_transformed.minPoint.y < waterLevel && hasCollidedWater) || (carPos.y < waterLevel))
	{
		// take damage from water
		if( carPos.y < waterLevel )
		{
			bool wasAlive = IsAlive();

			// set damage here to not trigger replay writes
			m_gameDamage = m_gameDamage + DAMAGE_WATERDAMAGE_RATE * fDt;

			if(m_gameDamage > m_gameMaxDamage)
				m_gameDamage = m_gameMaxDamage;

			if(!IsAlive()) // lock the car and disable
			{
				m_enabled = false;
				m_locked = true;
				m_sirenEnabled = false;
				m_lightsEnabled = 0;

				// trigger death
				if(wasAlive)
					OnDeath(NULL);
			}
		}

		Vector3D newVelocity = carBody->GetLinearVelocity();

		newVelocity.x *= 0.99f;
		newVelocity.z *= 0.99f;

		newVelocity.y *= 0.99f;

		carBody->SetLinearVelocity( newVelocity );

		return true;
	}

	return false;
}

void CCar::OnPhysicsFrame( float fDt )
{
	CEqRigidBody* carBody = m_physObj->GetBody();

	//
	// update car collision sounds
	//

	float		fHitImpulse = 0.0f;
	int			numHitTimes = 0.0f;
	Vector3D	carPos = carBody->GetPosition();
	Vector3D	hitPos = carPos;
	Vector3D	hitNormal = Vector3D(0,0,1);
	float		collSpeed = 0.0f;

	bool		doImpulseSound = false;
	bool		hasHitWater = false;

	Matrix4x4 worldMatrix;
	carBody->ConstructRenderMatrix(worldMatrix);
	m_worldMatrix = worldMatrix;

	// TODO: move all processing to OnPhysicsCollide
	for(int i = 0; i < m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& coll = m_collisionList[i];

		// skip some of them
		if (coll.flags & COLLPAIRFLAG_USER_PROCESSED)
			continue;

		// we went underwater
		if( coll.bodyB->GetContents() & OBJECTCONTENTS_WATER )
		{
			hasHitWater = true;

			// make particles
			Vector3D collVelocity = carBody->GetVelocityAtWorldPoint(coll.position);
			collSpeed = length(collVelocity);
		}

		CGameObject* hitGameObject = NULL;

		// TODO: make it safe
		if (!(coll.bodyB->GetContents() & OBJECTCONTENTS_SOLID_GROUND) &&
			coll.bodyB->GetUserData() != NULL &&
			coll.appliedImpulse != 0.0f)
		{
			hitGameObject = (CGameObject*)coll.bodyB->GetUserData();
			hitGameObject->OnCarCollisionEvent(coll, this);
		}

		// don't apply collision damage if this is a trigger or water
		if ((coll.flags & COLLPAIRFLAG_OBJECTA_NO_RESPONSE) || (coll.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE))
			continue;

		Vector3D wVelocity = carBody->GetVelocityAtWorldPoint( coll.position );

		if(!(coll.flags & COLLPAIRFLAG_NO_SOUND))
			doImpulseSound = true;

		if( coll.bodyB->IsDynamic() )
		{
			CEqRigidBody* body = (CEqRigidBody*)coll.bodyB;
			wVelocity -= body->GetVelocityAtWorldPoint( coll.position );
		}

		EmitCollisionParticles(coll.position, wVelocity, coll.normal, (coll.appliedImpulse/3500.0f)+1, coll.appliedImpulse);

		float fDotUp = clamp(1.0f-(float)fabs(dot(vec3_up, coll.normal)), 0.5f, 1.0f);
		float invMassA = carBody->GetInvMass();

		float fDamageImpact = (coll.appliedImpulse * fDotUp) * invMassA * 0.5f;

		Vector3D localHitPos = (!(Matrix4x4(worldMatrix))*Vector4D(coll.position, 1.0f)).xyz();

		if(fDamageImpact > DAMAGE_MINIMPACT
#ifndef EDITOR
			&& g_pGameSession->IsServer()				// process damage model only on server
#endif // EDITOR
			)

#ifndef EDITOR
		// process damage and death only when not playing replays
		if (g_replayData->m_state != REPL_PLAYING)
#endif // EDITOR
		{
			// apply visual damage
			for(int cb = 0; cb < CB_PART_COUNT; cb++)
			{
				float fDot = dot(fastNormalize(s_BodyPartDirections[cb]), fastNormalize(localHitPos));

				if(fDot < 0.2)
					continue;

				float fDamageToApply = pow(fDot, 2.0f) * fDamageImpact*DAMAGE_SCALE;
				m_bodyParts[cb].damage += fDamageToApply * DAMAGE_VISUAL_SCALE;

				bool isWasAlive = IsAlive();
				SetDamage(GetDamage() + fDamageToApply);
				bool isStillAlive = IsAlive();

				if(isWasAlive && !isStillAlive)
				{
					// trigger death
					OnDeath( hitGameObject );
				}

				// clamp
				m_bodyParts[cb].damage = min(m_bodyParts[cb].damage, 1.0f);

				OnNetworkStateChanged(NULL);
			}

			int wheelCount = GetWheelCount();

			// make damage to wheels
			// hubcap effects
			for(int w = 0; w < wheelCount; w++)
			{
				Vector3D pos = m_wheels[w].GetOrigin()-Vector3D(coll.position);

				float dmgDist = length(pos);

				if(dmgDist < 1.0f)
					m_wheels[w].m_damage += (1.0f-dmgDist)*fDamageImpact * DAMAGE_WHEEL_SCALE;
			}

			RefreshWindowDamageEffects();
		}

		fHitImpulse += fDamageImpact * DAMAGE_SOUND_SCALE;

		hitPos = coll.position;
		hitNormal = coll.normal;

		numHitTimes++;

		coll.flags |= COLLPAIRFLAG_USER_PROCESSED;
	}

	// spawn smoke
	if(hasHitWater && collSpeed > 5.0f)// && m_effectTime <= 0.0f)
	{
		Vector3D waterPos = carPos + GetVelocity()*0.1f - vec3_up*1.0f;
		Vector3D particleVelocity = reflect(GetVelocity(), vec3_up)*0.15f;

		MakeWaterSplash(waterPos, particleVelocity, Vector3D(30.0f), 4.0f, 8 );
	}

	if(hasHitWater && !m_inWater && collSpeed > 5.0f)
	{
		EmitSound_t ep;
		ep.name = "generic.waterHit";
		ep.fPitch = RandomFloat(0.92f, 1.08f);
		ep.fVolume = 1.0f;
		ep.origin = GetOrigin();

		EmitSoundWithParams(&ep);
	}

	// set water state
	m_inWater = UpdateWaterState(fDt, hasHitWater);

#ifndef NO_LUA
	if(numHitTimes > 0)
	{
		OOLUA::Script& state = GetLuaState();
		EqLua::LuaStackGuard g(state);

		if( m_luaOnCollision.Push() )
		{
			OOLUA::Table tab = OOLUA::new_table(state);

			tab.set("impulse", fHitImpulse);

			OOLUA::push(state, tab);

			if(!m_luaOnCollision.Call(1, 0, 0))
			{
				MsgError("CGameObject:OnCollide error:\n %s\n", OOLUA::get_last_error(state).c_str());
			}
		}
	}
#endif // NO_LUA

	int wheelCount = GetWheelCount();

	// wheel damage and hubcaps
	for(int i = 0; i < wheelCount; i++)
	{
		CCarWheel& wheel = m_wheels[i];

		//debugoverlay->Text3D(wheel.pWheelObject->GetOrigin(), 10.0f, color4_white, "damage: %.2f", wheel.damage);

		// clamp the damage
		if (wheel.m_damage >= 1.0f)
			wheel.m_damage = 1.0f;

		//
		// don't process if already lost a hubcap
		//
		if(wheel.m_flags.lostHubcap)
			continue;

		float lateralSliding = GetLateralSlidingAtWheel(i) * sign(m_conf->physics.wheels[i].suspensionTop.x);
		lateralSliding = max(0.0f,lateralSliding);

		float addLoose = 0.0f;

		// skidding can do this
		if (lateralSliding > 2.0f)
		{
			addLoose += lateralSliding * fDt * 0.0035f;

			float tractionSliding = GetTractionSlidingAtWheel(i);

			// if you burnout too much, or brake
			if (tractionSliding > 4.0f)
				addLoose += tractionSliding * fDt * 0.001f;
		}

		// if vehicle landing on ground hubcaps may go away
		if(wheel.m_flags.onGround && !wheel.m_flags.lastOnGround)
		{
			float wheelLandingVelocity = (-dot(wheel.m_collisionInfo.normal, wheel.m_velocityVec)) - 4.0f;

			if(wheelLandingVelocity > 0.0f)
				addLoose += pow(wheelLandingVelocity, 2.0f) * 0.05f;
		}

		wheel.m_hubcapLoose += addLoose;

		bool looseHubcap = false;

		if(wheel.m_hubcapLoose >= 1.0f)
		{
			wheel.m_hubcapLoose = 1.0f;
			looseHubcap = true;
		}
			
		if (wheel.m_damage >= WHEEL_MIN_DAMAGE_LOOSE_HUBCAP)
			looseHubcap = true;

		if(looseHubcap)
			ReleaseHubcap(i);
	}

	m_effectTime -= fDt;

	if(m_effectTime <= 0)
		m_effectTime = 0;

	if(numHitTimes > 0)
	{
		float fDotUp = dot(vec3_up, hitNormal);

		if( fHitImpulse > 0.08f )
		{
			if(doImpulseSound)
			{
				EmitSound_t ep;

				bool minImpactSound = (fDotUp > 0.5f);

				if(fHitImpulse < 0.2f && !minImpactSound)
					ep.name = "generic.hit";
				else if(fHitImpulse < 1.5f)
					ep.name = "generic.impact";
				else
					ep.name = "generic.crash";

				ep.fPitch = RandomFloat(0.92f, 1.08f);
				ep.fVolume = clamp(fHitImpulse, 0.7f, 1.0f);
				ep.origin = hitPos;

				EmitSoundWithParams(&ep);
			}
		}
	}
} 

void CCar::OnPhysicsCollide(CollisionPairData_t& pair)
{
	// we went underwater
	if (pair.bodyB->GetContents() & OBJECTCONTENTS_WATER)
	{
		// apply 25% impulse
		CEqRigidBody::ApplyImpulseResponseTo(GetPhysicsBody(), pair.position, pair.normal, 0.0f, 0.0f, 0.0f, 0.05f);
	}

	m_collisionList.append(pair);
}

void CCar::ReleaseHubcap(int wheel)
{
#ifndef EDITOR
	CCarWheel& wdata = m_wheels[wheel];
	carWheelConfig_t& wheelConf = m_conf->physics.wheels[wheel];

	if(wdata.m_flags.lostHubcap || wdata.m_hubcapBodygroup == -1)
		return;

	Matrix4x4 wheelTranslation;
	wdata.CalculateTransform(wheelTranslation, wheelConf, false);

	wheelTranslation = transpose(m_worldMatrix * wheelTranslation);

	Vector3D wheelPos = wheelTranslation.getTranslationComponent();

	float angularVel = wdata.m_pitchVel * PI_F * 2.0f;

	if(wdata.m_flags.isBurningOut)
		angularVel += (15.0f/wheelConf.radius) * PI_F * 0.5f;

	CObject_Debris* hubcapObj = new CObject_Debris(NULL);
	hubcapObj->SpawnAsHubcap(wdata.GetModel(), wdata.m_hubcapBodygroup);
	hubcapObj->SetOrigin( wheelPos );
	hubcapObj->m_physBody->SetOrientation( Quaternion(wheelTranslation.getRotationComponent()) );
	hubcapObj->m_physBody->SetLinearVelocity( wdata.m_velocityVec );
	hubcapObj->m_physBody->SetAngularVelocity( wheelTranslation.rows[0].xyz() * -sign(wheelConf.suspensionTop.x) * angularVel );
	g_pGameWorld->AddObject(hubcapObj);

	wdata.m_flags.lostHubcap = true;
	wdata.m_bodyGroupFlags = (1 << wdata.m_damagedBodygroup);
#endif // EDITOR
}

float triangleWave( float pos )
{
	float sinVal = sin(pos);
	return pow(sinVal, 2.0f)*sign(sinVal);
}

void CCar::UpdateLightsState()
{
	if(g_debug_car_lights.GetBool())
		SetLight(0xFFFFFFFF, true);

	SetLight(CAR_LIGHT_SERVICELIGHTS, m_sirenEnabled);
}

ConVar r_carLights("r_carLights", "1", "Car light rendering type", CV_ARCHIVE);
ConVar r_carLights_dist("r_carLights_dist", "50.0", NULL, CV_ARCHIVE);

void CCar::Simulate( float fDt )
{
	PROFILE_FUNC();

	if(!m_physObj)
		return;

	CEqRigidBody* carBody = m_physObj->GetBody();

	if(!carBody)
		return;

	// cleanup our collision list here
	m_collisionList.clear();

	bool isCar = m_conf->flags.isCar;

	int controlButtons = GetControlButtons();

	if(	(m_conf->visual.sirenType > SERVICE_LIGHTS_NONE) && (controlButtons & IN_SIREN) && !(m_oldControlButtons & IN_SIREN))
	{
		m_oldSirenState = m_sirenEnabled;
		m_sirenEnabled = !m_sirenEnabled;
	}

	if ((controlButtons & IN_SIGNAL_LEFT) && !(m_oldControlButtons & IN_SIGNAL_LEFT))
	{
		SetLight(CAR_LIGHT_DIM_LEFT, !IsLightEnabled(CAR_LIGHT_DIM_LEFT));
		SetLight(CAR_LIGHT_DIM_RIGHT, false);
	}
	
	if ((controlButtons & IN_SIGNAL_RIGHT) && !(m_oldControlButtons & IN_SIGNAL_RIGHT))
	{
		SetLight(CAR_LIGHT_DIM_RIGHT, !IsLightEnabled(CAR_LIGHT_DIM_RIGHT));
		SetLight(CAR_LIGHT_DIM_LEFT, false);
	}

	if ((controlButtons & IN_SIGNAL_EMERGENCY) && !(m_oldControlButtons & IN_SIGNAL_EMERGENCY))
	{
		// switch them off
		if (IsLightEnabled(CAR_LIGHT_DIM_LEFT) && !IsLightEnabled(CAR_LIGHT_DIM_RIGHT))
			SetLight(CAR_LIGHT_DIM_LEFT, false);
		else if (IsLightEnabled(CAR_LIGHT_DIM_RIGHT) && !IsLightEnabled(CAR_LIGHT_DIM_LEFT))
			SetLight(CAR_LIGHT_DIM_RIGHT, false);

		SetLight(CAR_LIGHT_EMERGENCY, !IsLightEnabled(CAR_LIGHT_EMERGENCY));
	}

	if ((controlButtons & IN_SWITCH_BEAMS) && !(m_oldControlButtons & IN_SWITCH_BEAMS))
	{
		SetLight(CAR_LIGHT_LOWBEAMS, !IsLightEnabled(CAR_LIGHT_LOWBEAMS));
	}

	//
	//------------------------------------
	//

	Vector3D cam_pos = g_pGameWorld->m_view.GetOrigin();

	Vector3D rightVec = GetRightVector();
	Vector3D forwardVec = GetForwardVector();
	Vector3D upVec = GetUpVector();

	Matrix4x4 worldMatrix = m_worldMatrix;

	if( isCar && r_carLights.GetBool() && IsLightEnabled(CAR_LIGHT_LOWBEAMS) && (m_bodyParts[CB_FRONT_LEFT].damage < 1.0f || m_bodyParts[CB_FRONT_RIGHT].damage < 1.0f) )
	{
		float lightIntensity = 1.0f;
		float decalIntensity = 1.0f;
		float decalScale = 1.0f;

		int lightSide = 0;

		if( m_bodyParts[CB_FRONT_LEFT].damage > MIN_VISUAL_BODYPART_DAMAGE*0.5f )
		{
			if(m_bodyParts[CB_FRONT_LEFT].damage > MIN_VISUAL_BODYPART_DAMAGE)
			{
				lightSide += 1;
				lightIntensity -= 0.5f;
				decalScale -= 0.5f;
			}
			else
			{
				decalIntensity -= 0.5f;
				lightIntensity -= 0.25f;
			}

		}

		if( m_bodyParts[CB_FRONT_RIGHT].damage > MIN_VISUAL_BODYPART_DAMAGE*0.5f )
		{
			if(m_bodyParts[CB_FRONT_RIGHT].damage > MIN_VISUAL_BODYPART_DAMAGE)
			{
				lightSide -= 1;
				lightIntensity -= 0.5f;
				decalScale -= 0.5f;
			}
			else
			{
				decalIntensity -= 0.5f;
				lightIntensity -= 0.25f;
			}
		}

		float worldIntensityMod = g_pGameWorld->m_envConfig.headLightIntensity;

		lightIntensity *= max(0.1f, worldIntensityMod);
		decalIntensity *= worldIntensityMod;

		if(lightIntensity > 0.0f)
		{
			Vector3D startLightPos = GetOrigin() + forwardVec*m_conf->visual.headlightPosition.z;
			ColorRGBA lightColor(0.7, 0.6, 0.5, lightIntensity*0.7f);

			// show the light decal
			float distToCam = length(cam_pos-GetOrigin());

			if(distToCam < r_carLights_dist.GetFloat())
			{
				Vector3D lightPos = startLightPos + forwardVec*13.8f + (rightVec*lightSide*m_conf->visual.headlightPosition.x);

				float headlightsWidth = m_conf->visual.headlightPosition.x * 5.0f * decalScale;

				// project from top
				Matrix4x4 proj, view, viewProj;
				proj = orthoMatrix(-headlightsWidth, headlightsWidth, 0.0f, 14.0f, -1.5f, 0.5f);
				view = Matrix4x4( rotateX3(DEG2RAD(-90)) * !worldMatrix.getRotationComponent());
				view.translate(-lightPos);

				viewProj = proj*view;

				float intensityMod = (1.0f - (distToCam / r_carLights_dist.GetFloat()));

				Vector4D lightDecalColor = lightColor * pow(intensityMod, 0.8f) * decalIntensity * 0.5f;
				lightDecalColor.w = 1.0f;

				TexAtlasEntry_t* entry = lightSide == 0 ? g_vehicleLights->FindEntry("light1_d") : g_vehicleLights->FindEntry("light1_s");
				Rectangle_t flipRect = entry ? entry->rect : Rectangle_t(0,0,1,1);

				decalPrimitives_t lightDecal;
				lightDecal.settings.avoidMaterialFlags = MATERIAL_FLAG_WATER; // only avoid water
				lightDecal.settings.facingDir = normalize(vec3_up - forwardVec);

				// might be slow on mobile device
				lightDecal.settings.processFunc = LightDecalTriangleProcessFunc;

				ProjectDecalToSpriteBuilder(lightDecal, g_vehicleLights, flipRect, viewProj, lightDecalColor);
			}

			if(m_isLocalCar && r_carLights.GetInt() == 2)
			{
				Vector3D lightPos = startLightPos + forwardVec*10.0f;
				CollisionData_t light_coll;

				btSphereShape sphere(0.35f);
				g_pPhysics->TestConvexSweep(&sphere, identity(), startLightPos, lightPos, light_coll, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT);

				wlight_t light;
				light.position = Vector4D(lerp(startLightPos, lightPos + Vector3D(0,2,0), light_coll.fract), 12.0f*(light_coll.fract+0.15f));

				light.color = lightColor;

				g_pGameWorld->AddLight(light);
			}
		}
	}

	UpdateSounds(fDt);

	m_oldControlButtons = controlButtons;

	m_curTime += fDt;
	m_engineSmokeTime += fDt;

	bool visible = g_pGameWorld->m_occludingFrustum.IsSphereVisible(GetOrigin(), length(m_conf->physics.body_size));
	m_visible = visible;

#ifndef EDITOR
	// don't render car
	if(	g_pCameraAnimator->GetRealMode() == CAM_MODE_INCAR &&
		g_pGameSession->GetViewObject() == this)
		visible = false;
#endif // EDITOR

	float frontDamageSum = m_bodyParts[CB_FRONT_LEFT].damage+m_bodyParts[CB_FRONT_RIGHT].damage;

	if(	isCar && visible &&
		(!m_inWater || IsAlive()) &&
		m_engineSmokeTime > 0.1f &&
		GetSpeed() < 80.0f)
	{
		if(frontDamageSum > 0.55f)
		{
			ColorRGB smokeCol(0.9,0.9,0.9);

			if(frontDamageSum > 1.35f)
				smokeCol = ColorRGB(0.0);

			float rand_size = RandomFloat(-m_conf->physics.body_size.x*0.5f,m_conf->physics.body_size.x*0.5f);

			float alphaModifier = 1.0f - RemapValClamp(GetSpeed(), 0.0f, 80.0f, 0.0f, 1.0f);

			Vector3D smokePos = (worldMatrix * Vector4D(m_conf->visual.enginePosition + Vector3D(rand_size,0,0), 1.0f)).xyz();

			CSmokeEffect* pSmoke = new CSmokeEffect(smokePos, Vector3D(0,1, 1),
													RandomFloat(0.1, 0.3), RandomFloat(1.0, 1.8),
													RandomFloat(1.2f),
													g_translParticles, g_worldGlobals.trans_smoke2,
													RandomFloat(25, 85), Vector3D(1,RandomFloat(-0.7, 0.2) , 1),
													smokeCol, smokeCol, alphaModifier);
			effectrenderer->RegisterEffectForRender(pSmoke);
		}

		// make exhaust light smoke
		if(	m_enabled && m_isLocalCar &&
			m_conf->visual.exhaustDir != -1 &&
			GetSpeed() < 15.0f)
		{
			Vector3D smokePos = (worldMatrix * Vector4D(m_conf->visual.exhaustPosition, 1.0f)).xyz();
			Vector3D smokeDir(0);

			smokeDir[m_conf->visual.exhaustDir] = 1.0f;

			if(m_conf->visual.exhaustDir == 1)
				smokeDir[m_conf->visual.exhaustDir] *= -1.0f;

			smokeDir = worldMatrix.getRotationComponent() * smokeDir;

			ColorRGB smokeCol(0.9,0.9,0.9);

			float alphaModifier = 1.0f - RemapValClamp(GetSpeed(), 0.0f, 15.0f, 0.0f, 1.0f);

			float alphaScale = 0.25f+m_fAccelEffect*0.45f;

			if(frontDamageSum > 1.35f)
			{
				alphaScale *= 2.0f;
				smokeCol = ColorRGB(0.0);
			}

			CSmokeEffect* pSmoke = new CSmokeEffect(smokePos, -smokeDir,
													RandomFloat(0.08, 0.12), RandomFloat(0.3, 0.4),
													RandomFloat(alphaModifier*alphaScale),
													g_translParticles, g_worldGlobals.trans_smoke2,
													RandomFloat(25,45), Vector3D(0.1,RandomFloat(0.0, 0.2), 0.05),
													smokeCol, smokeCol, alphaModifier*alphaScale);
			effectrenderer->RegisterEffectForRender(pSmoke);
		}


		m_engineSmokeTime = 0.0f;
	}


	// Draw light flares
	// render siren lights
	if ( isCar && (m_lightsEnabled & CAR_LIGHT_SERVICELIGHTS) )
	{
		Vector3D siren_pos_noX(0.0f, m_conf->visual.sirenPositionWidth.y, m_conf->visual.sirenPositionWidth.z);

		Vector3D siren_position = (worldMatrix * Vector4D(siren_pos_noX, 1.0f)).xyz();

		switch(m_conf->visual.sirenType)
		{
			case SERVICE_LIGHTS_BLUE:
			case SERVICE_LIGHTS_RED:
			{
				ColorRGB colors[2] = {
					ColorRGB(0, 0.25, 1),
					ColorRGB(1, 0.25, 0)
				};

				ColorRGB color = colors[m_conf->visual.sirenType-SERVICE_LIGHTS_BLUE];

				PoliceSirenEffect(m_curTime, color, siren_position, rightVec, m_conf->visual.sirenPositionWidth.x, m_conf->visual.sirenPositionWidth.w);

				float fSin = fabs(sinf(m_curTime*16.0f));
				float fSinFactor = clamp(fSin, 0.5f, 1.0f);

				wlight_t light;
				light.position = Vector4D(siren_position, 20.0f);
				light.color = ColorRGBA(color, fSinFactor);

				g_pGameWorld->AddLight(light);

				break;
			}
			case SERVICE_LIGHTS_DOUBLE_BLUE:
			case SERVICE_LIGHTS_DOUBLE_RED:
			case SERVICE_LIGHTS_DOUBLE_BLUERED:
			{
				ColorRGB colors[3][2] = {
					{ColorRGB(0, 0.25, 1),ColorRGB(0, 0.25, 1)},
					{ColorRGB(1, 0.25, 0),ColorRGB(1, 0.25, 0)},
					{ColorRGB(1, 0.25, 0),ColorRGB(0, 0.25, 1)},
				};

				ColorRGB col1(colors[m_conf->visual.sirenType-SERVICE_LIGHTS_DOUBLE_BLUE][0]);
				ColorRGB col2(colors[m_conf->visual.sirenType - SERVICE_LIGHTS_DOUBLE_BLUE][1]);

				PoliceSirenEffect(-m_curTime + PI_F*2.0f, col1, siren_position, rightVec, -m_conf->visual.sirenPositionWidth.x, m_conf->visual.sirenPositionWidth.w);
				PoliceSirenEffect(m_curTime, col2, siren_position, rightVec, m_conf->visual.sirenPositionWidth.x, m_conf->visual.sirenPositionWidth.w);

				float fSin = fabs(sinf(m_curTime*20.0f));
				float fSinFactor = clamp(fSin, 0.5f, 1.0f);

				wlight_t light;
				light.position = Vector4D(siren_position, 20.0f);
				light.color = ColorRGBA(lerp(col1, col2, fSin), fSinFactor);

				g_pGameWorld->AddLight(light);

				break;
			}
		}
	}

	if (visible)
	{
		Vector3D cam_forward;
		AngleVectors(g_pGameWorld->m_view.GetAngles(), &cam_forward);

		Vector3D headlight_pos_noX(0.0f, m_conf->visual.headlightPosition.yz());
		Vector3D headlight_position = (worldMatrix * Vector4D(headlight_pos_noX, 1.0f)).xyz();

		Vector3D brakelight_pos_noX(0.0f, m_conf->visual.brakelightPosition.yz());
		Vector3D brakelight_position = (worldMatrix * Vector4D(brakelight_pos_noX, 1.0f)).xyz();

		Vector3D backlight_pos_noX(0.0f, m_conf->visual.backlightPosition.yz());
		Vector3D backlight_position = (worldMatrix * Vector4D(backlight_pos_noX, 1.0f)).xyz();

		Vector3D frontdimlight_pos_noX(0.0f, m_conf->visual.frontDimLights.yz());
		Vector3D backdimlight_pos_noX(0.0f, m_conf->visual.backDimLights.yz());

		Vector3D frontdimlight_position = (worldMatrix * Vector4D(frontdimlight_pos_noX, 1.0f)).xyz();
		Vector3D backdimlight_position = (worldMatrix * Vector4D(backdimlight_pos_noX, 1.0f)).xyz();

		Plane front_plane(forwardVec, -dot(forwardVec, headlight_position));
		Plane brake_plane(-forwardVec, -dot(-forwardVec, brakelight_position));
		Plane back_plane(-forwardVec, -dot(-forwardVec, backlight_position));

		float fLightsAlpha = dot(-cam_forward, forwardVec);

		const float HEADLIGHT_RADIUS = 0.96f;
		const float HEADLIGHTGLOW_RADIUS = 0.86f;
		const float BRAKELIGHT_RADIUS = 0.25f;
		const float BRAKEBACKLIGHT_RADIUS = 0.2f;
		const float BACKLIGHT_RADIUS = 0.125f;
		const float DIMLIGHT_RADIUS = 0.17f;

		const float BRAKEBACKLIGHT_INTENSITY = 0.5f;
		const float BACKLIGHT_INTENSITY = 0.7f;
		const float DIMLIGHT_INTENSITY = 0.7f;

		float headlightDistance = front_plane.Distance(cam_pos);

		float fHeadlightsAlpha = clamp((fLightsAlpha > 0.0f ? fLightsAlpha : 0.0f) + headlightDistance*0.1f, 0.0f, 1.0f);
		float fBackLightAlpha = clamp((fLightsAlpha < 0.0f ? -fLightsAlpha : 0.0f) + back_plane.Distance(cam_pos)*0.1f, 0.0f, 1.0f);
		float fBrakeLightAlpha = clamp((fLightsAlpha < 0.0f ? -fLightsAlpha : 0.0f) + brake_plane.Distance(cam_pos)*0.1f, 0.0f, 1.0f);

		float lightsAmbientBrightnessFac = 1.0f - length(g_pGameWorld->m_envConfig.ambientColor);
		float clampedAmientBrightnessFac = max(0.6f, lightsAmbientBrightnessFac);

		fHeadlightsAlpha *= clampedAmientBrightnessFac;
		fBackLightAlpha *= clampedAmientBrightnessFac;
		fBrakeLightAlpha *= clampedAmientBrightnessFac;

		int headlightsLevel = (m_lightsEnabled & CAR_LIGHT_HIGHBEAMS) ? 2 : ((m_lightsEnabled & CAR_LIGHT_LOWBEAMS) ? 1 : 0);

		// render some lights
		if (isCar && headlightsLevel > 0 && fHeadlightsAlpha > 0.0f)
		{
			float frontSizeScale = fHeadlightsAlpha;
			float frontGlowSizeScale = fHeadlightsAlpha*0.28f;

			float fHeadlightsGlowAlpha = fHeadlightsAlpha;

			if(	m_conf->visual.headlightType > LIGHTS_SINGLE)
				fHeadlightsAlpha *= 0.65f;

			float lightLensPercentage = clamp((100.0f - headlightDistance) * 0.025f, 0.0f, 1.0f) *pow(lightsAmbientBrightnessFac, 2.0f);
			fHeadlightsAlpha *= lightLensPercentage;
			fHeadlightsGlowAlpha *= clamp(1.0f - lightLensPercentage, 0.25f, 1.0f);

			Vector4D headlightPosition = m_conf->visual.headlightPosition;

			Vector3D positionL = headlight_position - rightVec*headlightPosition.x;
			Vector3D positionR = headlight_position + rightVec*headlightPosition.x;

			ColorRGBA headlightColor(fHeadlightsAlpha, fHeadlightsAlpha, fHeadlightsAlpha, 0.6f);
			ColorRGBA headlightGlowColor(fHeadlightsGlowAlpha, fHeadlightsGlowAlpha, fHeadlightsGlowAlpha, 1);

			switch (m_conf->visual.headlightType)
			{
				case LIGHTS_SINGLE:
				{
					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionL, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionR, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					break;
				}
				case LIGHTS_DOUBLE:
				case LIGHTS_DOUBLE_VERTICAL:
				{
					Vector3D lDirVec = rightVec;

					if(m_conf->visual.headlightType == LIGHTS_DOUBLE_VERTICAL)
						lDirVec = upVec;

					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					{
						DrawLightEffect(positionL - lDirVec* headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL - lDirVec* headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionL + lDirVec* headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL + lDirVec* headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionR - lDirVec* headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR - lDirVec* headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					{
						DrawLightEffect(positionR + lDirVec* headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR + lDirVec* headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontGlowSizeScale, 1);
					}

					break;
				}
			}
		}

		if ((IsLightEnabled(CAR_LIGHT_BRAKE) || headlightsLevel > 0) && fBrakeLightAlpha > 0)
		{
			// draw brake lights
			float fBrakeLightAlpha2 = fBrakeLightAlpha*0.6f;

			Vector4D brakelightPosition = m_conf->visual.brakelightPosition;

			Vector3D positionL = brakelight_position - rightVec* brakelightPosition.x;
			Vector3D positionR = brakelight_position + rightVec* brakelightPosition.x;

			ColorRGBA brakelightColor(fBrakeLightAlpha, 0.15*fBrakeLightAlpha, 0, 1);
			ColorRGBA brakelightColor2(fBrakeLightAlpha2, 0.15*fBrakeLightAlpha2, 0, 1);

			switch (m_conf->visual.brakelightType)
			{
				case LIGHTS_SINGLE:
				{
					if (IsLightEnabled(CAR_LIGHT_BRAKE))
					{
						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionL, brakelightColor2, BRAKELIGHT_RADIUS, 2);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionR, brakelightColor2, BRAKELIGHT_RADIUS, 2);
					}

					if (headlightsLevel > 0)
					{
						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionL, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionR, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);
					}
					break;
				}
				case LIGHTS_DOUBLE:
				case LIGHTS_DOUBLE_VERTICAL:
				{
					Vector3D lDirVec = rightVec;

					if(m_conf->visual.headlightType == LIGHTS_DOUBLE_VERTICAL)
						lDirVec = upVec;

					if (IsLightEnabled(CAR_LIGHT_BRAKE))
					{
						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
							DrawLightEffect(positionL - lDirVec* brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionL + lDirVec* brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionR - lDirVec* brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
							DrawLightEffect(positionR + lDirVec* brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);
					}

					if (headlightsLevel > 0)
					{
						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
							DrawLightEffect(positionL - lDirVec* brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

						if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionL + lDirVec* brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
							DrawLightEffect(positionR - lDirVec* brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

						if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
							DrawLightEffect(positionR + lDirVec* brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);
					}
					break;
				}
			}
		}

		// any kind of dimension lights
		if(IsLightEnabled(CAR_LIGHT_DIM_LEFT) || IsLightEnabled(CAR_LIGHT_DIM_RIGHT))
		{
			Vector3D positionFL = frontdimlight_position - rightVec*m_conf->visual.frontDimLights.x;
			Vector3D positionFR = frontdimlight_position + rightVec*m_conf->visual.frontDimLights.x;

			Vector3D positionRL = backdimlight_position - rightVec*m_conf->visual.backDimLights.x;
			Vector3D positionRR = backdimlight_position + rightVec*m_conf->visual.backDimLights.x;

			ColorRGBA dimLightsColor(1,0.8,0.0f, 1.0f);

			dimLightsColor *= clamp(sinf(m_curTime*10.0f)*1.6f, 0.0f, 1.0f);

			if(IsLightEnabled(CAR_LIGHT_DIM_LEFT))
			{
				if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					DrawLightEffect(positionFL, dimLightsColor*DIMLIGHT_INTENSITY, DIMLIGHT_RADIUS, 1);

				if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					DrawLightEffect(positionRL, dimLightsColor*DIMLIGHT_INTENSITY, DIMLIGHT_RADIUS, 1);
			}

			if(IsLightEnabled(CAR_LIGHT_DIM_RIGHT))
			{
				if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					DrawLightEffect(positionFR, dimLightsColor*DIMLIGHT_INTENSITY, DIMLIGHT_RADIUS, 1);

				if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					DrawLightEffect(positionRR, dimLightsColor*DIMLIGHT_INTENSITY, DIMLIGHT_RADIUS, 1);
			}
		}

		// draw back lights
		if (IsLightEnabled(CAR_LIGHT_REVERSELIGHT))
		{
			// draw back lights
			float fBackLightAlpha2 = fBackLightAlpha * 0.3f;

			Vector3D positionL = backlight_position - rightVec*m_conf->visual.backlightPosition.x;
			Vector3D positionR = backlight_position + rightVec*m_conf->visual.backlightPosition.x;

			ColorRGBA backlightColor(fBackLightAlpha, fBackLightAlpha, fBackLightAlpha, 1);
			ColorRGBA backlightColor2(fBackLightAlpha2, fBackLightAlpha2, fBackLightAlpha2, 1);

			if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
			{
				DrawLightEffect(positionL, backlightColor2*BACKLIGHT_INTENSITY, BACKLIGHT_RADIUS, 2);
				DrawLightEffect(positionL, backlightColor*BACKLIGHT_INTENSITY, BACKLIGHT_RADIUS, 1);
			}

			if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
			{
				DrawLightEffect(positionR, backlightColor2*BACKLIGHT_INTENSITY, BACKLIGHT_RADIUS, 2);
				DrawLightEffect(positionR, backlightColor*BACKLIGHT_INTENSITY, BACKLIGHT_RADIUS, 1);
			}

		}
	}

	CCar* hingedCar = GetHingedVehicle();

	if(hingedCar)
	{
		hingedCar->SetLight(CAR_LIGHT_REVERSELIGHT, IsLightEnabled(CAR_LIGHT_REVERSELIGHT));
		hingedCar->SetLight(CAR_LIGHT_BRAKE, IsLightEnabled(CAR_LIGHT_BRAKE));
		hingedCar->SetLight(CAR_LIGHT_DIM_LEFT, IsLightEnabled(CAR_LIGHT_DIM_LEFT));
		hingedCar->SetLight(CAR_LIGHT_DIM_RIGHT, IsLightEnabled(CAR_LIGHT_DIM_RIGHT));
		hingedCar->SetLight(CAR_LIGHT_LOWBEAMS, IsLightEnabled(CAR_LIGHT_LOWBEAMS));
	}
}

void CCar::AddWheelWaterTrail(const CCarWheel& wheel, const carWheelConfig_t& wheelConf,
						const Vector3D& skidmarkPos,
						const Rectangle_t& trailCoords,
						const ColorRGB& ambientAndSun,
						float skidPitchVel,
						const Vector3D& wheelDir,
						const Vector3D& wheelRightDir)
{
	PFXVertexPair_t* trailPair;

	// begin horizontal
	if(g_vehicleEffects->AllocateGeom(4,4, (PFXVertex_t**)&trailPair, NULL, true) == -1)
		return;

	PFXVertexPair_t skidmarkPair;
	skidmarkPair.v0 = PFXVertex_t(skidmarkPos - wheelRightDir*wheelConf.width*0.5f, vec2_zero, 0.0f);
	skidmarkPair.v1 = PFXVertex_t(skidmarkPos + wheelRightDir*wheelConf.width*0.5f, vec2_zero, 0.0f);

	float wheelTrailFac = pow(fabs(skidPitchVel) - 2.0f, 0.5f);

	trailPair[0] = skidmarkPair;

	float alpha = g_pGameWorld->m_envWetness;

	trailPair[0].v0.color = ColorRGBA(ambientAndSun, alpha);
	trailPair[0].v1.color = ColorRGBA(ambientAndSun, alpha);

	trailPair[1].v0 = PFXVertex_t(skidmarkPos - wheelRightDir*wheelConf.width*0.75f, vec2_zero, ColorRGBA(ambientAndSun,0.0f));
	trailPair[1].v1 = PFXVertex_t(skidmarkPos + wheelRightDir*wheelConf.width*0.75f, vec2_zero, ColorRGBA(ambientAndSun,0.0f));

	// calculate trail velocity
	Vector3D velVecOffs = wheelDir * wheelTrailFac*0.25f;

	trailPair[1].v0.point -= velVecOffs;
	trailPair[1].v1.point -= velVecOffs;

	Rectangle_t rect(trailCoords.GetTopVertical(0.25f));

	// animate the texture coordinates
	Vector2D offset(0, fabs(triangleWave(wheel.m_pitch))*rect.GetSize().y*3.0f);

	trailPair[0].v0.texcoord = rect.GetLeftTop() + offset;
	trailPair[0].v1.texcoord = rect.GetRightTop() + offset;

	trailPair[1].v0.texcoord = rect.GetLeftBottom() + offset;
	trailPair[1].v1.texcoord = rect.GetRightBottom() + offset;

	// begin vertical
	if(g_vehicleEffects->AllocateGeom(4,4, (PFXVertex_t**)&trailPair, NULL, true) == -1)
		return;

	rect = rect.GetLeftHorizontal(0.35f);

	trailPair[0].v0 = PFXVertex_t(skidmarkPos + vec3_up*wheelConf.radius*0.5f, vec2_zero, ColorRGBA(ambientAndSun,alpha));
	trailPair[0].v1 = PFXVertex_t(skidmarkPos, vec2_zero, ColorRGBA(ambientAndSun,alpha));
	trailPair[1].v0 = PFXVertex_t(skidmarkPos + vec3_up*wheelConf.radius*0.5f, vec2_zero, ColorRGBA(ambientAndSun,0.0f));
	trailPair[1].v1 = PFXVertex_t(skidmarkPos, vec2_zero, ColorRGBA(ambientAndSun,0.0f));

	trailPair[1].v0.point -= velVecOffs;
	trailPair[1].v1.point -= velVecOffs;

	trailPair[0].v0.texcoord = rect.GetLeftTop() + offset;
	trailPair[0].v1.texcoord = rect.GetRightTop() + offset;
	trailPair[1].v0.texcoord = rect.GetLeftBottom() + offset;
	trailPair[1].v1.texcoord = rect.GetRightBottom() + offset;
}

void CCarWheel::CalcWheelSkidPair(PFXVertexPair_t& pair, const carWheelConfig_t& conf, const Vector3D& wheelRightVec)
{
	// exclude velocity in direction of normal
	Vector3D wheelVelocityOnGround = m_velocityVec * (Vector3D(1.0f) - m_collisionInfo.normal);

	// calculate side dir using movement direction
	Vector3D sideDir = cross(fastNormalize(wheelVelocityOnGround), m_collisionInfo.normal);

	// calc wheel center (because of offset in the raycast)
	Vector3D wheelCenterPos = m_collisionInfo.position - wheelRightVec * conf.woffset;
	
	// make skidmark position with some offset to not dig it under ground
	Vector3D skidmarkPos = wheelCenterPos + wheelVelocityOnGround * 0.0065f + m_collisionInfo.normal*0.02f;

	Vector3D skidmarkSideOffset = sideDir * conf.width * 0.5f;

	pair.v0.point = skidmarkPos - skidmarkSideOffset;
	pair.v1.point = skidmarkPos + skidmarkSideOffset;
}

const eqPhysSurfParam_t* CCarWheel::GetSurfaceParams() const
{
	return m_surfparam;
}

void CCar::ProcessWheelSkidmarkTrails(int wheelIdx)
{
	CCarWheel& wheel = m_wheels[wheelIdx];
	carWheelConfig_t& wheelConf = m_conf->physics.wheels[wheelIdx];

	if (!wheel.m_flags.doSkidmarks)
	{
		wheel.m_flags.lastDoSkidmarks = wheel.m_flags.doSkidmarks;
		return;
	}

	// remove skidmarks
	if (wheel.m_skidMarks.numElem() > SKIDMARK_MAX_LENGTH)
		wheel.m_skidMarks.removeIndex(0); // this is slow

	if (!wheel.m_flags.lastDoSkidmarks && wheel.m_flags.doSkidmarks && (wheel.m_skidMarks.numElem() > 0))
	{
		int nLast = wheel.m_skidMarks.numElem() - 1;

		if (wheel.m_skidMarks[nLast].v0.color.x < -10.0f && nLast > 0)
			nLast -= 1;

		PFXVertexPair_t lastPair = wheel.m_skidMarks[nLast];
		lastPair.v0.color.x = -100.0f;

		wheel.m_skidMarks.append(lastPair);
	}

	float tractionSlide = GetTractionSlidingAtWheel(wheelIdx);
	float lateralSlide = GetLateralSlidingAtWheel(wheelIdx);

	float fSkid = (tractionSlide + fabs(lateralSlide))*0.15f - 0.45f;

	// add new trail vertice
	if (fSkid > 0.0f)
	{
		PFXVertexPair_t skidmarkPair;
		wheel.CalcWheelSkidPair(skidmarkPair, wheelConf, GetRightVector());

		const float SKID_TRAIL_ALPHA = 0.32f;

		float skidAlpha = fSkid * SKID_TRAIL_ALPHA;
		skidAlpha = min(skidAlpha, SKID_TRAIL_ALPHA);

		// TODO: apply color of ground or asphalt...
		ColorRGBA skidColor(0.0f, 0.0f, 0.0f, skidAlpha);

		skidmarkPair.v0.color = skidmarkPair.v1.color = skidColor;

		int numMarkVertexPairs = wheel.m_skidMarks.numElem();

		if (wheel.m_skidMarks.numElem() > 1)
		{
			float pointDist = length(wheel.m_skidMarks[numMarkVertexPairs - 2].v0.point - skidmarkPair.v0.point);

			if (pointDist > SKIDMARK_MIN_INTERVAL)
			{
				wheel.m_skidMarks.append(skidmarkPair);
			}
			else if (wheel.m_skidMarks.numElem() > 2 &&
				wheel.m_skidMarks[wheel.m_skidMarks.numElem() - 1].v0.color.x >= 0.0f)
			{
				wheel.m_skidMarks[wheel.m_skidMarks.numElem() - 1] = skidmarkPair;
			}
		}
		else
			wheel.m_skidMarks.append(skidmarkPair);
	}

	wheel.m_flags.lastDoSkidmarks = wheel.m_flags.doSkidmarks;
}

void CCar::DrawWheelEffects(int wheelIdx)
{
	if (g_pGameWorld->m_envWetness <= SKID_WATERTRAIL_MIN_WETNESS)
		return;

	CCarWheel& wheel = m_wheels[wheelIdx];
	carWheelConfig_t& wheelConf = m_conf->physics.wheels[wheelIdx];

	if(!wheel.m_flags.onGround)
	{
		if(!wheel.m_flags.doSkidmarks)
			wheel.m_flags.lastDoSkidmarks = wheel.m_flags.doSkidmarks;

		return;
	}

	// make some trails on wet surfaces
	if(	wheel.m_surfparam != NULL && wheel.m_surfparam->word == 'C')
	{
		ColorRGB ambientAndSun = g_pGameWorld->m_info.rainBrightness;

		Matrix3x3 wheelMat = wheel.m_wheelOrient * transpose(m_worldMatrix.getRotationComponent());

		Vector3D velAtWheel = wheel.m_velocityVec;
		velAtWheel.y = 0.0f;

		Vector3D wheelDir = fastNormalize(velAtWheel);
		Vector3D wheelRightDir = cross(wheelDir, wheel.m_collisionInfo.normal);

		Vector3D skidmarkPos = (wheel.m_collisionInfo.position - wheelMat.rows[0] * wheelConf.width*sign(wheelConf.suspensionTop.x)*0.5f) + wheelDir * 0.05f;
		skidmarkPos += velAtWheel * 0.0065f + wheel.m_collisionInfo.normal*0.015f;

		float tractionSlide = GetTractionSlidingAtWheel(wheelIdx);
		float lateralSlide = GetLateralSlidingAtWheel(wheelIdx);

		//--------------------------------------------------------------------------

		float skidPitchVel = length(velAtWheel)*1.5f;

		if(skidPitchVel > 2.0f )
		{
			AddWheelWaterTrail(	wheel, wheelConf,
								skidmarkPos,
								g_worldGlobals.veh_raintrail->rect,
								ambientAndSun,
								skidPitchVel,
								wheelDir,
								wheelRightDir);
		}

		if(fabs(tractionSlide*wheel.m_pitchVel) > skidPitchVel)
		{
			Vector3D wheelTractionDir = wheelMat.rows[2];
			Vector3D wheelTractionRightDir = cross(wheelTractionDir, wheel.m_collisionInfo.normal);

			AddWheelWaterTrail(	wheel, wheelConf,
								skidmarkPos,
								g_worldGlobals.veh_raintrail->rect,
								ambientAndSun,
								tractionSlide*0.5f,
								wheelTractionDir,
								wheelTractionRightDir);
		}
	}
}

void CCar::DrawSkidmarkTrails(int wheelIdx)
{
	CCarWheel& wheel = m_wheels[wheelIdx];
	TexAtlasEntry_t* skidmarkEntry = g_worldGlobals.veh_skidmark_asphalt;

	//
	// add skidmark particle strips to the renderer
	//
	int numSkidMarks = 0;
	int nStartMark = 0;

	int vtxCounter = 0;

	for(int mark = 0; mark < wheel.m_skidMarks.numElem(); mark++)
	{
		PFXVertexPair_t& pair = wheel.m_skidMarks[mark];

		Vector2D coordV0 = (vtxCounter & 1) ? skidmarkEntry->rect.GetLeftBottom() : skidmarkEntry->rect.GetLeftTop();
		Vector2D coordV1 = (vtxCounter & 1) ? skidmarkEntry->rect.GetRightBottom() : skidmarkEntry->rect.GetRightTop();

		pair.v0.texcoord = coordV0;
		pair.v1.texcoord = coordV1;

		vtxCounter++;

		if(pair.v0.color.x < -10.0f)
		{
			// make them smooth
			wheel.m_skidMarks[nStartMark].v0.color.w = 0.0f;
			wheel.m_skidMarks[nStartMark].v1.color.w = 0.0f;

			//pair.v0.color.w = 0.0f;
			//pair.v1.color.w = 0.0f;

			numSkidMarks = mark-nStartMark;

			g_vehicleEffects->AddParticleStrip(&wheel.m_skidMarks[nStartMark].v0, numSkidMarks*2);

			nStartMark = mark+1;
			vtxCounter = 0;
		}

		if(mark == wheel.m_skidMarks.numElem()-1 && nStartMark < wheel.m_skidMarks.numElem())
		{
			wheel.m_skidMarks[nStartMark].v0.color.w = 0.0f;
			wheel.m_skidMarks[nStartMark].v1.color.w = 0.0f;

			numSkidMarks = wheel.m_skidMarks.numElem()-nStartMark;

			g_vehicleEffects->AddParticleStrip(&wheel.m_skidMarks[nStartMark].v0, numSkidMarks*2);

			vtxCounter = 0;
		}
	}
}

void CCar::PostDraw()
{
	bool drawOnLocal = m_isLocalCar && (r_drawSkidMarks.GetInt() != 0);
	bool drawOnOther = !m_isLocalCar && (r_drawSkidMarks.GetInt() == 2);

	bool doSkidMarks = drawOnLocal || drawOnOther;

	if (doSkidMarks)
	{
		int numWheels = GetWheelCount();

		for (int i = 0; i < numWheels; i++)
		{
			ProcessWheelSkidmarkTrails(i);
			DrawSkidmarkTrails(i);
		}
	}
}

void CCar::UpdateWheelEffect(int nWheel, float fDt)
{
	CCarWheel& wheel = m_wheels[nWheel];
	carWheelConfig_t& wheelConf = m_conf->physics.wheels[nWheel];

	Matrix3x3 wheelMat = wheel.m_wheelOrient * transpose(m_worldMatrix.getRotationComponent());

	wheel.m_flags.lastOnGround = wheel.m_flags.onGround;

	if( wheel.m_collisionInfo.fract >= 1.0f )
	{
		wheel.m_flags.doSkidmarks = false;
		wheel.m_flags.onGround = false;
		wheel.m_skidTime -= fDt;
		return;
	}

	float tractionSlide = GetTractionSlidingAtWheel(nWheel);
	float lateralSlide = GetLateralSlidingAtWheel(nWheel);

	wheel.m_flags.onGround = true;
	wheel.m_flags.doSkidmarks = (tractionSlide > 3.0f || fabs(lateralSlide) > 2.0f);

	wheel.m_smokeTime -= fDt;

	if(wheel.m_flags.doSkidmarks)
		wheel.m_skidTime += fDt*(1.0f - g_pGameWorld->m_envWetness*WHEEL_SKID_WETSCALING);
	else
		wheel.m_skidTime -= fDt;

	wheel.m_skidTime = clamp(wheel.m_skidTime, 0.0f, WHEEL_SKID_COOLDOWNTIME);

	if( wheel.m_flags.onGround && wheel.m_surfparam != NULL )
	{
		Vector3D smoke_pos = wheel.m_collisionInfo.position + wheel.m_collisionInfo.normal * wheelConf.radius*0.5f;

		if(wheel.m_surfparam->word == 'C')	// concrete/asphalt
		{
			float fSliding = tractionSlide+fabs(lateralSlide);

			if(fSliding < 5.0f)
				return;

			// spawn smoke
			if(wheel.m_smokeTime > 0.0)
				return;

			float efficency = RemapValClamp(fSliding, 5.0f, 40.0f, 0.4f, 1.0f);
			float timeScale = RemapValClamp(fSliding, 5.0f, 40.0f, 0.7f, 1.0f);

			if(g_pGameWorld->m_envWetness > SKID_WATERTRAIL_MIN_WETNESS)
			{
				// add the splashing on wet surfaces
				ColorRGB rippleColor(0.8f, 0.8f, 0.8f);

				Vector3D rightDir = wheelMat.rows[0] * 0.7f;
				Vector3D particleVel = wheel.m_velocityVec*0.35f + Vector3D(0.0f,1.2f,1.0f);

				CSmokeEffect* pSmoke = new CSmokeEffect(wheel.m_collisionInfo.position, particleVel + rightDir,
						RandomFloat(0.1f, 0.2f), RandomFloat(0.9f, 1.1f),
						RandomFloat(0.1f)*efficency,
						g_translParticles, g_worldGlobals.trans_raindrops,
						RandomFloat(5, 35), Vector3D(0,RandomFloat(-0.9, -8.2) , 0),
						rippleColor, rippleColor, g_pGameWorld->m_envWetness);

				effectrenderer->RegisterEffectForRender(pSmoke);

				pSmoke = new CSmokeEffect(wheel.m_collisionInfo.position, particleVel - rightDir,
						RandomFloat(0.1f, 0.2f), RandomFloat(0.9f, 1.1f),
						RandomFloat(0.1f),
						g_translParticles, g_worldGlobals.trans_raindrops,
						RandomFloat(5, 35), Vector3D(0,RandomFloat(-0.9, -8.2) , 0),
						rippleColor, rippleColor, g_pGameWorld->m_envWetness);

				effectrenderer->RegisterEffectForRender(pSmoke);

				wheel.m_smokeTime = 0.07f;
			}

			if(g_pGameWorld->m_envWetness < SKID_SMOKE_MAX_WETNESS)
			{
				// generate smoke
				float skidFactor = (wheel.m_skidTime-WHEEL_MIN_SKIDTIME)*WHEEL_SKIDTIME_EFFICIENCY;
				skidFactor = clamp(skidFactor, 0.0f, 1.0f);

				ColorRGB smokeCol(0.86f, 0.9f, 0.97f);

				CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, wheel.m_velocityVec*0.25f+Vector3D(0,1,1),
							RandomFloat(0.1, 0.3)*efficency, RandomFloat(1.0, 1.8)*timeScale+skidFactor*2.0f,
							RandomFloat(1.2f)*timeScale+skidFactor,
							g_translParticles, g_worldGlobals.trans_smoke2,
							RandomFloat(25, 85), Vector3D(1,RandomFloat(-0.7, 0.2) , 1),
							smokeCol, smokeCol, max(skidFactor, 0.45f));

				effectrenderer->RegisterEffectForRender(pSmoke);

				wheel.m_smokeTime = 0.1f;
			}
		}
		else if(wheel.m_surfparam->word == 'S')	// sand
		{
			if(tractionSlide > 5.0f || fabs(lateralSlide) > 2.5f)
			{
				// spawn smoke
				if(wheel.m_smokeTime > 0.0)
					return;

				wheel.m_smokeTime = 0.08f;

				Vector3D vel = -wheelMat.rows[2]*tractionSlide * 0.025f;
				vel += wheelMat.rows[0]*lateralSlide * 0.025f;

				CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, vel,
														RandomFloat(0.11, 0.14), RandomFloat(1.1, 1.5),
														RandomFloat(0.15f),
														g_translParticles, g_worldGlobals.trans_smoke2,
														RandomFloat(5, 35), Vector3D(1,RandomFloat(-3.9, -5.2) , 1),
														ColorRGB(0.95f,0.757f,0.611f), ColorRGB(0.95f,0.757f,0.611f));

				effectrenderer->RegisterEffectForRender(pSmoke);
			}
		}
		else if(wheel.m_surfparam->word == 'G' || wheel.m_surfparam->word == 'D')	// grass or dirt
		{
			if(tractionSlide > 5.0f || fabs(lateralSlide) > 2.5f)
			{
				// spawn smoke
				if(wheel.m_smokeTime > 0.0f)
					return;

				wheel.m_smokeTime = 0.08f;

				Vector3D vel = -wheelMat.rows[2]*tractionSlide * 0.025f;
				vel += wheelMat.rows[0]*lateralSlide * 0.025f;

				CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, vel,
														RandomFloat(0.11, 0.14), RandomFloat(1.5, 1.7),
														RandomFloat(0.35f),
														g_translParticles, g_worldGlobals.trans_smoke2,
														RandomFloat(25, 85), Vector3D(1,RandomFloat(-3.9, -5.2) , 1),
														ColorRGB(0.1f,0.08f,0.00f), ColorRGB(0.1f,0.08f,0.00f));

				effectrenderer->RegisterEffectForRender(pSmoke);

				if(wheel.m_surfparam->word != 'G')
					return;

				vel += Vector3D(0, 1.5f, 0);

				float len = length(vel);

				Vector3D grass_pos = wheel.m_collisionInfo.position;


				for(int i = 0; i < 4; i++)
				{
					Vector3D rnd_ang = VectorAngles(fastNormalize(vel)) + Vector3D(RandomFloat(-35,35),RandomFloat(-35,35),RandomFloat(-35,35));
					Vector3D n;
					AngleVectors(rnd_ang, &n);

					CParticleLine* pSpark = new CParticleLine(grass_pos,
														n*len,	// velocity
														Vector3D(0.0f,-3.0f, 0.0f),		// gravity
														RandomFloat(0.01f, 0.02f), RandomFloat(0.01f, 0.02f), // sizes
														RandomFloat(0.35f, 0.45f),// lifetime
														8.0f,
														g_translParticles, g_worldGlobals.trans_grasspart, // group - texture
														ColorRGB(1.0f,0.8f,0.0f), 1.0f);
					effectrenderer->RegisterEffectForRender(pSpark);
				}
			} // traction slide
		} // selector
	}
}

void CCar::UpdateSounds( float fDt )
{
	CEqRigidBody* carBody = m_physObj->GetBody();

	Vector3D pos = carBody->GetPosition();
	Vector3D velocity = carBody->GetLinearVelocity();

	for(int i = 0; i < CAR_SOUND_COUNT; i++)
	{
		if(!m_sounds[i])
			continue;

		m_sounds[i]->SetOrigin(pos);
		m_sounds[i]->SetVelocity(velocity);
	}

	bool isCar = m_conf->flags.isCar;

	if(isCar && m_sounds[CAR_SOUND_SIREN])
	{
		if( !IsAlive() && m_sirenDeathTime > 0 && m_conf->visual.sirenType != SERVICE_LIGHTS_NONE )
		{
			m_sounds[CAR_SOUND_SIREN]->SetPitch( m_sirenDeathTime / SIREN_SOUND_DEATHTIME );
			m_sirenDeathTime -= fDt;

			if(m_sirenDeathTime <= 0.0f)
				m_sirenEnabled = false;
		}
		else if(IsAlive())
			m_sirenDeathTime = SIREN_SOUND_DEATHTIME;
	}

	int wheelCount = GetWheelCount();
	float fTractionLevel = GetTractionSliding(true)*0.2f;
	
	// wet ground
	{
		bool anyWheelOnGround = false;

		for (int i = 0; i < wheelCount; i++)
		{
			if (m_wheels[i].m_collisionInfo.fract < 1.0f)
			{
				anyWheelOnGround = true;
				break;
			}
		}

		float fSpeedMod = clamp(GetSpeed() / 90.0f + fTractionLevel * 0.25f, 0.0f, 1.0f);
		float fSpeedModVol = clamp(GetSpeed() / 20.0f + fTractionLevel * 0.25f, 0.0f, 1.0f);

		m_sounds[CAR_SOUND_SURFACE]->SetPitch(fSpeedMod);
		m_sounds[CAR_SOUND_SURFACE]->SetVolume(fSpeedModVol*g_pGameWorld->m_envWetness);

		if (anyWheelOnGround && m_sounds[CAR_SOUND_SURFACE]->IsStopped() && m_isLocalCar && g_pGameWorld->m_envWetness > SKID_WATERTRAIL_MIN_WETNESS)
		{
			m_sounds[CAR_SOUND_SURFACE]->Play();
		}
		else if (!anyWheelOnGround && !m_sounds[CAR_SOUND_SURFACE]->IsStopped())
		{
			m_sounds[CAR_SOUND_SURFACE]->Stop();
		}
	}

	// skid sound
	{
		float fSlideLevel = fabs(GetLateralSlidingAtWheels(true)*0.5f) - 0.25;

		float fSkid = (fSlideLevel + fTractionLevel)*0.5f;

		float fSkidVol = clamp((fSkid - 0.5)*1.0f - g_pGameWorld->m_envWetness*3.0f, 0.0, 1.0);

		float fWheelRad = 0.0f;

		float inv_wheelCount = 1.0f / wheelCount;

		for (int i = 0; i < wheelCount; i++)
			fWheelRad += m_conf->physics.wheels[i].radius;

		fWheelRad *= inv_wheelCount;

		const float IDEAL_WHEEL_RADIUS = 0.35f;
		const float SKID_RADIAL_SOUNDPITCH_SCALE = 0.68f;

		float wheelSkidPitchModifier = IDEAL_WHEEL_RADIUS - fWheelRad;
		float fSkidPitch = clamp(0.7f*fSkid + 0.25f, 0.35f, 1.0f) - 0.15f*saturate(sinf(m_curTime*1.25f)*8.0f - fTractionLevel);

		m_sounds[CAR_SOUND_SKID]->SetVolume(fSkidVol);
		m_sounds[CAR_SOUND_SKID]->SetPitch(fSkidPitch + wheelSkidPitchModifier * SKID_RADIAL_SOUNDPITCH_SCALE);

		if (fSkidVol > 0.08)
		{
			if (m_sounds[CAR_SOUND_SKID]->IsStopped())
				m_sounds[CAR_SOUND_SKID]->Play();

			//if(!m_sounds[CAR_SOUND_SURFACE]->IsStopped())
			//	m_sounds[CAR_SOUND_SURFACE]->Stop();
		}
		else
		{
			m_sounds[CAR_SOUND_SKID]->Stop();
		}
	}

	//
	// skip other sounds if that's not a car
	//
	if(!isCar)
		return;

	int controlButtons = GetControlButtons();

	if(!m_sirenEnabled)
	{
		if((controlButtons & IN_HORN) && !(m_oldControlButtons & IN_HORN) && m_sounds[CAR_SOUND_HORN]->IsStopped())
		{
			m_sounds[CAR_SOUND_HORN]->Play();
		}
		else if(!(controlButtons & IN_HORN) && (m_oldControlButtons & IN_HORN))
		{
			m_sounds[CAR_SOUND_HORN]->Stop();
		}
	}

	if(!m_enabled)
	{
		m_sounds[CAR_SOUND_ENGINE_IDLE]->Stop();
		m_sounds[CAR_SOUND_ENGINE]->Stop();
		m_sounds[CAR_SOUND_ENGINE_LOW]->Stop();

		if(m_sounds[CAR_SOUND_SIREN])
			m_sounds[CAR_SOUND_SIREN]->Stop();

		return;
	}

	// update engine sound pitch by RPM value
	float fRpmC = RemapValClamp(m_fEngineRPM, 600.0f, 10000.0f, 0.35f, 3.5f);

	m_sounds[CAR_SOUND_ENGINE]->SetPitch(fRpmC);
	m_sounds[CAR_SOUND_ENGINE_LOW]->SetPitch(fRpmC);

	m_sounds[CAR_SOUND_ENGINE_IDLE]->SetPitch(fRpmC + 0.65f);

	if(m_isLocalCar && m_sounds[CAR_SOUND_WHINE] != nullptr && IsDriveWheelsOnGround())
	{
		if(m_nGear == 0 )
		{
			if(m_sounds[CAR_SOUND_WHINE]->IsStopped())
				m_sounds[CAR_SOUND_WHINE]->Play();

			float wheelSpeedFac = fabs(GetSpeedWheels()) * KPH_TO_MPS * 0.225f;

			m_sounds[CAR_SOUND_WHINE]->SetPitch(wheelSpeedFac);

			if(wheelSpeedFac > 4.0f)
				wheelSpeedFac -= (wheelSpeedFac - 4.0f)*10.0f;

			m_sounds[CAR_SOUND_WHINE]->SetVolume(clamp(wheelSpeedFac*m_fAccelEffect, 0.0f, 1.0f));		
		}
		else
		{
			if(!m_sounds[CAR_SOUND_WHINE]->IsStopped())
				m_sounds[CAR_SOUND_WHINE]->Stop();
		}
	}
	else
	{
		if(m_sounds[CAR_SOUND_WHINE] != nullptr && !m_sounds[CAR_SOUND_WHINE]->IsStopped())
			m_sounds[CAR_SOUND_WHINE]->Stop();
	}

	m_engineIdleFactor = 1.0f - RemapValClamp(m_fEngineRPM, 0.0f, 2600.0f, 0.0f, 1.0f);

	float fEngineSoundVol = clamp((1.0f - m_engineIdleFactor), 0.45f, 1.0f);

	float fRPMDiff = 1.0f;
	
	if(m_isLocalCar)
	{
		fRPMDiff = (GetRPM() - m_fEngineRPM);
		fRPMDiff = RemapValClamp(fRPMDiff, 0.0f, 100.0f, 0.0f, 1.0f) + pow(fabs(m_fAccelEffect), 2.0f);
		fRPMDiff = min(fRPMDiff, 1.0f);
	}

#ifndef EDITOR
	if(g_pCameraAnimator->GetRealMode() == CAM_MODE_INCAR &&
		g_pGameSession->GetViewObject() == this)
	{
		SetSoundVolumeScale(0.5f);
	}
	else
		SetSoundVolumeScale(1.0f);
#endif // EDITOR

	m_sounds[CAR_SOUND_ENGINE]->SetVolume(fEngineSoundVol * fRPMDiff);
	m_sounds[CAR_SOUND_ENGINE_LOW]->SetVolume(fEngineSoundVol * (1.0f-fRPMDiff));
	m_sounds[CAR_SOUND_ENGINE_IDLE]->SetVolume(m_engineIdleFactor);

	if(m_engineIdleFactor <= 0.05f && !m_sounds[CAR_SOUND_ENGINE_IDLE]->IsStopped())
	{
		m_sounds[CAR_SOUND_ENGINE_IDLE]->Stop();
	}
	else if(m_engineIdleFactor > 0.05f && m_sounds[CAR_SOUND_ENGINE_IDLE]->IsStopped())
	{
		m_sounds[CAR_SOUND_ENGINE_IDLE]->Play();
	}

	if (m_sounds[CAR_SOUND_ENGINE]->IsStopped())
		m_sounds[CAR_SOUND_ENGINE]->Play();

	if (m_isLocalCar && m_sounds[CAR_SOUND_ENGINE_LOW]->IsStopped())
		m_sounds[CAR_SOUND_ENGINE_LOW]->Play();

	if( m_sounds[CAR_SOUND_SIREN] )
	{
		if(m_sirenEnabled != m_oldSirenState)
		{
			if(m_sirenEnabled)
			{
				m_sounds[CAR_SOUND_HORN]->Stop();
				m_sounds[CAR_SOUND_SIREN]->Play();
			}
			else
				m_sounds[CAR_SOUND_SIREN]->Stop();

			m_oldSirenState = m_sirenEnabled;
		}

		if( m_sirenEnabled && IsAlive() )
		{
			int sampleId = m_sounds[CAR_SOUND_SIREN]->GetEmitParams().sampleId;

			if((controlButtons & IN_HORN) && sampleId == 0)
				sampleId = 1;
			else if(!(controlButtons & IN_HORN) && sampleId == 1)
				sampleId = 0;

			if( m_sounds[CAR_SOUND_SIREN]->GetEmitParams().sampleId != sampleId )
			{
				m_sounds[CAR_SOUND_SIREN]->Stop();
				m_sounds[CAR_SOUND_SIREN]->GetEmitParams().sampleId = sampleId;
				m_sounds[CAR_SOUND_SIREN]->Play();
			}
		}
	}
}

float CCar::GetSpeedWheels() const
{
	if(!m_conf)
		return 0.0f;

	int wheelCount = GetWheelCount();

	float wheelFac = 1.0f / (float)wheelCount;
	float fResult = 0.0f;

	for(int i = 0; i < wheelCount; i++)
	{
		fResult += m_wheels[i].m_pitchVel;
	}

	return fResult * wheelFac * MPS_TO_KPH;
}

int	CCar::GetWheelsOnGround(int wheelFlags) const
{
	if (!m_conf)
		return 0;

	int wheelCount = GetWheelCount();
	int numOnGround = 0;

	for (int i = 0; i < wheelCount; i++)
	{
		if(m_conf->physics.wheels[i].flags & wheelFlags && 
			m_wheels[i].m_collisionInfo.fract < 1.0f)
			numOnGround++;
	}

	return numOnGround;
}

int	CCar::GetWheelCount() const
{
	return m_conf->physics.numWheels;
}

CCarWheel& CCar::GetWheel(int index) const
{
	return m_wheels[index];
}

float CCar::GetWheelSpeed(int index) const
{
	return m_wheels[index].m_pitchVel * MPS_TO_KPH;
}

float CCar::GetSpeed() const
{
	CEqRigidBody* carBody = m_physObj->GetBody();
	return length(carBody->GetLinearVelocity().xz()) * MPS_TO_KPH;
}

bool CCar::IsAccelerating() const
{
	return (GetSpeedWheels() > 0 ? (m_accelRatio > 0) : (m_brakeRatio > 0)) || IsBurningOut();
}

bool CCar::IsBraking() const
{
	return GetSpeedWheels() > 0 ? (m_brakeRatio > 0) : (m_accelRatio > 0);
}

bool CCar::IsHandbraking() const
{
	int controlButtons = GetControlButtons();
	return (controlButtons & IN_HANDBRAKE);
}

bool CCar::IsBurningOut() const
{
	int controlButtons = GetControlButtons();
	return (controlButtons & IN_BURNOUT);
}

float CCar::GetRPM() const
{
	return fabs(float(m_radsPerSec) * ( 60.0f / ( 2.0f * PI_F ) ) );
}

int CCar::GetGear() const
{
	return m_nGear;
}

float CCar::GetLateralSlidingAtBody() const
{
	CEqRigidBody* carBody = m_physObj->GetBody();

	Vector3D velocityAtCollisionPoint = carBody->GetLinearVelocity();

	float slip = dot(velocityAtCollisionPoint, GetRightVector());
	return slip;
}

float CCar::GetLateralSlidingAtWheels(bool surfCheck) const
{
	float val = 0.0f;

	int nCount = 0;

	int wheelCount = GetWheelCount();

	for(int i = 0; i < wheelCount; i++)
	{
		if(surfCheck)
		{
			if(m_wheels[i].m_surfparam && m_wheels[i].m_surfparam->word == 'C')
			{
				val += fabs(GetLateralSlidingAtWheel(i));
				nCount++;
			}
		}
		else
		{
			val += fabs(GetLateralSlidingAtWheel(i));
			nCount++;
		}
	}

	float inv_count = 1.0f;

	if(nCount > 0)
		inv_count = 1.0f / (float)nCount;

	return val * inv_count;
}

float CCar::GetLateralSlidingAtWheel(int wheel) const
{
	CEqRigidBody* carBody = m_physObj->GetBody();

	if(m_wheels[wheel].m_collisionInfo.fract >= 1.0f)
		return 0.0f;

	// compute wheel world rotation (turn + body rotation)
	Matrix4x4 wheelTransform = Matrix4x4(m_wheels[wheel].m_wheelOrient) * transpose(m_worldMatrix);

	Vector3D wheel_pos = wheelTransform.rows[3].xyz();

	Vector3D velocityAtCollisionPoint = carBody->GetVelocityAtWorldPoint(wheel_pos);

	// rotate vector to get right vector
	float slip = dot(velocityAtCollisionPoint, GetRightVector() );
	return slip;
}

float CCar::GetTractionSliding(bool surfCheck) const
{
	float val = 0.0f;

	int nCount = 0;

	int wheelCount = GetWheelCount();

	for(int i = 0; i < wheelCount; i++)
	{
		if(surfCheck)
		{
			if(m_wheels[i].m_surfparam && m_wheels[i].m_surfparam->word == 'C')
			{
				val += fabs(GetTractionSlidingAtWheel(i));
				nCount++;
			}
		}
		else
		{
			val += fabs(GetTractionSlidingAtWheel(i));
			nCount++;
		}
	}

	float inv_count = 1.0f;

	if(nCount > 0)
		inv_count = 1.0f / (float)nCount;

	return val * inv_count;
}

float CCar::GetTractionSlidingAtWheel(int wheel) const
{
	if(!m_wheels[wheel].m_flags.onGround)
		return 0.0f;

	return abs(GetSpeed() - abs(GetWheelSpeed(wheel))) + m_wheels[wheel].m_flags.isBurningOut*m_conf->physics.burnoutMaxSpeed;
}

bool CCar::IsAnyWheelOnGround() const
{
	int numWheels = GetWheelCount();

	for(int i = 0; i < numWheels; i++)
	{
		if(m_wheels[i].m_collisionInfo.fract < 1.0f)
			return true;
	}

	return false;
}

bool CCar::IsDriveWheelsOnGround() const
{
	int numWheels = GetWheelCount();

	for(int i = 0; i < numWheels; i++)
	{
		if(!(m_conf->physics.wheels[i].flags & WHEEL_FLAG_DRIVE))
			continue;

		if(m_wheels[i].m_collisionInfo.fract < 1.0f)
			return true;
	}

	return false;
}


ConVar r_drawCars("r_drawCars", "1", "Render vehicles", CV_CHEAT);

extern ConVar r_enableObjectsInstancing;

bool CCar::GetBodyDamageValuesMappedToBones(float damageVals[16]) const
{
	bool applyDamage = false;

	for (int i = 0; i < CB_PART_WINDOW_PARTS; i++)
	{
		int idx = m_bodyParts[i].boneIndex;

		if (idx == -1)
			continue;

		damageVals[idx] = m_bodyParts[i].damage;

		if (m_bodyParts[i].damage > 0)
			applyDamage = true;
	}

	return applyDamage;
}

void CCar::DrawBody( int nRenderFlags, int nLOD)
{
	if( !m_pModel )
		return;

	studiohdr_t* pHdr = m_pModel->GetHWData()->studio;

	if(nLOD > 0 && r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		for(int i = 0; i < MAX_INSTANCE_BODYGROUPS; i++)
		{
			if(!(m_bodyGroupFlags & (1 << i)))
				continue;

			gameObjectInstance_t& inst = instancer->NewInstance( i , nLOD );
			inst.world = m_worldMatrix;
		}
		
		// g_pShaderAPI->SetShaderConstantVector4D("CarColor", m_carColor);
		return;
	}

	//--------------------------------------------------------

	ColorRGBA defaultColors[2] = { color4_white , color4_white };
	ColorRGBA* bodyColors = (ColorRGBA*)&m_carColor.col1;

	ColorRGBA* colors = m_conf->useBodyColor ? bodyColors : defaultColors;

	float bodyDamages[16] = { 0.0f };
	bool canApplyDamage = GetBodyDamageValuesMappedToBones(bodyDamages);

	IEqModel* damagedModel = (m_pDamagedModel && canApplyDamage) ? m_pDamagedModel : m_pModel;

	for(int i = 0; i < pHdr->numBodyGroups; i++)
	{
		// check bodygroups for rendering
		if(!(m_bodyGroupFlags & (1 << i)))
			continue;

		int bodyGroupLOD = nLOD;
		int nLodModelIdx = pHdr->pBodyGroups(i)->lodModelIndex;
		studiolodmodel_t* lodModel = pHdr->pLodModel(nLodModelIdx);

		int nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = lodModel->modelsIndexes[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;

		studiomodeldesc_t* modDesc = pHdr->pModelDesc(nModDescId);

		// render model groups that in this body group
		for(int j = 0; j < modDesc->numGroups; j++)
		{
			int materialIndex = modDesc->pGroup(j)->materialIndex;
			materials->BindMaterial( m_pModel->GetMaterial(materialIndex), 0);

			// setup our brand new vertex format
			// and bind required VBO streams by hand
			g_pShaderAPI->SetVertexFormat(g_worldGlobals.vehicleVertexFormat);
			m_pModel->SetupVBOStream(0);
			damagedModel->SetupVBOStream(1);

			// instead of prepare skinning, we send BodyDamage and car colours
			g_pShaderAPI->SetShaderConstantArrayFloat("BodyDamage", bodyDamages, 16);
			g_pShaderAPI->SetShaderConstantArrayVector4D("CarColor", bodyColors, 2);

			// then draw all of this shit
			m_pModel->DrawGroup( nModDescId, j, false );
		}
	}
}

void CCar::DrawShadow(float distance)
{
	Vector3D rightVec = GetRightVector();
	Vector3D forwardVec = GetForwardVector();
	TexAtlasEntry_t* carShadow = g_worldGlobals.veh_shadow;

	Vector2D shadowSize(m_conf->physics.body_size.x+0.35f, m_conf->physics.body_size.z+0.25f);

	if(distance < r_carShadowDetailDistance.GetFloat())
	{
		Rectangle_t flipRect = carShadow ? carShadow->rect : Rectangle_t(0,0,1,1);

		// project from top
		Matrix4x4 proj, view, viewProj;
		proj = orthoMatrix(-shadowSize.x, shadowSize.x, -shadowSize.y, shadowSize.y, -1.5f, 1.0f);
		view = Matrix4x4( rotateX3(DEG2RAD(-90)) * !m_worldMatrix.getRotationComponent());
		view.translate(-GetOrigin());

		viewProj = proj*view;

		decalPrimitives_t shadowDecal;
		shadowDecal.settings.avoidMaterialFlags = MATERIAL_FLAG_WATER; // only avoid water
		shadowDecal.settings.facingDir = vec3_up;

		// might be slow on mobile device
		shadowDecal.settings.processFunc = LightDecalTriangleProcessFunc;

		ProjectDecalToSpriteBuilder(shadowDecal, g_vehicleShadows, flipRect, viewProj, ColorRGBA(1.0f,1.0f,1.0f,1.0f));
	}
	else
	{
		PFXVertex_t* verts;
		if(carShadow && g_vehicleEffects->AllocateGeom(4,4, &verts, NULL, true) != -1)
		{
			verts[0].point = GetOrigin() + shadowSize.y*forwardVec + shadowSize.x*rightVec;
			verts[1].point = GetOrigin() + shadowSize.y*forwardVec + shadowSize.x*-rightVec;
			verts[2].point = GetOrigin() + shadowSize.y*-forwardVec + shadowSize.x*rightVec;
			verts[3].point = GetOrigin() + shadowSize.y*-forwardVec + shadowSize.x*-rightVec;

			verts[0].texcoord = carShadow->rect.GetRightTop();
			verts[1].texcoord = carShadow->rect.GetLeftTop();
			verts[2].texcoord = carShadow->rect.GetRightBottom();
			verts[3].texcoord = carShadow->rect.GetLeftBottom();

			eqPhysCollisionFilter collFilter;
			collFilter.flags = EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC;

			CollisionData_t shadowcoll;
			for(int i = 0; i < 4; i++)
			{
				Vector3D traceFrom = verts[i].point;
				Vector3D traceTo = traceFrom - Vector3D(0.0f,5.0f,0.0f);

				g_pPhysics->TestLine(traceFrom, traceTo, shadowcoll, OBJECTCONTENTS_SOLID_GROUND, &collFilter);

				verts[i].point = lerp(traceFrom, traceTo+Vector3D(0,0.01f,0), shadowcoll.fract);

				verts[i].color = ColorRGBA(1.0f, 1.0f, 1.0f, clamp(0.8f + 1.0f-(shadowcoll.fract*8.0f), 0.0f, 1.0f));
			}
		}
	}
}

void CCar::Draw( int nRenderFlags )
{
	if(!r_drawCars.GetBool())
		return;

	PROFILE_FUNC();

	bool isBodyDrawn = true;

#ifndef EDITOR
	// don't render car
	if(	!Director_FreeCameraActive() &&
		g_pCameraAnimator->GetRealMode() == CAM_MODE_INCAR &&
		g_pGameSession->GetViewObject() == this)
		isBodyDrawn = false;
#endif // EDITOR
	{

		float camDist = g_pGameWorld->m_view.GetLODScaledDistFrom(GetOrigin());
		int nLOD = m_pModel->SelectLod(camDist); // lod distance check

		CEqRigidBody* pCarBody = m_physObj->GetBody();

		if (isBodyDrawn)
		{
			// draw fake shadow
			DrawShadow(camDist);

			if (m_hasDriver && nLOD == 0)
			{
				m_driverModel.m_worldMatrix = m_worldMatrix * translate(m_conf->visual.driverPosition);
				m_driverModel.Draw(nRenderFlags);
			}

			materials->SetMatrix(MATRIXMODE_WORLD, m_worldMatrix);

			// draw car body with damage effects
			DrawBody(nRenderFlags, nLOD);
		}

		// draw wheels
		if (!pCarBody->IsFrozen())
			m_shadowDecal.dirty = true;

		pCarBody->UpdateBoundingBoxTransform();
		m_bbox = pCarBody->m_aabb_transformed;

		if (nLOD == 0)
		{
			int numWheels = GetWheelCount();

			for (int i = 0; i < numWheels; i++)
			{
				CCarWheel& wheel = m_wheels[i];
				carWheelConfig_t& wheelConf = m_conf->physics.wheels[i];

				wheel.CalculateTransform(wheel.m_worldMatrix, wheelConf);
				wheel.m_worldMatrix = m_worldMatrix * wheel.m_worldMatrix;

				wheel.SetOrigin(transpose(wheel.m_worldMatrix).getTranslationComponent());
				wheel.m_bbox = m_bbox;

				if (isBodyDrawn)
					wheel.Draw(nRenderFlags);

				DrawWheelEffects(i);
			}
		}
	}
}

CGameObject* CCar::GetChildShadowCaster(int idx) const
{
	return &m_wheels[idx];
}

int CCar::GetChildCasterCount() const
{
#ifndef EDITOR
	// don't render car
	if(	g_pCameraAnimator->GetRealMode() == CAM_MODE_INCAR &&
		g_pGameSession->GetViewObject() == this)
		return 0;
#endif // EDITOR

	return m_conf->physics.numWheels;
}

void CCar::OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList)
{
	// send extra vars
	half damageFloats[CB_PART_COUNT];

	for(int i = 0; i < CB_PART_COUNT; i++)
		damageFloats[i] = m_bodyParts[i].damage;

	buffer->WriteData(damageFloats, sizeof(half)*CB_PART_COUNT);

	BaseClass::OnPackMessage(buffer, changeList);
}

void CCar::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	// recv extra vars
	half damageFloats[CB_PART_COUNT];

	buffer->ReadData(damageFloats, sizeof(half)*CB_PART_COUNT);

	for(int i = 0; i < CB_PART_COUNT; i++)
		m_bodyParts[i].damage = damageFloats[i];

	BaseClass::OnUnpackMessage(buffer);
	
	RefreshWindowDamageEffects();
	UpdateLightsState();
}

//---------------------------------------------------------------------------
// Gameplay-related
//---------------------------------------------------------------------------

void CCar::OnDeath( CGameObject* deathBy )
{
	// TODO: set engine on fire

#ifndef EDITOR
	int deathByReplayId = deathBy ? deathBy->m_replayID : REPLAY_NOT_TRACKED;
	g_replayData->PushEvent( REPLAY_EVENT_CAR_DEATH, m_replayID, (void*)(intptr_t)deathByReplayId );
#endif // EDITOR

#ifndef NO_LUA
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);

	if( m_luaOnDeath.Push() )
	{
		if(deathBy != NULL)
		{
			switch(deathBy->ObjType())
			{
				case GO_CAR:
					OOLUA::push(state, (CCar*)deathBy);
					break;
				default:
					OOLUA::push(state, deathBy);
					break;
			}
		}
		else
			lua_pushnil(state);

		if(!m_luaOnDeath.Call(1, 0, 0))
		{
			MsgError("CGameObject:OnDeath error:\n %s\n", OOLUA::get_last_error(state).c_str());
		}
	}
#endif // NO_LUA
}

bool CCar::IsAlive() const
{
	return m_gameDamage < m_gameMaxDamage;
}

bool CCar::IsInWater() const
{
	return m_inWater;
}

bool CCar::IsFlippedOver( bool checkWheels ) const
{
	if(checkWheels)
		return !IsAnyWheelOnGround() || dot(vec3_up, GetUpVector()) < FLIPPED_TOLERANCE_COSINE;

	return dot(vec3_up, GetUpVector()) < FLIPPED_TOLERANCE_COSINE;
}

void CCar::SetMaxDamage( float fDamage )
{
	m_gameMaxDamage = fDamage;
}

float CCar::GetMaxDamage() const
{
	return m_gameMaxDamage;
}

void CCar::SetMaxSpeed( float fSpeed )
{
	m_maxSpeed = fSpeed;
}

float CCar::GetMaxSpeed() const
{
	return m_maxSpeed;
}

void CCar::SetTorqueScale( float fScale )
{
	m_torqueScale = fScale;
}

float CCar::GetTorqueScale() const
{
	return m_torqueScale;
}

void CCar::SetDamage( float damage )
{
	bool wasAlive = IsAlive();
	m_gameDamage = damage;

	if(m_gameDamage > m_gameMaxDamage)
		m_gameDamage = m_gameMaxDamage;
}

float CCar::GetDamage() const
{
	return m_gameDamage;
}

float CCar::GetBodyDamage() const // max. 6.0f
{
	float fDamage = 0.0f;

	for(int i = 0; i < CB_PART_COUNT; i++)
		fDamage += m_bodyParts[i].damage;

	return fDamage;
}

void CCar::Repair(bool unlock)
{
	SetDamage(0.0f);

	// visual damage
	for(int i = 0; i < CB_PART_COUNT; i++)
		m_bodyParts[i].damage = 0.0f;

	// restore hubcaps
	for(int i = 0; i < GetWheelCount(); i++)
	{
		m_wheels[i].m_damage = 0.0f;
		m_wheels[i].m_hubcapLoose = 0.0f;
		m_wheels[i].m_flags.lostHubcap = false;
		m_wheels[i].m_bodyGroupFlags = (1 << m_wheels[i].m_defaultBodyGroup);
	}

	if(unlock)
		m_locked = false;

	RefreshWindowDamageEffects();
}

void CCar::SetCarColour(const carColorScheme_t& col) 
{
	m_carColor.CopyFrom(col); 
}

carColorScheme_t CCar::GetCarColour() const
{
	return m_carColor; 
}

void CCar::SetColorScheme( int colorIdx )
{
	if(colorIdx >= 0 && colorIdx < m_conf->numColors)
		m_carColor.CopyFrom(m_conf->colors[colorIdx]);
	else
		m_carColor.CopyFrom(ColorRGBA(1,1,1,1));
}

void CCar::SetAutoHandbrake( bool enable )
{
	m_autohandbrake = enable;
}

bool CCar::HasAutoHandbrake() const
{
	return m_autohandbrake;
}

void CCar::SetAutoGearSwitch(bool enable)
{
	m_autogearswitch = enable;
}

bool CCar::HasAutoGearSwitch() const
{
	return m_autogearswitch;
}

void CCar::SetInfiniteMass( bool infMass )
{
	if(m_physObj == nullptr)
		return;

	if(infMass)
		m_physObj->GetBody()->m_flags |= BODY_INFINITEMASS;
	else
		m_physObj->GetBody()->m_flags &= ~BODY_INFINITEMASS;
}

bool CCar::HasInfiniteMass() const
{
	return (m_physObj->GetBody()->m_flags & BODY_INFINITEMASS) > 0;
}

void CCar::Lock(bool lock)
{
	m_locked = lock;
}

bool CCar::IsLocked() const
{
	return m_locked;
}

void CCar::Enable(bool enable)
{
	m_enabled = enable;
	UpdateLightsState();
}

bool CCar::IsEnabled() const
{
	return m_enabled;
}

void CCar::SetFelony(float percentage)
{
	if (percentage > 1.0f)
		percentage = 1.0f;

	m_pursuerData.felonyRating = percentage;
}

float CCar::GetFelony() const
{
	return m_pursuerData.felonyRating;
}

void CCar::IncrementPursue()
{
	m_pursuerData.pursuedByCount++;
}

void CCar::DecrementPursue()
{
	m_pursuerData.pursuedByCount--;

	if (m_pursuerData.pursuedByCount == 0)
	{ 
		memset(&m_pursuerData.lastInfractionTime, 0, sizeof(m_pursuerData.lastInfractionTime));
		memset(&m_pursuerData.hasInfraction, 0, sizeof(m_pursuerData.hasInfraction));
		m_pursuerData.announced = false;
	}
		
}

int CCar::GetPursuedCount() const
{
	return m_pursuerData.pursuedByCount;
}

PursuerData_t& CCar::GetPursuerData()
{
	return m_pursuerData;
}

void CCar::TurnOffLights()
{
	m_lightsEnabled = 0;
}

void CCar::SetLight(int light, bool enabled)
{
	if(enabled)
		m_lightsEnabled |= light;
	else
		m_lightsEnabled &= ~light;
}

bool CCar::IsLightEnabled(int light) const
{
	return (m_lightsEnabled & light) > 0;
}

#ifndef NO_LUA
void CCar::L_RegisterEventHandler(const OOLUA::Table& tableRef)
{
	BaseClass::L_RegisterEventHandler(tableRef);
	m_luaOnCollision.Get(m_luaEvtHandler, "OnCollide", true);
	m_luaOnDeath.Get(m_luaEvtHandler, "OnDeath", true);
}
#endif // NO_LUA

#ifndef NO_LUA
OOLUA_EXPORT_FUNCTIONS(
	CCar,
	AlignToGround,
	SetColorScheme,
	SetMaxDamage,
	SetMaxSpeed,
	SetTorqueScale,
	SetDamage,
	Repair,
	SetLight,
	SetFelony,
	Lock,
	Enable,
	SetInfiniteMass,
	HingeVehicle,
	ReleaseHingedVehicle
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CCar,
	IsAlive,
	IsInWater,
	IsFlippedOver,
	IsAnyWheelOnGround,
	GetMaxDamage,
	GetMaxSpeed,
	GetTorqueScale,
	GetDamage,
	GetFelony,
	GetBodyDamage,
	GetAngularVelocity,
	GetForwardVector,
	GetRightVector,
	GetUpVector,
	GetSpeed,
	GetSpeedWheels,
	GetLateralSliding,
	GetTractionSliding,
	IsAccelerating,
	IsBraking,
	IsHandbraking,
	IsBurningOut,
	IsLocked,
	IsEnabled,
	GetPursuedCount,
	GetHingedVehicle
)

#endif // NO_LUA
