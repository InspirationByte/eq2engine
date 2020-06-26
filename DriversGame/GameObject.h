//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Game object
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#define EQ_DRVSYN_DEFAULT_SOUND_DISTANCE (100.0f)

#include "LuaBinding_Drivers.h"

#include "occluderSet.h"
#include "IEqModel.h"
#include "GameSoundEmitterSystem.h"
#include "physics.h"

#include "EGFInstancer.h"

#include "DrvSynDecals.h"

#include "network/netpropvar.h"

using namespace Networking;

#ifdef DECLARE_CLASS
#undef DECLARE_CLASS
#endif

#define DECLARE_CLASS_NOBASE( className )			\
	typedef className ThisClass;

#define DECLARE_CLASS( className, baseClassName )	\
	typedef baseClassName BaseClass; \
	typedef className ThisClass;

class CBaseNetworkedObject
{
public:
	//PPMEM_MANAGED_OBJECT()

	DECLARE_NETWORK_TABLE_PUREVIRTUAL()
	DECLARE_CLASS_NOBASE( CBaseNetworkedObject )

							CBaseNetworkedObject()	{}
	virtual					~CBaseNetworkedObject() {}

	void					OnNetworkStateChanged();
	void					OnNetworkStateChanged(void* ptr);

	virtual void			OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList );
	virtual void			OnUnpackMessage( CNetMessageBuffer* buffer );

	DECLARE_NETWORK_CHANGELIST(NetGame)
	DECLARE_NETWORK_CHANGELIST(Replay)
protected:

	void				PackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer, DkList<int>& changeList);
	void				UnpackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer);
};

// COLLISION_MASK_ALL defined in eqCollision_Object.h

// here we assembly the custom collision mask to use
enum EPhysCollisionContents
{
	OBJECTCONTENTS_SOLID_GROUND		= (1 << 0),	// ground objects
	OBJECTCONTENTS_SOLID_OBJECTS	= (1 << 1),	// other world objects

	OBJECTCONTENTS_WATER			= (1 << 2), // water surface

	OBJECTCONTENTS_DEBRIS			= (1 << 3),
	OBJECTCONTENTS_OBJECT			= (1 << 4),
	OBJECTCONTENTS_VEHICLE			= (1 << 5),
	OBJECTCONTENTS_PEDESTRIAN		= (1 << 6),

	OBJECTCONTENTS_CUSTOM_START		= (1 << 16),
};

#define COLLIDEMASK_VEHICLE (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_DEBRIS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE | OBJECTCONTENTS_WATER)
#define COLLIDEMASK_PEDESTRIAN (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_PEDESTRIAN)
#define COLLIDEMASK_DEBRIS (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE)
#define COLLIDEMASK_OBJECT (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_DEBRIS | OBJECTCONTENTS_PEDESTRIAN)

#define NETWORK_ID_OFFLINE		-1
#define SCRIPT_ID_NOTSCRIPTED	-1

enum EGameObjectState
{
	GO_STATE_NOTSPAWN = -1,

	GO_STATE_IDLE = 0,

	GO_STATE_REMOVE,
	GO_STATE_REMOVE_BY_SCRIPT,

	GO_STATE_REMOVED	// extra state for checking
};

static inline bool IsRemoveState(EGameObjectState state)
{
	return state == GO_STATE_REMOVE || state == GO_STATE_REMOVE_BY_SCRIPT;
}

//-------------------------------------------------------------------------------------------

enum EGameObjectType
{
	GO_DEFAULT = 0,	// no object type

	GO_CAR,
	GO_CAR_AI,			// FIXME: needs to be removed

	GO_PEDESTRIAN,

	GO_MISC,
	GO_DEBRIS,
	GO_PHYSICS,
	GO_STATIC,
	GO_LIGHT_TRAFFIC,

	GO_SCRIPTED,

	GO_TYPES,
};

static const char* s_objectTypeNames[GO_TYPES] =
{
	"GO_DEFAULT",
	"GO_CAR",
	"GO_CAR_AI",
	"GO_MISC",
	"GO_DEBRIS",
	"GO_PHYSICS",
	"GO_STATIC",
	"GO_LIGHT_TRAFFIC",
	"GO_SCRIPTED"
};

static bool s_networkedObjectTypes[GO_TYPES] =
{
	false,

	true,
	true,

	false,

	false,
	false,
	true,
	false,
	false,
	false,
};

enum EGameObjectDrawFlags {
	GO_DRAW_FLAG_SHADOW = (1 << 0),
	GO_DRAW_FLAG_NODRAW = (1 << 1),
	GO_DRAW_FLAG_ILLUMINATED = (1 << 2),
};

//-------------------------------------------------------------------------------------------

// Default instancer for game objects

struct gameObjectInstance_t
{
	Matrix4x4	world;

	// make this true:
	// Vector4D quaternion;				= rotation
	// Vector4D position				= position + light index
	// Vector4D scale					= scale + light index
	// Vector4D lightidx				= 4 light indexes
	//
	// In total: 4 regs of rotation, position, scale, 6 light indexes
};

typedef CEGFInstancer<gameObjectInstance_t> CGameObjectInstancer;

static VertexFormatDesc_t s_gameObjectInstanceFormat[] = {
	// model at stream 2
	{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Matrix4x4  (TC2-TC5)
	{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
	{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
	{ 2, 4, VERTEXATTRIB_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
};

//-------------------------------------------------------------------------------------------

class CGameObject : public CBaseNetworkedObject, public CSoundChannelObject, public RefCountedObject
{
	friend class CNetGameSession;
	friend class CGameSessionBase;
	friend class CGameWorld;
	friend class CPhysicsHFObject;
	friend class CShadowRenderer;
	friend class CCar;

public:
	DECLARE_NETWORK_TABLE();
	DECLARE_CLASS( CGameObject, CBaseNetworkedObject )
	PPMEM_MANAGED_OBJECT();

	CGameObject();

	void						Ref_DeleteObject();

	virtual void				Precache();

	virtual void				Spawn();
	virtual void				OnRemove();

	void						SetName(const char* pszName);
	const char*					GetName() const;

	const char*					GetDefName() const;

	virtual void				SetModel(const char* pszModelName);
	virtual void				SetModelPtr(IEqModel* modelPtr);
	IEqModel*					GetModel() const {return m_pModel;}

	virtual void				Simulate(float fDt);

	virtual void				SetOrigin(const Vector3D& origin);
	virtual void				SetAngles(const Vector3D& angles);
	virtual void				SetVelocity(const Vector3D& vel);

	virtual const Vector3D&		GetOrigin() const;
	virtual const Vector3D&		GetAngles() const;
	virtual const Vector3D&		GetVelocity() const;

	virtual	bool				CheckVisibility( const occludingFrustum_t& frustum ) const;

	virtual void				PreDraw() {}
	virtual void				PostDraw() {}
	virtual void				Draw( int nRenderFlags );

	virtual CGameObject*		GetChildShadowCaster(int idx) const;
	virtual int					GetChildCasterCount() const;

	virtual void				ConfigureCamera(struct cameraConfig_t& conf, struct eqPhysCollisionFilter& filter) const;

	void						SetDrawFlags(ubyte newFlags);
	ubyte						GetDrawFlags() const;

	void						SetBodyGroups(ubyte newBodyGroup);
	ubyte						GetBodyGroups() const;

	void						SetUserData(void* dataPtr);
	void*						GetUserData() const;

	virtual void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	virtual int					ObjType() const		{return GO_DEFAULT;}

	int							GetScriptID() const {return m_scriptID;}

	//------------------------
	// fast lua helpers

	// Adds object like g_gameWorld->AddObject, but made for script registrator.
	void						L_Remove();
	void						L_Activate();
	virtual void				L_SetContents(int contents)		{}
	virtual void				L_SetCollideMask(int contents)	{}

	virtual int					L_GetContents()	const			{return 0;}
	virtual int					L_GetCollideMask()	const		{ return 0; }

#ifndef NO_LUA
	virtual void				L_RegisterEventHandler(const OOLUA::Table& tableRef);
	OOLUA::Table&				L_GetEventHandler() const;
#endif // NO_LUA

	//------------------------

	virtual void				OnPackMessage( CNetMessageBuffer* buffer, DkList<int>& changeList );
	virtual void				OnUnpackMessage( CNetMessageBuffer* buffer );

	Matrix4x4					m_worldMatrix;
	BoundingBox					m_bbox;

	EGameObjectState			m_state;

	int							m_objId;
	int							m_networkID;
	int							m_replayID;
	int							m_scriptID;			// object script ID for replay purposes	

protected:

	virtual void				DrawEGF(int nRenderFlags, Matrix4x4* boneTransforms, int materialGroup = 0);

	virtual void				OnPhysicsFrame(float fDt);
	virtual void				OnPrePhysicsFrame(float fDt) {}

	virtual void				OnPhysicsPreCollide(ContactPair_t& pair) {}
	virtual void				OnPhysicsCollide(const CollisionPairData_t& pair) {}

	virtual void				UpdateTransform();

	decalPrimitives_t			m_shadowDecal;

	EqString					m_name;

	Vector3D					m_vecOrigin;
	Vector3D					m_vecAngles;

	IEqModel*					m_pModel;

	kvkeybase_t*				m_keyValues;	// DON'T COPY PLS

	EqString					m_defname;

	void*						m_userData;
	int							m_drawFlags;		// EGameObjectDrawFlags

	uint16						m_bodyGroupFlags;


#ifndef NO_LUA
	OOLUA::Table				m_luaEvtHandler;

	EqLua::LuaTableFuncRef		m_luaOnCarCollision;
	EqLua::LuaTableFuncRef		m_luaOnRemove;
	EqLua::LuaTableFuncRef		m_luaOnSimulate;

	EqLua::LuaTableFuncRef		m_luaOnPackMessage;
	EqLua::LuaTableFuncRef		m_luaOnUnpackMessage;

#endif // NO_LUA
};


#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CGameObject)
	OOLUA_TAGS( Abstract )

	OOLUA_MEM_FUNC(void, SetModel, const char*)

	OOLUA_MEM_FUNC_RENAME(Remove, void, L_Remove)

	OOLUA_MFUNC(SetOrigin)
	OOLUA_MFUNC(SetAngles)
	OOLUA_MFUNC(SetVelocity)

	OOLUA_MFUNC_CONST(GetOrigin)
	OOLUA_MFUNC_CONST(GetAngles)
	OOLUA_MFUNC_CONST(GetVelocity)

	OOLUA_MFUNC(SetName)
	OOLUA_MFUNC_CONST(GetName)

	OOLUA_MFUNC(SetDrawFlags)
	OOLUA_MFUNC_CONST(GetDrawFlags)

	OOLUA_MFUNC(SetBodyGroups)
	OOLUA_MFUNC_CONST(GetBodyGroups)

	OOLUA_MFUNC_CONST(GetScriptID)

	OOLUA_MEM_FUNC_CONST_RENAME(GetType, int, ObjType)

	OOLUA_MEM_FUNC_RENAME(Spawn, void, L_Activate)

	OOLUA_MEM_FUNC_RENAME(SetEventHandler, void, L_RegisterEventHandler, const OOLUA::Table&)
	OOLUA_MEM_FUNC_CONST_RENAME(GetEventHandler, OOLUA::Table&, L_GetEventHandler)
	
	OOLUA_MEM_FUNC_RENAME(SetContents, void, L_SetContents, int)
	OOLUA_MEM_FUNC_RENAME(SetCollideMask, void, L_SetCollideMask, int)

	OOLUA_MEM_FUNC_CONST_RENAME(GetContents, int, L_GetContents)
	OOLUA_MEM_FUNC_CONST_RENAME(GetCollideMask, int, L_GetCollideMask)
OOLUA_PROXY_END
#endif //  __INTELLISENSE__
#endif // NO_LUA

#define PrecacheScriptSound(snd)	g_sounds->PrecacheSound(snd)

#define PrecacheStudioModel(mod)	g_studioModelCache->PrecacheModel(mod)

#define PrecacheObject( className )								\
	{	className* pCacheObj = new className;					\
			pCacheObj->Precache();								\
			delete pCacheObj;									\
	}

#endif // GAMEOBJECT_H
