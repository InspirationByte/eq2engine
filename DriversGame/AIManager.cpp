//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI car manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "AIManager.h"

#include "heightfield.h"
#include "session_stuff.h"

#include "world.h"

#include "pedestrian.h"

//------------------------------------------------------------------------------------------

ConVar g_trafficMaxCars("g_trafficMaxCars", "48", nullptr, CV_CHEAT);
ConVar g_pedestriansMax("g_pedestriansMax", "40", nullptr, CV_CHEAT);

ConVar g_traffic_initial_mindist("g_traffic_initial_mindist", "15", nullptr, CV_CHEAT);
ConVar g_traffic_mindist("g_traffic_mindist", "50", nullptr, CV_CHEAT);
ConVar g_traffic_maxdist("g_traffic_maxdist", "51", nullptr, CV_CHEAT);

ConVar g_pedestrians_initial_mindist("g_pedestrians_initial_mindist", "10", nullptr, CV_CHEAT);
ConVar g_pedestrians_mindist("g_pedestrians_mindist", "30", nullptr, CV_CHEAT);
ConVar g_pedestrians_maxdist("g_pedestrians_maxdist", "32", nullptr, CV_CHEAT);

const int	INITIAL_SPAWN_INTERVAL = 10;
const int	PEDESTRIAN_SPAWN_INTERVAL = 0;

const int	AI_COP_SPEECH_QUEUE = 6;

const float AI_COP_SPEECH_DELAY = 4.0f;		// delay before next speech
const float AI_COP_TAUNT_DELAY = 5.0f;		// delay before next taunt

const float AI_COP_DEFAULT_DAMAGE = 4.0f;
const float AI_COP_DEFAULT_MAXSPEED = 160.0f;

const int MIN_ROADBLOCK_CARS = 2;

const int MIN_COPS_TO_INIT_SURVIVAL = 4;

const float AI_TRAFFIC_RESPAWN_TIME	= 0.1f;
const float AI_TRAFFIC_SPAWN_DISTANCE_THRESH = 5.0f;

const float AI_PEDESTRIAN_RESPAWN_TIME = 0.5f;

//------------------------------------------------------------------------------------------

static CAIManager s_AIManager;
CAIManager* g_pAIManager = &s_AIManager;

CAIManager::CAIManager()
{
	m_trafficUpdateTime = 0.0f;
	m_pedsUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;
	m_trafficSpawnInterval = 0;

	m_enableCops = true;
	m_enableTrafficCars = true;

	m_copAccelerationModifier = 1.0f;
	m_copMaxDamage = AI_COP_DEFAULT_DAMAGE;
	m_copMaxSpeed = AI_COP_DEFAULT_MAXSPEED;

	m_numMaxCops = 2;
	m_copRespawnInterval = 5;	// spawn on every 20th traffic car
	m_numMaxTrafficCars = 32;

	m_spawnedTrafficCars = 0;
	//m_carEntryIdx = 0;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;
}

CAIManager::~CAIManager()
{

}

void CAIManager::Init(const DkList<pedestrianConfig_t*>& pedConfigs)
{
	PrecacheObject(CAIPursuerCar);

	// reset variables
	m_copAccelerationModifier = 1.0f;
	m_copMaxDamage = AI_COP_DEFAULT_DAMAGE;
	m_copMaxSpeed = AI_COP_DEFAULT_MAXSPEED;
	
	m_numMaxTrafficCars = g_trafficMaxCars.GetInt();
	m_spawnedTrafficCars = 0;
	//m_carEntryIdx = 0;

	m_trafficUpdateTime = 0.0f;
	m_pedsUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;
	m_trafficSpawnInterval = 0;

	m_enableTrafficCars = true;
	m_enableCops = true;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;

	for (int i = 0; i < PURSUER_TYPE_COUNT; i++)
		m_copCarName[i] = "";

	m_copLoudhailerTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY + 5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);

	// load car and pedestrian zones
	InitZoneEntries(pedConfigs);
}

void CAIManager::Shutdown()
{
	m_civCarEntries.clear();
	m_pedEntries.clear();

	m_trafficCars.clear();
	m_pedestrians.clear();

	m_speechQueue.clear();

	for (int i = 0; i < m_roadBlocks.numElem(); i++)
		delete m_roadBlocks[i];

	m_roadBlocks.clear();
}

void CAIManager::InitZoneEntries(const DkList<pedestrianConfig_t*>& pedConfigs)
{
	EqString vehicleZoneFilename(varargs("scripts/levels/%s_vehiclezones.def", g_pGameWorld->GetLevelName()));

	EqString defaultCopCar;

	bool vehZoneError = false;

	KeyValues kvs;
	if (!kvs.LoadFromFile(vehicleZoneFilename.c_str()))
	{
		MsgWarning("Cannot load vehicle zone file '%s', using default!\n", vehicleZoneFilename.c_str());
		vehicleZoneFilename = "scripts/levels/default_vehiclezones.def";

		if (!kvs.LoadFromFile(vehicleZoneFilename.c_str()))
		{
			MsgError("Failed to load vehicle zone file '%s'!\n", vehicleZoneFilename.c_str());
			vehZoneError = true;
		}
	}
	
	if(!vehZoneError)
	{
		kvkeybase_t* zone_presets = kvs.GetRootSection();

		// thru all zone presets
		for (int i = 0; i < zone_presets->keys.numElem(); i++)
		{
			kvkeybase_t* zone_kv = zone_presets->keys[i];

			// thru vehicles in zone preset
			for (int j = 0; j < zone_kv->keys.numElem(); j++)
			{
				vehicleConfig_t* carConfig = g_pGameSession->GetVehicleConfig(zone_kv->keys[j]->name);

				if (!carConfig)
				{
					MsgWarning("Unknown vehicle '%s' for zone '%s'\n", zone_kv->keys[j]->name, zone_kv->name);
					continue;
				}

				if (carConfig->flags.isCop)
					defaultCopCar = carConfig->carName;

				civCarEntry_t* civCarEntry = FindCivCarEntry(carConfig->carName.c_str());

				// add new car entry if not exist
				if (!civCarEntry)
				{
					civCarEntry_t entry;
					entry.config = carConfig;

					bool isRegisteredCop = !m_copCarName[PURSUER_TYPE_COP].CompareCaseIns(entry.config->carName);
					if (isRegisteredCop)
						entry.nextSpawn = m_copRespawnInterval;

					entry.nextSpawn += INITIAL_SPAWN_INTERVAL + m_trafficSpawnInterval;

					int idx = g_pAIManager->m_civCarEntries.append(entry);
					civCarEntry = &g_pAIManager->m_civCarEntries[j];
				}

				int spawnInterval = KV_GetValueInt(zone_kv->keys[j], 0, 0);

				// append zone with given spawn interval
				civCarEntry->zoneList.append(spawnZoneInfo_t{ zone_kv->name, spawnInterval });
			}
		}
	}

	// precache any cars used by zones
	for (int i = 0; i < m_civCarEntries.numElem(); i++)
	{
		vehicleConfig_t* conf = m_civCarEntries[i].config;

		// FIXME: it's bad to precache models here
		PrecacheStudioModel(conf->visual.cleanModelName.c_str());
		PrecacheStudioModel(conf->visual.damModelName.c_str());
	}

	// init the cop car
	kvkeybase_t* copConfigName = kvs.GetRootSection()->FindKeyBase("copcar");
	SetCopCarConfig(KV_GetValueString(copConfigName, 0, defaultCopCar.c_str()), PURSUER_TYPE_COP);

	// init peds
	for (int i = 0; i < pedConfigs.numElem(); i++)
	{
		if(pedConfigs[i]->hasAI)
			m_pedEntries.append(pedestrianEntry_t{ pedConfigs[i], 0 });
	}
}

CCar* CAIManager::SpawnTrafficCar(const IVector2D& globalCell)
{
	if (!m_civCarEntries.numElem())
		return nullptr;

	if (m_spawnedTrafficCars >= GetMaxTrafficCars())
		return nullptr;

	CLevelRegion* pReg = nullptr;
	levroadcell_t* roadCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(globalCell, &pReg);

	// no tile - no spawn
	if (!pReg || !roadCell)
		return nullptr;

	if (!pReg->m_isLoaded)
		return nullptr;

	bool isParkingLot = (roadCell->type == ROADTYPE_PARKINGLOT);

	// parking lots are non-straight road cells
	if (!(roadCell->type == ROADTYPE_STRAIGHT || isParkingLot))
		return nullptr;

	// don't spawn cars on short roads
	if (!isParkingLot)
	{
		straight_t str = g_pGameWorld->m_level.Road_GetStraightAtPoint(globalCell, 2);

		if (str.breakIter <= 1)
			return nullptr;
	}

	// don't spawn if distance between cars is too short
	for (int j = 0; j < m_trafficCars.numElem(); j++)
	{
		IVector2D trafficPosGlobal;
		if (!g_pGameWorld->m_level.GetTileGlobal(m_trafficCars[j]->GetOrigin(), trafficPosGlobal))
			continue;

		if (distance(trafficPosGlobal, globalCell) < AI_TRAFFIC_SPAWN_DISTANCE_THRESH*AI_TRAFFIC_SPAWN_DISTANCE_THRESH)
			return nullptr;
	}

	IVector2D leadCellPos = g_pGameWorld->m_level.PositionToGlobalTilePoint(m_leadPosition);

	// this one would lead to problems with pre-recorded cars
	{
		if (length(IVector2D(m_leadVelocity.xz())) > 3)
		{
			// if velocity is negative to new spawn origin, cancel spawning
			if (dot(globalCell - leadCellPos, IVector2D(m_leadVelocity.xz())) < 0)
				return nullptr;
		}
	}


	// if this is a parking straight, the cars might start here stopped or even empty
	bool isParkingStraight = (roadCell->flags & ROAD_FLAG_PARKING) > 0;

	CCar* spawnedCar = nullptr;

	// count the cops
	int numCopsSpawned = 0;

	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CAIPursuerCar* pursuer = UTIL_CastToPursuer(m_trafficCars[i]);

		// cops from roadblocks are not counted here!
		if (pursuer && !pursuer->m_assignedRoadblock)
			numCopsSpawned++;
	}

	int carEntryIdx = g_replayRandom.Get(0, m_civCarEntries.numElem() - 1);

	// pick first
	civCarEntry_t* carEntry = &m_civCarEntries[carEntryIdx];
	vehicleConfig_t* carConf = carEntry->config;

	if (isParkingLot)
	{
		bool isRegisteredCop = !m_copCarName[PURSUER_TYPE_COP].CompareCaseIns(carConf->carName);
		bool isRegisteredGang = !m_copCarName[PURSUER_TYPE_GANG].CompareCaseIns(carConf->carName);

		if (--carEntry->nextSpawn > 0)
			return nullptr;

		carEntry->nextSpawn = (isRegisteredCop ? m_copRespawnInterval : carEntry->GetZoneSpawnInterval("default")) + m_trafficSpawnInterval;

		if (carConf->flags.allowParked)
		{
			spawnedCar = new CCar(carConf);
			spawnedCar->Enable(false);
		}
	}
	else
	{
		if (m_enableCops && numCopsSpawned < GetMaxCops())
		{
			carEntry = &m_civCarEntries[m_copsEntryIdx];
			carConf = carEntry->config;

			if (--carEntry->nextSpawn > 0)	// no luck? switch to civcars
			{
				carEntry = &m_civCarEntries[carEntryIdx];
				carConf = carEntry->config;

				if (--carEntry->nextSpawn > 0)
					return nullptr;
			}
		}
		else if (--carEntry->nextSpawn > 0)
			return nullptr;

		bool isRegisteredCop = !m_copCarName[PURSUER_TYPE_COP].CompareCaseIns(carConf->carName);
		bool isRegisteredGang = !m_copCarName[PURSUER_TYPE_GANG].CompareCaseIns(carConf->carName);

		carEntry->nextSpawn = (isRegisteredCop ? m_copRespawnInterval : carEntry->GetZoneSpawnInterval("default")) + m_trafficSpawnInterval;

		if (isRegisteredCop || isRegisteredGang)
		{
			// don't add cops on initial spawn
			if (m_enableCops && numCopsSpawned < GetMaxCops() && (g_replayTracker->m_tick > 0 || m_numMaxCops >= MIN_COPS_TO_INIT_SURVIVAL))
			{
				CAIPursuerCar* pursuer = new CAIPursuerCar(carConf, isRegisteredGang ? PURSUER_TYPE_GANG : PURSUER_TYPE_COP);
				pursuer->SetTorqueScale(m_copAccelerationModifier);
				pursuer->SetMaxDamage(m_copMaxDamage);
				pursuer->SetMaxSpeed(m_copMaxSpeed);

				spawnedCar = pursuer;
			}
		}
		else
		{
			spawnedCar = new CAITrafficCar(carConf);
		}
	}

	if(!spawnedCar)
		return nullptr;

	// regenerate if car has to be spawned
	g_replayRandom.Regenerate();
	g_replayTracker->PushEvent(REPLAY_EVENT_FORCE_RANDOM);

	// Spawn required for PlaceOnRoadCell
	spawnedCar->Spawn();
	spawnedCar->PlaceOnRoadCell(pReg, roadCell);

	if( carConf->numColors )
	{
		int col_idx = g_replayRandom.Get(0, carConf->numColors - 1);
		spawnedCar->SetColorScheme(col_idx);
	}

	if(spawnedCar->ObjType() == GO_CAR_AI)
		((CAITrafficCar*)spawnedCar)->InitAI( false ); // TODO: chance of stoped, empty and active car on parking lane

	g_pGameWorld->AddObject(spawnedCar);
	m_trafficCars.append(spawnedCar);

	m_spawnedTrafficCars++;

	return spawnedCar;
}

OOLUA::Table CAIManager::L_QueryTrafficCars(float radius, const Vector3D& position) const
{
	OOLUA::Script& state = GetLuaState();
	EqLua::LuaStackGuard g(state);
	int numObjs = 0;

	OOLUA::Table resultObjects = OOLUA::new_table(state);

	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CCar* car = m_trafficCars[i];

		Vector3D dirVec = car->GetOrigin() - position;

		float dotDist = dot(dirVec, dirVec);

		if (dotDist > radius*radius)
			continue;

		numObjs++;
		resultObjects.set(numObjs, car);
	}

	return resultObjects;
}

void CAIManager::QueryTrafficCars(DkList<CCar*>& list, float radius, const Vector3D& position, const Vector3D& direction, float queryCosAngle /* = 0.0f*/)
{
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CCar* car = m_trafficCars[i];

		Vector3D dirVec = car->GetOrigin()-position;

		float dotDist = dot(dirVec, dirVec);

		if (dotDist > radius*radius)
			continue;


		// is visible to cone?
		if (queryCosAngle > 0.0f)
		{
			float cosAngle = dot(fastNormalize(dirVec), direction);

			if (cosAngle > queryCosAngle)
				list.append(car);
		}
		else
			list.append(car);
	}
}

int CAIManager::CircularSpawnTrafficCars(int x0, int y0, int radius)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	int count = 0;
	
	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		count += SpawnTrafficCar(IVector2D(x0 + x, y0 + y)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 - x, y0 + y)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 + x, y0 - y)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 - x, y0 - y)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 + y, y0 + x)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 - y, y0 + x)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 + y, y0 - x)) != nullptr;
		count += SpawnTrafficCar(IVector2D(x0 - y, y0 - x)) != nullptr;
	}

	count += SpawnTrafficCar(IVector2D(x0, y0 + radius)) != nullptr;
	count += SpawnTrafficCar(IVector2D(x0, y0 - radius)) != nullptr;
	count += SpawnTrafficCar(IVector2D(x0 + radius, y0)) != nullptr;
	count += SpawnTrafficCar(IVector2D(x0 - radius, y0)) != nullptr;

	return count;
}

int CAIManager::CircularSpawnPedestrians(int x0, int y0, int radius)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;

	int count = 0;

	while (x < y)
	{
		if (f >= 0)
		{
			y--;
			ddF_y += 2;
			f += ddF_y;
		}

		x++;
		ddF_x += 2;
		f += ddF_x + 1;

		count += SpawnPedestrian(IVector2D(x0 + x, y0 + y)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 - x, y0 + y)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 + x, y0 - y)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 - x, y0 - y)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 + y, y0 + x)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 - y, y0 + x)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 + y, y0 - x)) != nullptr;
		count += SpawnPedestrian(IVector2D(x0 - y, y0 - x)) != nullptr;
	}

	count += SpawnPedestrian(IVector2D(x0, y0 + radius)) != nullptr;
	count += SpawnPedestrian(IVector2D(x0, y0 - radius)) != nullptr;
	count += SpawnPedestrian(IVector2D(x0 + radius, y0)) != nullptr;
	count += SpawnPedestrian(IVector2D(x0 - radius, y0)) != nullptr;

	return count;
}


void CAIManager::RemoveTrafficCar(CCar* car)
{
	if (m_trafficCars.fastRemove(car))
	{
		if(car->GetScriptID() == SCRIPT_ID_NOTSCRIPTED)
			m_spawnedTrafficCars--;

		g_pGameWorld->RemoveObject(car);
	}
}

void CAIManager::UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{
	if (g_replayTracker->m_state == REPL_PLAYING)
		return;

	m_trafficUpdateTime -= fDt;

	if (m_trafficUpdateTime <= 0.0f)
		m_trafficUpdateTime = AI_TRAFFIC_RESPAWN_TIME;
	else
		return;

	// roadblocks updated after traffic cars are removed
	UpdateRoadblocks();

	//-------------------------------------------------

	IVector2D spawnCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(spawnOrigin, spawnCenterCell))
		return;

	IVector2D removeCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(removeOrigin, removeCenterCell))
		return;

	m_leadPosition = spawnOrigin;
	m_leadRemovePosition = removeOrigin;
	m_leadVelocity = leadVelocity;

	// remove furthest cars from world
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CCar* car = m_trafficCars[i];

		Vector3D carPos = car->GetOrigin();

		// if it's on unloaded region, it should be forcely removed
		CLevelRegion* reg = g_pGameWorld->m_level.GetRegionAtPosition(carPos);

		if (!reg || (reg && !reg->m_isLoaded))
		{
			RemoveTrafficCar(car);
			i--;
			continue;
		}

		CAIPursuerCar* pursuerCheck = UTIL_CastToPursuer(car);

		// pursuers are not removed if in pursuit.
		// not in survival, not flipped over
		if (m_numMaxCops < MIN_COPS_TO_INIT_SURVIVAL && pursuerCheck && pursuerCheck->InPursuit() && !pursuerCheck->IsFlippedOver())
			continue;

		IVector2D trafficCell;
		reg->m_heightfield[0]->PointAtPos(carPos, trafficCell.x, trafficCell.y);

		g_pGameWorld->m_level.LocalToGlobalPoint(trafficCell, reg, trafficCell);

		int distToCell = length(trafficCell - removeCenterCell);
		int distToCell2 = length(trafficCell - spawnCenterCell);

		if (distToCell > g_traffic_maxdist.GetInt() && 
			distToCell2 > g_traffic_maxdist.GetInt())
		{
			RemoveTrafficCar(car);
			i--;
			continue;
		}
	}

	// now spawn new cars
	CircularSpawnTrafficCars(spawnCenterCell.x, spawnCenterCell.y, g_traffic_mindist.GetInt());
}

void CAIManager::InitialSpawns(const Vector3D& spawnOrigin)
{
	IVector2D spawnCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(spawnOrigin, spawnCenterCell))
		return;

	int count = 0;

	if (g_replayTracker->m_state != REPL_PLAYING)
	{
		for (int r = g_traffic_initial_mindist.GetInt(); r < g_traffic_mindist.GetInt(); r+=2)
		{
			count += CircularSpawnTrafficCars(spawnCenterCell.x, spawnCenterCell.y, r);
		}
	}

	for (int r = g_pedestrians_initial_mindist.GetInt(); r < g_pedestrians_mindist.GetInt(); r += 2)
	{
		count += CircularSpawnPedestrians(spawnCenterCell.x, spawnCenterCell.y, r);
	}
}

void CAIManager::UpdateNavigationVelocityMap(float fDt)
{
	/*
	m_velocityMapUpdateTime += fDt;

	if(m_velocityMapUpdateTime > 0.5f)
		m_velocityMapUpdateTime = 0.0f;
	else
		return;

	// clear navgrid dynamic obstacle
	g_pGameWorld->m_level.Nav_ClearDynamicObstacleMap();
	
	// draw all car velocities on dynamic obstacle
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		PaintVelocityMapFrom(m_trafficCars[i]);
	}
	*/
	// update debugging of navigation maps here
	g_pGameWorld->m_level.UpdateDebugMaps();
	
}

void CAIManager::PaintVelocityMapFrom(CCar* car)
{
	int dx[8] = NEIGHBOR_OFFS_XDX(0,1);
	int dy[8] = NEIGHBOR_OFFS_YDY(0,1);

	Vector3D carForward = car->GetForwardVector();
	Vector3D bodySizeOffset = carForward*car->m_conf->physics.body_size.z;

	Vector3D carPos = car->GetOrigin() - bodySizeOffset;

	Vector3D velocity = bodySizeOffset*2.0f + car->GetVelocity();

	Vector3D rightVec = cross( normalize(velocity+carForward), vec3_up);

	IVector2D navCellStartPos1 = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(carPos - rightVec*0.5f);
	IVector2D navCellEndPos1 = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(carPos+velocity - rightVec*0.5f);

	IVector2D navCellStartPos2 = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(carPos + rightVec*0.5f);
	IVector2D navCellEndPos2 = g_pGameWorld->m_level.Nav_PositionToGlobalNavPoint(carPos+velocity + rightVec*0.5f);

	// do the line
	PaintNavigationLine(navCellStartPos1, navCellEndPos1);
	PaintNavigationLine(navCellStartPos2, navCellEndPos2);
}

void CAIManager::PaintNavigationLine(const IVector2D& start, const IVector2D& end)
{
	int x1,y1,x2,y2;

	x1 = start.x;
	y1 = start.y;
	x2 = end.x;
	y2 = end.y;

    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    for (;;)
	{
		ubyte& navCellValue = g_pGameWorld->m_level.Nav_GetTileAtGlobalPoint(IVector2D(x1,y1), true);
		navCellValue = 0;

        if (x1 == x2 && y1 == y2)
			break;

        e2 = 2 * err;

        // EITHER horizontal OR vertical step (but not both!)
        if (e2 > dy)
		{
            err += dy;
            x1 += sx;
        }
		else if (e2 < dx)
		{
            err += dx;
            y1 += sy;
        }
    }
}

void CAIManager::UpdatePedestrianRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{
	m_pedsUpdateTime -= fDt;

	if (m_pedsUpdateTime <= 0.0f)
		m_pedsUpdateTime = AI_PEDESTRIAN_RESPAWN_TIME;
	else
		return;

	IVector2D spawnCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(spawnOrigin, spawnCenterCell))
		return;

	IVector2D removeCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(removeOrigin, removeCenterCell))
		return;

	// remove furthest cars from world
	for (int i = 0; i < m_pedestrians.numElem(); i++)
	{
		CPedestrian* ped = m_pedestrians[i];

		Vector3D pedPos = ped->GetOrigin();

		// if it's on unloaded region, it should be forcely removed
		CLevelRegion* reg = g_pGameWorld->m_level.GetRegionAtPosition(pedPos);

		if (!reg || (reg && !reg->m_isLoaded))
		{
			RemovePedestrian(ped);
			i--;
			continue;
		}

		IVector2D pedCell;
		reg->m_heightfield[0]->PointAtPos(pedPos, pedCell.x, pedCell.y);

		g_pGameWorld->m_level.LocalToGlobalPoint(pedCell, reg, pedCell);

		int distToCell = length(pedCell - removeCenterCell);
		int distToCell2 = length(pedCell - spawnCenterCell);

		if (distToCell > g_pedestrians_maxdist.GetInt() &&
			distToCell2 > g_pedestrians_maxdist.GetInt())
		{
			RemovePedestrian(ped);
			i--;
			continue;
		}
	}

	// now spawn new cars
	CircularSpawnPedestrians(spawnCenterCell.x, spawnCenterCell.y, g_pedestrians_mindist.GetInt());
}

void CAIManager::RemovePedestrian(CPedestrian* ped)
{
	if (m_pedestrians.fastRemove(ped))
		g_pGameWorld->RemoveObject(ped);
}

void CAIManager::RemoveAllPedestrians()
{
	for (int i = 0; i < m_pedestrians.numElem(); i++)
	{
		RemovePedestrian(m_pedestrians[i]);
		i--;
	}

	m_pedestrians.clear();
}

CPedestrian* CAIManager::SpawnPedestrian(const IVector2D& globalCell)
{
	if(!m_pedEntries.numElem())
		return nullptr;

	if (m_pedestrians.numElem() >= g_pedestriansMax.GetInt())
		return nullptr;

	CLevelRegion* pReg = nullptr;
	levroadcell_t* roadCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(globalCell, &pReg);

	// no tile - no spawn
	if (!pReg || !roadCell)
		return nullptr;

	if (!pReg->m_isLoaded)
		return nullptr;

	// parking lots are non-straight road cells
	if (roadCell->type != ROADTYPE_PAVEMENT)
		return nullptr;

	// don't spawn if distance between cars is too short
	for (int j = 0; j < m_pedestrians.numElem(); j++)
	{
		IVector2D pedPosGlobal;
		if (!g_pGameWorld->m_level.GetTileGlobal(m_pedestrians[j]->GetOrigin(), pedPosGlobal))
			continue;

		if (distance(pedPosGlobal, globalCell) < AI_TRAFFIC_SPAWN_DISTANCE_THRESH*AI_TRAFFIC_SPAWN_DISTANCE_THRESH)
			return nullptr;
	}

	IVector2D leadCellPos = g_pGameWorld->m_level.PositionToGlobalTilePoint(m_leadPosition);

	// this one would lead to problems with pre-recorded cars
	{
		if (length(IVector2D(m_leadVelocity.xz())) > 3)
		{
			// if velocity is negative to new spawn origin, cancel spawning
			if (dot(globalCell - leadCellPos, IVector2D(m_leadVelocity.xz())) < 0)
				return nullptr;
		}
	}

	Vector3D pedPos = g_pGameWorld->m_level.GlobalTilePointToPosition(globalCell) + Vector3D(0.0f,0.8f,0.0f);

	int randomPedEntry = g_replayRandom.Get(0, m_pedEntries.numElem()-1);

	pedestrianEntry_t& entry = m_pedEntries[randomPedEntry];
	if (entry.nextSpawn > 0)
	{
		entry.nextSpawn--;
		return nullptr;
	}

	entry.nextSpawn = entry.config->spawnInterval + PEDESTRIAN_SPAWN_INTERVAL;

	CPedestrian* spawnedPed = new CPedestrian(entry.config);
	spawnedPed->Spawn();
	spawnedPed->SetOrigin(pedPos);

	m_pedestrians.append(spawnedPed);

	g_pGameWorld->AddObject(spawnedPed);

	return spawnedPed;
}

void CAIManager::QueryPedestrians(DkList<CPedestrian*>& list, float radius, const Vector3D& position, const Vector3D& direction, float queryCosAngle)
{

}

//-----------------------------------------------------------------------------------------

void CAIManager::UpdateRoadblocks()
{
	// go through all roadblocks and count the cars
	// if there's insufficient car amount in some,
	// they will be deleted
	for (int i = 0; i < m_roadBlocks.numElem(); i++)
	{
		RoadBlockInfo_t* roadblock = m_roadBlocks[i];

		int numAliveCars = 0;
		for (int j = 0; j < roadblock->activeCars.numElem(); j++)
		{
			if(roadblock->activeCars[j]->IsAlive())
				numAliveCars++;
		}

		// dissolve road block
		if (numAliveCars < MIN_ROADBLOCK_CARS)
		{
			delete roadblock;

			m_roadBlocks.fastRemove(roadblock);
			i--;
		}
	}
}

void CAIManager::UpdateCopStuff(float fDt)
{
	//debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "cops spawned: %d (max %d) (lvl=%d)\n", m_copCars.numElem(), GetMaxCops(), m_copRespawnInterval);
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "num traffic cars: %d\n", m_trafficCars.numElem());
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "num road blocks: %d\n", m_roadBlocks.numElem());
	debugoverlay->Text(ColorRGBA(1, 1, 1, 1), "cop speech time: %.2f loudhailer: %.2f\n", m_copSpeechTime, m_copLoudhailerTime);

	m_copSpeechTime -= fDt;
	m_copLoudhailerTime -= fDt;

	ISoundPlayable* copVoiceChannel = soundsystem->GetStaticStreamChannel(CHAN_VOICE);

	// say something
	if (copVoiceChannel->GetState() == SOUND_STATE_STOPPED && m_speechQueue.numElem() > 0)
	{
		EmitSound_t ep;
		ep.name = m_speechQueue[0].c_str();

		g_sounds->Emit2DSound(&ep);

		m_speechQueue.removeIndex(0);
	}
}

void CAIManager::RemoveAllCars()
{
	// Try to remove cars
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		RemoveTrafficCar(m_trafficCars[i]);
		i--;
	}

	m_trafficCars.clear();
	m_spawnedTrafficCars = 0;
}

civCarEntry_t* CAIManager::FindCivCarEntry( const char* name )
{
	for(int i = 0; i < m_civCarEntries.numElem(); i++)
	{
		if(!m_civCarEntries[i].config->carName.CompareCaseIns(name))
		{
			return &m_civCarEntries[i];
		}
	}

	return NULL;
}

DECLARE_CMD(force_roadblock, "Forces spawn roadblock on cur car", CV_CHEAT)
{
	float targetAngle = VectorAngles(normalize(g_pGameSession->GetPlayerCar()->GetVelocity())).y+45.0f;

	g_pAIManager->SpawnRoadBlockFor( g_pGameSession->GetPlayerCar(), targetAngle );
}

IVector2D GetDirectionVec(int dirIdx);

DECLARE_CMD(road_info, "Road info on player car position", CV_CHEAT)
{
	Vector3D carPos = g_pGameSession->GetPlayerCar()->GetOrigin();

	IVector2D playerCarCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(carPos, playerCarCell))
		return;

	int curLane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(playerCarCell, 16) - 1;
	int numLanes = g_pGameWorld->m_level.Road_GetWidthInLanesAtPoint(playerCarCell, 32, 1);

	MsgInfo("Road width: %d, current lane: %d\n", numLanes, curLane);
}

bool CAIManager::SpawnRoadBlockFor( CCar* car, float directionAngle )
{
	if (g_replayTracker->m_state == REPL_PLAYING)
		return true;

	IVector2D playerCarCell;
	if (!g_pGameWorld->m_level.GetTileGlobal( car->GetOrigin(), playerCarCell ))
		return false;

	directionAngle = NormalizeAngle360( -directionAngle );

	int targetDir = directionAngle / 90.0f;

	if (targetDir < 0)
		targetDir += 4;

	if (targetDir > 3)
		targetDir -= 4;

	int radius = g_traffic_mindist.GetInt();

	IVector2D carDir = GetDirectionVec(targetDir);
	IVector2D placementVec = playerCarCell - carDir*radius;

	levroadcell_t* startCellPlacement = g_pGameWorld->m_level.Road_GetGlobalTileAt(placementVec);

	if(!startCellPlacement || (startCellPlacement && !(startCellPlacement->type == ERoadType::ROADTYPE_STRAIGHT || startCellPlacement->type == ERoadType::ROADTYPE_PARKINGLOT)))
		return false;

	IVector2D perpendicular = GetDirectionVec(startCellPlacement->direction-1);

	int curLane = g_pGameWorld->m_level.Road_GetLaneIndexAtPoint(placementVec, 16)-1;
	placementVec -= perpendicular*curLane;
	int numLanes = g_pGameWorld->m_level.Road_GetWidthInLanesAtPoint(placementVec, 32, 1);

	vehicleConfig_t* conf = g_pGameSession->GetVehicleConfig(m_copCarName[PURSUER_TYPE_COP].c_str());

	if (!conf)
		return 0;

	RoadBlockInfo_t* roadblock = new RoadBlockInfo_t();

	roadblock->roadblockPosA = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec);
	roadblock->roadblockPosB = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec+perpendicular*numLanes);

	debugoverlay->Line3D(roadblock->roadblockPosA + vec3_up, roadblock->roadblockPosB + vec3_up, ColorRGBA(1,0,0,1), ColorRGBA(0,1,0,1), 1000.0f);

	for(int i = 0; i < numLanes; i++)
	{
		IVector2D blockPoint = placementVec + carDir*IVector2D(i % 2)*3; // offset in 3 tiles while checker

		placementVec += perpendicular;

		CLevelRegion* pReg = NULL;
		levroadcell_t* roadCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(blockPoint, &pReg);

		Vector3D gPos = g_pGameWorld->m_level.GlobalTilePointToPosition(blockPoint);
		debugoverlay->Box3D(gPos - 0.5f, gPos + 0.5f, ColorRGBA(1, 1, 0, 0.5f), 1000.0f);

		// no tile - no spawn
		if (!pReg || !roadCell)
			continue;

		if (!pReg->m_isLoaded)
			continue;
		
		/*
		if (roadCell->type != ROADTYPE_STRAIGHT)
		{
			MsgError( "Can't spawn where no straight road (%d)\n", roadCell->type);
			continue;
		}*/

		CAIPursuerCar* copBlockCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);

		copBlockCar->m_sirenEnabled = true;

		copBlockCar->Spawn();
		copBlockCar->Enable(false);
		copBlockCar->PlaceOnRoadCell(pReg, roadCell);
		copBlockCar->InitAI(false);
		copBlockCar->SetTorqueScale(m_copAccelerationModifier);
		copBlockCar->SetMaxDamage(m_copMaxDamage);

		g_pGameWorld->AddObject(copBlockCar);

		float angle = 120.0f - (i % 2)*60.0f;

		copBlockCar->SetAngles(Vector3D(0.0f, targetDir*-90.0f + angle, 0.0f));

		// assign this car to roadblock
		copBlockCar->m_assignedRoadblock = roadblock;
		roadblock->activeCars.append( copBlockCar );

		// also it has to be added to traffic cars
		m_trafficCars.append( copBlockCar );
		m_spawnedTrafficCars++;
	}

	roadblock->totalSpawns = roadblock->activeCars.numElem();

	if (roadblock->totalSpawns == 0)
		delete roadblock;
	else
		m_roadBlocks.append(roadblock);

	return (roadblock != nullptr);
}

void CAIManager::MakePursued( CCar* car )
{
	for(int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CAIPursuerCar* pursuer = UTIL_CastToPursuer(m_trafficCars[i]);

		if(pursuer && !pursuer->InPursuit() && pursuer->IsAlive())
		{
			pursuer->SetPursuitTarget(car);
			pursuer->BeginPursuit(0.0f);
		}
	}
}

void CAIManager::StopPursuit( CCar* car )
{
	for(int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CAIPursuerCar* pursuer = UTIL_CastToPursuer(m_trafficCars[i]);

		if(pursuer && !pursuer->InPursuit() && pursuer->GetPursuitTarget() == car)
			pursuer->EndPursuit(!pursuer->IsAlive());
	}
}

bool CAIManager::IsRoadBlockSpawn() const
{
	return (m_roadBlocks.numElem() > 0);
}

// ----- TRAFFIC ------
void CAIManager::SetMaxTrafficCars(int count)
{
	m_numMaxTrafficCars = count;
}

int CAIManager::GetMaxTrafficCars() const
{
	return m_enableTrafficCars ? m_numMaxTrafficCars : 0;
}

void CAIManager::SetTrafficSpawnInterval(int count)
{
	m_trafficSpawnInterval = count;
}

int CAIManager::GetTrafficSpawnInterval() const
{
	return m_trafficSpawnInterval;
}

void CAIManager::SetTrafficCarsEnabled(bool enable)
{
	m_enableTrafficCars = enable;
}

bool CAIManager::IsTrafficCarsEnabled() const
{
	return m_enableTrafficCars;
}

// ----- COPS ------
// switch to spawn
void CAIManager::SetCopsEnabled(bool enable)
{
	m_enableCops = enable;
}

bool CAIManager::IsCopsEnabled() const
{
	return m_enableCops;
}

// maximum cop count
void CAIManager::SetMaxCops(int count)
{
	m_numMaxCops = count;
}

int CAIManager::GetMaxCops() const
{
	return m_numMaxCops;
}

// cop respawn interval between traffic car spawns
void CAIManager::SetCopRespawnInterval(int steps)
{
	m_copRespawnInterval = steps;
}

int CAIManager::GetCopRespawnInterval() const
{
	return m_copRespawnInterval;
}

// acceleration modifier (e.g. for survival)
void CAIManager::SetCopAccelerationModifier(float accel)
{
	m_copAccelerationModifier = accel;
}

float CAIManager::GetCopAccelerationModifier() const
{
	return m_copAccelerationModifier;
}

// sets the maximum hitpoints for cop cars
void CAIManager::SetCopMaxDamage(float maxHitPoints)
{
	m_copMaxDamage = maxHitPoints;
}
					
float CAIManager::GetCopMaxDamage() const
{
	return m_copMaxDamage;
}

// sets the maximum hitpoints for cop cars
void CAIManager::SetCopMaxSpeed(float maxSpeed)
{
	m_copMaxSpeed = maxSpeed;
}
					
float CAIManager::GetCopMaxSpeed() const
{
	return m_copMaxSpeed;
}

// sets cop car configuration
void CAIManager::SetCopCarConfig(const char* car_name, int type )
{
	m_copCarName[type] = car_name;
	m_copsEntryIdx = 0;

	for (int i = 0; i < m_civCarEntries.numElem(); i++)
	{
		bool isRegisteredCop = !m_copCarName[PURSUER_TYPE_COP].CompareCaseIns(m_civCarEntries[i].config->carName);

		if (isRegisteredCop)
		{
			m_copsEntryIdx = i;
			break;
		}
	}
}

// shedules a cop speech
bool CAIManager::MakeCopSpeech(const char* soundScriptName, bool force, float priority)
{
	// shedule speech
	if (m_copSpeechTime-(priority*AI_COP_SPEECH_DELAY) < 0 || force)
	{
		m_speechQueue.append(soundScriptName);

		if (m_speechQueue.numElem() > AI_COP_SPEECH_QUEUE)
			m_speechQueue.removeIndex(0);

		m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 2.0f);

		return true;
	}

	return false;
}

bool CAIManager::IsCopsCanUseLoudhailer(CCar* copCar, CCar* target) const
{
	CCar* nearestCar = nullptr;

	if (target)
	{
		float nearestDist = DrvSynUnits::MaxCoordInUnits;

		for (int i = 0; i < m_trafficCars.numElem(); i++)
		{
			CAIPursuerCar* pursuer = UTIL_CastToPursuer(m_trafficCars[i]);

			if (!pursuer)
				continue;
			
			float dist = length(target->GetOrigin() - pursuer->GetOrigin());
			if (dist < nearestDist)
			{
				nearestDist = dist;
				nearestCar = pursuer;
			}
		}
	}

	return (m_copSpeechTime < 0) && (m_copLoudhailerTime < 0) && (copCar == nearestCar);
}

void CAIManager::CopLoudhailerTold()
{
	m_copLoudhailerTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY+5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}

OOLUA_EXPORT_FUNCTIONS(
	CAIManager,

	MakePursued,
	StopPursuit,
	RemoveAllCars,
	SetMaxTrafficCars,
	SetTrafficSpawnInterval,
	SetTrafficCarsEnabled,
	SetCopsEnabled,
	SetCopCarConfig,
	SetCopMaxDamage,
	SetCopAccelerationModifier,
	SetCopMaxSpeed,
	SetMaxCops,
	SetCopRespawnInterval,
	MakeCopSpeech,
	SpawnRoadBlockFor
)

OOLUA_EXPORT_FUNCTIONS_CONST(
	CAIManager,

	GetMaxTrafficCars,
	IsTrafficCarsEnabled,
	GetTrafficSpawnInterval,
	IsCopsEnabled,
	GetCopMaxDamage,
	GetCopAccelerationModifier,
	GetCopMaxSpeed,
	GetMaxCops,
	GetCopRespawnInterval,
	IsRoadBlockSpawn,
	QueryTrafficCars
)
