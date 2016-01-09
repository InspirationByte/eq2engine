//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Game object
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "LuaBinding_Drivers.h"

#include "occluderSet.h"
#include "IEqModel.h"
#include "GameSoundEmitterSystem.h"
#include "physics.h"

#include "EGFInstancer.h"

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

						CBaseNetworkedObject() : m_isNetworkStateChanged(false)	{}
	virtual				~CBaseNetworkedObject() {}

	void				OnNetworkStateChanged(void* ptr);

	virtual void		OnPackMessage( CNetMessageBuffer* buffer );
	virtual void		OnUnpackMessage( CNetMessageBuffer* buffer );

	bool				m_isNetworkStateChanged;

protected:

	void				PackNetworkVariables(const netvariablemap_t* map, CNetMessageBuffer* buffer);
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

	OBJECTCONTENTS_CUSTOM_START		= (1 << 8),
};

#define COLLIDEMASK_VEHICLE (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_DEBRIS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE | OBJECTCONTENTS_WATER)
#define COLLIDEMASK_DEBRIS (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT | OBJECTCONTENTS_VEHICLE)
#define COLLIDEMASK_OBJECT (OBJECTCONTENTS_SOLID_GROUND | OBJECTCONTENTS_SOLID_OBJECTS | OBJECTCONTENTS_OBJECT)

#define NETWORK_ID_OFFLINE		-1
#define SCRIPT_ID_NOTSCRIPTED	-1

enum EGameObjectState
{
	GO_STATE_NOTSPAWN = -1,

	GO_STATE_IDLE = 0,
	GO_STATE_REMOVE,

	GO_STATE_REMOVED	// extra state for checking
};

//-------------------------------------------------------------------------------------------

enum EGameObjectType
{
	GO_DEFAULT = 0,	// no object type

	GO_CAR,

	GO_CAR_AI,	// same as GO_CAR, but simply networked, don't think that is needed

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
	true,
	false,
	false,
	false,
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

static VertexFormatDesc_t s_gameObjectInstanceFmtDesc[] = {
	// model at stream 1
	{ 0, 3, VERTEXTYPE_VERTEX, ATTRIBUTEFORMAT_FLOAT },	  // position 0
	{ 0, 2, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // texcoord 0

	{ 0, 4, VERTEXTYPE_NONE, ATTRIBUTEFORMAT_HALF }, // Tangent UNUSED
	{ 0, 4, VERTEXTYPE_NONE, ATTRIBUTEFORMAT_HALF }, // Binormal UNUSED
	{ 0, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_HALF }, // Normal (TC1)

	{ 0, 4, VERTEXTYPE_NONE, ATTRIBUTEFORMAT_HALF }, // Bone indices UNUSED
	{ 0, 4, VERTEXTYPE_NONE, ATTRIBUTEFORMAT_HALF },  // Bone weights UNUSED

	// model at stream 2
	{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT }, // Matrix4x4  (TC2-TC5)
	{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
	{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },
	{ 2, 4, VERTEXTYPE_TEXCOORD, ATTRIBUTEFORMAT_FLOAT },

};

//-------------------------------------------------------------------------------------------

class CGameObject : public CBaseNetworkedObject, public CSoundChannelObject, public RefCountedObject
{
	friend class CNetGameSession;
	friend class CGameSession;
	friend class CGameWorld;
	friend class CPhysicsHFObject;
	friend class CCar;

public:
	DECLARE_NETWORK_TABLE();
	DECLARE_CLASS( CGameObject, CBaseNetworkedObject )
	PPMEM_MANAGED_OBJECT();

	CGameObject();

	void						Ref_DeleteObject();

	virtual void				Precache();
	void						Remove();

	virtual void				Spawn();
	virtual void				OnRemove();

	void						SetName(const char* pszName);
	const char*					GetName() const;

	virtual void				SetModel(const char* pszModelName);
	virtual void				SetModelPtr(IEqModel* modelPtr);
	IEqModel*					GetModel();

	virtual void				Simulate(float fDt);

	virtual void				SetOrigin(const Vector3D& origin);
	virtual void				SetAngles(const Vector3D& angles);
	virtual void				SetVelocity(const Vector3D& vel);

	virtual const Vector3D&		GetOrigin() const;
	virtual const Vector3D&		GetAngles() const;
	virtual const Vector3D&		GetVelocity() const;

	virtual	bool				CheckVisibility( const occludingFrustum_t& frustum ) const;
	virtual void				Draw( int nRenderFlags );

	void						SetUserData(void* dataPtr);
	void*						GetUserData() const;

	virtual void				OnCarCollisionEvent(const CollisionPairData_t& pair, CGameObject* hitBy);

	virtual int					ObjType() const		{return GO_DEFAULT;}

	int							GetScriptID() const {return m_scriptID;}

	//------------------------
	// fast lua helpers

	// Adds object like g_gameWorld->AddObject, but made for script registrator. DON'T USE IN C++
	void						L_Activate();
	virtual void				L_SetContents(int contents)		{}
	virtual void				L_SetCollideMask(int contents)	{}

	virtual int					L_GetContents()	const			{return 0;}
	virtual int					L_GetCollideMask()	const		{ return 0; }

#ifndef NO_LUA
	virtual void				L_RegisterEventHandler(const OOLUA::Table& tableRef);
	OOLUA::Table&				L_GetEventHandler();
#endif // NO_LUA

	//------------------------

	virtual void				OnPackMessage( CNetMessageBuffer* buffer );
	virtual void				OnUnpackMessage( CNetMessageBuffer* buffer );

	BoundingBox					m_bbox;
	int							m_networkID;
	int							m_replayID;

	int							m_scriptID;	// object script ID for replay purposes

	int							m_state;

	Matrix4x4					m_worldMatrix;

protected:

	virtual void				OnPhysicsFrame(float fDt);
	virtual void				OnPrePhysicsFrame(float fDt) {}

	EqString					m_name;

	Vector3D					m_vecOrigin;
	Vector3D					m_vecAngles;

	IEqModel*					m_pModel;

	kvkeybase_t*				m_keyValues;	// DON'T COPY PLS

	EqString					m_defname;

	void*						m_userData;

	ubyte						m_bodyGroupFlags;

#ifndef NO_LUA
	OOLUA::Table				m_luaEvtHandler;


	EqLua::LuaTableFuncRef		m_luaOnCarCollision;
	EqLua::LuaTableFuncRef		m_luaOnPackMessage;
	EqLua::LuaTableFuncRef		m_luaOnUnpackMessage;

#endif // NO_LUA
};

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CGameObject)
	OOLUA_TAGS( Abstract )

	OOLUA_MEM_FUNC(void, SetModel, const char*)

	OOLUA_MFUNC(Remove)

	OOLUA_MFUNC(SetOrigin)
	OOLUA_MFUNC(SetAngles)
	OOLUA_MFUNC(SetVelocity)

	OOLUA_MFUNC_CONST(GetOrigin)
	OOLUA_MFUNC_CONST(GetAngles)
	OOLUA_MFUNC_CONST(GetVelocity)

	OOLUA_MFUNC(SetName)
	OOLUA_MFUNC_CONST(GetName)

	OOLUA_MFUNC_CONST(GetScriptID)

	OOLUA_MEM_FUNC_CONST_RENAME(GetType, int, ObjType)

	OOLUA_MEM_FUNC_RENAME(Spawn, void, L_Activate)

	OOLUA_MEM_FUNC_RENAME(SetContents, void, L_SetContents, int)
	OOLUA_MEM_FUNC_RENAME(SetCollideMask, void, L_SetCollideMask, int)

	OOLUA_MEM_FUNC_RENAME(SetEventHandler, void, L_RegisterEventHandler, const OOLUA::Table&)
	OOLUA_MEM_FUNC_RENAME(GetEventHandler, OOLUA::Table&, L_GetEventHandler)

	OOLUA_MEM_FUNC_CONST_RENAME(GetContents, int, L_GetContents)
	OOLUA_MEM_FUNC_CONST_RENAME(GetCollideMask, int, L_GetCollideMask)
OOLUA_PROXY_END
#endif //  __INTELLISENSE__
#endif // NO_LUA

#define PrecacheScriptSound(snd)	ses->PrecacheSound(snd)

#define PrecacheStudioModel(mod)	g_pModelCache->PrecacheModel(mod)

#define PrecacheObject( className )								\
	{	className* pCacheObj = new className;					\
			pCacheObj->Precache();								\
			delete pCacheObj;									\
	}

#endif // GAMEOBJECT_H
