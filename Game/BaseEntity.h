//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base entity class
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEENTITY_H
#define BASEENTITY_H

#include "math\DkMath.h"
#include <stdlib.h>
#include <stdio.h>
#include "utils\strtools.h"
#include "IEntityFactory.h"
#include "DamageInfo.h"
#include "Decals.h"

//#include "EqGMS.h"
#include "BaseEngineHeader.h"

#include "GameSoundEmitterSystem.h"

struct variable_t;

#include "DataMap.h"

#define TICK_RATE				(.015)  // 15 msec ticks
#define TIME_TO_TICKS( dt )		( (int)( 0.5f + (float)dt / TICK_RATE ) )
#define ROUND_TO_TICKS( t )		( TICK_RATE * TIME_TO_TICKS( t ) )
#define TICK_NEVER_THINK		(-1)

class BaseEntity;

class gmTableObject;
class gmUserObject;

// entity think function
typedef void (BaseEntity::*BASE_THINK_PTR)( void );

// Physics type
enum EntPhysicsType_e
{
	PHYSTYPE_STATIC = 0, // Static physics
	PHYSTYPE_NORMAL = 1, // Dynamic physics
};

// Physics type
enum EntRelClass_e
{
	ENTCLASS_NONE = 0,
	ENTCLASS_PLAYER,	// player
	
#ifdef PIGEONGAME
	ENTCLASS_PIGEON,
	ENTCLASS_KID,
	ENTCLASS_GRANDMA,
#endif

#ifdef STDGAME
	ENTCLASS_PLAYER_ALLY,
	ENTCLASS_CYBORG,
	ENTCLASS_CYBORG_FRIENDLY,
	ENTCLASS_CLEANINGBOT,
	ENTCLASS_REBEL,
#endif // STDGAME

	ENTCLASS_CITIZEN,

	ENTCLASS_COUNT,
};

enum EntType_e
{
	// generic types
	ENTTYPE_GENERIC = 0,
	ENTTYPE_ACTOR,
	ENTTYPE_ITEM,

	// AI types
	ENTTYPE_AI_HINT,
	ENTTYPE_AI_NODEGROUND,
	ENTTYPE_AI_NODEAIR,
};

// Entity processing
enum EntProcType_e
{
	ENTPROC_GLOBAL	 = 0,		// global entity processing (for simple ents)
	ENTPROC_SECTORAL,			// full sectoral entity processing
	ENTPROC_DISTANT,			// update rate is depending on view distance
};

#pragma warning(disable:4121)

class BaseEntity : public CBaseRenderableObject, public CSoundChannelObject
{
public:
	PPMEM_MANAGED_OBJECT();

	enum
	{
		ENTITY_IDLE = 0,
		ENTITY_NO_UPDATE,
		ENTITY_REMOVE
	};

								BaseEntity();
	virtual						~BaseEntity() {}

	//---------------------------------------------------------------------------------------

	// sets new entity index
	void						SetIndex(int idx);

	// returns entity index
	int							EntIndex();

	//---------------------------------------------------------------------------------------

	// precaches entity
	virtual void 				Precache();

	// applies parameters and creating required data when spawning
	virtual void 				Spawn();

	// Adds object to world, and changes activation state
	virtual void 				Activate();

	// simulates entity, call update
	virtual void 				Simulate(float decaytime);

	// update function
	virtual void 				Update(float decaytime);

	//Calls if m_flNextThink > gpGlobals->curtime;
	virtual void 				ThinkFrame();

	//---------------------------------------------------------------------------------------

	// first update handling
	virtual void				FirstUpdate() {}

	// called before rendering scene
	virtual void 				OnPreRender() {}

	// called after rendering scene
	virtual void 				OnPostRender();

	// called when game starts
	virtual void 				OnGameStart();

	// called when game ends, then level unloads
	virtual void 				OnGameEnd() {}

	// called when this entity being removed
	virtual void 				OnRemove();

	// called when all objects are ready and spawns
	virtual void				OnMapSpawn();

	//---------------------------------------------------------------------------------------

	// state update
	bool						StateUpdate();

	// entity update before removal
	void						UpdateOnRemove();

	// sets entity life state
	void						SetState(int state) {m_nEntityState = state;}

	// returns entity life state
	int							GetState() {return m_nEntityState;}

	//---------------------------------------------------------------------------------------

	// sets absolute position of this entity
	void 						SetAbsOrigin(const Vector3D &pos);

	// sets absolute angles of this entity
	void 						SetAbsAngles(const Vector3D &ang);

	// sets absolute velocity of this entity
	void 						SetAbsVelocity(const Vector3D &vel);

	// returns absolute position
	Vector3D					GetAbsOrigin();

	// returns absolute angles
	Vector3D					GetAbsAngles();

	// returns absolute velocity
	Vector3D					GetAbsVelocity();

	// sets absolute position of this entity
	void						SetLocalOrigin(const Vector3D &pos);

	// sets absolute angles of this entity
	void						SetLocalAngles(const Vector3D &ang);

	// returns absolute position
	Vector3D					GetLocalOrigin();

	// returns absolute angles
	Vector3D					GetLocalAngles();

	//---------------------------------------------------------------------------------------

	// sets new classname of this entity
	void						SetClassName(const char* newclassname);

	// sets target name of this entity
	void						SetName(const char* newname);

	// returns current class name of this entity
	const char*					GetClassname();

	// returns target name
	const char*					GetName();

	//---------------------------------------------------------------------------------------

	// applies damage to entity (use this)
	virtual void				ApplyDamage( const damageInfo_t& info );

	// sets entity health
	virtual void				SetHealth(int nHealth);

	// returns current health of entity
	int							GetHealth();

	// is dead?
	bool						IsDead();

	// is alive?
	bool						IsAlive();

	//---------------------------------------------------------------------------------------

	// entity view - computation for rendering
	// use SetActiveViewEntity to set view from this entity
	virtual void				CalcView(Vector3D &origin, Vector3D &angles, float &fov);

	virtual Vector3D			GetEyeOrigin();
	virtual	Vector3D			GetEyeAngles();

	virtual void				MouseMoveInput(float x, float y) {}

	//---------------------------------------------------------------------------------------

	// processing of coming input
	void						AcceptInput(char* pszInputName, variable_t &Value, BaseEntity *pActivator, BaseEntity *pCaller);

	//
	// basic inputs
	//

	void						InputAttach(inputdata_t& data);
	void						InputKill(inputdata_t& data);
	void						InputScriptFunc(inputdata_t& data);

public:

	// sets think function ptr
	void						ThinkSet(BASE_THINK_PTR fnThink);

	// sets next think time, called in the function
	void						SetNextThink(float flTime);

	// basic thinking function
	virtual void				Think();

	//---------------------------------------------------------------------------------------

	// sets model for this entity
	virtual void				SetModel(const char* pszModelName);

	// sets model for this entity
	virtual void				SetModel(IEqModel* pModel);

	// returns model of this entity
	IEqModel*					GetModel() {return m_pModel;}

	// returns model name
	const char*					GetModelName();

	//---------------------------------------------------------------------------------------
	//					Physics things
	//---------------------------------------------------------------------------------------

	// creates static object from model that you've set.
	IPhysicsObject*				PhysicsInitStatic();

	// creates dynamic object from model that you've set. SetAbsOrigin(), etc do not affect the object!
	IPhysicsObject*				PhysicsInitNormal(int nSolidFlags, bool makeAsleep, int objectIndex = 0);

	IPhysicsObject*				PhysicsInitCustom(int nSolidFlags, bool makeAsleep, int numShapes, int* shapeIdxs, const char* surfProps, float mass);

	// destroys physics object
	void						PhysicsDestroyObject();

	// sets physics object
	void						PhysicsSetObject(IPhysicsObject* pObject)		{m_pPhysicsObject = pObject;}

	// returns current physics object
	IPhysicsObject*				PhysicsGetObject()								{return m_pPhysicsObject;}

	// a procedure that creates physics objects. You must use it.
	virtual void				PhysicsCreateObjects() {}

	//---------------------------------------------------------------------------------------
public:
	// entity class for filtering
	virtual EntRelClass_e		Classify();

	// entity type for filtering
	virtual EntType_e			EntType();

	// entity processing type for engine perfomance
	virtual EntProcType_e		GetProcessingType();

	//---------------------------------------------------------------------------------------

	// applies damage to entity (use this)
	virtual void				MakeDecal( const decalmakeinfo_t& info );

	// removes decals
	void						ClearDecals();

	//---------------------------------------------------------------------------------------

	// checks object for usage ray/box
	virtual bool				CheckUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir);

	// onUse handler
	virtual bool				OnUse(BaseEntity* pActivator, const Vector3D &origin, const Vector3D &dir);

	//---------------------------------------------------------------------------------------

	// returns owner (parent entity)
	BaseEntity*					GetOwner();

	// sets parent entity
	void						SetOwner(BaseEntity *pOwner);

	// fills matrix with parent transformation
	void						GetParentToWorldTransform( Matrix4x4& matrix );

	// updates parent transformation
	virtual void				UpdateTransform();

	// fills matrix with this entity transformation
	void						GetWorldTransform(Matrix4x4& matrix);

	//---------------------------------------------------------------------------------------
	//				Rendering stuff
	//---------------------------------------------------------------------------------------

	// sets new target entity
	void						SetTargetEntity(BaseEntity* pEnt);

	// returns target entity
	BaseEntity*					GetTargetEntity() {return m_pTargetEntityPointer;}

	// check thee target entity for existence
	void						CheckTargetEntity();

	// min bbox dimensions
	Vector3D					GetBBoxMins();

	// max bbox dimensions
	Vector3D					GetBBoxMaxs();

	// sets bounding box
	void						SetBBox(const Vector3D &mins, const Vector3D &maxs);

	// renders model
	virtual void				Render(int nViewRenderFlags);

	// renders EGF model
	void						RenderEGFModel( int nViewRenderFlags, Matrix4x4* bones );

	// updates rendering stuff
	void						UpdateRenderStuff();

	// compute matrix transformation for this object
	virtual void				ComputeTransformMatrix();

	//---------------------------------------------------------------------------------------

	// sets key
	void						SetKey(const char* pszName, const char* pszValue);
	void						SetKey(const char* pszName, const variable_t& value);

	// returns key value
	variable_t					GetKey(const char* pszName);

	// additional script hooks
	// this is a kind of using non-oop languages in oop's way
	virtual void				InitScriptHooks();				// TODO: remove, replace by OOLUA

	DECLARE_DATAMAP();

	//EQGMS_DEFINE_SCRIPTOBJECT( GMTYPE_EQENTITY );

protected:

	// sets key
	void						SetKeyDatamap(const char* pszName, const char* pszValue, datamap_t* pDataMap);
	void						SetKeyDatamap(const char* pszName, const variable_t& var, datamap_t* pDataMap);

	variable_t					GetKeyDatamap(const char* pszName, datamap_t* pDataMap);

	variable_t					GetDataVariableOf(const datavariant_t& pVar);

protected:

	// current main thinking function
	BASE_THINK_PTR				m_fnThink;

	int							m_nHealth;
	int							m_nMaxHealth;

	EqString					m_szClassName; //Entity class name
	EqString					m_pszTargetName; // map name

	EqString					m_pszTargetEntity; // target entity

	BaseEntity*					m_pTargetEntityPointer; // target entity pointer TODO: handle

	// Next think time
	float						m_fNextThink;

	bool						m_bThinked;

	// entity life state
	int							m_nEntityState;

	// global vectors
	Vector3D					m_vecAbsOrigin;
	Vector3D					m_vecAbsAngles;
	Vector3D					m_vecAbsVelocity;

	Matrix4x4					m_matWorldTransform;

	// local position if it's a child of some object
	Vector3D					m_vecOrigin;
	Vector3D					m_vecAngles;
	// TODO: local velocity!
	//Vector3D					m_vecVelocity;

	bool						m_bRegistered;

	EqString					m_pszScriptName;

	EqString					m_pszModelName;
	IEqModel*					m_pModel;

	DkList<tempdecal_t*>		m_pDecals;

	EntPhysicsType_e			m_pPhysicsType;
	IPhysicsObject*				m_pPhysicsObject;

	// bounding box
	BoundingBox					m_vBBox;

	// parenting
	BaseEntity*					m_pOwner;

	// entity index in proc list
	int							m_nEntIndex;

	// unique entity ID. ENTHANDLE
	int							m_nUniqueID;

	// basic OnGameStart output
	CBaseEntityOutput			m_OnGameStart;

	// basic OnMapSpawn output
	CBaseEntityOutput			m_OnMapSpawn;

	// basic OnRemove output
	CBaseEntityOutput			m_OnRemove;

	// basic OnUse output
	CBaseEntityOutput			m_OnUse;

	// render stuff
	slight_t					m_static_lights[8];
	Vector3D					m_vLastPos;

	Vector3D					m_vecLightingOrigin;

public:
	int							m_pRooms[32];
	int							m_nRooms;
};

#pragma warning(default:4121)

#define SetThink( a ) ThinkSet(static_cast <BASE_THINK_PTR>(a));

#endif //BASEENTITY_H