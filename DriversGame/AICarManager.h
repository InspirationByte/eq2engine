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

struct	vehicleConfig_t;
class	CAITrafficCar;
class	CAIPursuerCar;

struct carZoneInfo_t
{
	EqString				name;
	int						spawnInterval;
};

struct civCarEntry_t
{
	civCarEntry_t()
	{
		nextSpawn = 0;
	}

	int	GetZoneSpawnInterval( const char* zone ) const
	{
		for(int i = 0; i < zoneList.numElem(); i++)
		{
			if(!zoneList[i].name.CompareCaseIns(zone))
				return zoneList[i].spawnInterval;
		}

		return -1; // don't spawn
	}

	vehicleConfig_t*		config;
	int						nextSpawn;

	DkList<carZoneInfo_t>	zoneList;
};

//-------------------------------------------------------------------------------

class CAICarManager
{
	friend class				CGameSessionBase;

public:
								CAICarManager();
								~CAICarManager();

	void						Init();
	void						Shutdown();

	void						UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity);
	void						UpdateCopStuff(float fDt);

	void						RemoveAllCars();

	civCarEntry_t*				FindCivCarEntry( const char* name );

	// ----- TRAFFIC ------
	void						SetMaxTrafficCars(int count);
	int							GetMaxTrafficCars() const;		

	void						SetTrafficCarsEnabled(bool enable);
	bool						IsTrafficCarsEnabled() const;
	
	CCar*						SpawnTrafficCar( const IVector2D& globalCell );

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

	void						SetCopMaxSpeed(float maxSpeed);							// sets the maximum hitpoints for cop cars
	float						GetCopMaxSpeed() const;

	bool						MakeCopSpeech(const char* soundScriptName, bool force);			// shedules a cop speech
	bool						IsCopsCanUseLoudhailer() const;

	void						CopLoudhailerTold();

	void						SetCopCarConfig(const char* car_name, int type = COP_LIGHT);	// sets cop car configuration

	bool						SpawnRoadBlockFor( CCar* car, float directionAngle);
	bool						IsRoadBlockSpawn() const;

	void						MakePursued( CCar* car );
	void						StopPursuit( CCar* car );

	void						UpdateNavigationVelocityMap(float fDt);

protected:

	void						PaintVelocityMapFrom(CCar* car);
	void						PaintNavigationLine(const IVector2D& start, const IVector2D& end);
	
	void						RemoveTrafficCar(CCar* car);

	void						CircularSpawnTrafficCars( int x0, int y0, int radius );

	DkList<civCarEntry_t>		m_civCarEntries;

	Vector3D					m_leadPosition;
	Vector3D					m_leadRemovePosition;
	Vector3D					m_leadVelocity;

	EqString					m_copCarName[COP_NUMTYPES];
	bool						m_enableCops;

	bool						m_enableTrafficCars;

	int							m_numMaxTrafficCars;

	float						m_copMaxDamage;
	float						m_copAccelerationModifier;
	float						m_copMaxSpeed;

	int							m_numMaxCops;

	int							m_copRespawnInterval;
	int							m_copSpawnIntervalCounter;

	float						m_copLoudhailerTime;
	float						m_copSpeechTime;

	DkList<EqString>			m_speechQueue;

	//---------------------------------------------------------

	float						m_trafficUpdateTime;
	float						m_velocityMapUpdateTime;

	DkList<CCar*>				m_trafficCars;

public:
	DkList<CAIPursuerCar*>		m_copCars;

	DkList<CCar*>				m_roadBlockCars;
	int							m_roadBlockSpawnedCount;
	IVector2D					m_roadBlockPosition;
};

extern CAICarManager*			g_pAIManager;

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAICarManager)
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC(RemoveAllCars)

	OOLUA_MFUNC(MakePursued)

	OOLUA_MFUNC(SetMaxTrafficCars)
	OOLUA_MFUNC_CONST(GetMaxTrafficCars)

	OOLUA_MFUNC(SetTrafficCarsEnabled)
	OOLUA_MFUNC_CONST(IsTrafficCarsEnabled)

	OOLUA_MFUNC(SetCopsEnabled)
	OOLUA_MFUNC_CONST(IsCopsEnabled)

	OOLUA_MFUNC(SetCopCarConfig)

	OOLUA_MFUNC(SetCopMaxDamage)
	OOLUA_MFUNC_CONST(GetCopMaxDamage)

	OOLUA_MFUNC(SetCopAccelerationModifier)
	OOLUA_MFUNC_CONST(GetCopAccelerationModifier)

	OOLUA_MFUNC(SetCopMaxSpeed)
	OOLUA_MFUNC_CONST(GetCopMaxSpeed)

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