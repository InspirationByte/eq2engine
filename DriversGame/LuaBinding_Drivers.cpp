//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Driver Syndicate OOLua binding
//////////////////////////////////////////////////////////////////////////////////

#include "LuaBinding_Drivers.h"

#include "world.h"
#include "car.h"
#include "game_multiplayer.h"
#include "StateManager.h"
#include "DrvSynHUD.h"
#include "system.h"

static OOLUA::Script g_luastate;
OOLUA::Script& GetLuaState()
{
	return g_luastate;
}

class CLCollisionData
{
public:
	CollisionData_t data;

	Vector3D		GetPosition() const	{ return data.position; }
	Vector3D		GetNormal() const		{ return data.normal; }
	float			GetLineFract() const	{ return data.fract; }
	float			GetFract() const		{ return data.fract; }

	CGameObject*	GetHitObject() const
	{
		if( data.hitobject )
		{
			if(	(data.hitobject->GetContents() & OBJECTCONTENTS_SOLID_GROUND) ||
				(data.hitobject->GetContents() & OBJECTCONTENTS_SOLID_OBJECTS))
				return NULL;

			CGameObject* obj = (CGameObject*)data.hitobject->GetUserData();

			return obj;
		}

		return NULL;
	}
};

OOLUA_PROXY(CLCollisionData)
	OOLUA_TAGS(No_public_constructors)
	OOLUA_MFUNC_CONST(GetPosition)
	OOLUA_MFUNC_CONST(GetNormal)
	OOLUA_MFUNC_CONST(GetLineFract)
	OOLUA_MFUNC_CONST(GetFract)
	OOLUA_MFUNC_CONST(GetHitObject)
OOLUA_PROXY_END

OOLUA_EXPORT_FUNCTIONS(CLCollisionData)
OOLUA_EXPORT_FUNCTIONS_CONST(CLCollisionData, GetPosition, GetNormal, GetLineFract, GetFract, GetHitObject)

OOLUA_PROXY(CCameraAnimator)
	OOLUA_TAGS( Abstract )

	OOLUA_MFUNC( SetScripted )
	OOLUA_MFUNC_CONST( IsScripted )

	OOLUA_MEM_FUNC_RENAME( Update, void, L_Update, float, CCar* )

	OOLUA_MFUNC_CONST(GetRealMode)
	OOLUA_MFUNC_CONST(GetMode)
	OOLUA_MFUNC(SetMode)

	OOLUA_MFUNC(SetOrigin)
	OOLUA_MFUNC(SetAngles)
	OOLUA_MFUNC(SetFOV)

	OOLUA_MFUNC_CONST(GetOrigin)
	OOLUA_MFUNC_CONST(GetAngles)
	OOLUA_MFUNC_CONST(GetFOV)

OOLUA_PROXY_END

OOLUA_EXPORT_FUNCTIONS(
	CCameraAnimator, 
	SetScripted, 
	Update,
	SetMode,
	SetOrigin,
	SetAngles,
	SetFOV
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CCameraAnimator,
	IsScripted,
	GetRealMode,
	GetMode,
	GetOrigin,
	GetAngles,
	GetFOV
)

CLCollisionData Lua_Util_TestLine(const Vector3D& start, const Vector3D& end, int contents)
{
	CLCollisionData coll;

	g_pPhysics->TestLine(start,end, coll.data, contents, NULL);

	return coll;
}

OOLUA_CFUNC(SetCurrentStateType, L_SetCurrentStateType)
OOLUA_CFUNC(GetCurrentStateType, L_GetCurrentStateType)
OOLUA_CFUNC(SheduleNextStateType, L_SheduleNextStateType)

//OOLUA_CFUNC(SetCurrentState, L_SetCurrentState)
//OOLUA_CFUNC(GetCurrentState, L_GetCurrentState)

OOLUA_CFUNC(Lua_Util_TestLine, L_Lua_Util_TestLine)

bool LuaBinding_InitDriverSyndicateBindings(lua_State* state)
{
	MsgInfo("Initializing script system...\n");

	EqLua::LuaBinding_Initialize(state);

	LuaBinding_InitEngineBindings(state);

	//-------------------
	// states

	LUA_SET_GLOBAL_ENUMCONST(state, GAME_STATE_NONE);
	LUA_SET_GLOBAL_ENUMCONST(state, GAME_STATE_TITLESCREEN);
	LUA_SET_GLOBAL_ENUMCONST(state, GAME_STATE_MAINMENU);
	LUA_SET_GLOBAL_ENUMCONST(state, GAME_STATE_GAME);
	LUA_SET_GLOBAL_ENUMCONST(state, GAME_STATE_MPLOBBY);

	LUA_SET_GLOBAL_ENUMCONST(state, MIS_STATUS_INGAME);
	LUA_SET_GLOBAL_ENUMCONST(state, MIS_STATUS_SUCCESS);
	LUA_SET_GLOBAL_ENUMCONST(state, MIS_STATUS_FAILED);

	//--------------------
	// MP stuff
	LUA_SET_GLOBAL_ENUMCONST(state, SESSION_SINGLE);
	LUA_SET_GLOBAL_ENUMCONST(state, SESSION_NETWORK);

	//-------------------
	// object stuff

	LUA_SET_GLOBAL_ENUMCONST(state, GO_STATE_IDLE);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_STATE_REMOVE);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_STATE_REMOVE_BY_SCRIPT);

	LUA_SET_GLOBAL_ENUMCONST(state, GO_DEFAULT);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_CAR);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_CAR_AI);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_MISC);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_DEBRIS);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_PHYSICS);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_STATIC);
	LUA_SET_GLOBAL_ENUMCONST(state, GO_LIGHT_TRAFFIC);



	//-------------------
	// vehicle stuff

	LUA_SET_GLOBAL_ENUMCONST(state, CAR_TYPE_NORMAL);
	LUA_SET_GLOBAL_ENUMCONST(state, CAR_TYPE_TRAFFIC_AI);
	LUA_SET_GLOBAL_ENUMCONST(state, CAR_TYPE_PURSUER_COP_AI);
	LUA_SET_GLOBAL_ENUMCONST(state, CAR_TYPE_PURSUER_GANG_AI);
	LUA_SET_GLOBAL_ENUMCONST(state, CAR_TYPE_GETAWAY_AI);

	LUA_SET_GLOBAL_ENUMCONST(state, PURSUER_TYPE_COP);
	LUA_SET_GLOBAL_ENUMCONST(state, PURSUER_TYPE_GANG);


	//-------------------
	// physics stuff
	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_SOLID_GROUND);
	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_SOLID_OBJECTS);
	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_DEBRIS);
	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_OBJECT);
	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_VEHICLE);

	LUA_SET_GLOBAL_ENUMCONST(state, OBJECTCONTENTS_CUSTOM_START);

	// HUD
	LUA_SET_GLOBAL_ENUMCONST(state, HUD_DOBJ_IS_TARGET);
	LUA_SET_GLOBAL_ENUMCONST(state, HUD_DOBJ_CAR_DAMAGE);
	//LUA_SET_GLOBAL_ENUMCONST(state, HUDTARGET_FLAG_PLAYERINFO);

	// CAMERA
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_OUTCAR);
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_INCAR);
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_TRIPOD_ZOOM);
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_TRIPOD_FIXEDZOOM);
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_TRIPOD_STATIC);
	LUA_SET_GLOBAL_ENUMCONST(state, CAM_MODE_OUTCAR_FIXED);

	OOLUA::register_class<CLCollisionData>(state);

	OOLUA::register_class<CGameWorld>(state);
	OOLUA::register_class<CGameSession>(state);
	OOLUA::register_class<CAICarManager>(state);
	OOLUA::register_class<CNetGameSession>(state);
	OOLUA::register_class<CViewParams>(state);
	OOLUA::register_class<CDrvSynHUDManager>(state);
	OOLUA::register_class<CCameraAnimator>(state);

	OOLUA::set_global(state, "SetCurrentStateType", L_SetCurrentStateType);
	OOLUA::set_global(state, "GetCurrentStateType", L_GetCurrentStateType);
	OOLUA::set_global(state, "SheduleNextStateType", L_SheduleNextStateType);


	//OOLUA::set_global(state, "SetCurrentState", L_SetCurrentState);
	//OOLUA::set_global(state, "GetCurrentState", L_GetCurrentState);

	CCar* CCarNULLPointer = NULL;

	// init object classes
	OOLUA::register_class<CCar>(state);
	//OOLUA::register_class_static<CCar>(state, "nullptr", CCarNULLPointer );

	OOLUA::register_class<CAIPursuerCar>(state);

	OOLUA::set_global(state, "world", g_pGameWorld);
	OOLUA::set_global_to_nil(state, "game");
	OOLUA::set_global(state, "ai", g_pAIManager);

	OOLUA::set_global(state, "cameraAnimator", g_pCameraAnimator);

	OOLUA::Table utilTable = OOLUA::new_table(state);
	utilTable.set("TestLine", L_Lua_Util_TestLine);
	//utilTable.set("TestModel", LLua_Console_ExecuteString);

	OOLUA::set_global(state, "gameutil", utilTable);

	// initialize library
	return EqLua::LuaBinding_LoadAndDoFile(state, "scripts/lua/_init.lua", "__init");
}

DECLARE_CMD(lexec, "Executes Lua string", 0)
{
	if(CMD_ARGC == 0)
		return;

	if(!EqLua::LuaBinding_DoBuffer(GetLuaState(), CMD_ARGV(0).c_str(), CMD_ARGV(0).Length(), "lexec"))
	{
		MsgError("lua_exec error:\n\n%s\n", OOLUA::get_last_error(GetLuaState()).c_str());
	}
}
