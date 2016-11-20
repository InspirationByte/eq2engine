//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef CAR_H
#define CAR_H

#include "world.h"
#include "CameraAnimator.h"
#include "EqParticles.h"

const float _oneBy1024 = 1.0f / 1023.0f;

#define MPS_TO_KPH		(3.6f)
#define KPH_TO_MPS		(0.27778f)

inline bool IsCar(CGameObject* obj) { return obj->ObjType() >= GO_CAR && obj->ObjType() <= GO_CAR_AI;}

// body part ids
enum ECarBodyPart
{
	CB_FRONT_LEFT,
	CB_FRONT_RIGHT,

	CB_SIDE_LEFT,
	CB_SIDE_RIGHT,

	CB_BACK_LEFT,
	CB_BACK_RIGHT,

	CB_TOP_ROOF,

	CB_PART_COUNT,
};

// window bodypart ids
enum ECarWindowParts
{
	CB_WINDOW_FRONT = 7,
	CB_WINDOW_BACK,

	CB_WINDOW_LEFT,
	CB_WINDOW_RIGHT,

	CB_PART_WINDOW_PARTS = CB_PART_COUNT + 4
};

// front and back light types
enum ELightsType
{
	LIGHTS_SINGLE,
	LIGHTS_DOUBLE,
	LIGHTS_DOUBLE_VERTICAL,
};

// siren light types
enum EServiceLightsType
{
	SERVICE_LIGHTS_NONE = 0,
	SERVICE_LIGHTS_SINGLE,
	SERVICE_LIGHTS_DOUBLE,
	SERVICE_LIGHTS_DOUBLE_SINGLECOLOR,
	SERVICE_LIGHTS_DOUBLE_SINGLECOLOR_RED,
};

enum ECarLightTypeFlags
{
	CAR_LIGHT_HEADLIGHTS		= (1 << 0),		// front lights
	CAR_LIGHT_HEADLIGHTS_FAR	= (1 << 1),		// front far lights
	CAR_LIGHT_BRAKE				= (1 << 2),		// brake lights
	CAR_LIGHT_REVERSELIGHT		= (1 << 3),		// white back lights
	CAR_LIGHT_DIM_LEFT			= (1 << 4),		// dimensional left lights
	CAR_LIGHT_DIM_RIGHT			= (1 << 5),		// dimensional right lights
	CAR_LIGHT_SERVICELIGHTS		= (1 << 6),		// flashing lights of service cars

	CAR_LIGHT_EMERGENCY			= (CAR_LIGHT_DIM_LEFT | CAR_LIGHT_DIM_RIGHT),	// emergency light flags (dim. left and right)
};

enum EEngineType
{
	CAR_ENGINE_PETROL = 0,
	CAR_ENGINE_DIESEL,
};

// wheel flags
enum EWheelFlags
{
	WHEEL_FLAG_DRIVE		= (1 << 0),
	WHEEL_FLAG_STEER		= (1 << 1),
	WHEEL_FLAG_HANDBRAKE	= (1 << 2),
};

// wheel configuration
struct carWheelConfig_t
{
	char				wheelName[16];
	char				hubcapWheelName[16];
	char				hubcapName[16];

	Vector3D			suspensionTop;
	float				steerMultipler;

	Vector3D			suspensionBottom;
	float				brakeTorque;

	float				springConst;
	float				dampingConst;
	float				width;
	float				radius;

	float				woffset;
	float				visualTop;

	int					wheelBodyPart;	// damage effets on wheels like bad breakage, suspension effects
	int					flags;			// EWheelFlags
};

// color scheme
struct carColorScheme_t
{
	char name[16];

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
	uint						scriptCRC;	// for network and replays

	Vector3D					m_body_size;
	float						m_antiRoll;

	Vector3D					m_body_center;
	float						m_handbrakeScale;

	Vector3D					m_virtualMassCenter;
	float						m_mass;
	
	EEngineType					m_engineType;
	float						m_differentialRatio;
	float						m_torqueMult;
	float						m_transmissionRate;
	float						m_maxSpeed;

	float						m_burnoutMaxSpeed;
	float						m_steeringSpeed;

	carCameraConfig_t			m_cameraConf;

	Vector4D					m_sirenPositionWidth;
	int							m_sirenType;

	Vector4D					m_headlightPosition;
	int							m_headlightType;

	Vector4D					m_backlightPosition;

	Vector4D					m_brakelightPosition;
	int							m_brakelightType;

	Vector4D					m_frontDimLights;
	Vector4D					m_backDimLights;

	Vector3D					m_enginePosition;

	Vector3D					m_exhaustPosition;
	int							m_exhaustDir;		// 0 - back, 1 - left, 2 - up

	bool						m_useBodyColor;

	DkList<carWheelConfig_t>	m_wheels;
	DkList<float>				m_gears;
	DkList<carColorScheme_t>	m_colors;

	//--------------------------------------------

	EqString					m_sndEngineIdle;
	EqString					m_sndEngineRPMLow;
	EqString					m_sndEngineRPMHigh;
	EqString					m_sndHornSignal;
	EqString					m_sndSiren;
	EqString					m_sndBrakeRelease;

	EqString					carName;
	EqString					carScript;
	EqString					m_cleanModelName;
	EqString					m_damModelName;

	float						GetMixedBrakeTorque() const;
	float						GetFullBrakeTorque() const;
	float						GetBrakeIneffectiveness() const;
	float						GetBrakeEffectPerSecond() const; // returns the vehicle speed (meters per sec) that can be reduced by one second
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

struct wheelData_t
{
	wheelData_t()
	{
		pWheelObject = NULL;
		pitch = 0.0f;
		pitchVel = 0.0f;

		flags.isBurningOut = false;
		flags.onGround = true;
		flags.lastOnGround = true;
		flags.lostHubcap = false;
		flags.doSkidmarks = false;
		flags.lastDoSkidmarks = false;

		smokeTime = 0.0f;
		surfparam = NULL;
		damage = 0.0f;
		hubcapBodygroup = -1;
		damagedWheelBodygroup = -1;
		velocityVec = vec3_zero;
	}

	DkList<PFXVertexPair_t>	skidMarks;

	CCarWheelModel*			pWheelObject;

	eqPhysSurfParam_t*		surfparam;

	CollisionData_t			collisionInfo;
	Matrix3x3				wheelOrient;

	Vector3D				velocityVec;

	int8					hubcapBodygroup;	// loose hubcaps
	int8					damagedWheelBodygroup;

	float					pitch;
	float					pitchVel;

	float					smokeTime;
	float					damage;				// this parameter affects hubcaps

	// 1 byte
	struct
	{
		bool					isBurningOut : 1;
		bool					onGround : 1;
		bool					lastOnGround : 1;
		bool					doSkidmarks : 1;
		bool					lastDoSkidmarks : 1;
		bool					lostHubcap : 1;
	} flags;
};

// bodypart
struct carBodyPart_t
{
	carBodyPart_t()
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

// handling data
struct carHandlingInput_t
{
	short	brake_accel;
	short	steering;
};

//-----------------------------------------------------------------------
// wheel model that does instancing
//-----------------------------------------------------------------------
class CCarWheelModel : public CGameObject
{
public:
	DECLARE_CLASS( CCarWheelModel, CGameObject )

	void			SetModelPtr(IEqModel* modelPtr);
	void			Draw( int nRenderFlags );
};

typedef float (*TORQUECURVEFUNC)( float rpm );

//-----------------------------------------------------------------------
// The vehicle itself
//-----------------------------------------------------------------------
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
	virtual void			PlaceOnRoadCell(CLevelRegion* reg, levroadcell_t* cell);

	void					Draw( int nRenderFlags );

	void					DrawEffects( int lod );

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

	void					ReleaseHubcap(int wheel);

	//-----------------------------------

	void					EmitCollisionParticles(const Vector3D& position, const Vector3D& velocity, const Vector3D& normal, int numSparks = 1, float fCollImpulse = 180.0f);

	void					SetControlButtons(int flags);
	int						GetControlButtons();

	void					SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering);
	void					GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering);

	void					AnalogSetControls(float accel_brake, float steering, bool extendSteer, bool handbrake, bool burnout);

	void					SetOrigin(const Vector3D& origin);
	void					SetAngles(const Vector3D& angles);
	void					SetOrientation(const Quaternion& q);

	void					SetVelocity(const Vector3D& vel);
	void					SetAngularVelocity(const Vector3D& vel);

	const Quaternion&		GetOrientation() const;

	const Vector3D			GetForwardVector() const;
	const Vector3D			GetUpVector() const;
	const Vector3D			GetRightVector() const;

	const Vector3D&			GetVelocity() const;
	const Vector3D&			GetAngularVelocity() const;

	CEqRigidBody*			GetPhysicsBody() const;

	void					L_SetContents(int contents);
	void					L_SetCollideMask(int contents);

	int						L_GetContents() const;
	int						L_GetCollideMask() const;

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
	bool					IsFlippedOver( bool checkWheels = false ) const;
	bool					IsInWater() const;

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

	void					IncrementPursue();
	void					DecrementPursue();
	int						GetPursuedCount() const;

	void					TurnOffLights();
	void					SetLight(int light, bool enabled);
	bool					IsLightEnabled(int light) const;

	virtual void			UpdateLightsState();

#ifndef NO_LUA
	virtual void			L_RegisterEventHandler(const OOLUA::Table& tableRef);
#endif // NO_LUA

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
	bool					UpdateWaterState( float fDt, bool hasCollidedWater );

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

	DkList<wheelData_t>		m_pWheels;

	carBodyPart_t			m_bodyParts[CB_PART_WINDOW_PARTS];

	float					m_fEngineRPM;
	float					m_engineIdleFactor;

	short					m_nGear;
	short					m_nPrevGear;

	FReal					m_radsPerSec;	// current rotations per sec

	FReal					m_fAcceleration;
	FReal					m_fBreakage;

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

	short					m_controlButtons;
	short					m_oldControlButtons;

	float					m_curTime;

	float					m_effectTime;
	float					m_engineSmokeTime;

	// gameplay
	CNetworkVar(float,		m_gameDamage);
	CNetworkVar(float,		m_gameMaxDamage);

	CNetworkVar(float,		m_gameFelony);	// felony percentage
	CNetworkVar(short,		m_numPursued);

	CNetworkVar(ubyte,		m_lightsEnabled);

	bool					m_visible;

	CNetworkVar(bool,		m_locked);
	CNetworkVar(bool,		m_enabled);
	CNetworkVar(bool,		m_autohandbrake);
	CNetworkVar(bool,		m_inWater);

#ifndef NO_LUA
	EqLua::LuaTableFuncRef	m_luaOnCollision;
#endif // NO_LUA

	TexAtlasEntry_t*		m_trans_grasspart;
	TexAtlasEntry_t*		m_trans_smoke2;
	TexAtlasEntry_t*		m_trans_raindrops;
	TexAtlasEntry_t*		m_trans_fleck;
	TexAtlasEntry_t*		m_veh_skidmark_asphalt;
	TexAtlasEntry_t*		m_veh_raintrail;
	TexAtlasEntry_t*		m_veh_shadow;
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CCar, CGameObject)

	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC(AlignToGround)

	OOLUA_MFUNC_CONST(IsAnyWheelOnGround)

	OOLUA_MFUNC_CONST(IsAlive)
	OOLUA_MFUNC_CONST(IsInWater)
	OOLUA_MFUNC_CONST(IsFlippedOver)

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

	OOLUA_MFUNC(SetLight)

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
	/*
	OOLUA_MEM_FUNC_RENAME(SetContents, void, L_SetContents, int)
	OOLUA_MEM_FUNC_RENAME(SetCollideMask, void, L_SetCollideMask, int)

	OOLUA_MEM_FUNC_CONST_RENAME(GetContents, int, L_GetContents)
	OOLUA_MEM_FUNC_CONST_RENAME(GetCollideMask, int, L_GetCollideMask)
	*/
OOLUA_PROXY_END
#endif // #ifndef __INTELLISENSE__
#endif // NO_LUA

#endif // CAR_H
