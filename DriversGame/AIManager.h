//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: AI car manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#ifndef AIMANAGER_H
#define AIMANAGER_H

#include "utils/eqstring.h"

#include "AIPursuerCar.h"

#include "oolua.h"

enum ECarType
{
	CAR_TYPE_NORMAL = 0,

	CAR_TYPE_TRAFFIC_AI,
	CAR_TYPE_PURSUER_COP_AI,
	CAR_TYPE_PURSUER_GANG_AI,
	CAR_TYPE_GETAWAY_AI
};

struct	vehicleConfig_t;
struct	pedestrianConfig_t;

class	CAITrafficCar;
class	CAIPursuerCar;
class	CPedestrian;

struct spawnZoneInfo_t
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

	DkList<spawnZoneInfo_t>	zoneList;
};

struct pedestrianEntry_t
{
	pedestrianConfig_t*		config;
	int						nextSpawn;

	DkList<spawnZoneInfo_t>	zoneList;
};

//-------------------------------------------------------------------------------

class CAIManager
{
	friend class				CGameSessionBase;
	friend class				CCar;

public:
								CAIManager();
								~CAIManager();

	void						Init(const DkList<pedestrianConfig_t*>& pedConfigs);
	void						Shutdown();

	void						InitialSpawns(const Vector3D& spawnOrigin);

	void						UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity);
	void						UpdateCopStuff(float fDt);

	void						RemoveAllCars();

	civCarEntry_t*				FindCivCarEntry( const char* name );

	// ----- PEDESTRIANS -----
	void						UpdatePedestrianRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity);

	void						RemoveAllPedestrians();

	CPedestrian*				SpawnPedestrian(const IVector2D& globalCell);

	void						QueryPedestrians(DkList<CPedestrian*>& list, float radius, const Vector3D& position, const Vector3D& direction, float queryCosAngle);

	// ----- TRAFFIC ------
	void						SetMaxTrafficCars(int count);
	int							GetMaxTrafficCars() const;

	void						SetTrafficSpawnInterval(int count);
	int							GetTrafficSpawnInterval() const;

	void						SetTrafficCarsEnabled(bool enable);
	bool						IsTrafficCarsEnabled() const;
	
	CCar*						SpawnTrafficCar( const IVector2D& globalCell );

	void						QueryTrafficCars( DkList<CCar*>& list, float radius, const Vector3D& position, const Vector3D& direction, float queryCosAngle = 0.0f );
	OOLUA::Table				L_QueryTrafficCars(float radius, const Vector3D& position) const;

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

	bool						MakeCopSpeech(const char* soundScriptName, bool force, float priority);			// shedules a cop speech
	bool						IsCopsCanUseLoudhailer(CCar* copCar, CCar* target) const;

	void						CopLoudhailerTold();

	void						SetCopCar(const char* car_name, int type);	// sets cop car configuration

	bool						CreateRoadBlock( const Vector3D& position, float directionAngle, float distance = -1);
	bool						IsRoadBlockSpawn() const;

	void						UpdateRoadblocks();

	void						MakePursued( CCar* car );
	void						StopPursuit( CCar* car );

	void						UpdateNavigationVelocityMap(float fDt);

	void						TrackCar(CCar* car);
	void						UntrackCar(CCar* car);

protected:
	void						RemoveCarsInSphere(const Vector3D& pos, float radius);

	void						PaintVelocityMapFrom(CCar* car);
	void						PaintNavigationLine(const IVector2D& start, const IVector2D& end);
	
	void						RemoveTrafficCar(CCar* car);
	void						RemovePedestrian(CPedestrian* ped);

	int							CircularSpawnTrafficCars( int x0, int y0, int radius );
	int							CircularSpawnPedestrians(int x0, int y0, int radius);

	void						InitZoneEntries(const DkList<pedestrianConfig_t*>& pedConfigs);

	DkList<civCarEntry_t>		m_civCarEntries;
	DkList<pedestrianEntry_t>	m_pedEntries;

	Vector3D					m_leadPosition;
	Vector3D					m_leadRemovePosition;
	Vector3D					m_leadVelocity;

	EqString					m_patrolCarNames[PURSUER_TYPE_COUNT];
	bool						m_enableCops;

	bool						m_enableTrafficCars;
	int							m_numMaxTrafficCars;

	int							m_trafficSpawnInterval;
	int							m_copsEntryIdx;

	float						m_copMaxDamage;
	float						m_copAccelerationModifier;
	float						m_copMaxSpeed;

	int							m_numMaxCops;

	int							m_copRespawnInterval;

	float						m_copLoudhailerTime;
	float						m_copSpeechTime;

	DkList<EqString>			m_speechQueue;

	//---------------------------------------------------------

	float						m_trafficUpdateTime;
	float						m_pedsUpdateTime;

	float						m_velocityMapUpdateTime;

public:
	int							m_spawnedTrafficCars;
	DkList<CCar*>				m_trafficCars;
	DkList<CPedestrian*>		m_pedestrians;
	DkList<RoadBlockInfo_t*>	m_roadBlocks;
};

extern CAIManager*			g_pAIManager;

#ifndef NO_LUA
#ifndef __INTELLISENSE__
OOLUA_PROXY(CAIManager)
	OOLUA_TAGS(Abstract)

	OOLUA_MFUNC(RemoveAllCars)
	OOLUA_MFUNC(RemoveAllPedestrians)

	OOLUA_MFUNC(MakePursued)
	OOLUA_MFUNC(StopPursuit)

	OOLUA_MFUNC(TrackCar)
	OOLUA_MFUNC(UntrackCar)

	OOLUA_MFUNC(SetMaxTrafficCars)
	OOLUA_MFUNC_CONST(GetMaxTrafficCars)

	OOLUA_MFUNC(SetTrafficSpawnInterval)
	OOLUA_MFUNC_CONST(GetTrafficSpawnInterval)
	
	OOLUA_MFUNC(SetTrafficCarsEnabled)
	OOLUA_MFUNC_CONST(IsTrafficCarsEnabled)

	OOLUA_MFUNC(SetCopsEnabled)
	OOLUA_MFUNC_CONST(IsCopsEnabled)

	OOLUA_MFUNC(SetCopCar)

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

	OOLUA_MFUNC(CreateRoadBlock)
	OOLUA_MFUNC_CONST(IsRoadBlockSpawn)

	OOLUA_MEM_FUNC_CONST_RENAME(QueryTrafficCars, OOLUA::Table, L_QueryTrafficCars, float, const Vector3D&)
OOLUA_PROXY_END
#endif // __INTELLISENSE__
#endif // NO_LUA

#endif // AIMANAGER_H