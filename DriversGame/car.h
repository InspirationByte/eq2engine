//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Drivers vehicle
//////////////////////////////////////////////////////////////////////////////////

#ifndef CAR_H
#define CAR_H

#include "GameObject.h"
#include "ControllableObject.h"

#include "CameraAnimator.h"
#include "EqParticles.h"

#include "eqPhysics/eqPhysics_HingeJoint.h"

#include "GameDefs.h"

#define MPS_TO_KPH		(3.6f)
#define KPH_TO_MPS		(0.27778f)

inline bool IsCar(CGameObject* obj) { return obj ? obj->ObjType() >= GO_CAR && obj->ObjType() <= GO_CAR_AI : false;}

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

	SERVICE_LIGHTS_BLUE,
	SERVICE_LIGHTS_RED,

	SERVICE_LIGHTS_DOUBLE_BLUE,
	SERVICE_LIGHTS_DOUBLE_RED,

	SERVICE_LIGHTS_DOUBLE_BLUERED,
};

enum ECarLightTypeFlags
{
	CAR_LIGHT_LOWBEAMS			= (1 << 0),		// front headlights (low beam)
	CAR_LIGHT_HIGHBEAMS			= (1 << 1),		// front headlights (high beam)
	CAR_LIGHT_BRAKE				= (1 << 2),		// brake lights
	CAR_LIGHT_REVERSELIGHT		= (1 << 3),		// white back lights
	CAR_LIGHT_DIM_LEFT			= (1 << 4),		// dimensional left lights
	CAR_LIGHT_DIM_RIGHT			= (1 << 5),		// dimensional right lights
	CAR_LIGHT_SERVICELIGHTS		= (1 << 6),		// flashing lights of service cars

	CAR_LIGHT_EMERGENCY			= (CAR_LIGHT_DIM_LEFT | CAR_LIGHT_DIM_RIGHT),	// emergency light flags (dim. left and right)
};

enum ECarSound
{
	CAR_SOUND_ENGINE = 0,
	CAR_SOUND_ENGINE_LOW,
	CAR_SOUND_ENGINE_IDLE,

	CAR_SOUND_HORN,
	CAR_SOUND_SIREN,

	CAR_SOUND_BRAKERELEASE,

	CAR_SOUND_WHINE,
	CAR_SOUND_SKID,
	CAR_SOUND_SURFACE,

	CAR_SOUND_COUNT,
	CAR_SOUND_CONFIGURABLE = CAR_SOUND_BRAKERELEASE+1,
};

enum ECarLightFlags
{
	CAR_LIGHT_FLAG_DOUBLE = (1 << 0),
	CAR_LIGHT_FLAG_VERTICAL = (1 << 1),
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
	float				camber;

	float				woffset;
	float				visualTop;

	int					wheelBodyPart;	// damage effets on wheels like bad breakage, suspension effects
	int					flags;			// EWheelFlags
};

// color scheme
struct carColorScheme_t
{
public:
	DECLARE_CLASS_NOBASE(carColorScheme_t);
	DECLARE_NETWORK_TABLE();
	DECLARE_EMBEDDED_NETWORKVAR();

	char name[16];

	carColorScheme_t() {}
	carColorScheme_t(const ColorRGBA& col)
	{
		col1 = col;
		col2 = col;
	}

	CNetworkVar(Vector4D, col1);
	CNetworkVar(Vector4D, col2);
};

struct lightConfig_t
{
	Vector4D value;		// x y z double_width
	int8 type;			// ECarLightTypeFlags
	int8 flags;			// ECarLightFlags
};

struct vehicleConfig_t
{
	vehicleConfig_t()
	{
		colors = nullptr;
		physics.wheels = nullptr;
		physics.gears = nullptr;

		numColors = 0;
		physics.numWheels = 0;
		physics.numGears = 0;
	}

	~vehicleConfig_t()
	{
		DestroyCleanup();
	}

	void DestroyCleanup()
	{
		delete colors;
		delete physics.wheels;
		delete physics.gears;
	}

	uint						scriptCRC;	// for network and replays

	EqString					carName;
	EqString					carScript;

	struct{
		bool						isCar : 1;			// for hinged only vehicle it must be 'true'
		bool						isCop : 1;
		bool						allowParked : 1;
		bool						reverseWhine: 1;
	} flags;

	struct {
		Vector3D					body_size;
		float						antiRoll;

		Vector3D					body_center;
		float						handbrakeScale;

		Vector3D					virtualMassCenter;
		float						mass;
		float						inertiaScale;

		Vector3D					hingePoints[2];

		EEngineType					engineType;
		float						differentialRatio;
		float						torqueMult;
		float						transmissionRate;
		float						maxSpeed;

		float						burnoutMaxSpeed;

		float*						gears;
		int8						numGears;

		float						shiftAccelFactor;

		float						steeringSpeed;
		float						noseDownScale;
		float						downForce;

		carWheelConfig_t*			wheels;
		int8						numWheels;
	} physics;

	struct {
		EqString					cleanModelName;
		EqString					damModelName;
		EqString					wheelModelName;

		Vector3D					enginePosition;
		Vector3D					exhaustPosition;

		Vector3D					driverPosition;

		lightConfig_t				lights[16];
		int8						numLights;

		Vector4D					sirenPositionWidth;
		Vector4D					headlightPosition;
		Vector4D					backlightPosition;
		Vector4D					brakelightPosition;
		Vector4D					frontDimLights;
		Vector4D					backDimLights;
		int8						headlightType;
		int8						brakelightType;
		int8						sirenType;

		bool						hasServiceLights;
		int8						exhaustType;		// 0 - single, 1 - double
		int8						exhaustDir;			// 0 - back, 1 - left, 2 - up
	} visual;

	/*
	struct{
		EqString					engineIdle;
		EqString					engineRPMLow;
		EqString					engineRPMHigh;
		EqString					hornSignal;
		EqString					siren;
		EqString					brakeRelease;
	} sounds;*/

	EqString					sounds[CAR_SOUND_CONFIGURABLE];

	carColorScheme_t*			colors;
	int8						numColors;
	bool						useBodyColor;

	cameraConfig_t				cameraConf;

	// calculations

	float						GetMixedBrakeTorque() const;
	float						GetFullBrakeTorque() const;
	float						GetBrakeIneffectiveness() const;
	float						GetBrakeEffectPerSecond() const; // returns the vehicle speed (meters per sec) that can be reduced by one second
};

bool ParseVehicleConfig( vehicleConfig_t* conf, const kvkeybase_t* kvs );

//------------------------------------

static carWheelConfig_t _wheelNull;

struct PFXVertexPair_t
{
	PFXVertex_t v0;
	PFXVertex_t v1;
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

//-----------------------------------------------------------------------
// wheel model that does instancing
//-----------------------------------------------------------------------
class CCarWheel : public CGameObject
{
	friend class CCar;
public:
	DECLARE_CLASS( CCarWheel, CGameObject )

	CCarWheel();
	~CCarWheel() { OnRemove(); }

	void						SetModelPtr(IEqModel* modelPtr);
	void						Draw( int nRenderFlags );

	void						CalculateTransform(Matrix4x4& out, const carWheelConfig_t& conf, bool applyScale = true);

	void						CalcWheelSkidPair(PFXVertexPair_t& pair, const carWheelConfig_t& conf, const Vector3D& wheelRightVec);

	const eqPhysSurfParam_t*	GetSurfaceParams() const;

protected:
	CollisionData_t				m_collisionInfo;
	Matrix3x3					m_wheelOrient;

	DkList<PFXVertexPair_t>		m_skidMarks;
	eqPhysSurfParam_t*			m_surfparam;

	Vector3D					m_velocityVec;

	float						m_pitch;
	float						m_pitchVel;

	float						m_smokeTime;
	float						m_skidTime;
	float						m_damage;				// this parameter affects hubcaps
	float						m_hubcapLoose;
	float						m_hubcapRandomFactor;

	int8						m_defaultBodyGroup;
	int8						m_hubcapBodygroup;	// loose hubcaps
	int8						m_hubcapPhysmodel;
	int8						m_damagedBodygroup;

	// 1 byte
	struct
	{
		bool					isBurningOut : 1;
		bool					onGround : 1;
		bool					lastOnGround : 1;
		bool					doSkidmarks : 1;
		bool					lastDoSkidmarks : 1;
		bool					lostHubcap : 1;
	} m_flags;
};

typedef float (*TORQUECURVEFUNC)( float rpm );

class CLevelRegion;

//-----------------------------------------------------------------------
// The vehicle itself
//-----------------------------------------------------------------------
class CCar : public CControllableGameObject
{
	friend class CAITrafficCar;
	friend class CAIPursuerCar;
	friend class CReplayTracker;
	friend class CDrvSynHUDManager;
public:
	DECLARE_NETWORK_TABLE();
	DECLARE_CLASS( CCar, CControllableGameObject)

	CCar();
	CCar( vehicleConfig_t* config );
	~CCar(){}

	virtual void			Precache();
	virtual void			OnRemove();

	void					DebugReloadCar();

	virtual void			Spawn();
	virtual void			PlaceOnRoadCell(CLevelRegion* reg, levroadcell_t* cell);

	void					Draw( int nRenderFlags );
	void					PreDraw();

	IEqModel*				GetDamagedModel() const { return m_pDamagedModel; }

	bool					GetBodyDamageValuesMappedToBones(float damageVals[16]) const;

	CGameObject*			GetChildShadowCaster(int idx) const;
	int						GetChildCasterCount() const;

	void					AlignToGround();	// align car to ground

	//-----------------------------------

	float					GetRPM() const;
	int						GetGear() const;

	int						GetWheelsOnGround(int wheelFlags = 0xFF) const;
	int						GetWheelCount() const;
	CCarWheel&				GetWheel(int indxe) const;
	float					GetWheelSpeed(int index) const;

	float					GetLateralSlidingAtBody() const;					// returns non-absolute lateral (side) sliding from body
	float					GetLateralSlidingAtWheels(bool surfCheck) const;	// returns non-absolute lateral (side) sliding from all wheels
	float					GetLateralSlidingAtWheel(int wheel) const;			// returns non-absolute lateral (side) sliding

	float					GetTractionSliding(bool surfCheck) const;			// returns absolute traction sliding
	float					GetTractionSlidingAtWheel(int wheel) const;			// returns absolute traction sliding from all wheels

	void					ReleaseHubcap(int wheel);

	//-----------------------------------

	void					EmitCollisionParticles(const Vector3D& position, const Vector3D& velocity, const Vector3D& normal, int numSparks = 1, float fCollImpulse = 180.0f);

	void					SetControlButtons(int flags);
	void					AnalogSetControls(float accel_brake, float steering, bool extendSteer, bool handbrake, bool burnout);

	void					SetOrigin(const Vector3D& origin);
	void					SetAngles(const Vector3D& angles);
	void					SetOrientation(const Quaternion& q);

	void					SetVelocity(const Vector3D& vel);
	void					SetAngularVelocity(const Vector3D& vel);

	void					HingeVehicle(int thisHingePoint, CCar* otherVehicle, int otherHingePoint);
	void					ReleaseHingedVehicle();

	CCar*					GetHingedVehicle() const;
	CEqRigidBody*			GetHingedBody() const;

	int						GetControlButtons() const;

	void					ConfigureCamera(cameraConfig_t& conf, eqPhysCollisionFilter& filter) const;

	const Quaternion&		GetOrientation() const;

	const Vector3D			GetForwardVector() const;
	const Vector3D			GetUpVector() const;
	const Vector3D			GetRightVector() const;

	const Vector3D&			GetVelocity() const;
	const Vector3D&			GetAngularVelocity() const;

	CEqRigidBody*			GetPhysicsBody() const;

#ifndef EDITOR
	void					L_SetBodyDamage(OOLUA::Table& table);
	OOLUA::Table			L_GetBodyDamage() const;

	void					L_SetContents(int contents);
	void					L_SetCollideMask(int contents);

	int						L_GetContents() const;
	int						L_GetCollideMask() const;
#endif // EDITOR

	void					RefreshWindowDamageEffects();

	virtual int				ObjType() const		{ return GO_CAR; }

	virtual void			OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList);
	virtual void			OnUnpackMessage( CNetMessageBuffer* buffer );

	//---------------------------------------------------------------------------
	// Gameplay-related
	//---------------------------------------------------------------------------

	void					SetCarColour(const carColorScheme_t& col);
	carColorScheme_t		GetCarColour() const;

	void					SetColorScheme( int colorIdx );

	bool					IsAnyWheelOnGround() const;
	bool					IsDriveWheelsOnGround() const;

	float					GetSpeedWheels() const;
	float					GetSpeed() const;

	bool					IsAccelerating() const;
	bool					IsBraking() const;
	bool					IsHandbraking() const;
	bool					IsBurningOut() const;

	float					GetAccelBrake() const;
	float					GetSteering() const;

	bool					IsAlive() const;
	bool					IsFlippedOver( bool checkWheels = false ) const;
	bool					IsInWater() const;

	void					SetMaxDamage( float fDamage );
	float					GetMaxDamage() const;

	virtual void			SetMaxSpeed( float fSpeed );
	float					GetMaxSpeed() const;

	virtual void			SetTorqueScale( float fScale );
	float					GetTorqueScale() const;

	void					SetDamage( float damage );
	float					GetDamage() const;

	void					SetBodyDamage(float damage[CB_PART_COUNT]);
	void					GetBodyDamage(float damage[CB_PART_COUNT]) const;

	void					SetFelony(float percentage);
	float					GetFelony() const;

	void					Repair(bool unlock = false);

	void					SetAutoHandbrake( bool enable);
	bool					HasAutoHandbrake() const;

	void					SetAutoGearSwitch(bool enable);
	bool					HasAutoGearSwitch() const;

	void					SetInfiniteMass( bool infMass );
	bool					HasInfiniteMass() const;

	void					Lock(bool lock = true);	// locks car, false to unlock
	bool					IsLocked() const;

	void					Enable(bool enable = true);	// disables or enables car
	bool					IsEnabled() const;

	void					IncrementPursue();
	void					DecrementPursue();
	int						GetPursuedCount() const;

	PursuerData_t&			GetPursuerData();

	void					TurnOffLights();
	void					SetLight(int light, bool enabled);
	bool					IsLightEnabled(int light) const;

	virtual void			UpdateLightsState();

	virtual void			Simulate( float fDt );

#ifndef NO_LUA
	virtual void			L_RegisterEventHandler(const OOLUA::Table& tableRef);
#endif // NO_LUA

public:
	vehicleConfig_t*		m_conf;

	bool					m_isLocalCar;

	CNetworkVar(bool,		m_sirenEnabled);
	bool					m_oldSirenState;

	RoadBlockInfo_t*		m_assignedRoadblock;

protected:
	void					ApplyDamage(Vector3D& position, float impulse, CGameObject* hitBy);

	void					CreateCarPhysics();
	void					InitCarSound();

	ISoundController*		CreateCarSound(const char* name, float radiusMult);

	virtual void			OnPrePhysicsFrame( float fDt );
	virtual void			OnPhysicsFrame( float fDt );
	
	virtual void			OnPhysicsPreCollide(ContactPair_t& pair);
	virtual void			OnPhysicsCollide(const CollisionPairData_t& pair);

	bool					UpdateWaterState( float fDt, bool hasCollidedWater );

	void					DrawBody( int nRenderFlags, int nLOD );
	void					DrawShadow( float distance );

	void					DrawWheelEffects(int wheelIdx);
	void					ProcessWheelSkidmarkTrails(int wheelIdx);

	void					DrawSkidmarkTrails(int wheelIdx);

	void					AddWheelWaterTrail(const CCarWheel& wheel, const carWheelConfig_t& wheelConf,
												const Vector3D& skidmarkPos, 
												const Rectangle_t& trailCoords, 
												const ColorRGB& ambientAndSun, 
												float skidPitchVel,
												const Vector3D& wheelDir,
												const Vector3D& wheelRightDir);

	void					UpdateSounds( float fDt );
	void					UpdateWheelEffect(int nWheel, float fDt);
	void					UpdateVehiclePhysics(float fDt);

	static void				UpdateVehiclePhysicsJob(void *data, int i);

	void					UpdateTransform();

	virtual void			OnDeath( CGameObject* deathBy );

	CPhysicsHFObject*		m_physObj;
	DkList<CollisionPairData_t>	m_collisionList;

	CEqPhysicsHingeJoint*	m_trailerHinge;

	bool					m_enablePhysics;
	
	//
	// realtime values
	//

	CGameObject				m_driverModel;

#ifndef NO_LUA
	EqLua::LuaTableFuncRef	m_luaOnCollision;
	EqLua::LuaTableFuncRef	m_luaOnDeath;
#endif // NO_LUA

	CCarWheel*				m_wheels;

	carBodyPart_t			m_bodyParts[CB_PART_WINDOW_PARTS];

	float					m_fEngineRPM;
	float					m_engineIdleFactor;

	short					m_nGear;
	short					m_nPrevGear;

	CNetworkVar(float,		m_gearboxShiftThreshold);

	float					m_radsPerSec;	// current rotations per sec

	float					m_shiftDelay;

	FReal					m_fAcceleration;
	FReal					m_fBreakage;

	float					m_fAccelEffect;
	FReal					m_autobrake;
	FReal					m_steering;

	//
	// dynamic config parts
	//

	CNetworkVar(float,		m_maxSpeed);
	CNetworkVar(float,		m_torqueScale);

	slipAngleCurveParams_t*	m_slipParams;

	//
	// visual properties
	//

	CNetworkVarEmbedded(PursuerData_t, m_pursuerData);
	CNetworkVarEmbedded(carColorScheme_t,	m_carColor);

	ISoundController*		m_sounds[CAR_SOUND_COUNT];

	IEqModel*				m_pDamagedModel;

	//---------------------------------------------

	float					m_lightsTime;

	float					m_effectTime;
	float					m_engineSmokeTime;

	float					m_sirenDeathTime;

	// gameplay
	CNetworkVar(int,		m_licPlateId);

	CNetworkVar(float,		m_gameDamage);
	CNetworkVar(float,		m_gameMaxDamage);

	CNetworkVar(ubyte, m_lightsEnabled);

	bool					m_visible;

	CNetworkVar(bool,		m_locked);
	CNetworkVar(bool,		m_enabled);
	CNetworkVar(bool,		m_autohandbrake);
	CNetworkVar(bool,		m_autogearswitch);
	CNetworkVar(bool,		m_inWater);

	CNetworkVar(bool,		m_hasDriver);
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

	OOLUA_MEM_FUNC_RENAME(SetBodyDamage, void, L_SetBodyDamage, OOLUA::Table&)
	OOLUA_MEM_FUNC_CONST_RENAME(GetBodyDamage, OOLUA::Table, L_GetBodyDamage)

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

	OOLUA_MFUNC_CONST(IsAccelerating)
	OOLUA_MFUNC_CONST(IsBraking)
	OOLUA_MFUNC_CONST(IsHandbraking)
	OOLUA_MFUNC_CONST(IsBurningOut)

	OOLUA_MFUNC(Enable)
	OOLUA_MFUNC_CONST(IsEnabled)

	OOLUA_MFUNC_CONST(GetPursuedCount)
	
	OOLUA_MFUNC(Lock)
	OOLUA_MFUNC_CONST(IsLocked)

	OOLUA_MFUNC(SetInfiniteMass)

	OOLUA_MEM_FUNC_RENAME(SetControls, void, AnalogSetControls, float, float, bool, bool, bool);
	OOLUA_MFUNC_CONST(GetAccelBrake)
	OOLUA_MFUNC_CONST(GetSteering)

	OOLUA_MFUNC(HingeVehicle);
	OOLUA_MFUNC(ReleaseHingedVehicle)
	OOLUA_MEM_FUNC_CONST(OOLUA::maybe_null<CCar*>, GetHingedVehicle)

	OOLUA_MEM_FUNC_CONST_RENAME(GetLateralSliding, float, GetLateralSlidingAtBody)
	OOLUA_MFUNC_CONST(GetTractionSliding)
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
