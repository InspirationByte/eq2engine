//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef CAR_H
#define CAR_H

#include "world.h"
#include "GameSoundEmitterSystem.h"
#include "CameraAnimator.h"

#include "EqParticles.h"

const float _oneBy1024 = 1.0f / 1023.0f;

#define SPEEDO_SCALE				(3.0f)

inline bool IsCar(CGameObject* obj) { return obj->ObjType() >= GO_CAR && obj->ObjType() <= GO_CAR_AI;}

enum EWheelFlags
{
	WHEEL_FLAG_DRIVE		= (1 << 0),
	WHEEL_FLAG_STEER		= (1 << 1),
	WHEEL_FLAG_HANDBRAKE	= (1 << 2),
};

struct carWheelConfig_t
{
	char				wheelName[16];

	Vector3D			suspensionTop;
	Vector3D			suspensionBottom;

	float				springConst;
	float				dampingConst;

	float				brakeTorque;

	int					flags; // EWheelFlags

	float				steerMultipler;

	float				width;
	float				radius;
	float				woffset;

	float				visualTop;

	int					wheelBodyPart;	// damage effets on wheels like bad breakage, suspension effects
};

struct carColorScheme_t
{
	carColorScheme_t() {}
	carColorScheme_t(const ColorRGBA& col)
	{
		col1 = col;
		col2 = col;
	}

	TVec4D<half> col1;
	TVec4D<half> col2;
};



struct carConfigEntry_t
{
	EqString					carName;
	EqString					carScript;
	uint						scriptCRC;	// for network and replays

	EqString					m_cleanModelName;
	EqString					m_damModelName;

	//kvkeybase_t				scriptKvs;	// script keyvalues

	DkList<carWheelConfig_t>	m_wheels;

	DkList<float>				m_gears;

	Vector3D					m_body_size;
	Vector3D					m_body_center;

	Vector3D					m_virtualMassCenter;

	float						m_mass;

	float						m_antiRoll;

	float						m_differentialRatio;
	float						m_torqueMult;
	float						m_transmissionRate;

	float						m_maxSpeed;

	carCameraConfig_t			m_cameraConf;

	Vector4D					m_sirenPositionWidth;

	int							m_headlightType;
	Vector4D					m_headlightPosition;

	Vector4D					m_backlightPosition;

	int							m_brakelightType;
	Vector4D					m_brakelightPosition;

	Vector3D					m_enginePosition;

	int							m_sirenType;

	bool						m_useBodyColor;

	DkList<carColorScheme_t>	m_colors;

	//--------------------------------------------

	EqString					m_sndEngineIdle;
	EqString					m_sndEngineRPMLow;
	EqString					m_sndEngineRPMHigh;
	EqString					m_sndHornSignal;
	EqString					m_sndSiren;
};

bool ParseCarConfig( carConfigEntry_t* conf, const kvkeybase_t* kvs );

//------------------------------------

static carWheelConfig_t _wheelNull;

struct PFXVertexPair_t
{
	PFXVertex_t v0;
	PFXVertex_t v1;
};

class CCarWheelModel;

struct wheelinfo_t
{
	wheelinfo_t()
	{
		pWheelObject = NULL;
		pitch = 0.0f;
		pitchVel = 0.0f;
		isBurningOut = false;
		onGround = true;
		lastOnGround = true;
		smokeTime = 0.0f;
		surfparam = NULL;

		doSkidmarks = false;
		lastDoSkidmarks = false;
		bodyIndex = 0;
		velocityVec = vec3_zero;
	}

	DkList<PFXVertexPair_t>	skidMarks;

	// visual object
	CCarWheelModel*			pWheelObject;

	//carWheelConfig_t*		config;

	eqPhysSurfParam_t*		surfparam;

	CollisionData_t			collisionInfo;
	Matrix3x3				wheelOrient;

	Vector3D				velocityVec;

	float					pitch;
	float					pitchVel;
	bool					isBurningOut;

	//char					surfWord;
	bool					onGround;
	bool					lastOnGround;

	bool					doSkidmarks;
	bool					lastDoSkidmarks;

	float					smokeTime;

	int						bodyIndex;
};

enum ELightsType
{
	LIGHTS_SINGLE,
	LIGHTS_DOUBLE,
};

enum ESirenType
{
	SIREN_NONE = 0,
	SIREN_SINGLE,
	SIREN_DOUBLE,
	SIREN_DOUBLE_SINGLECOLOR,
	SIREN_DOUBLE_SINGLECOLOR_RED,
};

enum ECarBodyPart
{
	CB_FRONT_LEFT,
	CB_FRONT_RIGHT,

	CB_SIDE_LEFT,
	CB_SIDE_RIGHT,

	CB_BACK_LEFT,
	CB_BACK_RIGHT,

	CB_PART_COUNT,
};

enum ECarWindowParts
{
	CB_WINDOW_FRONT = 6,
	CB_WINDOW_BACK,

	CB_WINDOW_LEFT,
	CB_WINDOW_RIGHT,

	CB_PART_WINDOW_PARTS = CB_PART_COUNT + 4
};

static const char* s_pBodyPartsNames[] =
{
	"front.left",
	"front.right",

	"side.left",
	"side.right",

	"back.left",
	"back.right",

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
};

struct carbodypart_t
{
	carbodypart_t()
	{
		boneIndex = -1;
		damage = 0.0f;
		pos = vec3_zero;
		radius = 0.0f;
	}

	short		boneIndex;
	float		damage;

	Vector3D	pos;
	float		radius;
};

class CCarWheelModel : public CGameObject
{
public:
	DECLARE_CLASS( CCarWheelModel, CGameObject )

	void			SetModelPtr(IEqModel* modelPtr);
	void			Draw( int nRenderFlags );
};

class CCar : public CGameObject
{
	friend class CAITrafficCar;
	friend class CAIPursuerCar;
	friend class CReplayData;

public:
	DECLARE_NETWORK_TABLE();
	DECLARE_CLASS( CCar, CGameObject )

	CCar();
	CCar( carConfigEntry_t* config );
	~CCar(){}

	virtual void			Precache();
	virtual void			OnRemove();

	void					DebugReloadCar();

	virtual void			Spawn();

	void					AlignToGround();	// align car to ground

	//-----------------------------------

	float					GetRPM();
	int						GetGear();

	int						GetWheelCount();
	float					GetWheelSpeed(int index);

	float					GetLateralSlidingAtBody();						// returns non-absolute lateral (side) sliding from body
	float					GetLateralSlidingAtWheels(bool surfCheck);		// returns non-absolute lateral (side) sliding from all wheels
	float					GetLateralSlidingAtWheel(int wheel);			// returns non-absolute lateral (side) sliding

	float					GetTractionSliding(bool surfCheck);		// returns absolute traction sliding
	float					GetTractionSlidingAtWheel(int wheel);	// returns absolute traction sliding from all wheels

	//-----------------------------------

	void					EmitCollisionParticles(const Vector3D& position, const Vector3D& velocity, const Vector3D& normal, int numSparks = 1, float fCollImpulse = 180.0f);

	void					SetControlButtons(int flags);
	int						GetControlButtons();

	void					SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering);
	void					GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering);

	void					SetOrigin(const Vector3D& origin);
	void					SetAngles(const Vector3D& angles);
	void					SetOrientation(const Quaternion& q);

	void					SetVelocity(const Vector3D& vel);
	void					SetAngularVelocity(const Vector3D& vel);

	Vector3D				GetOrigin() const;
	Vector3D				GetAngles() const;
	Quaternion				GetOrientation() const;

	Vector3D				GetForwardVector() const;
	Vector3D				GetUpVector() const;
	Vector3D				GetRightVector() const;

	Vector3D				GetVelocity() const;
	Vector3D				GetAngularVelocity() const;

	CEqRigidBody*			GetPhysicsBody() const;

	void					L_SetContents(int contents);
	void					L_SetCollideMask(int contents);

	int						L_GetContents();
	int						L_GetCollideMask();

	void					RefreshWindowDamageEffects();

	virtual int				ObjType() const		{ return GO_CAR; }

	virtual void			OnPackMessage( CNetMessageBuffer* buffer );
	virtual void			OnUnpackMessage( CNetMessageBuffer* buffer );

	//---------------------------------------------------------------------------
	// Gameplay-related
	//---------------------------------------------------------------------------

	void					SetCarColour( const carColorScheme_t& col ) { m_carColor = col; }
	carColorScheme_t		GetCarColour() const { return m_carColor; }

	void					SetColorScheme( int colorIdx );

	bool					IsAnyWheelOnGround() const;

	float					GetSpeedWheels() const;
	float					GetSpeed() const;

	bool					IsAlive() const;

	void					SetMaxDamage( float fDamage );
	float					GetMaxDamage() const;

	void					SetMaxSpeed( float fSpeed );
	float					GetMaxSpeed() const;

	void					SetTorqueScale( float fScale );
	float					GetTorqueScale() const;

	void					SetDamage( float damage );

	float					GetDamage() const;
	float					GetBodyDamage() const; // max. 6.0f

	void					SetFelony(float percentage);
	float					GetFelony() const;

	void					Repair(bool unlock = false);

	void					SetInfiniteMass( bool infMass );

	void					Lock(bool lock = true);	// locks car, false to unlock
	bool					IsLocked() const;

	void					Enable(bool enable = true);	// disables or enables car
	bool					IsEnabled() const;

	bool					IsInWater() const;

	void					IncrementPursue();
	void					DecrementPursue();
	int						GetPursuedCount() const;

public:
	carConfigEntry_t*		m_conf;

	bool					m_isLocalCar;

	short					m_accelRatio;
	short					m_brakeRatio;
	short					m_steerRatio;

	CNetworkVar(bool,		m_sirenEnabled);
	bool					m_oldSirenState;

protected:
	void					CreateCarPhysics();
	void					InitCarSound();

	virtual void			Simulate( float fDt );

	virtual void			OnPrePhysicsFrame( float fDt );
	virtual void			OnPhysicsFrame( float fDt );

	void					Draw( int nRenderFlags );
	void					DrawBody( int nRenderFlags );

	void					UpdateSounds( float fDt );
	void					UpdateWheelEffect(int nWheel, float fDt);

	void					UpdateCarPhysics(float fDt);

	CPhysicsHFObject*		m_pPhysicsObject;

	CEqCollisionObject*		m_lastCollidingObject;

	bool					m_enablePhysics;
	
	//
	// realtime values
	//

	DkList<wheelinfo_t>		m_pWheels;

	carbodypart_t			m_bodyParts[CB_PART_WINDOW_PARTS];

	float					m_fEngineRPM;
	float					m_engineIdleFactor;

	int						m_nGear;
	int						m_nPrevGear;

	FReal					m_radsPerSec;	// current rotations per sec

	FReal					m_fAcceleration;
	FReal					m_fAccelEffect;
	FReal					m_steeringHelper;
	FReal					m_steering;

	//
	// dynamic config parts
	//

	float					m_maxSpeed;
	float					m_torqueScale;

	//
	// visual properties
	//

	carColorScheme_t		m_carColor;

	IEqModel*				m_pDamagedModel;

	//---------------------------------------------

	ISoundController*		m_pSkidSound;
	ISoundController*		m_pSurfSound;

	ISoundController*		m_pEngineSound;
	ISoundController*		m_pEngineSoundLow;

	ISoundController*		m_pIdleSound;
	ISoundController*		m_pHornSound;
	ISoundController*		m_pSirenSound;
	
	bool					m_brakeLightsEnabled;

	int						m_controlButtons;
	int						m_oldControlButtons;

	float					m_curTime;

	float					m_effectTime;
	float					m_engineSmokeTime;

	// gameplay
	CNetworkVar(float,		m_gameDamage);
	CNetworkVar(float,		m_gameMaxDamage);

	CNetworkVar(float,		m_gameFelony);	// felony percentage
	CNetworkVar(int,		m_numPursued);

	CNetworkVar(bool,		m_locked);
	CNetworkVar(bool,		m_enabled);
	CNetworkVar(bool,		m_autohandbrake);
	CNetworkVar(bool,		m_inWater);

	TexAtlasEntry_t*		m_trans_grasspart;
	TexAtlasEntry_t*		m_trans_smoke2;
	TexAtlasEntry_t*		m_trans_fleck;
	TexAtlasEntry_t*		m_veh_skidmark_asphalt;
	TexAtlasEntry_t*		m_veh_shadow;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CCar, CGameObject)

	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(AlignToGround)

	OOLUA_MFUNC_CONST(IsAnyWheelOnGround)

	OOLUA_MFUNC_CONST(IsAlive)

	OOLUA_MFUNC(SetMaxDamage)
	OOLUA_MFUNC_CONST(GetMaxDamage)

	OOLUA_MFUNC(SetMaxSpeed)
	OOLUA_MFUNC_CONST(GetMaxSpeed)

	OOLUA_MFUNC(SetTorqueScale)
	OOLUA_MFUNC_CONST(GetTorqueScale)

	OOLUA_MFUNC(SetDamage)

	OOLUA_MFUNC_CONST(GetDamage)
	OOLUA_MFUNC_CONST(GetBodyDamage)

	OOLUA_MFUNC(Repair)

	OOLUA_MFUNC(SetFelony)
	OOLUA_MFUNC_CONST(GetFelony)

	OOLUA_MFUNC(SetColorScheme)

	OOLUA_MFUNC_CONST(GetAngularVelocity)

	OOLUA_MFUNC_CONST(GetForwardVector)
	OOLUA_MFUNC_CONST(GetRightVector)
	OOLUA_MFUNC_CONST(GetUpVector)

	OOLUA_MFUNC_CONST(GetSpeed)
	OOLUA_MFUNC_CONST(GetSpeedWheels)

	OOLUA_MFUNC(Enable)
	OOLUA_MFUNC_CONST(IsEnabled)

	OOLUA_MFUNC_CONST(GetPursuedCount)
	
	OOLUA_MFUNC(Lock)
	OOLUA_MFUNC_CONST(IsLocked)

	OOLUA_MFUNC(SetInfiniteMass)

	OOLUA_MEM_FUNC_RENAME(GetLateralSliding, float, GetLateralSlidingAtBody)
	OOLUA_MFUNC(GetTractionSliding)
OOLUA_PROXY_END
#endif // #ifndef __INTELLISENSE__
#endif // NO_LUA

#endif // CAR_H
