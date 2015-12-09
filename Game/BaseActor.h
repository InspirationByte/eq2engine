//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Base actor class
//////////////////////////////////////////////////////////////////////////////////

#ifndef BASEACTOR_H
#define BASEACTOR_H

#include "BaseEngineHeader.h"
#include "BaseAnimating.h"
#include "Ladder.h"
#include "ragdoll.h"
#include "IGameRules.h"

#define DEFAULT_ACTOR_EYE_HEIGHT	82

#define DEFAULTACTOR_HEIGHT		110

#define PART_CAPSULE_SIZE(x)	(x/4)
#define PART_HEIGHT_OFFSET(x)	(PART_CAPSULE_SIZE(x)/2)

#define MIN_SPEED				80
#define MAX_SPEED				300

#define WALK_SPEED				210
#define CROUCH_SPEED			80

#define CROUCH_STAND_SPEED		210
#define CROUCH_STAND_ACCEL		25

#define MIN_FALLDMG_SPEED		650

enum MoveType_e
{
	MOVE_TYPE_WALK = 0,
	MOVE_TYPE_LADDER,
	MOVE_TYPE_NOCLIP,
	MOVE_TYPE_FLY,		// unsupported by BaseActor, but can be extended in your classes
};

enum ActorPose_e
{
	POSE_STAND = 0,
	POSE_CROUCH,
	POSE_CINEMATIC,			// scenes, etc
	POSE_CINEMATIC_LADDER,	// ladder cinematics
};

enum WeaponSlot_e
{
	WEAPONSLOT_NONE	= -1,
	WEAPONSLOT_PRIMARY = 0,
	WEAPONSLOT_SECONDARY,
	WEAPONSLOT_GRENADE,

	WEAPONSLOT_COUNT
};

struct movementdata_t
{
	float forward;
	float right;
};

class CBaseWeapon;

extern int nGPFlags;

class CBaseActor : public BaseAnimating
{
	DEFINE_CLASS_BASE(BaseAnimating);
public:
							CBaseActor();
							~CBaseActor();
	
	// Spawns the entity
	virtual void			Spawn();
	virtual void			Precache();

	void					FirstUpdate();

	virtual void			OnRemove();

	void					Update(float decaytime);

	virtual void			OnPreRender();
	virtual void			OnPostRender();

	virtual void			UpdateView();

	void					SnapEyeAngles(const Vector3D &angles);

	virtual Vector3D		GetEyeOrigin();
	virtual	Vector3D		GetEyeAngles();

	virtual void			EyeVectors(Vector3D* forward, Vector3D* right = NULL, Vector3D* up = NULL );

	// entity view - computation for rendering
	// use SetActiveViewEntity to set view from this entity
	virtual void			CalcView(Vector3D &origin, Vector3D &angles, float &fov);

	// shakes view of actor
	virtual void			ViewShake(float fMagnutude, float fTime) {}

	// punches actor view
	virtual void			ViewPunch(Vector3D& angleOfs, bool bWeapon = false) {}

	//------------------------------------------------------------------------

	virtual MoveType_e		GetMoveType();

	virtual Vector3D		GetFootOrigin();

	void					SetPose(int nPose, float fNextTimeToChange = 0);
	int						GetPose()	{return m_nCurPose;}

	void					SetLadderEntity(CInfoLadderPoint* pLadder);

	//------------------------------------------------------------------------

	virtual void			MakeHit( const damageInfo_t& info, bool sound = true );

	virtual void			ApplyDamage( const damageInfo_t& info );
	
	// thinking functions
	virtual void			AliveThink();
	virtual void			DeathThink();
	virtual void			OnDeath( const damageInfo_t& info );

	//------------------------------------------------------------------------

	// attack buttons checking
	virtual void			CheckAttacking();

	// pickups weapon
	virtual bool			PickupWeapon(CBaseWeapon *pWeapon);

	// drops weapon
	virtual void			DropWeapon(CBaseWeapon *pWeapon);

	// returns equipped weapons list
	DkList<CBaseWeapon*>*	GetEquippedWeapons();

	// returns current active weapon
	CBaseWeapon*			GetActiveWeapon();

	// sets active weapon (if weapon is not in list, it will be ignored.)
	virtual void			SetActiveWeapon(CBaseWeapon* pWeapon);

	// equipped by this weapon?
	bool					IsEquippedWithWeapon(CBaseWeapon* pWeapon);

	// Is empty slot?
	bool					IsWeaponSlotEmpty(int slot);

	// actor has this weapon class?
	bool					HasWeaponClass(const char* classname);

	virtual void			GiveWeapon(const char *classname) {}

	int						GetClipCount(int nAmmoType);
	void					SetClipCount(int nAmmoType, int nClipCount);

	//------------------------------------------------------------------------

	EntType_e				EntType()			{return ENTTYPE_ACTOR;}

	virtual int				GetButtons()		{return m_nActorButtons;}
	virtual int				GetOldButtons()		{return m_nOldActorButtons;}

	virtual void			PhysicsCreateObjects();

	// updates parent transformation
	virtual void			UpdateTransform();

	virtual void			SetHealth(int nHealth);

protected:

	virtual void			ProcessMovement( movementdata_t& data );

	virtual void			PainSound();
	virtual void			DeathSound();

	void					PlayStepSound(const char* soundname, float fVol);
	virtual void			CheckLanding();

	int						m_nPose; // standing/crouching, cinematic mode for scenes, etc
	int						m_nCurPose; // standing/crouching, cinematic mode for scenes, etc
	float					m_fNextPoseTime;
	float					m_fNextPoseChangeTime;

	float					m_fMaxSpeed;
	float					m_fEyeHeight;
	float					m_fMass;
	float					m_fActorHeight;

	float					m_fPainPercent;

	BaseEntity*				m_pLastDamager;

	Vector3D				m_vecEyeAngles;
	Vector3D				m_vecEyeOrigin;

	bool					m_bOnGround;
	bool					m_bLastOnGround;
	float					m_fJumpTime;

	float					m_fJumpPower;
	float					m_fJumpMoveDirPower;

	float					m_fNextStepTime;
	float					m_fMaxFallingVelocity;

	Vector3D				m_vPrevPhysicsVelocity;
	Vector3D				m_vMoveDir;

	// equipment of actor
	DkList<CBaseWeapon*>	m_pEquipment;
	CBaseWeapon*			m_pEquipmentSlots[WEAPONSLOT_COUNT];

	int						m_nCurrentSlot;

	// current weapon
	CBaseWeapon*			m_pActiveWeapon;

	// material
	phySurfaceMaterial_t*	m_pCurrentMaterial;

	// actor buttons pressed and old pressed
	int						m_nActorButtons;
	int						m_nOldActorButtons;

	CInfoLadderPoint*		m_pLadder;

	ragdoll_t*				m_pRagdoll;

	// ammo carrying
	int						m_nClipCount[AMMOTYPE_MAX];

protected:
	DECLARE_DATAMAP();
};

#endif // BASEACTOR_H