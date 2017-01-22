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
#include "eqPhysics/eqPhysics_HingeJoint.h"

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

	bool						isCar;			// for hinged only vehicle it must be 'true'

	struct {
		Vector3D					body_size;
		float						antiRoll;

		Vector3D					body_center;
		float						handbrakeScale;

		Vector3D					virtualMassCenter;
		float						mass;

		Vector3D					hingePoints[2];

		EEngineType					engineType;
		float						differentialRatio;
		float						torqueMult;
		float						transmissionRate;
		float						maxSpeed;

		float						burnoutMaxSpeed;

		float*						gears;
		int8						numGears;

		carWheelConfig_t*			wheels;
		int8						numWheels;

		float						steeringSpeed;
	} physics;

	struct {
		EqString					cleanModelName;
		EqString					damModelName;

		Vector4D					sirenPositionWidth;
		Vector4D					headlightPosition;
		Vector4D					backlightPosition;
		Vector4D					brakelightPosition;
		Vector4D					frontDimLights;
		Vector4D					backDimLights;
		Vector3D					enginePosition;
		Vector3D					exhaustPosition;

		int8						sirenType;
		int8						headlightType;
		int8						brakelightType;
		int8						exhaustDir;		// 0 - back, 1 - left, 2 - up
	} visual;

	struct{
		EqString					engineIdle;
		EqString					engineRPMLow;
		EqString					engineRPMHigh;
		EqString					hornSignal;
		EqString					siren;
		EqString					brakeRelease;
	} sounds;

	carColorScheme_t*			colors;
	int8						numColors;
	bool						useBodyColor;

	carCameraConfig_t			cameraConf;

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

// handling data
struct carHandlingInput_t
{
	short	brake_accel;
	short	steering;
};

//-----------------------------------------------------------------------
// wheel model that does instancing
//-----------------------------------------------------------------------
class CCarWheel : public CGameObject
{
	friend class CCar;
	friend class CAITrafficCar;
	friend class CAIPursuerCar;
public:
	DECLARE_CLASS( CCarWheel, CGameObject )

	CCarWheel();
	~CCarWheel() { OnRemove(); }

	void					SetModelPtr(IEqModel* modelPtr);
	void					Draw( int nRenderFlags );

protected:
	DkList<PFXVertexPair_t>	m_skidMarks;

	eqPhysSurfParam_t*		m_surfparam;

	CollisionData_t			m_collisionInfo;
	Matrix3x3				m_wheelOrient;

	Vector3D				m_velocityVec;

	int8					m_defaultBodyGroup;
	int8					m_hubcapBodygroup;	// loose hubcaps
	int8					m_damagedBodygroup;

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

	float					m_pitch;
	float					m_pitchVel;

	float					m_smokeTime;
	float					m_skidTime;
	float					m_damage;				// this parameter affects hubcaps
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
	CCar( vehicleConfig_t* config );
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

	float					GetRPM() const;
	int						GetGear() const;

	int						GetWheelCount() const;
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
	int						GetControlButtons();

	void					SetControlVars(float fAccelRatio, float fBrakeRatio, float fSteering);
	void					GetControlVars(float& fAccelRatio, float& fBrakeRatio, float& fSteering);

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
	vehicleConfig_t*		m_conf;

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
	void					UpdateVehiclePhysics(float fDt);

	CPhysicsHFObject*		m_pPhysicsObject;
	CEqCollisionObject*		m_lastCollidingObject;

	CEqPhysicsHingeJoint*	m_trailerHinge;

	bool					m_enablePhysics;
	
	//
	// realtime values
	//

	CCarWheel*				m_wheels;

	carBodyPart_t			m_bodyParts[CB_PART_WINDOW_PARTS];

	float					m_fEngineRPM;
	float					m_engineIdleFactor;

	short					m_nGear;
	short					m_nPrevGear;

	FReal					m_radsPerSec;	// current rotations per sec

	FReal					m_fAcceleration;
	FReal					m_fBreakage;

	float					m_fAccelEffect;
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
