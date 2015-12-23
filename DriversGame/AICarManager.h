//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI car manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef AICARMANAGER_H
#define AICARMANAGER_H

#include "utils/eqstring.h"

#include "AIPursuerCar.h"

#include "oolua.h"

enum EPursuerCarType
{
	COP_LIGHT = 0,
	COP_HEAVY,

	COP_NUMTYPES,
};

enum ECarType
{
	CAR_TYPE_NORMAL = 0,

	CAR_TYPE_TRAFFIC_AI,
	CAR_TYPE_PURSUER_COP_AI,
	CAR_TYPE_PURSUER_GANG_AI,
	CAR_TYPE_GETAWAY_AI
};

struct	carConfigEntry_t;
class	CAITrafficCar;
class	CAIPursuerCar;

//-------------------------------------------------------------------------------

class CAICarManager
{
	friend class				CGameSession;

public:
								CAICarManager();
								~CAICarManager();

	void						Init();
	void						Shutdown();

	void						UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& leadVelocity);
	void						UpdateCopStuff(float fDt);

	// ----- TRAFFIC ------
	void						SetMaxTrafficCars(int count);
	int							GetMaxTrafficCars() const;		
	
	CCar*						SpawnRandomTrafficCar(const IVector2D& globalCell, int carType = CAR_TYPE_NORMAL, bool doChecks = true);

	// ----- COPS ------
	void						SetCopsEnabled(bool enable);									// switch to spawn
	bool						IsCopsEnabled() const;

	void						SetMaxCops(int count);											// maximum cop count
	int							GetMaxCops() const;

	void						SetCopRespawnInterval(int steps);								// cop respawn interval between traffic car spawns
	int							GetCopRespawnInterval() const;

	void						SetCopAccelerationModifier(float accel);						// acceleration modifier (e.g. for survival)
	float						GetCopAccelerationModifier() const;

	void						SetCopMaxDamage(float maxHitPoints);							// sets the maximum hitpoints for cop cars
	float						GetCopMaxDamage() const;

	bool						MakeCopSpeech(const char* soundScriptName, bool force);			// shedules a cop speech
	bool						IsCopCanSayTaunt() const;

	void						GotCopTaunt();

	void						SetCopCarConfig(const char* car_name, int type = COP_LIGHT);	// sets cop car configuration

	bool						SpawnRoadBlockFor( CCar* car, float directionAngle);
	bool						IsRoadBlockSpawn() const;

protected:

	void						RemoveTrafficCar(CAITrafficCar* car);

	void						CircularSpawnTrafficCars( int x0, int y0, int radius );

	DkList<carConfigEntry_t*>	m_civCarEntries;

	Vector3D					m_leadPosition;
	Vector3D					m_leadVelocity;

	EqString					m_copCarName[COP_NUMTYPES];
	bool						m_enableCops;

	int							m_numMaxTrafficCars;

	float						m_copMaxDamage;
	float						m_copAccelerationModifier;

	int							m_numMaxCops;

	int							m_copRespawnInterval;
	int							m_copSpawnIntervalCounter;

	float						m_copTauntTime;
	float						m_copSpeechTime;

	DkList<EqString>			m_speechQueue;

	//---------------------------------------------------------

	float						m_trafficUpdateTime;

	DkList<CAITrafficCar*>		m_trafficCars;

public:
	DkList<CAIPursuerCar*>		m_copCars;

	DkList<CCar*>				m_roadBlockCars;
};

extern CAICarManager*			g_pAIManager;

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAICarManager)
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC(SetMaxTrafficCars)
	OOLUA_MFUNC_CONST(GetMaxTrafficCars)

	OOLUA_MFUNC(SetCopsEnabled)
	OOLUA_MFUNC_CONST(IsCopsEnabled)

	OOLUA_MFUNC(SetCopCarConfig)

	OOLUA_MFUNC(SetCopMaxDamage)
	OOLUA_MFUNC_CONST(GetCopMaxDamage)

	OOLUA_MFUNC(SetCopAccelerationModifier)
	OOLUA_MFUNC_CONST(GetCopAccelerationModifier)

	OOLUA_MFUNC(SetMaxCops)
	OOLUA_MFUNC_CONST(GetMaxCops)

	OOLUA_MFUNC(SetCopRespawnInterval)
	OOLUA_MFUNC_CONST(GetCopRespawnInterval)

	OOLUA_MFUNC(MakeCopSpeech)

	OOLUA_MFUNC(SpawnRoadBlockFor)
	OOLUA_MFUNC_CONST(IsRoadBlockSpawn)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // AICARMANAGER_H