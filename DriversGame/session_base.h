//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2018
//////////////////////////////////////////////////////////////////////////////////
// Description: Game session base implementations
//////////////////////////////////////////////////////////////////////////////////

#ifndef SESSION_BASE_H
#define SESSION_BASE_H

#include "physics.h"
#include "AIManager.h"
#include "LuaBinding_Drivers.h"

class CCar;

enum ESessionType
{
	SESSION_SINGLE = 0,
	SESSION_SPLITSCREEN,
	SESSION_NETWORK,
};

enum EMissionStatus
{
	MIS_STATUS_INGAME = 0,
	MIS_STATUS_SUCCESS,
	MIS_STATUS_FAILED,

	MIS_STATUS_REPLAY_END
};

struct playerControl_t
{
	int		buttons;
	float	steeringValue;
	float	accelBrakeValue;
};

//--------------------------------------------------------------------------------------

class CGameSessionBase
{
public:
	CGameSessionBase();
	virtual						~CGameSessionBase();

	virtual int					GetSessionType() const = 0;

	virtual bool				IsClient() const = 0;
	virtual bool				IsServer() const = 0;

	virtual void				Init();
	virtual void				Shutdown();

	virtual void				OnLoadingDone();

	int							GetPhysicsIterations() const;

	vehicleConfig_t*			FindCarEntryByName(const char* name) const;

	void						GetCarNames(DkList<EqString>& list) const;

	void						FinalizeMissionManager();

	//---------------------------------------------------

	virtual void				Update(float fDt);
	virtual void				UpdateLocalControls(int nControls, float steering, float accel_brake) = 0;



	CCar*						CreateCar(const char* name, int type = CAR_TYPE_NORMAL);
	CAIPursuerCar*				CreatePursuerCar(const char* name, int type = PURSUER_TYPE_COP);

	virtual CCar*				GetPlayerCar() const = 0;
	virtual	void				SetPlayerCar(CCar* pCar) = 0;

	virtual CGameObject*		GetViewObject() const;
	void						SetViewObject(CGameObject* viewObj);
	void						SetViewObjectToNone();

	CCar*						GetLeadCar() const;
	void						SetLeadCar(CCar* pCar);
	void						SetLeadCarNone();

	float						LoadCarReplay(CCar* pCar, const char* filename);
	void						StopCarReplay(CCar* pCar);

	virtual bool				IsGameDone(bool checkTime = true) const;
	void						SignalMissionStatus(int missionStatus, float waitTime);
	int							GetMissionStatus() const;

	bool						IsReplay() const;
	void						ResetReplay();

	int							GenScriptID();
	CGameObject*				FindScriptObjectById(int scriptID);

	double						GetGameTime() const { return m_gameTime; }

	//-------------------------------------------------------------------------
	// functions only used by Lua

	CCar*						Lua_CreateCar(const char* name, int type = CAR_TYPE_NORMAL);
	CAIPursuerCar*				Lua_CreatePursuerCar(const char* name, int type = PURSUER_TYPE_COP);

protected:

	void						LoadCarsPedsRegistry();
	void						ClearCarsPedsRegistry();

	virtual void				UpdatePlayerControls() = 0;
	void						UpdateAsPlayerCar(const playerControl_t& control, CCar* car);

	float						m_waitForExitTime;
	int							m_missionStatus;

	double						m_gameTime;

	int							m_scriptIDCounter;

	DkList<vehicleConfig_t*>	m_carEntries;
	DkList<pedestrianConfig_t*>	m_pedEntries;

	CGameObject*				m_viewObject;				// object from which is camera calculated
	CCar*						m_leadCar;					// lead car is 

	OOLUA::Table				m_missionManagerTable;		// missionmanager
	EqLua::LuaTableFuncRef		m_lua_misman_InitMission;	// lua CMissionManager:InitMission
	EqLua::LuaTableFuncRef		m_lua_misman_Update;		// lua CMissionManager:Update
	EqLua::LuaTableFuncRef		m_lua_misman_Finalize;		// lua CMissionManager:Finalize
};

extern CGameSessionBase* g_pGameSession;

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CGameSessionBase)
OOLUA_TAGS(Abstract)

OOLUA_MEM_FUNC_RENAME(CreateCar, maybe_null<CCar*>, Lua_CreateCar, const char*, int)
OOLUA_MEM_FUNC_RENAME(CreatePursuerCar, maybe_null<CAIPursuerCar*>, Lua_CreatePursuerCar, const char*, int)

OOLUA_MFUNC(LoadCarReplay)
OOLUA_MFUNC(StopCarReplay)

OOLUA_MFUNC_CONST(IsClient)
OOLUA_MFUNC_CONST(IsServer)

OOLUA_MFUNC_CONST(GetSessionType)

OOLUA_MFUNC(SetPlayerCar)
OOLUA_MFUNC_CONST(GetPlayerCar)

OOLUA_MFUNC(SetLeadCar)
OOLUA_MFUNC(SetLeadCarNone)
OOLUA_MFUNC_CONST(GetLeadCar)

OOLUA_MFUNC(SetViewObject)
OOLUA_MFUNC(SetViewObjectToNone)
OOLUA_MFUNC_CONST(GetViewObject)

OOLUA_MFUNC_CONST(IsGameDone)
OOLUA_MFUNC(SignalMissionStatus)
OOLUA_MFUNC_CONST(GetMissionStatus)

OOLUA_MFUNC_CONST(IsReplay)
OOLUA_MFUNC(ResetReplay)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // SESSION_BASE_H