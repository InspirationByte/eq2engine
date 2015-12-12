//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#include "car.h"
//#include "math/math_common.h"

#include "DebugOverlay.h"
#include "session_stuff.h"
#include "EqParticles.h"
#include "replay.h"

#include "Shiny.h"

#define DEFAULT_CAMERA_FOV			(60.0f) // old: 52
#define CAMERA_MIN_FOV				(50.0f)
#define CAMERA_MAX_FOV				(90.0f)

extern int g_CurrCameraMode;

bool ParseCarConfig( carConfigEntry_t* conf, const kvkeybase_t* kvs )
{
	conf->m_cleanModelName = KV_GetValueString(kvs->FindKeyBase("cleanmodel"));
	conf->m_damModelName = KV_GetValueString(kvs->FindKeyBase("damagedmodel"));

	const char* defaultWheelName = KV_GetValueString(kvs->FindKeyBase("wheeltype"), 0, "wheel1");

	conf->m_body_size = KV_GetVector3D(kvs->FindKeyBase("bodysize"));
	conf->m_body_center = KV_GetVector3D(kvs->FindKeyBase("center"));
	conf->m_virtualMassCenter = KV_GetVector3D(kvs->FindKeyBase("gravitycenter"));

	conf->m_differentialRatio = KV_GetValueFloat(kvs->FindKeyBase("differential"));
	conf->m_torqueMult = KV_GetValueFloat(kvs->FindKeyBase("torquemultipler"));
	conf->m_transmissionRate = KV_GetValueFloat(kvs->FindKeyBase("transmissionrate"));

	conf->m_maxSpeed = KV_GetValueFloat(kvs->FindKeyBase("maxspeed"), 0, 110);

	//conf->m_differentialRatio *= 2.7f;
	//conf->m_transmissionRate *= 0.34f;

	conf->m_mass = KV_GetValueFloat(kvs->FindKeyBase("mass"));

	conf->m_antiRoll = KV_GetValueFloat(kvs->FindKeyBase("antiroll"));

	float suspensionLift = KV_GetValueFloat(kvs->FindKeyBase("suspensionLift"));

	kvkeybase_t* pGears = kvs->FindKeyBase("gears");

	if(pGears)
	{
		for(int i = 0; i < pGears->keys.numElem(); i++)
		{
			float fRatio = KV_GetValueFloat( pGears->keys[i]);
			conf->m_gears.append(fRatio);
		}
	}
	else
		MsgError("no gears found in file!");


	kvkeybase_t* pWheels = kvs->FindKeyBase("wheels");

	if( pWheels )
	{
		// time for wheels
		for(int i = 0; i < pWheels->keys.numElem(); i++)
		{
			if(!pWheels->keys[i]->IsSection())
				continue;

			kvkeybase_t* pWheelKv = pWheels->keys[i];

			Vector3D suspensiontop = KV_GetVector3D(pWheelKv->FindKeyBase("SuspensionTop"));
			Vector3D suspensionbottom = KV_GetVector3D(pWheelKv->FindKeyBase("SuspensionBottom"));

			float wheel_width = KV_GetValueFloat(pWheelKv->FindKeyBase("width"), 0.0f, 1.0f);

			float rot_sign = (suspensiontop.x > 0) ? 1 : -1;
			float widthAdd = (wheel_width / 2)*rot_sign;

			suspensiontop.x += widthAdd;
			suspensionbottom.x += widthAdd;
			//suspensiontop.y += suspensionLift;
			suspensionbottom.y += suspensionLift;

			bool bDriveWheel = KV_GetValueBool(pWheelKv->FindKeyBase("drive"));
			bool bHandbrakeWheel = KV_GetValueBool(pWheelKv->FindKeyBase("handbrake"));
			bool bSteerWheel = fabs( KV_GetValueFloat(pWheelKv->FindKeyBase("steer")) ) > 0.0f;

			carWheelConfig_t wconf;

			wconf.radius = KV_GetValueFloat(pWheelKv->FindKeyBase("radius"), 0, 1.0f);
			wconf.width = wheel_width;
			wconf.woffset = widthAdd;

			const char* wheelName = KV_GetValueString(kvs->FindKeyBase("wheelname"), 0, NULL);

			if(wheelName)
				strncpy(wconf.wheelName, wheelName, 16 );
			else
				strncpy(wconf.wheelName, defaultWheelName, 16 );

			wconf.flags = (bDriveWheel ? WHEEL_FLAG_DRIVE : 0) | (bHandbrakeWheel ? WHEEL_FLAG_HANDBRAKE : 0) | (bSteerWheel ? WHEEL_FLAG_STEER : 0);
			wconf.springConst = KV_GetValueFloat(pWheelKv->FindKeyBase("SuspensionSpringConstant"), 0, 100.0f);
			wconf.dampingConst = KV_GetValueFloat(pWheelKv->FindKeyBase("SuspensionDampingConstant"), 0, 50.0f);
			wconf.suspensionTop = suspensiontop;
			wconf.suspensionBottom = suspensionbottom;
			wconf.brakeTorque = KV_GetValueFloat(pWheelKv->FindKeyBase("BrakeTorque"), 0, 100.0f);
			wconf.visualTop = KV_GetValueFloat(pWheelKv->FindKeyBase("visualTop"), 0, 0.7);
			wconf.steerMultipler = KV_GetValueFloat(pWheelKv->FindKeyBase("steer"));

			conf->m_wheels.append(wconf);
		}
	}

	#define CAMERA_DISTANCE_BIAS 0.25f

	conf->m_cameraConf.dist = KV_GetValueFloat(kvs->FindKeyBase("camera_distance"), 0, 7.0f);
	conf->m_cameraConf.distInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_distance_in"), 0, conf->m_body_size.z - CAMERA_DISTANCE_BIAS);
	conf->m_cameraConf.height = KV_GetValueFloat(kvs->FindKeyBase("camera_height"), 0, 1.3f);
	conf->m_cameraConf.heightInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_height_in"), 0, conf->m_body_center.y);
	conf->m_cameraConf.widthInCar = KV_GetValueFloat(kvs->FindKeyBase("camera_side_in"), 0, conf->m_body_size.x - CAMERA_DISTANCE_BIAS);
	conf->m_cameraConf.fov = KV_GetValueFloat(kvs->FindKeyBase("camera_fov"), 0, DEFAULT_CAMERA_FOV);

	// parse visuals
	kvkeybase_t* visuals = kvs->FindKeyBase("visuals");

	if(visuals)
	{
		conf->m_sirenType = KV_GetValueInt(visuals->FindKeyBase("siren_lights"), 0, SIREN_NONE);
		conf->m_sirenPositionWidth = KV_GetVector4D(visuals->FindKeyBase("siren_lights"), 1, vec4_zero);

		conf->m_headlightType = KV_GetValueInt(visuals->FindKeyBase("headlights"), 0, LIGHTS_SINGLE);
		conf->m_headlightPosition = KV_GetVector4D(visuals->FindKeyBase("headlights"), 1, vec4_zero);

		conf->m_backlightPosition = KV_GetVector4D(visuals->FindKeyBase("backlights"), 0, vec4_zero);

		conf->m_brakelightType = KV_GetValueInt(visuals->FindKeyBase("brakelights"), 0, LIGHTS_SINGLE);
		conf->m_brakelightPosition = KV_GetVector4D(visuals->FindKeyBase("brakelights"), 1, vec4_zero);

		conf->m_enginePosition = KV_GetVector3D(visuals->FindKeyBase("engine"), 0, vec3_zero);
	}


	kvkeybase_t* colors = kvs->FindKeyBase("colors");

	if(colors)
	{
		for(int i = 0; i < colors->keys.numElem(); i++)
		{
			kvkeybase_t* colKey = colors->keys[i];

			carColorScheme_t colScheme;

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

				conf->m_colors.append(colScheme);
			}
			else
			{	// simple colors
				ColorRGBA c1 = KV_GetVector4D(colKey, 0, color4_white);
				
				if( c1.w > 1.0f )
					c1 /= 255.0f;

				colScheme.col1 = c1;
				colScheme.col2 = colScheme.col1;

				conf->m_colors.append(colScheme);
			}
		}
	}

	conf->m_useBodyColor = KV_GetValueBool(kvs->FindKeyBase("useBodyColor"), 0, true) && (conf->m_colors.numElem() > 0);

	kvkeybase_t* pSoundSec = kvs->FindKeyBase("sounds", KV_FLAG_SECTION);

	if(pSoundSec)
	{
		conf->m_sndEngineIdle = KV_GetValueString(pSoundSec->FindKeyBase("engine_idle"), 0, "defaultcar.engine_idle");

		conf->m_sndEngineRPMLow = KV_GetValueString(pSoundSec->FindKeyBase("engine_low"), 0, "defaultcar.engine_low");
		conf->m_sndEngineRPMHigh = KV_GetValueString(pSoundSec->FindKeyBase("engine"), 0, "defaultcar.engine");

		conf->m_sndHornSignal = KV_GetValueString(pSoundSec->FindKeyBase("horn"), 0, "generic.horn1");
		conf->m_sndSiren = KV_GetValueString(pSoundSec->FindKeyBase("siren"), 0, "generic.copsiren1");
	}

	return true;
}

#pragma todo("better gearbox code for shifting effect")
#pragma todo("make correct lateral sliding on steering wheels")
#pragma todo("non-realtime FindAtlasTexture lookups")

#define ANTIROLL_FACTOR_DEADZONE	(0.01f)
#define ANTIROLL_FACTOR_MAX			(1.0f)
#define ANTIROLL_SCALE				(4.0f)

#define MIN_VISUAL_BODYPART_DAMAGE	(0.32f)

#define DAMAGE_MINIMPACT			(0.45f)
#define DAMAGE_SCALE				(0.12f)
#define	DAMAGE_VISUAL_SCALE			(0.75f)

#define DAMAGE_SOUND_SCALE			(0.25f)

#define	DAMAGE_WATERDAMAGE_RATE		(5.5f)

#define CAR_DEFAULT_MAX_DAMAGE		(16.0f)

#define SKIDMARK_MAX_LENGTH			(256)
#define SKIDMARK_MIN_INTERVAL		(0.25f)

// wheel friction modifier on diferrent weathers
static float weatherTireFrictionMod[WEATHER_COUNT] = 
{
	1.0f,
	0.8f,
	0.75f
};

ConVar g_night("g_night", "0");
ConVar g_autohandbrake("g_autohandbrake", "1", "Auto handbrake steering helper", CV_CHEAT);

extern CPFXAtlasGroup* g_vehicleEffects;
extern CPFXAtlasGroup* g_translParticles;
extern CPFXAtlasGroup* g_additPartcles;

void DrawLightEffect(const Vector3D& position, const ColorRGBA& color, float size, int type = 0)
{
	PFXBillboard_t effect;

	effect.vColor = color;

	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_CameraParams.GetAngles().y;

	effect.group = g_additPartcles;

	effect.fWide = size;
	effect.fTall = size;

	effect.vOrigin = position;

	if(type == 0)
		effect.tex = g_additPartcles->FindEntry("light1");
	else if(type == 1)
		effect.tex = g_additPartcles->FindEntry("glow2");
	else if(type == 2)
		effect.tex = g_additPartcles->FindEntry("light1a");
	else if(type == 3)
		effect.tex = g_additPartcles->FindEntry("glow1");

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
}

void PoliceSirenEffect(float fCurTime, const ColorRGB& color, const Vector3D& pos, const Vector3D& dir_right, float rDist, float width)
{
	PFXBillboard_t effect;

	float fSinFactor = sinf(fCurTime*16.0f);

	effect.vColor = Vector4D(color * fSinFactor, 1.0f);
	effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
	effect.fZAngle = g_pGameWorld->m_CameraParams.GetAngles().y;

	effect.group = g_additPartcles;

	float min_size = 0.025f;

	float max_size = 0.55f;
	float max_glow_size = 1.05f;

	// TODO: trace particle visibility

	effect.fWide = lerp(min_size, max_size, fabs(fSinFactor));
	effect.fTall = lerp(min_size, max_size, fabs(fSinFactor));

	effect.vOrigin = pos + dir_right*rDist - dir_right*width;
	effect.tex = g_additPartcles->FindEntry("light1");

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
	effect.tex = g_additPartcles->FindEntry("glow1");
	effect.fWide = lerp(min_size, max_glow_size, fabs(fSinFactor));
	effect.fTall = lerp(min_size, max_glow_size, fabs(fSinFactor));
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

	float fCosFactor = sinf((fCurTime*16.0f)+PI_F);

	effect.vColor = Vector4D(color * fCosFactor, 1.0f);
	effect.fWide = lerp(min_size, max_size, fabs(fCosFactor));
	effect.fTall = lerp(min_size, max_size, fabs(fCosFactor));

	effect.vOrigin = pos + dir_right*rDist + dir_right*width;
	effect.tex = g_additPartcles->FindEntry("light1a");

	// no frustum for now
	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
	effect.tex = g_additPartcles->FindEntry("glow1");
	effect.fWide = lerp(min_size, max_glow_size, fabs(fCosFactor));
	effect.fTall = lerp(min_size, max_glow_size, fabs(fCosFactor));

	Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);
}

class CFleckEffect : public IEffect
{
public:
	CFleckEffect(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity, float StartSize, float lifetime, int nMaterial, float rotation, const ColorRGB& color, CPFXAtlasGroup* group, TexAtlasEntry_t* entry)
	{
		InternalInit(position, lifetime, nMaterial);

		fStartSize = StartSize;

		m_vVelocity = velocity;

		rotate = rotation;

		nDraws = 0;

		m_vGravity = gravity;

		m_vLastColor = color * g_pGameWorld->m_info.ambientColor.xyz()*2.0f;

		m_group = group;
		m_entry = entry;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime*1.25f;

		m_vVelocity += m_vGravity*dTime*1.25f;

		SetSortOrigin(m_vOrigin);

		float lifeTimePerc = GetLifetimePercent();

		PFXBillboard_t effect;

		effect.group = m_group;
		effect.tex = m_entry;

		effect.vOrigin = m_vOrigin;
		effect.vColor = Vector4D(m_vLastColor,lifeTimePerc*2);
		effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;

		effect.fZAngle = lifeTimePerc*rotate;

		//internaltrace_t tr;
		//physics->InternalTraceLine(m_vOrigin,m_vOrigin+normalize(m_vVelocity), COLLISION_GROUP_WORLD | COLLISION_GROUP_OBJECTS, &tr);
		/*
		CollisionData_t coll;
		g_pPhysics->TestLine(m_vOrigin, m_vOrigin+normalize(m_vVelocity), coll);

		if(coll.fract < 1.0f)
		{
			m_vVelocity = reflect(m_vVelocity, coll.normal)*0.25f;
			m_fLifeTime -= dTime*8;

			effect.fZAngle = 0.0f;
		}*/
		
		effect.fWide = fStartSize;
		effect.fTall = fStartSize;

		Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

		nDraws++;

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_vVelocity;
	Vector3D	m_vGravity;

	Vector3D	m_vLastColor;

	float		fStartSize;
	float		rotate;

	int			nDraws;

	CPFXAtlasGroup*		m_group;
	TexAtlasEntry_t*	m_entry;
};

class CSmokeEffect : public IEffect
{
public:
	CSmokeEffect(const Vector3D &position, const Vector3D &velocity, float StartSize, float EndSize, float lifetime, int nMaterial, float rotation, const Vector3D &gravity = vec3_zero, const Vector3D &color1 = color3_white, const Vector3D &color2 = color3_white, float alpha = 1.0f)
	{
		InternalInit( position, lifetime, nMaterial );

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		rotate = rotation;

		m_vGravity = gravity;

		m_color1 = color1;
		m_color2 = color2;

		m_alpha = alpha;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		m_vOrigin += m_vVelocity*dTime;

		m_vCurrColor = (g_pGameWorld->m_info.ambientColor.xyz()+g_pGameWorld->m_info.sunColor.xyz());

		float lifeTimePerc = GetLifetimePercent();

		float fStartAlpha = clamp((1.0f-lifeTimePerc) * 50.0f, 0.0f, 1.0f);

		Vector3D col_lerp = lerp(m_color2, m_color1, lifeTimePerc);

		m_vVelocity += m_vGravity*dTime;

		SetSortOrigin(m_vOrigin);

		PFXBillboard_t effect;

		effect.vOrigin = m_vOrigin;
		effect.vColor = Vector4D(m_vCurrColor * col_lerp, lifeTimePerc*m_alpha*fStartAlpha);
		effect.group = g_translParticles;
		effect.tex = g_translParticles->FindEntry("smoke2");
		effect.nFlags = EFFECT_FLAG_NO_FRUSTUM_CHECK;
		effect.fZAngle = lifeTimePerc*rotate;

		effect.fWide = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);
		effect.fTall = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		// no frustum for now
		Effects_DrawBillboard(&effect, &g_pGameWorld->m_CameraParams, NULL);

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	m_color1;
	Vector3D	m_color2;

	Vector3D	m_vGravity;

	Vector3D	m_vVelocity;

	Vector3D	m_vCurrColor;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		rotate;

	float		m_alpha;
};

class CSparkLine : public IEffect
{
public:
	CSparkLine(const Vector3D &position, const Vector3D &velocity, const Vector3D &gravity, float length, float StartSize, float EndSize, float lifetime, CPFXAtlasGroup* group, TexAtlasEntry_t* entry)
	{
		InternalInit(position, lifetime, -1);

		fCurSize = StartSize;
		fStartSize = StartSize;
		fEndSize = EndSize;

		m_vVelocity = velocity;

		fLength = length;

		vGravity = gravity;

		m_group = group;
		m_entry = entry;
	}

	bool DrawEffect(float dTime)
	{
		m_fLifeTime -= dTime;

		if(m_fLifeTime <= 0)
			return false;

		PFXVertex_t* verts;
		if(m_group->AllocateGeom( 4, 4, &verts, NULL, true ) < 0)
			return false;

		m_vOrigin += m_vVelocity*dTime*2.0f;

		SetSortOrigin(m_vOrigin);

		float lifeTimePerc = GetLifetimePercent();

		Vector3D viewDir = fastNormalize(m_vOrigin - effectrenderer->GetViewSortPosition());
		Vector3D lineDir = m_vVelocity;

		Vector3D vEnd = m_vOrigin + fastNormalize(m_vVelocity)*(fLength*length(m_vVelocity)*0.001f);

		Vector3D ccross = fastNormalize(cross(lineDir, viewDir));

		float scale = lerp(fStartSize, fEndSize, 1.0f-lifeTimePerc);

		Vector3D temp;

		VectorMA( vEnd, scale, ccross, temp );

		Vector4D color(Vector3D(1)*lifeTimePerc, 1);

		verts[0].point = temp;
		verts[0].texcoord = m_entry->rect.GetLeftTop();
		verts[0].color = color;

		VectorMA( vEnd, -scale, ccross, temp );

		verts[1].point = temp;
		verts[1].texcoord = m_entry->rect.GetLeftBottom();
		verts[1].color = color;

		VectorMA( m_vOrigin, scale, ccross, temp );

		verts[2].point = temp;
		verts[2].texcoord = m_entry->rect.GetRightTop();
		verts[2].color = color;

		VectorMA( m_vOrigin, -scale, ccross, temp );

		verts[3].point = temp;
		verts[3].texcoord = m_entry->rect.GetRightBottom();
		verts[3].color = color;

		CollisionData_t coll;
		g_pPhysics->TestLine(m_vOrigin, vEnd, coll);

		m_vVelocity += vGravity*dTime*2.0f;

		if(coll.fract < 1.0f)
		{
			m_vVelocity = reflect(m_vVelocity, coll.normal)*0.8f;
			m_fLifeTime -= dTime*2;
		}

		return true;
	}

protected:
	Vector3D	tangent;
	Vector3D	binormal;
	Vector3D	normal;

	Vector3D	vGravity;

	Vector3D	m_vVelocity;

	float		fCurSize;

	float		fStartSize;
	float		fEndSize;
	float		fLength;

	CPFXAtlasGroup*		m_group;
	TexAtlasEntry_t*	m_entry;
};

#define ACCELERATION_CONST			(2.0f)
#define	ACCELERATION_SOUND_CONST	(10.0f)
#define STEERING_CONST				(2.0f)
#define STEERING_HELP_CONST			(0.5f)

float DBTorqueCurveFromRPM( float fRPM )
{
	float fTorque = ( fRPM * ( 0.225f * 0.001f ) );

	fTorque *= fTorque;
	fTorque -= 0.8f;
	fTorque *= fTorque;

	return ( 6.4f - fTorque ) * 90.0f;
}

static float DBTorqueCurve ( float radsPerSec )
{
	float fRPM = radsPerSec * 60.0f / ( 2.0f * PI_F );

	//if ( fRPM < 2450.0f )
	//	fRPM = 2450.0f;

	/*
	if ( fRPM > 9800.0f )
	{
		fRPM = 9800.0f;

		return 0;
	}*/

	return DBTorqueCurveFromRPM( fRPM );
}

/*
ConVar sa_initialGrad("sa_initialGrad", "24.0");
ConVar sa_endGrad("sa_endGrad", "1.46");
ConVar sa_endOffset("sa_endOffset", "2.9");
ConVar sa_segEndA("sa_segEndA", "0.1");
ConVar sa_segEndB("sa_segEndB", "0.2");
*/
float DBSlipAngleToLateralForce(float fSlipAngle, float fLongitudinalForce, eqPhysSurfParam_t* surfParam)
{
	const float fInitialGradient = 22.0f;
	const float fEndGradient = 1.5f;
	const float fEndOffset = 3.7f;

	const float fSegmentEndA = 0.06f;
	const float fSegmentEndB = 0.2f;

	const float fSegmentEndAOut = fInitialGradient * fSegmentEndA;
	const float fSegmentEndBOut = fEndGradient * fSegmentEndB + fEndOffset;

	const float fInvSegBLength = 1.0f / (fSegmentEndB - fSegmentEndA);
	const float fCubicGradA = fInitialGradient * (fSegmentEndB - fSegmentEndA);
	const float fCubicGradB = fEndGradient * (fSegmentEndB - fSegmentEndA);

	float fSign = sign(fSlipAngle);
	fSlipAngle *= fSign;

	float fResult;
	if (fSlipAngle < fSegmentEndA)
	{
		fResult = fSign * fInitialGradient * fSlipAngle;
	}
	else if (fSlipAngle < fSegmentEndB)
	{
		fResult = fSign * cerp(
			/*
			fSegmentEndAOut,
			fSegmentEndBOut,
			fCubicGradA,
			fCubicGradB,
			*/
			fCubicGradB, fCubicGradA, fSegmentEndBOut, fSegmentEndAOut,
			(fSlipAngle - fSegmentEndA) * fInvSegBLength);
	}
	else
	{
		float fValue = fEndGradient * fSlipAngle + fEndOffset;
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

	return fResult * weatherTireFrictionMod[g_pGameWorld->m_envConfig.weatherType];
}

//-------------------------------------------------------------------------------------------------------------------------
//
//
//
//-------------------------------------------------------------------------------------------------------------------------


void CCarWheelModel::SetModelPtr(IEqModel* modelPtr)
{
	BaseClass::SetModelPtr( modelPtr );

	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_objectInstVertexFormat, g_pGameWorld->m_objectInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}
}

extern ConVar r_enableObjectsInstancing;

void CCarWheelModel::Draw( int nRenderFlags )
{
	if(r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		gameObjectInstance_t& inst = instancer->NewInstance( m_bodyGroupFlags, nLOD );
		inst.world = m_worldMatrix;
	}
	else
		BaseClass::Draw( nRenderFlags );
}



BEGIN_NETWORK_TABLE( CCar )
	DEFINE_SENDPROP_BYTE(m_sirenEnabled),
	DEFINE_SENDPROP_FLOAT(m_gameDamage),
	DEFINE_SENDPROP_FLOAT(m_gameMaxDamage),
	DEFINE_SENDPROP_BYTE(m_locked),
END_NETWORK_TABLE()

CCar::CCar()
{

}

CCar::CCar( carConfigEntry_t* config ) :
	m_pSkidSound(NULL),
	m_pEngineSound(NULL),
	m_pEngineSoundLow(NULL),
	m_pPhysicsObject(NULL),
	m_pHornSound(NULL),
	m_pSirenSound(NULL),
	m_pSurfSound(NULL),
	m_fEngineRPM(0.0f),
	m_nPrevGear(-1),
	m_steering(0.0f),
	m_steeringHelper(0.0f),
	m_fAcceleration(0.0f),
	m_fAccelEffect(0.0f),
	m_controlButtons(0),
	m_oldControlButtons(0),
	m_engineIdleFactor(1.0f),
	m_curTime(0),
	m_sirenEnabled(false),
	m_oldSirenState(false),
	m_isLocalCar(false),
	m_brakeLightsEnabled(false),
	m_nGear(1),
	m_radsPerSec(0.0f),
	m_effectTime(0.0f),
	m_pDamagedModel(NULL),
	m_engineSmokeTime(0.0f),
	m_carColor(ColorRGBA(0.9,0.25,0.25,1.0)),
	m_enablePhysics(true),
	
	m_accelRatio(1023),
	m_brakeRatio(1023),
	m_steerRatio(1023),

	m_gameDamage(0.0f),
	m_gameFelony(0.0f),
	m_numPursued(0),
	m_gameMaxDamage(CAR_DEFAULT_MAX_DAMAGE),
	m_locked(false),
	m_enabled(true),
	m_inWater(false),
	m_autohandbrake(true),
	m_torqueScale(1.0f),
	m_maxSpeed(125.0f)
{
	m_conf = config;
	memset(m_bodyParts, 0,sizeof(m_bodyParts));

	if(m_conf)
	{
		SetColorScheme(0);
		m_maxSpeed = m_conf->m_maxSpeed;
	}
		
}

void CCar::DebugReloadCar()
{
	Vector3D vel = GetVelocity();
	Vector3D pos = GetOrigin();
	Quaternion ang = GetOrientation();
	Vector3D angVel = GetAngularVelocity();

	OnRemove();

	Repair();

	Spawn();

	SetVelocity(vel);
	SetOrigin(pos);
	SetOrientation(ang);
	SetAngularVelocity(angVel);
}

ConVar g_infinitemass("g_infinitemass", "0", "Infinite mass vehicle", CV_CHEAT);

void CCar::Precache()
{
	PrecacheScriptSound("tarmac.skid");
	PrecacheScriptSound("defaultcar.engine");
	PrecacheScriptSound("generic.horn1");
	PrecacheScriptSound("generic.copsiren1");

	PrecacheScriptSound("generic.hit");
	PrecacheScriptSound("generic.impact");
	PrecacheScriptSound("generic.crash");
	PrecacheScriptSound("generic.wheelOnGround");
	PrecacheScriptSound("generic.waterHit");
	
}

void CCar::CreateCarPhysics()
{
	CEqRigidBody* body = new CEqRigidBody();
	
	int wheelCacheId = g_pModelCache->PrecacheModel( "models/vehicles/wheel.egf" );
	IEqModel* wheelModel = g_pModelCache->GetModel( wheelCacheId );

	for(int i = 0; i < m_conf->m_wheels.numElem(); i++)
	{
		carWheelConfig_t& wconf = m_conf->m_wheels[i];

		CCarWheelModel* pWheelObject = new CCarWheelModel();

		wheelinfo_t winfo;

		winfo.bodyIndex = Studio_FindBodyGroupId(wheelModel->GetHWData()->pStudioHdr, wconf.wheelName);

		winfo.pWheelObject = pWheelObject;

		pWheelObject->SetModelPtr( wheelModel );

		pWheelObject->m_bodyGroupFlags = (1 << winfo.bodyIndex);

		m_pWheels.append(winfo);
	}

	body->m_flags |= BODY_COLLISIONLIST | BODY_ISCAR | (g_infinitemass.GetBool() ? BODY_INFINITEMASS : 0);

	// CREATE BODY
	Vector3D body_mins = m_conf->m_body_center - m_conf->m_body_size;
	Vector3D body_maxs = m_conf->m_body_center + m_conf->m_body_size;


	if(m_pModel->GetHWData()->m_physmodel.numobjects)
	{
		body->Initialize(&m_pModel->GetHWData()->m_physmodel, 0);
	}
	else
		body->Initialize(body_mins, body_maxs);

	// CREATE BODY
	body->m_aabb.minPoint = m_conf->m_body_center - m_conf->m_body_size;
	body->m_aabb.maxPoint = m_conf->m_body_center + m_conf->m_body_size;

	body->SetFriction( 0.4f );
	body->SetRestitution( 0.5f );

	body->SetMass( m_conf->m_mass );
	body->SetCenterOfMass( m_conf->m_virtualMassCenter );

	// don't forget about contents!
	body->SetContents( OBJECTCONTENTS_VEHICLE );
	body->SetCollideMask( COLLIDEMASK_VEHICLE );

	body->SetUserData(this);
	body->SetDebugName(m_conf->carName.c_str());

	body->SetPosition(m_vecOrigin);
	body->SetOrientation(Quaternion(DEG2RAD(m_vecAngles.x),DEG2RAD(m_vecAngles.y),DEG2RAD(m_vecAngles.z)));

	m_pPhysicsObject = new CPhysicsHFObject( body, this );

	g_pPhysics->AddObject( m_pPhysicsObject );
}

void CCar::InitCarSound()
{
	EmitSound_t skid_ep;

	skid_ep.name = "tarmac.skid";

	skid_ep.fPitch = 1.0f;
	skid_ep.fVolume = 1.0f;
	skid_ep.fRadiusMultiplier = 0.55f;
	skid_ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
	skid_ep.origin = m_vecOrigin;

	m_pSkidSound = ses->CreateSoundController(&skid_ep);

	skid_ep.name = "tarmac.wetnoise";

	skid_ep.fPitch = 1.0f;
	skid_ep.fVolume = 1.0f;
	skid_ep.fRadiusMultiplier = 0.55f;
	skid_ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
	skid_ep.origin = m_vecOrigin;

	m_pSurfSound = ses->CreateSoundController(&skid_ep);

	EmitSound_t ep;

	ep.name = m_conf->m_sndEngineRPMHigh.c_str();

	ep.fPitch = 1.0f;
	ep.fVolume = 1.0f;
	ep.fRadiusMultiplier = 0.45f;
	ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
	ep.origin = m_vecOrigin;

	m_pEngineSound = ses->CreateSoundController(&ep);

	ep.name = m_conf->m_sndEngineRPMLow.c_str();

	m_pEngineSoundLow = ses->CreateSoundController(&ep);

	EmitSound_t idle_ep;

	idle_ep.name = m_conf->m_sndEngineIdle.c_str();

	idle_ep.fPitch = 1.0f;
	idle_ep.fVolume = 1.0f;
	idle_ep.fRadiusMultiplier = 0.45f;
	idle_ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
	idle_ep.origin = m_vecOrigin;

	m_pIdleSound = ses->CreateSoundController(&idle_ep);

	EmitSound_t horn_ep;

	horn_ep.name = m_conf->m_sndHornSignal.c_str();

	horn_ep.fPitch = 1.0f;
	horn_ep.fVolume = 1.0f;
	horn_ep.fRadiusMultiplier = 1.0f;
	horn_ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
	horn_ep.origin = m_vecOrigin;

	m_pHornSound = ses->CreateSoundController(&horn_ep);

	if(m_conf->m_sirenType != SIREN_NONE)
	{
		EmitSound_t siren_ep;

		siren_ep.name = m_conf->m_sndSiren.c_str();

		siren_ep.fPitch = 1.0f;
		siren_ep.fVolume = 1.0f;

		siren_ep.fRadiusMultiplier = 1.0f;
		siren_ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;
		siren_ep.origin = m_vecOrigin;
		siren_ep.sampleId = 0;

		m_pSirenSound = ses->CreateSoundController(&siren_ep);
	}
}

void CCar::Spawn()
{
	// init default vehicle
	// load visuals
	SetModel( m_conf->m_cleanModelName.c_str() );
	/*
	// create instancer for lod models only
	if(g_pShaderAPI->GetCaps().isInstancingSupported &&
		m_pModel && m_pModel->GetInstancer() == NULL)
	{
		CGameObjectInstancer* instancer = new CGameObjectInstancer();

		// init with this preallocated buffer and format
		instancer->Init( g_pGameWorld->m_vehicleInstVertexFormat, g_pGameWorld->m_vehicleInstVertexBuffer );

		m_pModel->SetInstancer( instancer );
	}*/

	int nDamModelIndex = g_pModelCache->PrecacheModel( m_conf->m_damModelName.c_str() );

	if(nDamModelIndex != -1)
		m_pDamagedModel = g_pModelCache->GetModel( nDamModelIndex );
	else
		m_pDamagedModel = NULL;

	// Init car body parts
	for(int i = 0; i < CB_PART_WINDOW_PARTS; i++)
	{
		m_bodyParts[i].boneIndex = Studio_FindBoneId(m_pModel->GetHWData()->pStudioHdr, s_pBodyPartsNames[i]);

		//if(m_bodyParts[i].boneIndex == -1)
		//	MsgError("Error in model '%s' - can't find bone '%s' which is required\n", m_pModel->GetHWData()->pStudioHdr->modelname, s_pBodyPartsNames[i]);
	}

	CreateCarPhysics();

	InitCarSound();

	// baseclass spawn
	CGameObject::Spawn();

	if(m_replayID == REPLAY_NOT_TRACKED)
	{
#ifndef EDITOR
#ifndef BIG_REPLAYS
		if(dynamic_cast<CAITrafficCar*>(this) == NULL)		// don't spawn traffic cars
#endif // BIG_REPLAYS
		{
			g_replayData->PushEvent( REPLAY_EVENT_SPAWN, this );
		}
#endif // EDITOR
	}

	m_trans_grasspart = g_translParticles->FindEntry("grasspart");
	//m_trans_smoke2 = g_translParticles->FindAtlasTexture("smoke2");
	m_trans_fleck = g_translParticles->FindEntry("fleck");
	m_veh_skidmark_asphalt = g_vehicleEffects->FindEntry("skidmark_asphalt");

	// TODO: own car shadow texture
	m_veh_shadow = g_vehicleEffects->FindEntry("carshad");
}

void CCar::OnRemove()
{
	CGameObject::OnRemove();

	if(m_replayID != REPLAY_NOT_TRACKED)
	{
#ifndef EDITOR
#ifndef BIG_REPLAYS
		if(dynamic_cast<CAITrafficCar*>(this) == NULL)		// don't spawn traffic cars
#endif // BIG_REPLAYS
			g_replayData->PushEvent( REPLAY_EVENT_REMOVE, this );
#endif // EDITOR
	}

	ses->RemoveSoundController(m_pIdleSound);
	ses->RemoveSoundController(m_pEngineSound);
	ses->RemoveSoundController(m_pEngineSoundLow);
	ses->RemoveSoundController(m_pSkidSound);
	ses->RemoveSoundController(m_pHornSound);
	ses->RemoveSoundController(m_pSirenSound);
	ses->RemoveSoundController(m_pSurfSound);

	for(int i = 0; i < m_pWheels.numElem(); i++)
	{
		m_pWheels[i].pWheelObject->OnRemove();
		delete m_pWheels[i].pWheelObject;
	}

	m_pWheels.clear();

	g_pPhysics->RemoveObject(m_pPhysicsObject);

	m_pPhysicsObject = NULL;
	m_pIdleSound = NULL;
	m_pEngineSound = NULL;
	m_pEngineSoundLow = NULL;
	m_pSkidSound = NULL;
	m_pPhysicsObject = NULL;
	m_pHornSound = NULL;
	m_pSirenSound = NULL;
	m_pSurfSound = NULL;
}

void CCar::SetOrigin(const Vector3D& origin)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->m_object->SetPosition(origin);

	m_vecOrigin = origin;
}

void CCar::SetAngles(const Vector3D& angles)
{
	if(m_pPhysicsObject)
		m_pPhysicsObject->m_object->SetOrientation(Quaternion(DEG2RAD(angles.x),DEG2RAD(angles.y),DEG2RAD(angles.z)));

	m_vecAngles = angles;
}

void CCar::SetOrientation(const Quaternion& q)
{
	m_pPhysicsObject->m_object->SetOrientation(q);
}

void CCar::SetVelocity(const Vector3D& vel)
{
	m_pPhysicsObject->m_object->SetLinearVelocity( vel  );
}

void CCar::SetAngularVelocity(const Vector3D& vel)
{
	m_pPhysicsObject->m_object->SetAngularVelocity( vel );
}

Vector3D CCar::GetOrigin() const
{
	return m_pPhysicsObject->m_object->GetPosition();
}

Vector3D CCar::GetAngles() const
{
	Vector3D eangles = eulers(m_pPhysicsObject->m_object->GetOrientation());

	return VRAD2DEG(eangles);
}

Quaternion CCar::GetOrientation() const
{
	return m_pPhysicsObject->m_object->GetOrientation();
}

Vector3D CCar::GetForwardVector() const
{
	Matrix4x4 m(m_pPhysicsObject->m_object->GetOrientation());

	m = transpose(m);

	return m.rows[2].xyz();
}

Vector3D CCar::GetUpVector() const
{
	Matrix4x4 m(m_pPhysicsObject->m_object->GetOrientation());

	m = transpose(m);

	return m.rows[1].xyz();
}

Vector3D CCar::GetRightVector() const
{
	Matrix4x4 m(m_pPhysicsObject->m_object->GetOrientation());

	m = transpose(m);

	return m.rows[0].xyz();
}

Vector3D CCar::GetVelocity() const
{
	return m_pPhysicsObject->m_object->GetLinearVelocity();
}

Vector3D CCar::GetAngularVelocity() const
{
	return m_pPhysicsObject->m_object->GetAngularVelocity();
}

CEqRigidBody* CCar::GetPhysicsBody() const
{
	if(!m_pPhysicsObject)
		return NULL;

	return m_pPhysicsObject->m_object;
}

void CCar::L_SetContents(int contents)
{
	if (!m_pPhysicsObject)
		return;

	m_pPhysicsObject->m_object->SetContents(contents);
}

void CCar::L_SetCollideMask(int contents)
{
	if (!m_pPhysicsObject)
		return;

	m_pPhysicsObject->m_object->SetCollideMask(contents);
}

int	CCar::L_GetContents()
{
	if (!m_pPhysicsObject)
		return 0;

	return m_pPhysicsObject->m_object->GetCollideMask();
}

int	CCar::L_GetCollideMask()
{
	if (!m_pPhysicsObject)
		return 0;

	return m_pPhysicsObject->m_object->GetCollideMask();
}

void CCar::SetControlButtons(int flags)
{
	// make no misc controls
	flags &= ~IN_MISC;

	m_controlButtons = flags;

	if(	m_conf->m_sirenType != SIREN_NONE &&
		(m_controlButtons & IN_SIREN) && !(m_oldControlButtons & IN_SIREN))
	{
		m_oldSirenState = m_sirenEnabled;
		m_sirenEnabled = !m_sirenEnabled;
	}
}

int	CCar::GetControlButtons()
{
	return m_controlButtons;
}

void CCar::SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering)
{
	m_accelRatio = fAccelRatio*1023.0f;
	m_brakeRatio = fBrakeRatio*1023.0f;
	m_steerRatio = fSteering*1023.0f;

	if(m_accelRatio > 1023)
		m_accelRatio = 1023;

	if(m_brakeRatio > 1023)
		m_brakeRatio = 1023;

	if(m_steerRatio > 1023)
		m_steerRatio = 1023;

	if(m_steerRatio < -1023)
		m_steerRatio = -1023;
}

void CCar::GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering)
{
	fAccelRatio = (double)m_accelRatio * _oneBy1024;
	fBrakeRatio = (double)m_brakeRatio * _oneBy1024;
	fSteering = (double)m_steerRatio * _oneBy1024;
}

#define BURNOUT_MAX_SPEED				(62.0f)
#define WHELL_ROLL_RESISTANCE_CONST		(150)
#define WHELL_ROLL_RESISTANCE_HALF		(WHELL_ROLL_RESISTANCE_CONST * 0.5f)

void CCar::UpdateCarPhysics(float delta)
{
	PROFILE_FUNC()

	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	// retrieve body matrix
	carBody->ConstructRenderMatrix(m_worldMatrix);

	if(m_locked) // TODO: cvar option to lock or not
	{
		m_oldControlButtons = m_controlButtons;
		m_controlButtons = IN_HANDBRAKE;
		m_controlButtons &= ~IN_HORN;
	}

	//
	// Update controls first
	//

	FReal fSteerAngle = 0;
	FReal fAccel = 0;
	FReal fBrake = 0;
	FReal fHandbrake = 0;

	bool bBurnout = false;
	bool bExtendTurn = false;

	if( m_enabled )
	{
		if( m_controlButtons & IN_ACCELERATE )
			fAccel = (float)((float)m_accelRatio*_oneBy1024);

		if( m_controlButtons & IN_BRAKE )
			fBrake = (float)((float)m_brakeRatio*_oneBy1024);

		if( m_controlButtons & IN_BURNOUT )
			bBurnout = true;

		if( m_controlButtons & IN_HANDBRAKE )
			fHandbrake = 1;
	}
	else
		fHandbrake = 1;

	if(m_controlButtons != m_oldControlButtons)
		carBody->TryWake(false);

	if(carBody->IsFrozen() && !bBurnout)
		return;

	if( m_controlButtons & IN_EXTENDTURN )
		bExtendTurn = true;

	// do acceleration and burnout
	if(bBurnout)
		fAccel = 1;

	if( m_controlButtons & IN_TURNLEFT )
		fSteerAngle -= (float)((float)m_steerRatio*_oneBy1024);
	else if( m_controlButtons & IN_TURNRIGHT )
		fSteerAngle += (float)((float)m_steerRatio*_oneBy1024);

	if(m_controlButtons & IN_ANALOGSTEER)
	{
		fSteerAngle = (float)((float)m_steerRatio*_oneBy1024);
		m_steering = fSteerAngle;
	}

	//------------------------------------------------------------------------------------------

	bool bDoBurnout = bBurnout && (abs(GetSpeed()) < BURNOUT_MAX_SPEED);// && (fHandbrake == 0);

	FReal accel_scale = 1;

	float fRPM = GetRPM();

	if(bDoBurnout)
	{
		m_engineIdleFactor = 0.0f;
		accel_scale = 5;
		fRPM = 5800.0f;
	}

	if(m_nGear > 1)
	{
		if(fAccel > m_fAcceleration)
			m_fAcceleration += delta * ACCELERATION_CONST * accel_scale;
		else
			m_fAcceleration -= delta * ACCELERATION_CONST;

		if(m_fAcceleration < 0)
			m_fAcceleration = 0;
	}
	else
		m_fAcceleration = fAccel;
	
	#define RPM_REFRESH_TIMES 10

	float fDeltaRpm = delta / RPM_REFRESH_TIMES;

	for(int i = 0; i < RPM_REFRESH_TIMES; i++)
	{
		if(fabs(fRPM - m_fEngineRPM) > 0.02f)
			m_fEngineRPM += sign(fRPM - m_fEngineRPM) * 12000.0f * fDeltaRpm;
	}

	m_fEngineRPM = clamp(m_fEngineRPM, 0.0f, 8500.0f);

	//--------------------------------------------------------

	// do acceleration and burnout
	if(bBurnout)
		fAccel = 1.0f;

	{
		FReal steer_diff = fSteerAngle-m_steering;

		if(FPmath::abs(steer_diff) > 0.1f)
			m_steering += FPmath::sign(steer_diff) * STEERING_CONST * delta;
		else
			m_steering = fSteerAngle;
	}
	
	if (FPmath::abs(fSteerAngle) > 0.5f)
	{
		FReal steerHelp_diff = fSteerAngle-m_steeringHelper;
		if(FPmath::abs(steerHelp_diff) > 0.1f)
			m_steeringHelper += FPmath::sign(steerHelp_diff) * STEERING_HELP_CONST * delta;
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

	int numDriveWheels = 0;
	int numHandbrakeWheelsOnGround = 0;
	int numDriveWheelsOnGround = 0;
	int numWheelsOnGround = 0;
	int numSteerWheels = 0;
	int numSteerWheelsOnGround = 0;

	float wheelsSpeed = 0.0f;

	float steerwheelsFriction = 0.0f;

	// final: trace new straight for blocking cars and deny if have any
	CollisionData_t coll_line;

	eqPhysCollisionFilter collFilter;
	collFilter.flags = EQPHYS_FILTER_FLAG_DISALLOW_DYNAMIC;
	collFilter.AddObject(carBody);

	// Raycast the wheels and do some simple calculations
	for(int i = 0; i < GetWheelCount(); i++)
	{
		wheelinfo_t& wheel = m_pWheels[i];
		carWheelConfig_t& wheelConf = m_conf->m_wheels[i];

		// Test rays of wheels
		Vector3D line_start = (m_worldMatrix*Vector4D(wheelConf.suspensionTop, 1)).xyz();
		Vector3D line_end = (m_worldMatrix*Vector4D(wheelConf.suspensionBottom, 1)).xyz();

		// trace solid ground only
		g_pPhysics->TestLine(line_start, line_end, wheel.collisionInfo, OBJECTCONTENTS_SOLID_GROUND, &collFilter);

		if(wheelConf.flags & WHEEL_FLAG_DRIVE)
		{
			numDriveWheels++;
			wheelsSpeed += wheel.pitchVel;
		}

		if(wheelConf.flags & WHEEL_FLAG_STEER)
			numSteerWheels++;

		if(wheel.collisionInfo.fract < 1.0f)
		{
			numWheelsOnGround++;

			if(wheelConf.flags & WHEEL_FLAG_DRIVE)
				numDriveWheelsOnGround++;

			if(wheelConf.flags & WHEEL_FLAG_HANDBRAKE)
				numHandbrakeWheelsOnGround++;

			eqPhysSurfParam_t* surfparam = g_pPhysics->GetSurfaceParamByID( wheel.collisionInfo.materialIndex );

			wheel.surfparam = surfparam;

			if(wheelConf.flags & WHEEL_FLAG_STEER)
			{
				numSteerWheelsOnGround++;

				if(wheel.surfparam)
					steerwheelsFriction += wheel.surfparam->tirefriction;
			}
		}
		else
		{
			if(wheelConf.flags & WHEEL_FLAG_STEER)
			{
				steerwheelsFriction += 0.2f;
			}
		}

		// update effects
		UpdateWheelEffect(i, delta);
	}

	float wheelMod = 1.0f / numDriveWheels; //(numDriveWheelsOnGround > 0) ? (1.0f / (float)numDriveWheelsOnGround) : 1.0f;

	float driveGroundWheelMod = (numDriveWheelsOnGround > 0) ? (1.0f / (float)numDriveWheelsOnGround) : 1.0f;

	wheelsSpeed *= wheelMod;
	steerwheelsFriction /= numSteerWheels;

	if(numHandbrakeWheelsOnGround > 0 && fHandbrake > 0.0f)
	{
		carBody->m_flags |= BODY_DISABLE_DAMPING;
	}
	else
	{
		carBody->m_flags &= ~BODY_DISABLE_DAMPING;
	}

	//
	// Some steering stuff from Driver 1
	//

	FReal autoHandbrakeHelper = 0.0f;

	if( g_autohandbrake.GetBool() && m_autohandbrake && GetSpeedWheels() > 0.0f && !bDoBurnout )
	{
		const FReal AUTOHANDBRAKE_MIN_SPEED		= 10.0f;
		const FReal AUTOHANDBRAKE_MAX_SPEED		= 30.0f;
		const FReal AUTOHANDBRAKE_MAX_FACTOR	= 5.0f;
		const FReal AUTOHANDBRAKE_SCALE			= 6.7f;

		float handbrakeFactor = RemapVal((GetSpeed()-AUTOHANDBRAKE_MIN_SPEED), 0.0f, AUTOHANDBRAKE_MAX_SPEED, 0.0f, 1.0f);

		FReal fLateral = GetLateralSlidingAtBody();
		FReal fLateralSign = sign(GetLateralSlidingAtBody())*-0.01f;

		if( fLateralSign > 0 && (m_steeringHelper > fLateralSign)
			|| fLateralSign < 0 && (m_steeringHelper < fLateralSign) || fabs(fLateral) < 5.0f)
		{
			if(FPmath::abs(m_steeringHelper) > 0.15f)
				autoHandbrakeHelper = (FPmath::abs(m_steeringHelper)-0.15f)*AUTOHANDBRAKE_SCALE * handbrakeFactor;
		}

		autoHandbrakeHelper = clamp(autoHandbrakeHelper,FReal(0),AUTOHANDBRAKE_MAX_FACTOR);
	}

	if( GetSpeed() <= 0.25f )
	{
		m_fAcceleration -= fBrake;
		fBrake -= fAccel;
	}


	//
	// Update engine
	//

	FReal differentialRatio = m_conf->m_differentialRatio;
	FReal transmissionRate = m_conf->m_transmissionRate;

	FReal torqueConvert = differentialRatio * m_conf->m_gears[m_nGear];
	m_radsPerSec = (float)fabs(wheelsSpeed)*torqueConvert;
	FReal torque = DBTorqueCurve( m_radsPerSec ) * m_conf->m_torqueMult;

	float gbxDecelRate = 1.0f - clamp(1.0f-m_fAcceleration, 0.0f, 0.0f);
	float gbxAccelRate = 1.0f;

 	if(torque < 0)
		torque = 0.0f;

	torque *= torqueConvert * transmissionRate;

	float car_speed = wheelsSpeed;

	if(numWheelsOnGround)
	{
		//
		// update gearbox
		//

		if(m_nGear > 0)
		{
			int newGear = m_nGear;

			for ( int nGear = 1; nGear < m_nGear; nGear++ )
			{
				// find gear to diffential
				torqueConvert = differentialRatio * m_conf->m_gears[nGear];
				FReal gearRadsPerSecond = wheelsSpeed * torqueConvert;

				FReal gearTorque = DBTorqueCurve(gearRadsPerSecond) * m_conf->m_torqueMult;
 				if(gearTorque < 0)
					gearTorque = 0.0f;

				gearTorque *= torqueConvert * transmissionRate;

				// gear torque check
				if ( gearTorque*gbxDecelRate > torque )
				{
					newGear = nGear;
				}
			}

			for ( int nGear = newGear; nGear < m_conf->m_gears.numElem(); nGear++ )
			{
				// find gear to diffential
				torqueConvert = differentialRatio * m_conf->m_gears[nGear];
				FReal gearRadsPerSecond = wheelsSpeed * torqueConvert;

				FReal gearTorque = DBTorqueCurve(gearRadsPerSecond) * m_conf->m_torqueMult;
 				if(gearTorque < 0)
					gearTorque = 0.0f;

				gearTorque *= torqueConvert * transmissionRate;

				// gear torque check
				if ( gearTorque > torque && numDriveWheelsOnGround )
				{
					newGear = nGear;
					torque = gearTorque;

					m_radsPerSec = gearRadsPerSecond;
				}
			}

			m_nGear = newGear;
		}

		// if shifted up, reduce gas since we pressed clutch
		if(	m_nGear > 0 &&
			m_nGear > m_nPrevGear &&
			m_fAcceleration >= 0.9f &&
			!bDoBurnout &&
			numDriveWheelsOnGround)
		{
			m_fAcceleration *= 0.25f;
		}

		m_nPrevGear = m_nGear;
	}

	// check neutral zone
	if(fsimilar(car_speed, 0.0f, 1.0f) || !numDriveWheelsOnGround)
	{
		if(fBrake > 0)
			m_nGear = 0;
		else if(fAccel > 0)
			m_nGear = 1;
	}
	else
	{
		if(car_speed < 0.0f && m_nGear > 0)
			m_nGear = 0;
		else if(car_speed > 0.0f && m_nGear == 0)
			m_nGear = 1;
	}

	if( bDoBurnout && m_nGear == 0 )
		m_nGear = 1;

	if(m_nGear == 0)
	{
		// find gear to diffential
		torqueConvert = differentialRatio * m_conf->m_gears[m_nGear];
		FReal gearRadsPerSecond = wheelsSpeed * torqueConvert;

		FReal gearTorque = DBTorqueCurve(gearRadsPerSecond) * m_conf->m_torqueMult;
 		if(gearTorque < 0)
			gearTorque = 0.0f;

		torque = gearTorque * torqueConvert * transmissionRate;
	}

	FReal fAcceleration = m_fAcceleration;
	FReal fBreakage = fBrake;

	if( m_nGear == 0 && numWheelsOnGround)
	{
		// swap brake and acceleration
		swap(fAcceleration, fBreakage);

		fBreakage *= -1;
	}

	m_brakeLightsEnabled = FPmath::abs(fBreakage) > 0.05f && fabs(car_speed) > 0.01f;

	if(fHandbrake > 0)
		fAcceleration = 0;

	m_fAccelEffect += ((fAcceleration-FPmath::abs(fBreakage))-m_fAccelEffect) * delta * ACCELERATION_SOUND_CONST * accel_scale;
	m_fAccelEffect = clamp(m_fAccelEffect, -1.0f, 1.0f);

	float fRpm = m_radsPerSec * ( 60.0f / ( 2.0f * PI_F ));

	if(fRpm > 7500)
	{
		torque = 0;
		fAcceleration = 0;
	}
 	else if(fRpm < -7500)
	{
		torque = 0;
		fAcceleration = 0;
	}

	float fAccelerator = float(fAcceleration * torque) * m_torqueScale;
	float fBraker = fBreakage*pow(m_torqueScale, 0.5f);

	// Limit the speed
	if(GetSpeed() > m_maxSpeed)
		fAccelerator = 0;

	// springs for wheels
	for(int i = 0; i < GetWheelCount(); i++)
	{
		wheelinfo_t& wheel = m_pWheels[i];
		carWheelConfig_t& wheelConf = m_conf->m_wheels[i];

		wheel.wheelOrient = identity3();

		float fWheelSteerAngle = 0.0f;

		if(wheelConf.flags & WHEEL_FLAG_STEER)
		{
			fWheelSteerAngle = DEG2RAD(m_steering*40) * wheelConf.steerMultipler;
			wheel.wheelOrient = rotateY3( fWheelSteerAngle );
		}

		// apply car rays
		Vector3D line_start = (m_worldMatrix*Vector4D(wheelConf.suspensionTop, 1)).xyz();
		Vector3D line_end = (m_worldMatrix*Vector4D(wheelConf.suspensionBottom, 1)).xyz();

		// compute wheel world rotation (turn + body rotation)
		Matrix3x3 wrotation = wheel.wheelOrient*transpose(m_worldMatrix).getRotationComponent();

		// get wheel vectors
		Vector3D wheel_right = wrotation.rows[0];
		Vector3D wheel_forward = wrotation.rows[2];

		float fPitchFac = RemapVal(BURNOUT_MAX_SPEED - GetSpeed(), 0.0f, 70.0f, 0.0f, 1.0f);

		if(wheel.collisionInfo.fract < 1.0f)
		{
			float wheelTractionFrictionScale = 0.2f;

			if(wheel.surfparam)
				wheelTractionFrictionScale = wheel.surfparam->tirefriction;

			wheelTractionFrictionScale *= weatherTireFrictionMod[g_pGameWorld->m_envConfig.weatherType];

			// wheel ray direction

			float suspLength = length(line_start-line_end);

			Vector3D ray_dir = (line_start-line_end) / suspLength;
			Vector3D wheelVelAtPoint = carBody->GetVelocityAtWorldPoint(wheel.collisionInfo.position);

			wheel.velocityVec = wheelVelAtPoint;

			// use some waving for dirt
			if(wheel.surfparam && wheel.surfparam->word == 'G') // wheel on grass
			{
				float fac1 = wheel.collisionInfo.position.x*0.65f;
				float fac2 = wheel.collisionInfo.position.z*0.65f;

				wheel.collisionInfo.fract += ((0.5f-sin(fac1))+(0.5f-sin(fac2+0.5f)))*wheelConf.radius*0.11f;
			}

			float springPowerFac = 1.0f-wheel.collisionInfo.fract;

			float springPower = springPowerFac*wheelConf.springConst;
			float dampingFactor = dot(wheel.collisionInfo.normal, wheelVelAtPoint);

			if ( fabs( dampingFactor ) > 1.0f )
				dampingFactor = sign( dampingFactor ) * 1.0f;

			// apply spring damping now
			springPower -= wheelConf.dampingConst*dampingFactor;

			if(springPower <= 0.0f)
				continue;

			Vector3D springForce = wheel.collisionInfo.normal*springPower;

			//
			// processing slip
			//
			Vector3D wheelForward = wheel_forward;

			// apply velocity
			wheelForward -= wheel.collisionInfo.normal * dot( wheel.collisionInfo.normal, wheelVelAtPoint );

			// normalize
			float fwdLength = length(wheelForward);
			if ( fwdLength < 0.001f )
				continue;

			wheelForward /= fwdLength;

			//---------------------------------------------------------------------------------

			// get direction vectors based on surface normal
			Vector3D wheelTractionForceDir = cross(wheel_right, wheel.collisionInfo.normal);

			// determine wheel pitch speed applied from the ground
			float wheelPitchSpeed		= dot(wheelVelAtPoint, wheelTractionForceDir);

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

				if( bDoBurnout && (wheelConf.flags & WHEEL_FLAG_DRIVE) )
				{
					fLongitudinalForce += wheelTractionFrictionScale * (1.0f-fPitchFac);
				}

				float handbrakes = autoHandbrakeHelper+fHandbrake;

				if((wheelConf.flags & WHEEL_FLAG_HANDBRAKE) && handbrakes > 0.0f)
				{
					fLongitudinalForce += handbrakes;
				}
			}


			Vector3D wheelSlipForceDir	= cross(wheel.collisionInfo.normal, wheelForward);

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
				if ( velocityMagnitude > 2.0f )
				{
					float fSlipAngle = -atan2f( dot(wheelSlipForceDir,  wheelVelAtPoint ), fabs( dot(wheelForward, wheelVelAtPoint ) ) ) ;

					if(wheelPitchSpeed > 0.1f)
						fSlipAngle += fWheelSteerAngle*0.5f;

					wheelSlipOppositeForce = DBSlipAngleToLateralForce( fSlipAngle, fLongitudinalForce, wheel.surfparam);// , wheelSurfaceAttrib ) ;
				}
				else
				{
					// supress slip force on low speeds
					wheelSlipOppositeForce = -dot(wheelSlipForceDir, wheelVelAtPoint );// * 2.0f ;
				}


				// contact surface modifier (perpendicularness to ground)
				{
					const float SURFACE_GRIP_SCALE = 1.0f;
					const float SURFACE_GRIP_DEDZONE = 0.02f;

					float surfaceForceMod = dot( wheel_right, wheel.collisionInfo.normal );
					surfaceForceMod = 1.0f - fabs ( surfaceForceMod );

					float fGrip = surfaceForceMod;

					fGrip = clamp( (fGrip * SURFACE_GRIP_SCALE) + SURFACE_GRIP_DEDZONE, 0.0f , 1.0f );

					wheelSlipOppositeForce *= fGrip;
				}
			}

			// apply force by drive wheel
			if(wheelConf.flags & WHEEL_FLAG_DRIVE)
			{
				if(fAcceleration > 0.01f)
				{
					if(bDoBurnout && (wheelConf.flags & WHEEL_FLAG_DRIVE))
					{
						wheelTractionForce += fabs(fAccelerator) * driveGroundWheelMod * wheelTractionFrictionScale;
						wheelSlipOppositeForce *= wheelTractionFrictionScale * (1.0f-fPitchFac); // BY DIFFERENCE
					}

					wheelTractionForce += fAccelerator * driveGroundWheelMod;
				}
			}

			if(fabs(fBraker) > 0 && fabs(wheelPitchSpeed) > 0)
			{
				// strange, but effective
				if(fBraker > 0 && wheelPitchSpeed > 0)
				{
					wheelTractionForce -= fabs(fBraker * wheelConf.brakeTorque) * wheelTractionFrictionScale * 4.0f * (1.0f+springPowerFac);

					if(fBraker >= 0.95f)
						wheelPitchSpeed -= wheelPitchSpeed * 3.0f * wheelTractionFrictionScale;
				}
				else if(fBraker < 0 && wheelPitchSpeed < 0)
				{
					wheelTractionForce += fabs(fBraker * wheelConf.brakeTorque) * wheelTractionFrictionScale * 4.0f * (1.0f+springPowerFac);
				}
			}
			
			if((wheelConf.flags & WHEEL_FLAG_HANDBRAKE) && fHandbrake > 0.0f)
			{
				const float HANDBRAKE_TORQUE = 8500.0f;

				// strange, but effective
				if(wheelPitchSpeed > 0.1f)
				{
					wheelTractionForce -= HANDBRAKE_TORQUE * wheelTractionFrictionScale * 3.0f;
				}
				else if(wheelPitchSpeed < -0.1f)
				{
					wheelTractionForce += HANDBRAKE_TORQUE * wheelTractionFrictionScale * 3.0f;
				}
				else
				{
					springForce -= wheelTractionForceDir*(dot(wheelVelAtPoint, wheelTractionForceDir) * HANDBRAKE_TORQUE * 5.0f);
				}

				if(fHandbrake == 1.0f)
					wheelPitchSpeed = 0.0f;
			}

			{
				// set wheel velocity, add pitch radians
				wheel.pitchVel = wheelPitchSpeed;
			}

			wheelSlipOppositeForce *= springPower;

			springForce += wheelSlipForceDir * wheelSlipOppositeForce;
			springForce += wheelTractionForceDir * wheelTractionForce;


			float fVelDamp = sign(wheelPitchSpeed)*clamp(fabs(wheelPitchSpeed), 0.0f, 1.0f);

			springForce -= wheelTractionForceDir * fVelDamp * (1.0f-dampingFactor) * WHELL_ROLL_RESISTANCE_CONST;
			springForce -= wheelTractionForceDir * wheelPitchSpeed * (1.0f-dampingFactor)*(8.0f + (1.0f-clamp(fAccel+(float)fabs(fBrake), 0.0f, 1.0f))*WHELL_ROLL_RESISTANCE_CONST);

			// spring force position in a couple with antiroll
			Vector3D springForcePos = wheel.collisionInfo.position;

			// TODO: dependency on other wheels
			float fAntiRollFac = springPowerFac+ANTIROLL_FACTOR_DEADZONE;

			if(fAntiRollFac > ANTIROLL_FACTOR_MAX) fAntiRollFac = ANTIROLL_FACTOR_MAX;

			springForcePos += m_worldMatrix.rows[1].xyz() * fAntiRollFac * m_conf->m_antiRoll * ANTIROLL_SCALE;

			// apply force of wheel
			carBody->ApplyWorldForce(springForcePos, springForce);
		}
		else if(!numDriveWheelsOnGround)
		{
			if(fAccel > 0 || fabs(fBrake) > 0)
				wheel.pitchVel += torque*carBody->GetInvMass();
			else
				wheel.pitchVel = 0.0f;
		}
		else
		{
			wheel.pitchVel = dot(wheel_forward, carBody->GetLinearVelocity());
		}

		wheel.isBurningOut = bDoBurnout && (wheelConf.flags & WHEEL_FLAG_DRIVE);

		if(wheel.isBurningOut)
		{
			if((wheelConf.flags & WHEEL_FLAG_DRIVE) 
				&& wheel.pitchVel < 0 
				&& bDoBurnout)
				fPitchFac = 1.0f;

			wheel.pitch += (15.0f/wheelConf.radius) * fPitchFac * delta;
		}

		wheel.pitch += (wheel.pitchVel/wheelConf.radius)*delta;

		// normalize
		wheel.pitch = DEG2RAD(ConstrainAngle180(RAD2DEG(wheel.pitch)));
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
	//CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	float wlen = length(velocity);

	if(wlen > 6.0 && m_effectTime <= 0.0f )
	{
		CPFXAtlasGroup* effgroup = g_additPartcles;
		TexAtlasEntry_t* entry = effgroup->FindEntry("tracer");

		for(int i = 0; i < numDamageParticles; i++)
		{
			Vector3D reflDir = reflect(velocity, normal);

			Vector3D rnd_ang = VectorAngles(reflDir) + Vector3D(RandomFloat(-15,15),RandomFloat(-15,15),RandomFloat(-15,15));
			Vector3D n;
			AngleVectors(rnd_ang, &n);

			float rwlen = wlen + RandomFloat(wlen*0.15f, wlen*0.8f);

			CSparkLine* pSpark = new CSparkLine(Vector3D(position+normal*0.05f),
												n*rwlen*0.25f,	// velocity
												Vector3D(0.0f,-15.0f, 0.0f),		// gravity
												RandomFloat(50.8, 80.0), // len
												RandomFloat(0.01f, 0.02f), RandomFloat(0.01f, 0.02f), // sizes
												RandomFloat(0.8f, 1.45f),// lifetime
												effgroup, entry);  // group - texture
			effectrenderer->RegisterEffectForRender(pSpark);
		}

		m_effectTime = 0.05f;
	}

	if(wlen > 10.5f && fCollImpulse > 150 )
	{
		const float FLECK_SCALE = 0.8f;
		const int FLECK_COUNT = 2;

		for(int j = 0; j < numDamageParticles*FLECK_COUNT; j++)
		{
			ColorRGB randColor = m_carColor.col1.xyz() * RandomFloat(0.7f, 1.0f);

			Vector3D fleckPos = Vector3D(position) + Vector3D(RandomFloat(-0.05,0.05),RandomFloat(-0.05,0.05),RandomFloat(-0.05,0.05));

			CPFXAtlasGroup* feffgroup = g_translParticles;
			TexAtlasEntry_t* fentry = m_trans_fleck;

			Vector3D rnd_ang = VectorAngles(fastNormalize(normal)) + Vector3D(RandomFloat(-55,55),RandomFloat(-55,55),RandomFloat(-55,55));
			Vector3D n;
			AngleVectors(rnd_ang, &n);

			float rwlen = wlen + RandomFloat(-wlen*0.15f, wlen*0.8f);

			float fleckPower = clamp(rwlen, 3.0f, 5.0f);

			CFleckEffect* fleck = new CFleckEffect(fleckPos, n*fleckPower + Vector3D(0.0f,2.0f,0.0f),
													Vector3D(0,-16.0f,0),
													RandomFloat(0.04, 0.07)*FLECK_SCALE, RandomFloat(0.4, 0.8),0, RandomFloat(120, 300),
													randColor, feffgroup, fentry);

			effectrenderer->RegisterEffectForRender(fleck);
		}
	}
}

void CCar::OnPrePhysicsFrame(float fDt)
{
#ifndef EDITOR
	// record it
	g_replayData->UpdateReplayObject( m_replayID );
#endif // EDITOR

	if( m_enablePhysics )
	{
		// update vehicle wheels, suspension, engine
		UpdateCarPhysics(fDt);
	}
}

void CCar::OnPhysicsFrame( float fDt )
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	// retrieve body matrix again
	carBody->ConstructRenderMatrix(m_worldMatrix);

	//
	// update car collision sounds
	//

	float		fHitImpulse = 0;
	int			numHitTimes = 0;
	Vector3D	hitPos = GetOrigin();
	Vector3D	hitNormal = Vector3D(0,0,1);

	bool doImpulseSound = false;

	bool isInWater = false;

	bool velocitySet = false;

	for(int i = 0; i < carBody->m_collisionList.numElem(); i++)
	{
		CollisionPairData_t& coll = carBody->m_collisionList[i];

		// we went underwater
		if( coll.bodyB->GetContents() & OBJECTCONTENTS_WATER )
		{
			if(coll.position.y > GetOrigin().y+0.25f)
				SetDamage( GetDamage() + DAMAGE_WATERDAMAGE_RATE * fDt );

			// apply 25% impulse
			ApplyImpulseResponseTo(carBody, coll.position, coll.normal, 0.0f, 0.0f, 0.0f, 0.05f);

			if(!velocitySet)
			{
				Vector3D newVelocity = carBody->GetLinearVelocity()*0.992f;
				newVelocity.y = carBody->GetLinearVelocity().y;
				carBody->SetLinearVelocity( newVelocity );
			}
			
			carBody->TryWake();

			velocitySet = true;

			if(!IsAlive())
			{
				m_enabled = false;
				m_locked = true;
				m_sirenEnabled = false;
			}

			// make particles
			Vector3D collVelocity = carBody->GetVelocityAtWorldPoint(coll.position);

			isInWater = true;

			if(!m_inWater)
			{
				if(length(collVelocity) > 5.5f)
				{
					EmitSound_t ep;
					ep.name = "generic.waterHit";
					ep.fPitch = RandomFloat(0.92f, 1.08f);
					ep.fVolume = 1.0f;
					ep.origin = GetOrigin();
					ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;

					EmitSoundWithParams(&ep);
				}
				m_inWater = isInWater;
			}

			// spawn smoke
			if(length(collVelocity) > 2.5f && m_effectTime == 0.0f)
			{
				Vector3D waterPos = coll.position;

				Vector3D vel = collVelocity*0.85f;

				CSmokeEffect* pSmoke = new CSmokeEffect(waterPos, vel,
														RandomFloat(0.3, 0.44), RandomFloat(1.5, 1.9),
														RandomFloat(0.15f),
														-1,
														RandomFloat(5, 35), Vector3D(1,RandomFloat(-3.9, -5.2) , 1),
														ColorRGB(1), ColorRGB(1));

				effectrenderer->RegisterEffectForRender(pSmoke);
			}
		}

		if (coll.flags & COLLPAIRFLAG_OBJECTB_NO_RESPONSE)
			continue;

		Vector3D wVelocity = carBody->GetVelocityAtWorldPoint( coll.position );

		if(!(coll.flags & COLLPAIRFLAG_NO_SOUND))
			doImpulseSound = true;

		if(coll.bodyB->IsDynamic())
		{
			CEqRigidBody* body = (CEqRigidBody*)coll.bodyB;

			wVelocity -= body->GetVelocityAtWorldPoint( coll.position );
		}

		// TODO: make it safe
		if( !(coll.bodyB->GetContents() & OBJECTCONTENTS_SOLID_GROUND) &&
			coll.bodyB->GetUserData() != NULL &&
			coll.appliedImpulse != 0.0f)
		{
			CGameObject* obj = (CGameObject*)coll.bodyB->GetUserData();

			obj->OnCarCollisionEvent( coll, this );
		}

		EmitCollisionParticles(coll.position, wVelocity, coll.normal, coll.appliedImpulse/3500.0f, coll.appliedImpulse);

		float fDotUp = clamp(1.0f-fabs(dot(vec3_up, coll.normal)), 0.5f, 1.0f);
		float invMassA = carBody->GetInvMass();

		float fDamageImpact = (coll.appliedImpulse * fDotUp) * invMassA * 0.5f;

		Vector3D localHitPos = (!(Matrix4x4(m_worldMatrix))*Vector4D(coll.position, 1.0f)).xyz();

		if(fDamageImpact > DAMAGE_MINIMPACT
#ifndef EDITOR
			&& g_pGameSession->IsServer()			// process damage model only on server
#endif // EDITOR
			)
		{
			// apply visual damage
			for(int i = 0; i < CB_PART_COUNT; i++)
			{
				float fDot = dot(fastNormalize(s_BodyPartDirections[i]), fastNormalize(localHitPos));

				if(fDot < 0.2)
					continue;

				float fDamageToApply = pow(fDot, 2.0f) * fDamageImpact*DAMAGE_SCALE;

				m_bodyParts[i].damage += fDamageToApply * DAMAGE_VISUAL_SCALE;
				m_gameDamage += fDamageToApply;

				if(m_gameDamage > m_gameMaxDamage)
					m_gameDamage = m_gameMaxDamage;

				if(m_bodyParts[i].damage > 1.0f)
					m_bodyParts[i].damage = 1.0f;

				OnNetworkStateChanged(NULL);
			}

			RefreshWindowDamageEffects();
		}

		fHitImpulse += fDamageImpact * DAMAGE_SOUND_SCALE;

		hitPos = coll.position;
		hitNormal = coll.normal;

		numHitTimes++;
	}

	if(IsAlive() || isInWater)	// if car goes underwater and dies here, we need to keep underwater state
		m_inWater = isInWater;

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
				ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;

				EmitSoundWithParams(&ep);
			}
		}
	}
}

ConVar r_drawSkidMarks("r_drawSkidMarks", "1", "Draw skidmarks, 1 - player, 2 - all cars", CV_ARCHIVE);

void CCar::Simulate( float fDt )
{
	PROFILE_FUNC();

	if(!m_pPhysicsObject)
		return;

	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	if(!carBody)
		return;

	bool bLightsOn = m_enabled && (g_pGameWorld->m_envConfig.lightsType & WLIGHTS_CARS) || g_night.GetBool();
	
	if( bLightsOn && m_isLocalCar && 
		(m_bodyParts[CB_FRONT_LEFT].damage < 1.0f || m_bodyParts[CB_FRONT_RIGHT].damage < 1.0f) )
	{
		float lightIntensity = 1.0f;

		if( m_bodyParts[CB_FRONT_LEFT].damage > MIN_VISUAL_BODYPART_DAMAGE*0.5f )
		{
			if(m_bodyParts[CB_FRONT_LEFT].damage > MIN_VISUAL_BODYPART_DAMAGE)
				lightIntensity -= 0.5f;
			else
				lightIntensity -= 0.25f;
		}

		if( m_bodyParts[CB_FRONT_RIGHT].damage > MIN_VISUAL_BODYPART_DAMAGE*0.5f )
		{
			if(m_bodyParts[CB_FRONT_RIGHT].damage > MIN_VISUAL_BODYPART_DAMAGE)
				lightIntensity -= 0.5f;
			else
				lightIntensity -= 0.25f;
		}

		Vector3D startLightPos = GetOrigin() + GetForwardVector()*m_conf->m_headlightPosition.z;

		Vector3D lightPos = startLightPos + GetForwardVector()*12.0f;

		CollisionData_t light_coll;

		btSphereShape sphere(0.35f);
		g_pPhysics->TestConvexSweep(&sphere, identity(), startLightPos, lightPos, light_coll, OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT);

		wlight_t light;
		light.position = Vector4D(lerp(startLightPos, lightPos + Vector3D(0,2,0), light_coll.fract), 18.0f*(light_coll.fract+0.15f));

		light.color = ColorRGBA(0.7, 0.6, 0.5, lightIntensity);
			
		g_pGameWorld->AddLight(light);
	}

	UpdateSounds(fDt);

	m_oldControlButtons = m_controlButtons;

	m_curTime += fDt;

	m_engineSmokeTime += fDt;

	bool bDraw = g_pGameWorld->m_occludingFrustum.IsSphereVisible(GetOrigin(), length(m_conf->m_body_size));

#ifndef EDITOR
	// don't render car
	if(	g_CurrCameraMode == CAM_MODE_INCAR && 
		g_pGameSession->GetViewCar() == this)
		bDraw = false;
#endif // EDITOR

	float frontDamageSum = m_bodyParts[CB_FRONT_LEFT].damage+m_bodyParts[CB_FRONT_RIGHT].damage;

	if(	bDraw && 
		(!m_inWater || IsAlive()) && 
		frontDamageSum > 0.55f && 
		m_engineSmokeTime > 0.1f && 
		GetSpeed() < 80.0f)
	{
		ColorRGB smokeCol(0.9,0.9,0.9);

		if(frontDamageSum > 1.35f)
			smokeCol = ColorRGB(0.0);

		float rand_size = RandomFloat(-m_conf->m_body_size.x*0.5f,m_conf->m_body_size.x*0.5f);

		float alphaModifier = 1.0f - RemapValClamp(GetSpeed(), 0.0f, 80.0f, 0.0f, 1.0f);

		Vector3D smokePos = (m_worldMatrix * Vector4D(m_conf->m_enginePosition + Vector3D(rand_size,0,0), 1.0f)).xyz();

		CSmokeEffect* pSmoke = new CSmokeEffect(smokePos, Vector3D(0,1, 1),
												RandomFloat(0.1, 0.3), RandomFloat(1.0, 1.8),
												RandomFloat(1.2f),
												/*g_translParticles->FindAtlasTexture("smoke")*/-1,
												RandomFloat(25, 85), Vector3D(1,RandomFloat(-0.7, 0.2) , 1),
												smokeCol, smokeCol, alphaModifier);
		effectrenderer->RegisterEffectForRender(pSmoke);

		m_engineSmokeTime = 0.0f;
	}

	// render siren lights
	if (m_sirenEnabled)
	{
		Vector3D siren_pos_noX(0.0f, m_conf->m_sirenPositionWidth.y, m_conf->m_sirenPositionWidth.z);

		Vector3D siren_position = (m_worldMatrix * Vector4D(siren_pos_noX, 1.0f)).xyz();

		Vector3D rightVec = GetRightVector();

		switch(m_conf->m_sirenType)
		{
			case SIREN_DOUBLE:
			{
				ColorRGB col1(1,0.25,0);
				ColorRGB col2(0,0.25,1);

				PoliceSirenEffect(m_curTime, col1, siren_position, rightVec, -m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);
				PoliceSirenEffect(-m_curTime, col2, siren_position, rightVec, m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);
				
				float fSin = fabs(sinf(m_curTime*16.0f));
				float fSinFactor = clamp(fSin, 0.5f, 1.0f);

				wlight_t light;
				light.position = Vector4D(siren_position, 20.0f);
				light.color = ColorRGBA(lerp(col1,col2, fSin), fSinFactor);

				g_pGameWorld->AddLight(light);

				break;
			}
			case SIREN_DOUBLE_SINGLECOLOR:
			{
				PoliceSirenEffect(m_curTime, ColorRGB(0,0.2,1), siren_position, rightVec, -m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);
				PoliceSirenEffect(m_curTime+PI_F, ColorRGB(0,0.2,1), siren_position, rightVec, m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);

				break;
			}
			case SIREN_DOUBLE_SINGLECOLOR_RED:
			{
				PoliceSirenEffect(m_curTime, ColorRGB(1,0.2,0), siren_position, rightVec, -m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);
				PoliceSirenEffect(m_curTime+PI_F, ColorRGB(1,0.2,0), siren_position, rightVec, m_conf->m_sirenPositionWidth.x, m_conf->m_sirenPositionWidth.w);

				float fSinFactor = clamp(fabs(sinf(m_curTime*16.0f)), 0.5f, 1.0f);

				wlight_t light;
				light.position = Vector4D(siren_position, 20.0f);
				light.color = ColorRGBA(1,0.2,0, fSinFactor);

				g_pGameWorld->AddLight(light);

				break;
			}

		}
	}

	if (bDraw)
	{
		// draw effects
		Vector3D rightVec = GetRightVector();
		Vector3D forward = GetForwardVector();

		Vector3D cam_pos = g_pGameWorld->m_CameraParams.GetOrigin();

		Vector3D cam_forward;
		AngleVectors(g_pGameWorld->m_CameraParams.GetAngles(), &cam_forward);

		Vector3D headlight_pos_noX(0.0f, m_conf->m_headlightPosition.y, m_conf->m_headlightPosition.z);
		Vector3D headlight_position = (m_worldMatrix * Vector4D(headlight_pos_noX, 1.0f)).xyz();

		Vector3D brakelight_pos_noX(0.0f, m_conf->m_brakelightPosition.y, m_conf->m_brakelightPosition.z);
		Vector3D brakelight_position = (m_worldMatrix * Vector4D(brakelight_pos_noX, 1.0f)).xyz();

		Vector3D backlight_pos_noX(0.0f, m_conf->m_backlightPosition.y, m_conf->m_backlightPosition.z);
		Vector3D backlight_position = (m_worldMatrix * Vector4D(backlight_pos_noX, 1.0f)).xyz();

		Plane front_plane(forward, -dot(forward, headlight_position));
		Plane brake_plane(-forward, -dot(-forward, brakelight_position));
		Plane back_plane(-forward, -dot(-forward, backlight_position));

		bool bLightsOn = m_enabled && (g_pGameWorld->m_envConfig.lightsType & WLIGHTS_CARS) || g_night.GetBool();

		float fLightsAlpha = dot(-cam_forward, forward);

		const float HEADLIGHT_RADIUS = 0.96f;
		const float HEADLIGHTGLOW_RADIUS = 0.86f;
		const float BRAKELIGHT_RADIUS = 0.25f;
		const float BRAKEBACKLIGHT_RADIUS = 0.2f;
		const float BACKLIGHT_RADIUS = 0.125f;

		const float BRAKEBACKLIGHT_INTENSITY = 0.7f;
		const float BACKLIGHT_INTENSITY = 0.7f;

		float fHeadlightsAlpha = clamp((fLightsAlpha > 0.0f ? fLightsAlpha : 0.0f) + front_plane.Distance(cam_pos)*0.1f, 0.0f, 1.0f);

		// render some lights
		if (bLightsOn && fHeadlightsAlpha > 0.0f)
		{
			float fHeadlightsGlowAlpha = fHeadlightsAlpha*0.25f;

			float frontSizeScale = fHeadlightsAlpha;//1.0f-pow(1.0f-max(0,fLightsAlpha), 2.0f);

			Vector3D positionL = headlight_position - rightVec*m_conf->m_headlightPosition.x;
			Vector3D positionR = headlight_position + rightVec*m_conf->m_headlightPosition.x;

			ColorRGBA headlightColor(fHeadlightsAlpha, fHeadlightsAlpha, fHeadlightsAlpha, 0.6f);
			ColorRGBA headlightGlowColor(fHeadlightsGlowAlpha, fHeadlightsGlowAlpha, fHeadlightsGlowAlpha, 1);

			switch (m_conf->m_headlightType)
			{
				case LIGHTS_SINGLE:
				{
					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionL, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionR, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					break;
				}
				case LIGHTS_DOUBLE:
				{
					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					{
						DrawLightEffect(positionL - rightVec*m_conf->m_headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL - rightVec*m_conf->m_headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionL + rightVec*m_conf->m_headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionL + rightVec*m_conf->m_headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
					{
						DrawLightEffect(positionR - rightVec*m_conf->m_headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR - rightVec*m_conf->m_headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					if (m_bodyParts[CB_FRONT_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
					{
						DrawLightEffect(positionR + rightVec*m_conf->m_headlightPosition.w, headlightColor, HEADLIGHT_RADIUS*frontSizeScale, 0);
						DrawLightEffect(positionR + rightVec*m_conf->m_headlightPosition.w, headlightGlowColor, HEADLIGHTGLOW_RADIUS*frontSizeScale, 1);
					}

					break;
				}
			}
		}

		float fBrakeLightAlpha = clamp((fLightsAlpha < 0.0f ? -fLightsAlpha : 0.0f) + brake_plane.Distance(cam_pos)*0.1f, 0.0f, 1.0f);

		if ((m_brakeLightsEnabled || bLightsOn) && fBrakeLightAlpha > 0)
		{
			// draw brake lights
			float fBrakeLightAlpha2 = fBrakeLightAlpha*0.6f;

			Vector3D positionL = brakelight_position - rightVec*m_conf->m_brakelightPosition.x;
			Vector3D positionR = brakelight_position + rightVec*m_conf->m_brakelightPosition.x;

			ColorRGBA brakelightColor(fBrakeLightAlpha, 0.15*fBrakeLightAlpha, 0, 1);
			ColorRGBA brakelightColor2(fBrakeLightAlpha2, 0.15*fBrakeLightAlpha2, 0, 1);

			switch (m_conf->m_brakelightType)
			{
			case LIGHTS_SINGLE:
			{
				if (m_brakeLightsEnabled)
				{
					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionL, brakelightColor2, BRAKELIGHT_RADIUS, 2);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionR, brakelightColor2, BRAKELIGHT_RADIUS, 2);
				}

				if (bLightsOn)
				{
					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionL, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionR, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);
				}
				break;
			}
			case LIGHTS_DOUBLE:
			{
				if (m_brakeLightsEnabled)
				{
					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
						DrawLightEffect(positionL - rightVec*m_conf->m_brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionL + rightVec*m_conf->m_brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionR - rightVec*m_conf->m_brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
						DrawLightEffect(positionR + rightVec*m_conf->m_brakelightPosition.w, brakelightColor2, BRAKELIGHT_RADIUS, 2);
				}

				if (bLightsOn)
				{
					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
						DrawLightEffect(positionL - rightVec*m_conf->m_brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

					if (m_bodyParts[CB_BACK_LEFT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionL + rightVec*m_conf->m_brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE)
						DrawLightEffect(positionR - rightVec*m_conf->m_brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);

					if (m_bodyParts[CB_BACK_RIGHT].damage < MIN_VISUAL_BODYPART_DAMAGE*0.5f)
						DrawLightEffect(positionR + rightVec*m_conf->m_brakelightPosition.w, brakelightColor*BRAKEBACKLIGHT_INTENSITY, BRAKEBACKLIGHT_RADIUS, 1);
				}
				break;
			}
			}
		}

		// draw back lights
		if (m_nGear == 0 && (m_controlButtons & IN_BRAKE) && GetSpeedWheels() < 0.0f)
		{
			// draw back lights

			float fBackLightAlpha = clamp((fLightsAlpha < 0.0f ? -fLightsAlpha : 0.0f) + back_plane.Distance(cam_pos)*0.1f, 0.0f, 1.0f);
			float fBackLightAlpha2 = fBackLightAlpha * 0.3f;

			Vector3D positionL = backlight_position - rightVec*m_conf->m_backlightPosition.x;
			Vector3D positionR = backlight_position + rightVec*m_conf->m_backlightPosition.x;

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
	
	bool drawOnLocal = m_isLocalCar && (r_drawSkidMarks.GetInt() != 0);
	bool drawOnOther = !m_isLocalCar && (r_drawSkidMarks.GetInt() == 2);

	TexAtlasEntry_t* skidmarkEntry = m_veh_skidmark_asphalt;

	if( drawOnLocal || drawOnOther )
	{
		for(int i = 0; i < m_pWheels.numElem(); i++)
		{
			wheelinfo_t& wheel = m_pWheels[i];
			carWheelConfig_t& wheelConf = m_conf->m_wheels[i];

			if(!wheel.doSkidmarks)
			{
				wheel.lastDoSkidmarks = wheel.doSkidmarks;
				continue;
			}

			Matrix3x3 wheelMat = transpose(m_worldMatrix.getRotationComponent() * wheel.wheelOrient);

			if(wheel.skidMarks.numElem() > SKIDMARK_MAX_LENGTH)
			{
				wheel.skidMarks.removeIndex(0); // this is slow
			}

			if(!wheel.lastDoSkidmarks && wheel.doSkidmarks && (wheel.skidMarks.numElem() > 0))
			{
				int nLast = wheel.skidMarks.numElem()-1;

				if(wheel.skidMarks[nLast].v0.color.x < -10.0f && nLast > 0 )
					nLast -= 1;

				PFXVertexPair_t lastPair = wheel.skidMarks[nLast];
				lastPair.v0.color.x = -100.0f;

				wheel.skidMarks.append(lastPair);
			}

			

			float fSkid = (GetTractionSlidingAtWheel(i)+fabs(GetLateralSlidingAtWheel(i)))*0.15f;

			//float fSpeedSign = sign(GetSpeedWheels());

			if( fSkid > 0.05f )
			{
				float fAlpha = clamp(0.32f*fSkid, 0.0f, 0.32f);

				Vector3D velAtWheel = carBody->GetVelocityAtWorldPoint( wheel.collisionInfo.position );
				Vector3D wheelDir = fastNormalize(velAtWheel);
				Vector3D wheelRightDir = cross(wheelDir, wheel.collisionInfo.normal);//*fSpeedSign;

				PFXVertexPair_t skidmarkPair;

				int numMarkVertexPairs = wheel.skidMarks.numElem();

				Vector3D skidmarkPos = ( wheel.collisionInfo.position - wheelMat.rows[0]*wheelConf.width*sign(wheelConf.suspensionTop.x)*0.5f ) + wheel.collisionInfo.normal*0.008f + wheelDir*0.05f;

				skidmarkPair.v0 = PFXVertex_t(skidmarkPos - wheelRightDir*wheelConf.width*0.5f, vec2_zero, ColorRGBA(1,1,1,fAlpha));
				skidmarkPair.v1 = PFXVertex_t(skidmarkPos + wheelRightDir*wheelConf.width*0.5f, vec2_zero, ColorRGBA(1,1,1,fAlpha));

				if( wheel.skidMarks.numElem() > 1 )
				{
					float pointDist = length(wheel.skidMarks[numMarkVertexPairs - 2].v0.point-skidmarkPair.v0.point);

					if( pointDist > SKIDMARK_MIN_INTERVAL )
					{
						wheel.skidMarks.append(skidmarkPair);
					}
					else if(wheel.skidMarks.numElem() > 2 && 
							wheel.skidMarks[wheel.skidMarks.numElem()-1].v0.color.x >= 0.0f)
					{
						wheel.skidMarks[wheel.skidMarks.numElem()-1] = skidmarkPair;
					}
				}
				else
					wheel.skidMarks.append(skidmarkPair);
			}

			wheel.lastDoSkidmarks = wheel.doSkidmarks;
		}

		for(int i = 0; i < m_pWheels.numElem(); i++)
		{
			wheelinfo_t& wheel = m_pWheels[i];

			// add skidmark particle strip
			int numSkidMarks = 0;
			int nStartMark = 0;

			int vtxCounter = 0;

			for(int mark = 0; mark < wheel.skidMarks.numElem(); mark++)
			{
				PFXVertexPair_t& pair = wheel.skidMarks[mark];

				Vector2D coordV0 = (vtxCounter & 1) ? skidmarkEntry->rect.GetLeftBottom() : skidmarkEntry->rect.GetLeftTop();
				Vector2D coordV1 = (vtxCounter & 1) ? skidmarkEntry->rect.GetRightBottom() : skidmarkEntry->rect.GetRightTop();

				pair.v0.texcoord = coordV0;
				pair.v1.texcoord = coordV1;

				vtxCounter++;

				if(pair.v0.color.x < -10.0f)
				{
					// make them smooth
					wheel.skidMarks[nStartMark].v0.color.w = 0.0f;
					wheel.skidMarks[nStartMark].v1.color.w = 0.0f;

					//pair.v0.color.w = 0.0f;
					//pair.v1.color.w = 0.0f;


					numSkidMarks = mark-nStartMark;

					g_vehicleEffects->AddParticleStrip(&wheel.skidMarks[nStartMark].v0, numSkidMarks*2);

					nStartMark = mark+1;
					vtxCounter = 0;
				}

				if(mark == wheel.skidMarks.numElem()-1 && nStartMark < wheel.skidMarks.numElem())
				{
					wheel.skidMarks[nStartMark].v0.color.w = 0.0f;
					wheel.skidMarks[nStartMark].v1.color.w = 0.0f;

					//pair.v0.color.w = 0.0f;
					//pair.v1.color.w = 0.0f;


					numSkidMarks = wheel.skidMarks.numElem()-nStartMark;

					g_vehicleEffects->AddParticleStrip(&wheel.skidMarks[nStartMark].v0, numSkidMarks*2);

					vtxCounter = 0;
				}
			}		
		}
	}
}

void CCar::UpdateWheelEffect(int nWheel, float fDt)
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	wheelinfo_t& wheel = m_pWheels[nWheel];
	carWheelConfig_t& wheelConf = m_conf->m_wheels[nWheel];

	Matrix3x3 wheelMat = transpose(m_worldMatrix.getRotationComponent() * wheel.wheelOrient);

	wheel.lastOnGround = wheel.onGround;

	if( wheel.collisionInfo.fract >= 1.0f )
	{
		wheel.doSkidmarks = false;
		wheel.onGround = false;
		return;
	}

	wheel.onGround = true;
	wheel.doSkidmarks = (GetTractionSlidingAtWheel(nWheel) > 3.0f || fabs(GetLateralSlidingAtWheel(nWheel)) > 2.0f);

	if( wheel.onGround && wheel.surfparam != NULL )
	{
		if(wheel.surfparam->word == 'C')	// concrete/asphalt
		{
			if(!wheel.lastOnGround && m_isLocalCar)
			{
				EmitSound_t ep;

				ep.name = "generic.wheelOnGround";

				ep.origin = GetOrigin();
				ep.nFlags = EMITSOUND_FLAG_FORCE_CACHED;

				EmitSoundWithParams(&ep);
			}

			wheel.smokeTime -= fDt;

			float fSliding = GetTractionSlidingAtWheel(nWheel)+fabs(GetLateralSlidingAtWheel(nWheel));

			if(fSliding > 5.0f && g_pGameWorld->m_envConfig.weatherType == WEATHER_TYPE_CLEAR)
			{
				// spawn smoke
				if(wheel.smokeTime <= 0.0)
				{
					wheel.smokeTime = 0.1f;

					float efficency = RemapValClamp(fSliding, 5.0f, 40.0f, 0.4f, 1.0f);
					float timeScale = RemapValClamp(fSliding, 5.0f, 40.0f, 0.7f, 1.0f);

					Vector3D smoke_pos = wheel.collisionInfo.position + wheel.collisionInfo.normal * wheelConf.radius*0.5f;

					ColorRGB smokeCol(0.86f, 0.9f, 0.97f);

					CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, wheel.velocityVec*0.25f+Vector3D(0,1, 1),
															RandomFloat(0.1, 0.3)*efficency, RandomFloat(1.0, 1.8)*timeScale,
															RandomFloat(1.2f)*timeScale,
															/*g_translParticles->FindAtlasTexture("smoke")*/-1,
															RandomFloat(25, 85), Vector3D(1,RandomFloat(-0.7, 0.2) , 1),
															smokeCol, smokeCol);
					effectrenderer->RegisterEffectForRender(pSmoke);
				}
			}
		}
		else if(wheel.surfparam->word == 'S')	// sand
		{
			wheel.smokeTime -= fDt;

			if(GetTractionSlidingAtWheel(nWheel) > 5.0f || fabs(GetLateralSlidingAtWheel(nWheel)) > 2.5f)
			{
				// spawn smoke
				if(wheel.smokeTime <= 0.0)
				{
					wheel.smokeTime = 0.08f;

					Vector3D smoke_pos = wheel.pWheelObject->GetOrigin();

					Vector3D vel = -wheelMat.rows[2]*GetTractionSlidingAtWheel(nWheel) * 0.025f;
					vel += wheelMat.rows[0]*GetLateralSlidingAtWheel(nWheel) * 0.025f;

					CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, vel,
															RandomFloat(0.11, 0.14), RandomFloat(1.1, 1.5),
															RandomFloat(0.15f),
															-1,
															RandomFloat(5, 35), Vector3D(1,RandomFloat(-3.9, -5.2) , 1),
															ColorRGB(0.95f,0.757f,0.611f), ColorRGB(0.95f,0.757f,0.611f));

					effectrenderer->RegisterEffectForRender(pSmoke);
				}
			}
		}
		else if(wheel.surfparam->word == 'G' || wheel.surfparam->word == 'D')	// grass or dirt
		{
			wheel.smokeTime -= fDt;
			

			if(GetTractionSlidingAtWheel(nWheel) > 5.0f || fabs(GetLateralSlidingAtWheel(nWheel)) > 2.5f)
			{
				// spawn smoke
				if(wheel.smokeTime <= 0.0)
				{
					wheel.smokeTime = 0.08f;


					Vector3D smoke_pos = wheel.collisionInfo.position + Vector3D(0,wheelConf.radius*0.5f,0);

					Vector3D vel = -wheelMat.rows[2]*GetTractionSlidingAtWheel(nWheel) * 0.025f;
					vel += wheelMat.rows[0]*GetLateralSlidingAtWheel(nWheel) * 0.025f;

					CSmokeEffect* pSmoke = new CSmokeEffect(smoke_pos, vel,
															RandomFloat(0.11, 0.14), RandomFloat(1.5, 1.7),
															RandomFloat(0.35f),
															-1,
															RandomFloat(25, 85), Vector3D(1,RandomFloat(-3.9, -5.2) , 1),
															ColorRGB(0.1f,0.08f,0.00f), ColorRGB(0.1f,0.08f,0.00f));

					effectrenderer->RegisterEffectForRender(pSmoke);


					if(wheel.surfparam->word == 'G')
					{
						vel += Vector3D(0, 1.5f, 0);

						float len = length(vel);

						Vector3D grass_pos = wheel.collisionInfo.position;


						for(int i = 0; i < 4; i++)
						{
							Vector3D rnd_ang = VectorAngles(fastNormalize(vel)) + Vector3D(RandomFloat(-35,35),RandomFloat(-35,35),RandomFloat(-35,35));
							Vector3D n;
							AngleVectors(rnd_ang, &n);

							CSparkLine* pSpark = new CSparkLine(grass_pos,
																n*len,	// velocity
																Vector3D(0.0f,-3.0f, 0.0f),		// gravity
																RandomFloat(90.0, 100.0), // len
																RandomFloat(0.01f, 0.02f), RandomFloat(0.01f, 0.02f), // sizes
																RandomFloat(0.35f, 0.45f),// lifetime
																g_translParticles, m_trans_grasspart);  // group - texture
							effectrenderer->RegisterEffectForRender(pSpark);
						}
					}
				} // smoke time
			} // traction slide
		} // selector
	}
}

void CCar::UpdateSounds( float fDt )
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	Vector3D pos = carBody->GetPosition();
	//float fAngVelocity = length(carBody->GetAngularVelocity());

	m_pEngineSound->SetOrigin(pos);
	m_pEngineSoundLow->SetOrigin(pos);
	m_pSkidSound->SetOrigin(pos);
	m_pIdleSound->SetOrigin(pos);
	m_pHornSound->SetOrigin(pos);
	m_pSurfSound->SetOrigin(pos);

	if(m_pSirenSound)
		m_pSirenSound->SetOrigin(pos);

	Vector3D velocity = carBody->GetLinearVelocity();

	m_pEngineSound->SetVelocity(velocity);
	m_pEngineSoundLow->SetVelocity(velocity);
	m_pSkidSound->SetVelocity(velocity);
	m_pIdleSound->SetVelocity(velocity);
	m_pHornSound->SetVelocity(velocity);
	m_pSurfSound->SetVelocity(velocity);

	if(m_pSirenSound)
		m_pSirenSound->SetVelocity(velocity);

	float fTractionLevel = GetTractionSliding(true)*0.2f;
	float fSlideLevel = fabs(GetLateralSlidingAtWheels(true)*0.5f) - 0.25;

	float fSkid = (fSlideLevel + fTractionLevel)*0.5f;

	float fSkidVol = clamp((fSkid-0.5)*1.0f, 0.0, 1.0);

	if(fSkidVol > 0.08 && g_pGameWorld->m_envConfig.weatherType == WEATHER_TYPE_CLEAR)
	{
		if(m_pSkidSound->IsStopped())
			m_pSkidSound->StartSound();

		if(!m_pSurfSound->IsStopped())
			m_pSurfSound->StopSound();
	}
	else
	{
		m_pSkidSound->StopSound();

		bool anyWheelOnGround = false;

		for(int i = 0; i < m_pWheels.numElem(); i++)
		{
			if(m_pWheels[i].collisionInfo.fract < 1.0f)
			{
				anyWheelOnGround = true;
				break;
			}
		}

		if(anyWheelOnGround && m_pSurfSound->IsStopped() && g_pGameWorld->m_envConfig.weatherType != WEATHER_TYPE_CLEAR && m_isLocalCar)
		{
			m_pSurfSound->StartSound();
		}
		else if(!anyWheelOnGround && !m_pSurfSound->IsStopped())
		{
			m_pSurfSound->StopSound();
		}

		float fSpeedMod = clamp(GetSpeed()/90.0f + fTractionLevel*0.25f, 0.0f, 1.0f);
		float fSpeedModVol = clamp(GetSpeed()/20.0f + fTractionLevel*0.25f, 0.0f, 1.0f);

		m_pSurfSound->SetPitch(fSpeedMod);
		m_pSurfSound->SetVolume(fSpeedModVol);
	}

	float fWheelRad = 0.0f;

	float inv_wheelCount = 1.0f / m_pWheels.numElem();

	for(int i = 0; i < m_pWheels.numElem(); i++)
		fWheelRad += m_conf->m_wheels[i].radius;

	fWheelRad *= inv_wheelCount;

	const float IDEAL_WHEEL_RADIUS = 0.35f;
	const float SKID_RADIAL_SOUNDPITCH_SCALE = 0.68f;

	float wheelSkidPitchModifier = IDEAL_WHEEL_RADIUS - fWheelRad;

	m_pSkidSound->SetVolume(fSkidVol);

	float fSkidPitch = clamp(0.7f*fSkid+0.25f, 0.35f, 1.0f) - 0.15f*saturate(sin(m_curTime*1.25f)*8.0f - fTractionLevel);

	m_pSkidSound->SetPitch( fSkidPitch + wheelSkidPitchModifier*SKID_RADIAL_SOUNDPITCH_SCALE );

	// update engine sound
	float fRpmC = (m_fEngineRPM + 1300.0f) / 3300.0f;

	if(!m_sirenEnabled)
	{
		if((m_controlButtons & IN_HORN) && !(m_oldControlButtons & IN_HORN) && m_pHornSound->IsStopped())
		{
			m_pHornSound->StartSound();
		}
		else if(!(m_controlButtons & IN_HORN) && (m_oldControlButtons & IN_HORN))
		{
			m_pHornSound->StopSound();
		}
	}

	if(!m_enabled)
	{
		m_pEngineSound->StopSound();
		m_pEngineSoundLow->StopSound();
		m_pIdleSound->StopSound();

		if(m_pSirenSound)
			m_pSirenSound->StopSound();

		return;
	}

	m_pEngineSound->SetPitch(fRpmC);
	m_pEngineSoundLow->SetPitch(fRpmC);

	m_pIdleSound->SetPitch(1.0f + m_fEngineRPM / 4000.0f);

	//if(m_pIdleSound->IsStopped())
	//	m_pIdleSound->StartSound();

	if(m_pEngineSound->IsStopped())
	{
		m_pEngineSound->StartSound();

		if(m_isLocalCar)
			m_pEngineSoundLow->StartSound();
	}

	float fAccelOrBrake = ((m_controlButtons & IN_ACCELERATE) || (m_controlButtons & IN_BRAKE)) && !(m_controlButtons & IN_HANDBRAKE);

	float fIdleFac = (m_fEngineRPM > 1800) || fAccelOrBrake ? 0.0f : 1.0f;

	m_engineIdleFactor = clamp(m_engineIdleFactor - sign(m_engineIdleFactor-fIdleFac)*fDt*(1.5f+m_fAcceleration), 0.0f, 1.0f);

	float fEngineSoundVol = clamp((1.0f - m_engineIdleFactor), 0.45f, 1.0f);
	
	float fRPMDiff = clamp( RemapVal(fabs(GetRPM() - m_fEngineRPM), 0, 50.0f, 0.0f, 1.0f) + m_fAccelEffect, 0.0f, 1.0f);

	if(!m_isLocalCar)
		fRPMDiff = 1.0f;

	m_pEngineSound->SetVolume(fEngineSoundVol * fRPMDiff);

	if(m_isLocalCar)
		m_pEngineSoundLow->SetVolume(fEngineSoundVol * (1.0f-fRPMDiff));

	if(m_engineIdleFactor <= 0.05f && !m_pIdleSound->IsStopped())
	{
		m_pIdleSound->StopSound();
	}
	else if(m_engineIdleFactor > 0.05f && m_pIdleSound->IsStopped())
	{
		m_pIdleSound->StartSound();
	}

	m_pIdleSound->SetVolume(m_engineIdleFactor);

	if( m_pSirenSound )
	{
		if(m_sirenEnabled != m_oldSirenState)
		{
			if(m_sirenEnabled)
			{
				m_pHornSound->StopSound();
				m_pSirenSound->StartSound();
			}
			else
				m_pSirenSound->StopSound();

			m_oldSirenState = m_sirenEnabled;
		}

		if(m_sirenEnabled)
		{
			int sampleId = m_pSirenSound->GetEmitParams().sampleId;

			if((m_controlButtons & IN_HORN) && sampleId == 0)
				sampleId = 1;
			else if(!(m_controlButtons & IN_HORN) && sampleId == 1)
				sampleId = 0;

			if( m_pSirenSound->GetEmitParams().sampleId != sampleId )
			{
				m_pSirenSound->StopSound();
				m_pSirenSound->GetEmitParams().sampleId = sampleId;
				m_pSirenSound->StartSound();
			}
		}
	}
}

float CCar::GetSpeedWheels() const
{
	if(!m_conf)
		return 0.0f;

	float wheelFac = 1.0f / m_pWheels.numElem();
	float fResult = 0.0f;

	for(int i = 0; i < m_pWheels.numElem(); i++)
	{
		fResult += m_pWheels[i].pitchVel * m_conf->m_wheels[i].radius;
	}

	return fResult * SPEEDO_SCALE * wheelFac;
}

int	CCar::GetWheelCount()
{
	return m_pWheels.numElem();
}

float CCar::GetWheelSpeed(int index)
{
	return m_pWheels[index].pitchVel/* * m_pWheels[index].radius*/ * SPEEDO_SCALE;
}

float CCar::GetSpeed() const
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;
	return length(carBody->GetLinearVelocity().xz()) * SPEEDO_SCALE;
}

float CCar::GetRPM()
{
	return fabs(m_radsPerSec * ( 60.0f / ( 2.0f * PI_F ) ) );
}

int CCar::GetGear()
{
	return m_nGear;
}

float CCar::GetLateralSlidingAtBody()
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	Vector3D velocityAtCollisionPoint = carBody->GetLinearVelocity();

	float slip = dot(velocityAtCollisionPoint, GetRightVector());
	return slip;
}

float CCar::GetLateralSlidingAtWheels(bool surfCheck)
{
	float val = 0.0f;

	int nCount = 0;

	for(int i = 0; i < m_pWheels.numElem(); i++)
	{
		if(surfCheck)
		{
			if(m_pWheels[i].surfparam && m_pWheels[i].surfparam->word == 'C')
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

float CCar::GetLateralSlidingAtWheel(int wheel)
{
	CEqRigidBody* carBody = m_pPhysicsObject->m_object;

	if(m_pWheels[wheel].collisionInfo.fract >= 1.0f)
		return 0.0f;

	// compute wheel world rotation (turn + body rotation)
	Matrix4x4 wheelTransform = Matrix4x4(m_pWheels[wheel].wheelOrient) * transpose(m_worldMatrix);

	Vector3D wheel_pos = wheelTransform.rows[3].xyz();

	Vector3D velocityAtCollisionPoint = carBody->GetVelocityAtWorldPoint(wheel_pos);

	// rotate vector to get right vector
	float slip = dot(velocityAtCollisionPoint, GetRightVector() );
	return slip;
}

float CCar::GetTractionSliding(bool surfCheck)
{
	float val = 0.0f;

	int nCount = 0;

	for(int i = 0; i < m_pWheels.numElem(); i++)
	{
		if(surfCheck)
		{
			if(m_pWheels[i].surfparam && m_pWheels[i].surfparam->word == 'C')
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

float CCar::GetTractionSlidingAtWheel(int wheel)
{
	if(!m_pWheels[wheel].onGround)
		return 0.0f;

	return abs(GetSpeed() - abs(GetWheelSpeed(wheel))) + m_pWheels[wheel].isBurningOut*100.0f;
}

bool CCar::IsAnyWheelOnGround() const
{
	for(int i = 0; i < m_pWheels.numElem(); i++)
	{
		if(m_pWheels[i].collisionInfo.fract < 1.0f)
			return true;
	}

	return false;
}

ConVar r_drawCars("r_drawCars", "1", "Render vehicles", CV_CHEAT);

extern ConVar r_enableObjectsInstancing;

void CCar::DrawBody( int nRenderFlags )
{
	if( !m_pModel )
		return;

	studiohdr_t* pHdr = m_pModel->GetHWData()->pStudioHdr;

	float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );

	int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

	if(nLOD > 0 && r_enableObjectsInstancing.GetBool() && m_pModel->GetInstancer())
	{
		float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );
		int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

		CGameObjectInstancer* instancer = (CGameObjectInstancer*)m_pModel->GetInstancer();
		gameObjectInstance_t& inst = instancer->NewInstance( m_bodyGroupFlags, nLOD );
		inst.world = m_worldMatrix;

		// g_pShaderAPI->SetShaderConstantVector4D("CarColor", m_carColor);
		return;
	}

	//--------------------------------------------------------

	static float bodyDamages[16];

	//for(int i = 0; i < 64; i++)
	//	bodyDamages[i] = 0.0f;

	bool bApplyDamage = false;

	for(int i = 0; i < CB_PART_WINDOW_PARTS; i++)
	{
		int idx = m_bodyParts[i].boneIndex;

		if(idx == -1)
			continue;

		bodyDamages[idx] = m_bodyParts[i].damage;

		if(m_bodyParts[i].damage > 0)
			bApplyDamage = true;
	}

	materials->SetCullMode((nRenderFlags & RFLAG_FLIP_VIEWPORT_X) ? CULL_FRONT : CULL_BACK);

	for(int i = 0; i < pHdr->numbodygroups; i++)
	{
		int bodyGroupLOD = nLOD;

		// TODO: check bodygroups for rendering

		int nLodModelIdx = pHdr->pBodyGroups(i)->lodmodel_index;
		int nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];

		// get the right LOD model number
		while(nModDescId == -1 && bodyGroupLOD > 0)
		{
			bodyGroupLOD--;
			nModDescId = pHdr->pLodModel(nLodModelIdx)->lodmodels[ bodyGroupLOD ];
		}

		if(nModDescId == -1)
			continue;

		// render model groups that in this body group
		for(int j = 0; j < pHdr->pModelDesc(nModDescId)->numgroups; j++)
		{
			//materials->SetSkinningEnabled(true);

			// reset shader constants (required)
			g_pShaderAPI->Reset(STATE_RESET_SHADERCONST);

			IMaterial* pMaterial = m_pModel->GetMaterial(nModDescId, j);
			materials->BindMaterial(pMaterial, false);

			g_pShaderAPI->SetShaderConstantArrayFloat("BodyDamage", bodyDamages, 16);

			ColorRGBA colors[2];

			if(m_conf->m_useBodyColor)
			{
				colors[0] = m_carColor.col1;
				colors[1] = m_carColor.col2;
			}
			else
			{
				colors[0] = color4_white;
				colors[1] = color4_white;
			}

			g_pShaderAPI->SetShaderConstantArrayVector4D("CarColor", colors, 2);

			// setup our brand new vertex format
			g_pShaderAPI->SetVertexFormat( g_pGameWorld->m_vehicleVertexFormat );

			// bind
			m_pModel->SetupVBOStream( 0 );

			if( m_pDamagedModel && bApplyDamage )
				m_pDamagedModel->SetupVBOStream( 1 );
			else
				m_pModel->SetupVBOStream( 1 );

			//m_pModel->PrepareForSkinning( m_BoneMatrixList );

			m_pModel->DrawGroup( nModDescId, j, false );
		}
	}
}

void CCar::Draw( int nRenderFlags )
{
	if(!r_drawCars.GetBool())
		return;

	bool bDraw = true;

#ifndef EDITOR
	// don't render car
	if(	g_pCameraAnimator->GetMode() == CAM_MODE_INCAR && 
		g_pGameSession->GetViewCar() == this)
		bDraw = false;
#endif // EDITOR

	CEqRigidBody* pCarBody = m_pPhysicsObject->m_object;

	//Matrix4x4 m;
	//pCarBody->ConstructRenderMatrix(m);

	pCarBody->UpdateBoundingBoxTransform();
	m_bbox = pCarBody->m_aabb_transformed;

	float camDist = g_pGameWorld->m_CameraParams.GetLODScaledDistFrom( GetOrigin() );
	int nLOD = m_pModel->SelectLod( camDist ); // lod distance check

	if(nLOD == 0)
	{
		//
		// SHADOW TEST CODE
		//
		if(bDraw)
		{
			TexAtlasEntry_t* carShadow = m_veh_shadow;

			if(carShadow)
			{
				PFXVertex_t verts[4];

				Vector3D shadow_size = m_conf->m_body_size + Vector3D(0.35f,0,0.25f);

				verts[0].point = GetOrigin() + shadow_size.z*GetForwardVector() + shadow_size.x*GetRightVector();
				verts[1].point = GetOrigin() + shadow_size.z*GetForwardVector() + shadow_size.x*-GetRightVector();
				verts[2].point = GetOrigin() + shadow_size.z*-GetForwardVector() + shadow_size.x*GetRightVector();
				verts[3].point = GetOrigin() + shadow_size.z*-GetForwardVector() + shadow_size.x*-GetRightVector();

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

					verts[i].point = lerp(traceFrom, traceTo, shadowcoll.fract) + Vector3D(0.0f, 0.01f, 0.0f);

					verts[i].color = ColorRGBA(1.0f, 1.0f, 1.0f, clamp(0.8f + 1.0f-(shadowcoll.fract*8.0f), 0.0f, 1.0f));
				}

				g_vehicleEffects->AddParticleQuad(verts);
			}
		}

		for(int i = 0; i < m_pWheels.numElem(); i++)
		{
			//Matrix4x4 w = m;
			wheelinfo_t& wheel = m_pWheels[i];
			carWheelConfig_t& wheelConf = m_conf->m_wheels[i];

			Vector3D wheels_offs = vec3_zero;

			float wheelVisualPosFactor = wheel.collisionInfo.fract;

			if(wheelVisualPosFactor < wheelConf.visualTop)
				wheelVisualPosFactor = wheelConf.visualTop;

			Vector3D wheelSuspDir = fastNormalize(wheelConf.suspensionTop - wheelConf.suspensionBottom);
			Vector3D wheelCenterPos = lerp(wheelConf.suspensionTop, wheelConf.suspensionBottom, wheelVisualPosFactor) + wheelSuspDir*wheelConf.radius;

			Matrix4x4 wheelTranslationLocal = m_worldMatrix*(translate(Vector3D(wheelCenterPos + wheels_offs - Vector3D(wheelConf.woffset, 0, 0)))*Matrix4x4(transpose(wheel.wheelOrient)*rotateX3(wheel.pitch))*scale4(wheelConf.width,wheelConf.radius*2,wheelConf.radius*2));

			wheel.pWheelObject->SetOrigin(transpose(wheelTranslationLocal).getTranslationComponent());

			if(bDraw)
			{
				wheel.pWheelObject->m_bbox = m_bbox;
				wheel.pWheelObject->m_worldMatrix = wheelTranslationLocal;

				wheel.pWheelObject->Draw(nRenderFlags);
			}
		}
	}

	if (bDraw)
	{
		materials->SetMatrix( MATRIXMODE_WORLD, m_worldMatrix);

		// draw car body with damage effects
		DrawBody(nRenderFlags);
	}
}


void CCar::OnPackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnPackMessage(buffer);

	// send extra vars
	float damageFloats[CB_PART_COUNT];

	float fOverall = 0.0f;

	for(int i = 0; i < CB_PART_COUNT; i++)
	{
		fOverall += m_bodyParts[i].damage;
		damageFloats[i] = m_bodyParts[i].damage;
	}

	buffer->WriteData(damageFloats, sizeof(float)*CB_PART_COUNT);
}

void CCar::OnUnpackMessage( CNetMessageBuffer* buffer )
{
	BaseClass::OnUnpackMessage(buffer);

	// recv extra vars
	float damageFloats[CB_PART_COUNT];

	buffer->ReadData(damageFloats, sizeof(float)*CB_PART_COUNT);

	float fOverall = 0.0f;
	for(int i = 0; i < CB_PART_COUNT; i++)
	{
		fOverall += damageFloats[i];
		m_bodyParts[i].damage = damageFloats[i];
	}

	RefreshWindowDamageEffects();
}

//---------------------------------------------------------------------------
// Gameplay-related
//---------------------------------------------------------------------------

bool CCar::IsAlive() const
{
	return m_gameDamage < m_gameMaxDamage;
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
	m_gameDamage = 0.0f;

	for(int i = 0; i < CB_PART_COUNT; i++)
		m_bodyParts[i].damage = 0.0f;

	if(unlock)
		m_locked = false;

	RefreshWindowDamageEffects();
}

void CCar::SetColorScheme( int colorIdx )
{
	if(colorIdx >= 0 && colorIdx < m_conf->m_colors.numElem())
		m_carColor = m_conf->m_colors[colorIdx];
	else
		m_carColor = ColorRGBA(1,1,1,1);
}

void CCar::SetInfiniteMass( bool infMass )
{
	if(infMass)
		m_pPhysicsObject->m_object->m_flags |= BODY_INFINITEMASS;
	else
		m_pPhysicsObject->m_object->m_flags &= ~BODY_INFINITEMASS;
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
}

bool CCar::IsEnabled() const
{
	return m_enabled;
}

bool CCar::IsInWater() const
{
	return m_inWater;
}

void CCar::SetFelony(float percentage)
{
	m_gameFelony = percentage;
}

float CCar::GetFelony() const
{
	return m_gameFelony;
}

void CCar::IncrementPursue()
{
	m_numPursued++;
}

void CCar::DecrementPursue()
{
	m_numPursued--;
}

int CCar::GetPursuedCount() const
{
	return m_numPursued;
}

#ifndef NO_LUA
OOLUA_EXPORT_FUNCTIONS(
	CCar, 
	SetColorScheme, 
	SetMaxDamage, 
	SetMaxSpeed,
	SetTorqueScale,
	SetDamage, 
	Repair, 
	SetFelony,
	Lock, 
	Enable, 
	GetLateralSliding, 
	GetTractionSliding, 
	SetInfiniteMass
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CCar, 
	IsAlive,
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
	IsLocked,
	IsEnabled,
	GetPursuedCount
)

#endif // NO_LUA