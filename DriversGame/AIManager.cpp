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

ConVar g_trafficMaxCars("g_trafficMaxCars", "48", "Maximum traffic cars", CV_CHEAT);

ConVar g_traffic_initial_mindist("g_traffic_initial_mindist", "15", "Min traffic car distance to spawn", CV_CHEAT);
ConVar g_traffic_mindist("g_traffic_mindist", "50", "Min traffic car distance to spawn", CV_CHEAT);
ConVar g_traffic_maxdist("g_traffic_maxdist", "51", "Max traffic car distance, to disappear", CV_CHEAT);

const int	AI_COP_SPEECH_QUEUE = 6;

const float AI_COP_SPEECH_DELAY = 4.0f;		// delay before next speech
const float AI_COP_TAUNT_DELAY = 5.0f;		// delay before next taunt

const float AI_COP_DEFAULT_DAMAGE = 4.0f;
const float AI_COP_DEFAULT_MAXSPEED = 160.0f;

const int MIN_ROADBLOCK_CARS = 2;

const float AI_TRAFFIC_RESPAWN_TIME	= 0.1f;
const float AI_TRAFFIC_SPAWN_DISTANCE_THRESH = 5.0f;

//------------------------------------------------------------------------------------------

static CAIManager s_AIManager;
CAIManager* g_pAIManager = &s_AIManager;

CAIManager::CAIManager()
{
	m_trafficUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;

	m_enableCops = true;
	m_enableTrafficCars = true;

	m_copAccelerationModifier = 1.0f;
	m_copMaxDamage = AI_COP_DEFAULT_DAMAGE;
	m_copMaxSpeed = AI_COP_DEFAULT_MAXSPEED;

	m_numMaxCops = 2;
	m_copRespawnInterval = 20;	// spawn on every 20th traffic car
	m_numMaxTrafficCars = 32;
	
	m_copSpawnIntervalCounter = 0;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;
}

CAIManager::~CAIManager()
{

}

void CAIManager::Init()
{
	PrecacheObject(CAIPursuerCar);

	// reset variables
	m_copAccelerationModifier = 1.0f;
	m_copMaxDamage = AI_COP_DEFAULT_DAMAGE;
	m_copMaxSpeed = AI_COP_DEFAULT_MAXSPEED;
	
	m_numMaxTrafficCars = g_trafficMaxCars.GetInt();

	m_trafficUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;
	m_copSpawnIntervalCounter = 0;
	m_enableTrafficCars = true;
	m_enableCops = true;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;

	for (int i = 0; i < PURSUER_TYPE_COUNT; i++)
		m_copCarName[i] = "";

	m_copLoudhailerTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY + 5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}

void CAIManager::Shutdown()
{
	m_civCarEntries.clear();

	m_trafficCars.clear();
	m_speechQueue.clear();

	for (int i = 0; i < m_roadBlocks.numElem(); i++)
		delete m_roadBlocks[i];

	m_roadBlocks.clear();
}

CCar* CAIManager::SpawnTrafficCar(const IVector2D& globalCell)
{
	if (m_trafficCars.numElem() >= GetMaxTrafficCars())
		return NULL;

	CLevelRegion* pReg = NULL;
	levroadcell_t* roadCell = g_pGameWorld->m_level.Road_GetGlobalTileAt(globalCell, &pReg);

	// no tile - no spawn
	if (!pReg || !roadCell)
		return NULL;

	if (!pReg->m_isLoaded)
		return NULL;

	bool isParkingLot = (roadCell->type == ROADTYPE_PARKINGLOT);

	// parking lots are non-straight road cells
	if (!(roadCell->type == ROADTYPE_STRAIGHT || isParkingLot))
		return NULL;

	if(!isParkingLot)
	{
		straight_t str = g_pGameWorld->m_level.Road_GetStraightAtPoint(globalCell, 2);

		if (str.breakIter <= 1)
			return NULL;
	}
	
	// don't spawn if distance between cars is too short
	for (int j = 0; j < m_trafficCars.numElem(); j++)
	{
		if (!m_trafficCars[j]->GetPhysicsBody())
			continue;

		IVector2D trafficPosGlobal;
		if (!g_pGameWorld->m_level.GetTileGlobal(m_trafficCars[j]->GetOrigin(), trafficPosGlobal))
			continue;

		if (distance(trafficPosGlobal, globalCell) < AI_TRAFFIC_SPAWN_DISTANCE_THRESH*AI_TRAFFIC_SPAWN_DISTANCE_THRESH)
			return NULL;
	}

	Vector3D newSpawnPos = g_pGameWorld->m_level.GlobalTilePointToPosition(globalCell);

	float velocity = length(m_leadVelocity.xz());

	if(velocity > 3.0f)
	{
		// if velocity is negative to new spawn origin, cancel spawning
		if( dot(newSpawnPos.xz()-m_leadPosition.xz(), m_leadVelocity.xz()) < 0 )
			return NULL;
	}

	// if this is a parking straight, the cars might start here stopped or even empty
	bool isParkingStraight = (roadCell->flags & ROAD_FLAG_PARKING) > 0;

	if(isParkingLot)
	{
		CCar* newCar = NULL;

		int randCar = g_replayRandom.Get(0, m_civCarEntries.numElem() - 1);

		vehicleConfig_t* conf = m_civCarEntries[randCar].config;

		// don't spawn if not allowed
		if(conf->flags.allowParked == false)
			return NULL;

		m_civCarEntries[randCar].nextSpawn--;

		if(m_civCarEntries[randCar].nextSpawn <= 0)
		{
			newCar = new CCar(conf);
		
			// car will be spawn, regenerate random
			g_replayRandom.Regenerate();
			g_replayData->PushEvent( REPLAY_EVENT_FORCE_RANDOM );

			newCar->Spawn();
			newCar->Enable(false);
			newCar->PlaceOnRoadCell(pReg, roadCell);

			if( conf->numColors )
			{
				int col_idx = g_replayRandom.Get(0, conf->numColors - 1);
				newCar->SetColorScheme(col_idx);

				// set car color
				//g_replayData->PushEvent(REPLAY_EVENT_CAR_SETCOLOR, newCar->m_replayID, (void*)(intptr_t)col_idx);
			}

			g_pGameWorld->AddObject(newCar);

			m_trafficCars.append(newCar);

			m_civCarEntries[randCar].nextSpawn = m_civCarEntries[randCar].GetZoneSpawnInterval("default");

			return newCar;
		}

		return nullptr;
	}

	CAITrafficCar* pNewCar = NULL;

	if(m_enableCops)
		m_copSpawnIntervalCounter++;

	int numCopsSpawned = 0;

	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CAIPursuerCar* pursuer = UTIL_CastToPursuer(m_trafficCars[i]);

		// cops from roadblocks are not counted here!
		if(pursuer && !pursuer->m_assignedRoadblock)
			numCopsSpawned++;
	}

	// try to spawn a cop car, otherwise we spawn simple traffic car
	if (m_copSpawnIntervalCounter >= m_copRespawnInterval && m_enableCops)
	{
		// reset interval if it's possible to spawn
		m_copSpawnIntervalCounter = 0;

		if (numCopsSpawned >= GetMaxCops())
			return nullptr;

		//EPursuerAIType pursuerType = (EPursuerAIType)(g_replayRandom.Get(0, PURSUER_TYPE_COUNT - 1));

		vehicleConfig_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[PURSUER_TYPE_COP].c_str());

		if (!conf)
			return NULL;

		// initialize new car
		CAIPursuerCar* pCopCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);
		pCopCar->SetTorqueScale(m_copAccelerationModifier);
		pCopCar->SetMaxDamage(m_copMaxDamage);
		pCopCar->SetMaxSpeed(m_copMaxSpeed);
		
		pNewCar = pCopCar;

		numCopsSpawned++;
	}
	else
	{
		int randCar = g_replayRandom.Get(0, m_civCarEntries.numElem() - 1);

		vehicleConfig_t* conf = m_civCarEntries[randCar].config;

		// don't spawn cop usual way
		if(conf->flags.isCop)
			return NULL;

		m_civCarEntries[randCar].nextSpawn--;

		if(m_civCarEntries[randCar].nextSpawn <= 0)
		{
			pNewCar = new CAITrafficCar(m_civCarEntries[randCar].config);
			m_civCarEntries[randCar].nextSpawn = m_civCarEntries[randCar].GetZoneSpawnInterval("default");

			// car will be spawn, regenerate random
			g_replayRandom.Regenerate();
			g_replayData->PushEvent(REPLAY_EVENT_FORCE_RANDOM);
		}
	}

	if(!pNewCar)
		return NULL;

	pNewCar->Spawn();
	pNewCar->PlaceOnRoadCell(pReg, roadCell);

	vehicleConfig_t* conf = pNewCar->m_conf;

	if( conf->numColors )
	{
		int col_idx = g_replayRandom.Get(0, conf->numColors - 1);
		pNewCar->SetColorScheme(col_idx);

		// set car color
		//g_replayData->PushEvent(REPLAY_EVENT_CAR_SETCOLOR, pNewCar->m_replayID, (void*)(intptr_t)col_idx);
	}

	pNewCar->InitAI( false ); // TODO: chance of stoped, empty and active car on parking lane

	g_pGameWorld->AddObject(pNewCar);

	m_trafficCars.append(pNewCar);

	return pNewCar;
}

void CAIManager::QueryTrafficCars(DkList<CCar*>& list, float radius, const Vector3D& position, const Vector3D& direction, float queryCosAngle)
{
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CCar* car = m_trafficCars[i];

		Vector3D dirVec = car->GetOrigin()-position;

		float dotDist = dot(dirVec, dirVec);

		if (dotDist > radius*radius)
			continue;

		float cosAngle = dot(fastNormalize(dirVec), direction);

		// is visible to cone?
		if (cosAngle > queryCosAngle)
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

void CAIManager::RemoveTrafficCar(CCar* car)
{
	if( m_trafficCars.fastRemove(car) )
		g_pGameWorld->RemoveObject(car);
}

void CAIManager::UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{
	if (g_replayData->m_state == REPL_PLAYING)
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
		if (pursuerCheck && pursuerCheck->InPursuit())
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

void CAIManager::InitialSpawnCars(const Vector3D& spawnOrigin)
{
	if (g_replayData->m_state == REPL_PLAYING)
		return;

	IVector2D spawnCenterCell;
	if (!g_pGameWorld->m_level.GetTileGlobal(spawnOrigin, spawnCenterCell))
		return;

	int count = 0;

	//for (int i = 0; i < g_trafficMaxCars.GetInt(); i++)
	{
		for (int r = g_traffic_initial_mindist.GetInt(); r < g_traffic_mindist.GetInt(); r+=2)
		{
			count += CircularSpawnTrafficCars(spawnCenterCell.x, spawnCenterCell.y, r);
		}
	}

	MsgWarning("InitialSpawnCars: spawned %d cars\n", count);
}

void CAIManager::UpdateNavigationVelocityMap(float fDt)
{
	
	m_velocityMapUpdateTime += fDt;

	if(m_velocityMapUpdateTime > 0.5f)
		m_velocityMapUpdateTime = 0.0f;
	else
		return;

	// clear navgrid dynamic obstacle
	g_pGameWorld->m_level.Nav_ClearDynamicObstacleMap();
	/*
	// draw all car velocities on dynamic obstacle
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		PaintVelocityMapFrom(m_trafficCars[i]);
	}*/

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

void CAIManager::UpdatePedestrainRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{

}

void CAIManager::RemoveAllPedestrians()
{

}

void CAIManager::SpawnPedestrian(const IVector2D& globalCell)
{

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
	//debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "cops spawned: %d (max %d) (cntr=%d, lvl=%d)\n", m_copCars.numElem(), GetMaxCops(), m_copSpawnIntervalCounter, m_copRespawnInterval);
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
		RemoveTrafficCar(m_trafficCars[i]);

	m_trafficCars.clear();
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
	if (g_replayData->m_state == REPL_PLAYING)
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

	vehicleConfig_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[PURSUER_TYPE_COP].c_str());

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
	IsCopsEnabled,
	GetCopMaxDamage,
	GetCopAccelerationModifier,
	GetCopMaxSpeed,
	GetMaxCops,
	GetCopRespawnInterval,
	IsRoadBlockSpawn
)
