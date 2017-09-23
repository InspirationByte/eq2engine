//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: AI car manager for Driver Syndicate
//////////////////////////////////////////////////////////////////////////////////

#include "AICarManager.h"
#include "heightfield.h"
#include "session_stuff.h"

//------------------------------------------------------------------------------------------

ConVar g_trafficMaxCars("g_trafficMaxCars", "48", "Maximum traffic cars", CV_ARCHIVE);
ConVar g_traffic_mindist("g_traffic_mindist", "50", "Min traffic car distance to spawn", CV_CHEAT);
ConVar g_traffic_maxdist("g_traffic_maxdist", "51", "Max traffic car distance, to disappear", CV_CHEAT);

const float AI_COP_SPEECH_DELAY = 3.0f;		// delay before next speech
const float AI_COP_TAUNT_DELAY = 11.0f;		// delay before next taunt

#define COP_DEFAULT_DAMAGE			(4.0f)

#define TRAFFIC_BETWEEN_DISTANCE	5

//------------------------------------------------------------------------------------------

static CAICarManager s_AIManager;
CAICarManager* g_pAIManager = &s_AIManager;

DECLARE_CMD(aezakmi, "", CV_CHEAT)
{
	if(!g_pGameSession)
		return;

	g_pGameSession->GetPlayerCar()->SetFelony(0.0f);

	for(int i = 0; i < g_pAIManager->m_copCars.numElem(); i++)
	{
		g_pAIManager->m_copCars[i]->EndPursuit(false);
	}
}

CAICarManager::CAICarManager()
{
	m_trafficUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;

	m_enableCops = true;
	m_enableTrafficCars = true;

	m_copMaxDamage = COP_DEFAULT_DAMAGE;
	m_copAccelerationModifier = 1.0f;
	m_copMaxSpeed = 160.0f;

	m_numMaxCops = 2;
	m_copRespawnInterval = 20;	// spawn on every 20th traffic car
	m_numMaxTrafficCars = 32;
	
	m_copSpawnIntervalCounter = 0;

	m_leadVelocity = vec3_zero;
	m_leadPosition = vec3_zero;
}

CAICarManager::~CAICarManager()
{

}

void CAICarManager::Init()
{
	PrecacheObject(CAIPursuerCar);

	// reset variables
	m_copMaxDamage = COP_DEFAULT_DAMAGE;
	m_copAccelerationModifier = 1.0f;
	m_numMaxTrafficCars = g_trafficMaxCars.GetInt();

	m_trafficUpdateTime = 0.0f;
	m_velocityMapUpdateTime = 0.0f;
	m_copSpawnIntervalCounter = 0;
	m_enableTrafficCars = true;
	m_enableCops = true;

	for (int i = 0; i < COP_NUMTYPES; i++)
		m_copCarName[i] = "";

	m_copTauntTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY + 5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}

void CAICarManager::Shutdown()
{
	m_civCarEntries.clear();

	m_trafficCars.clear();
	m_copCars.clear();
	m_roadBlockCars.clear();
	m_speechQueue.clear();
}

#pragma fixme("Replay solution for cop road blocks")

CCar* CAICarManager::SpawnTrafficCar(const IVector2D& globalCell)
{
	if (m_trafficCars.numElem() >= GetMaxTrafficCars())
		return NULL;

	CLevelRegion* pReg = NULL;
	levroadcell_t* roadCell = g_pGameWorld->m_level.GetGlobalRoadTileAt(globalCell, &pReg);

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
		straight_t str = g_pGameWorld->m_level.GetStraightAtPoint(globalCell, 2);

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

		if (distance(trafficPosGlobal, globalCell) < TRAFFIC_BETWEEN_DISTANCE*TRAFFIC_BETWEEN_DISTANCE)
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
				g_replayData->PushEvent(REPLAY_EVENT_CAR_SETCOLOR, newCar->m_replayID, (void*)(intptr_t)col_idx);
			}

			g_pGameWorld->AddObject(newCar, true);

			m_trafficCars.append(newCar);

			m_civCarEntries[randCar].nextSpawn = m_civCarEntries[randCar].GetZoneSpawnInterval("default");

			return newCar;
		}

		return NULL;
	}

	CAITrafficCar* pNewCar = NULL;

	m_copSpawnIntervalCounter++;

	if (m_copSpawnIntervalCounter >= m_copRespawnInterval && m_enableCops)
	{
		// reset interval if it's possible to spawn
		m_copSpawnIntervalCounter = 0;

		if (m_copCars.numElem() >= GetMaxCops())
			return NULL;

		vehicleConfig_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[COP_LIGHT].c_str());

		if (!conf)
			return NULL;

		CAIPursuerCar* pCopCar = new CAIPursuerCar(conf, PURSUER_TYPE_COP);
		pCopCar->SetTorqueScale(m_copAccelerationModifier);
		pCopCar->SetMaxDamage(m_copMaxDamage);
		pCopCar->SetMaxSpeed(m_copMaxSpeed);
		
		pNewCar = pCopCar;

		m_copCars.append(pCopCar);
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
		g_replayData->PushEvent(REPLAY_EVENT_CAR_SETCOLOR, pNewCar->m_replayID, (void*)(intptr_t)col_idx);
	}

	pNewCar->InitAI( false ); // TODO: chance of stoped, empty and active car on parking lane

	g_pGameWorld->AddObject(pNewCar, true);

	m_trafficCars.append(pNewCar);

	return pNewCar;
}

void CAICarManager::CircularSpawnTrafficCars(int x0, int y0, int radius)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	
	SpawnTrafficCar(IVector2D(x0, y0 + radius));
	SpawnTrafficCar(IVector2D(x0, y0 - radius));
	SpawnTrafficCar(IVector2D(x0 + radius, y0));
	SpawnTrafficCar(IVector2D(x0 - radius, y0));
	
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

		SpawnTrafficCar(IVector2D(x0 + x, y0 + y));
		SpawnTrafficCar(IVector2D(x0 - x, y0 + y));
		SpawnTrafficCar(IVector2D(x0 + x, y0 - y));
		SpawnTrafficCar(IVector2D(x0 - x, y0 - y));
		SpawnTrafficCar(IVector2D(x0 + y, y0 + x));
		SpawnTrafficCar(IVector2D(x0 - y, y0 + x));
		SpawnTrafficCar(IVector2D(x0 + y, y0 - x));
		SpawnTrafficCar(IVector2D(x0 - y, y0 - x));
	}
}

void CAICarManager::RemoveTrafficCar(CCar* car)
{
	if(car->ObjType() == GO_CAR_AI)
	{
		if(((CAITrafficCar*)car)->IsPursuer())
		{
			m_copCars.remove((CAIPursuerCar*)car);
			m_roadBlockCars.remove(car);
		}
	}

	g_pGameWorld->RemoveObject(car);
}

void CAICarManager::UpdateCarRespawn(float fDt, const Vector3D& spawnOrigin, const Vector3D& removeOrigin, const Vector3D& leadVelocity)
{
	if (g_replayData->m_state == REPL_PLAYING)
		return;

	m_trafficUpdateTime -= fDt;

	if (m_trafficUpdateTime <= 0.0f)
		m_trafficUpdateTime = 0.5f;
	else
		return;

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

	// Try to remove cars
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		CCar* car = m_trafficCars[i];

		Vector3D carPos = car->GetOrigin();

		CLevelRegion* reg = g_pGameWorld->m_level.GetRegionAtPosition(carPos);

		if (!reg || (reg && !reg->m_isLoaded))
		{
			RemoveTrafficCar(car);
			m_trafficCars.fastRemoveIndex(i);
			i--;
			continue;
		}

		if(car->ObjType() == GO_CAR_AI)
		{
			// non-pursuer vehicles are removed by distance.
			// pursuers are not, if in pursuit only.
			if( ((CAIPursuerCar*)car)->IsPursuer() )
			{
				CAIPursuerCar* pursuer = (CAIPursuerCar*)car;

				if(pursuer->InPursuit())
					continue;
			}
		}

		IVector2D trafficCell;
		reg->m_heightfield[0]->PointAtPos(carPos, trafficCell.x, trafficCell.y);

		g_pGameWorld->m_level.LocalToGlobalPoint(trafficCell, reg, trafficCell);

		int distToCell = length(trafficCell - removeCenterCell);
		int distToCell2 = length(trafficCell - spawnCenterCell);

		if (distToCell > g_traffic_maxdist.GetInt() && 
			distToCell2 > g_traffic_maxdist.GetInt())
		{
			RemoveTrafficCar(car);
			m_trafficCars.fastRemoveIndex(i);
			i--;
			continue;
		}
	}

	CircularSpawnTrafficCars(spawnCenterCell.x, spawnCenterCell.y, g_traffic_mindist.GetInt());
}

void CAICarManager::UpdateNavigationVelocityMap(float fDt)
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

	// update debugging of navigation maps here
	g_pGameWorld->m_level.UpdateDebugMaps();
	*/
}

void CAICarManager::PaintVelocityMapFrom(CCar* car)
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

void CAICarManager::PaintNavigationLine(const IVector2D& start, const IVector2D& end)
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

//-----------------------------------------------------------------------------------------

void CAICarManager::UpdateCopStuff(float fDt)
{
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "cops spawned: %d (max %d) (cntr=%d, lvl=%d)\n", m_copCars.numElem(), GetMaxCops(), m_copSpawnIntervalCounter, m_copRespawnInterval);
	debugoverlay->Text(ColorRGBA(1, 1, 0, 1), "num traffic cars: %d\n", m_trafficCars.numElem());

	debugoverlay->Text(ColorRGBA(1, 1, 1, 1), "cop speech time: normal %g taunt %g\n", m_copSpeechTime, m_copTauntTime);

	m_copSpeechTime -= fDt;
	m_copTauntTime -= fDt;

	ISoundPlayable* copVoiceChannel = soundsystem->GetStaticStreamChannel(CHAN_VOICE);

	// say something
	if (copVoiceChannel->GetState() == SOUND_STATE_STOPPED && m_speechQueue.numElem() > 0)
	{
		EmitSound_t ep;
		ep.name = m_speechQueue[0].c_str();

		ses->Emit2DSound(&ep);

		m_speechQueue.removeIndex(0);
	}
}

void CAICarManager::RemoveAllCars()
{
	// Try to remove cars
	for (int i = 0; i < m_trafficCars.numElem(); i++)
	{
		RemoveTrafficCar(m_trafficCars[i]);
	}

	m_trafficCars.clear();
}

civCarEntry_t* CAICarManager::FindCivCarEntry( const char* name )
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

bool CAICarManager::SpawnRoadBlockFor( CCar* car, float directionAngle )
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

	levroadcell_t* startCellPlacement = g_pGameWorld->m_level.GetGlobalRoadTileAt(placementVec);

	if(!startCellPlacement || (startCellPlacement && !(startCellPlacement->type == ERoadType::ROADTYPE_STRAIGHT || startCellPlacement->type == ERoadType::ROADTYPE_PARKINGLOT)))
		return false;

	IVector2D perpendicular = GetDirectionVec(startCellPlacement->direction-1);

	int curLane = g_pGameWorld->m_level.GetLaneIndexAtPoint(placementVec, 16)-1;

	placementVec -= perpendicular*curLane;

	int numLanes = g_pGameWorld->m_level.GetRoadWidthInLanesAtPoint(placementVec, 32, 1);

	int nCars = 0;

	vehicleConfig_t* conf = g_pGameSession->FindCarEntryByName(m_copCarName[COP_LIGHT].c_str());

	if (!conf)
		return 0;

	Vector3D startPos = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec);
	Vector3D endPos = g_pGameWorld->m_level.GlobalTilePointToPosition(placementVec+perpendicular*numLanes);

	debugoverlay->Line3D(startPos, endPos, ColorRGBA(1,0,0,1), ColorRGBA(0,1,0,1), 1000.0f);

	for(int i = 0; i < numLanes; i++)
	{
		IVector2D blockPoint = placementVec + carDir*IVector2D(i % 2)*3; // offset in 3 tiles while checker

		placementVec += perpendicular;

		CLevelRegion* pReg = NULL;
		levroadcell_t* roadCell = g_pGameWorld->m_level.GetGlobalRoadTileAt(blockPoint, &pReg);

		// no tile - no spawn
		if (!pReg || !roadCell)
		{
			MsgError( "Can't spawn because no road\n" );
			continue;
		}

		if (!pReg->m_isLoaded)
			continue;

		Vector3D gPos = g_pGameWorld->m_level.GlobalTilePointToPosition(blockPoint);

		debugoverlay->Box3D(gPos-0.5f, gPos+0.5f, ColorRGBA(1,1,0,0.5f), 1000.0f);
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

		g_pGameWorld->AddObject(copBlockCar, true);

		float angle = 120.0f - (i % 2)*60.0f;

		copBlockCar->SetAngles(Vector3D(0.0f, targetDir*-90.0f + angle, 0.0f));

		m_roadBlockCars.append( copBlockCar );
		m_copCars.append( copBlockCar );
		m_trafficCars.append( copBlockCar );
		nCars++;
	}

	return m_roadBlockCars.numElem() > 0;
}

void CAICarManager::MakePursued( CCar* car )
{
	for(int i = 0; i < m_copCars.numElem(); i++)
	{
		CAIPursuerCar* cop = m_copCars[i];

		if(!cop->InPursuit() && cop->IsAlive())
		{
			cop->SetPursuitTarget(car);
			cop->BeginPursuit(0.0f);
		}
	}
}

/*
void CAICarManager::StopPursuit( CCar* car )
{
	for(int i = 0; i < m_copCars.numElem(); i++)
	{
		if(!m_copCars[i]->InPursuit() && m_copCars[i]->GetPursuitTarget() == car)
		{
			m_copCars[i]->EndPursuit();
		}
	}
}
*/

bool CAICarManager::IsRoadBlockSpawn() const
{
	return m_roadBlockCars.numElem() > 0;
}

// ----- TRAFFIC ------
void CAICarManager::SetMaxTrafficCars(int count)
{
	m_numMaxTrafficCars = count;
}

int CAICarManager::GetMaxTrafficCars() const
{
	return m_enableTrafficCars ? m_numMaxTrafficCars : 0;
}

void CAICarManager::SetTrafficCarsEnabled(bool enable)
{
	m_enableTrafficCars = enable;
}

bool CAICarManager::IsTrafficCarsEnabled() const
{
	return m_enableTrafficCars;
}

// ----- COPS ------
// switch to spawn
void CAICarManager::SetCopsEnabled(bool enable)
{
	m_enableCops = enable;
}

bool CAICarManager::IsCopsEnabled() const
{
	return m_enableCops;
}

// maximum cop count
void CAICarManager::SetMaxCops(int count)
{
	m_numMaxCops = count;
}

int CAICarManager::GetMaxCops() const
{
	return m_numMaxCops;
}

// cop respawn interval between traffic car spawns
void CAICarManager::SetCopRespawnInterval(int steps)
{
	m_copRespawnInterval = steps;
}

int CAICarManager::GetCopRespawnInterval() const
{
	return m_copRespawnInterval;
}

// acceleration modifier (e.g. for survival)
void CAICarManager::SetCopAccelerationModifier(float accel)
{
	m_copAccelerationModifier = accel;
}

float CAICarManager::GetCopAccelerationModifier() const
{
	return m_copAccelerationModifier;
}

// sets the maximum hitpoints for cop cars
void CAICarManager::SetCopMaxDamage(float maxHitPoints)
{
	m_copMaxDamage = maxHitPoints;
}
					
float CAICarManager::GetCopMaxDamage() const
{
	return m_copMaxDamage;
}

// sets the maximum hitpoints for cop cars
void CAICarManager::SetCopMaxSpeed(float maxSpeed)
{
	m_copMaxSpeed = maxSpeed;
}
					
float CAICarManager::GetCopMaxSpeed() const
{
	return m_copMaxSpeed;
}

// sets cop car configuration
void CAICarManager::SetCopCarConfig(const char* car_name, int type )
{
	m_copCarName[type] = car_name;
}

// shedules a cop speech
bool CAICarManager::MakeCopSpeech(const char* soundScriptName, bool force)
{
	// shedule speech
	if (m_copSpeechTime < 0 || force)
	{
		m_speechQueue.append(soundScriptName);

		m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);

		return true;
	}

	return false;
}

bool CAICarManager::IsCopCanSayTaunt() const
{
	return (m_copSpeechTime < 0) && (m_copTauntTime < 0);
}

void CAICarManager::GotCopTaunt()
{
	m_copTauntTime = RandomFloat(AI_COP_TAUNT_DELAY, AI_COP_TAUNT_DELAY+5.0f);
	m_copSpeechTime = RandomFloat(AI_COP_SPEECH_DELAY, AI_COP_SPEECH_DELAY + 5.0f);
}


OOLUA_EXPORT_FUNCTIONS(
	CAICarManager,

	MakePursued,
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
	CAICarManager,

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
