//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Replay data
//////////////////////////////////////////////////////////////////////////////////

#include "replay.h"
#include "StateManager.h"
#include "session_stuff.h"
#include "IDebugOverlay.h"

#define CONTROL_DETAILS_STEP (0.0025f)

// sort events in right order
int _sortEventsFunc(const replayevent_t& a, const replayevent_t& b)
{
	return a.frameIndex - b.frameIndex;
}

CReplayData::CReplayData()
{
	m_state = REPL_NONE;
	m_filename = "";
	m_currentEvent = 0;
	m_numFrames = 0;
	m_currentCamera = 0;
}

CReplayData::~CReplayData() {}

int CReplayData::Record(CCar* pCar, bool onlyCollisions)
{
	if(!pCar)
		return -1;

	// try re-record
	for(int i = 0; i < m_vehicles.numElem(); i++)
	{
		if(m_vehicles[i].obj_car == pCar)
		{
			m_vehicles.fastRemoveIndex(i);
			break;
		}
	}

	vehiclereplay_t veh;

	veh.obj_car = pCar;
	veh.recordOnlyCollisions = onlyCollisions;

	veh.car_initial_pos = pCar->GetPhysicsBody()->GetPosition();
	veh.car_initial_rot = pCar->GetPhysicsBody()->GetOrientation();
	veh.car_initial_vel = pCar->GetPhysicsBody()->GetLinearVelocity();
	veh.car_initial_angvel = pCar->GetPhysicsBody()->GetAngularVelocity();

	veh.scriptObjectId = pCar->GetScriptID();

	veh.name = pCar->m_conf->carName.c_str();

	veh.curr_frame = 0;
	veh.check = false;
	veh.done = false;

	veh.skipFrames = 0;
	veh.skeptFrames = 0;

	return m_vehicles.append(veh);
}

void CReplayData::Clear()
{
	m_currentEvent = 0;
	m_numFrames = 0;

	m_events.clear();
	m_vehicles.clear();
	m_cameras.clear();
	m_activeVehicles.clear();
}


void CReplayData::StartRecording()
{
	m_tick = 0;
	m_currentEvent = 0;
	m_state = REPL_RECORDING;
}

void CReplayData::StartPlay()
{
	m_tick = 0;
	m_currentEvent = 0;
	m_currentCamera = 0;

	ResetEvents();

	m_activeVehicles.clear();

	RaiseTickEvents();

	m_state = REPL_PLAYING;
}

void CReplayData::Stop()
{
	if(m_state == REPL_RECORDING)
		PushEvent( REPLAY_EVENT_END );

	for(int i = 0; i < m_vehicles.numElem(); i++)
	{
		m_vehicles[i].done = true;

		if(m_state == REPL_PLAYING)
		{
			m_vehicles[i].obj_car = NULL;	// release
			m_vehicles[i].curr_frame = 0;
			m_vehicles[i].skeptFrames = 0;
			m_vehicles[i].skipFrames = 0;
		}
	}

	ResetEvents();

	m_currentEvent = 0;
	m_tick = 0;
	m_currentCamera = 0;
	m_state = REPL_NONE;
}

void CReplayData::UpdatePlayback( float fDt )
{
	if(m_state == REPL_NONE)
	{
		return;
	}
	else if(m_state == REPL_PLAYING || m_state == REPL_RECORDING)
	{
		RaiseTickEvents();

		if(m_state == REPL_PLAYING)
		{
			for(int i = m_currentCamera+1; i < m_cameras.numElem(); i++)
			{
				if(m_tick >= m_cameras[i].startTick)
				{
					Msg("Replay switches camera to %d\n", i);
					m_currentCamera = i;
					break;
				}
			}
		}

		m_tick++;
	}
}

replaycamera_t* CReplayData::GetCurrentCamera()
{
	if(m_cameras.numElem() == 0)
		return NULL;

	return &m_cameras[m_currentCamera];
}

int CReplayData::AddCamera(const replaycamera_t& camera)
{
	// do not add the camera, just set
	for(int i = 0; i < m_cameras.numElem(); i++)
	{
		if(m_cameras[i].startTick == camera.startTick)
		{
			m_cameras[i] = camera;
			return i;
		}
		else if(camera.startTick > m_cameras[i].startTick)
		{
			if(i+1 < m_cameras.numElem() &&
				camera.startTick < m_cameras[i+1].startTick)
			{
				return m_cameras.insert(camera, i+1);
			}
		}
	}

	return m_cameras.append(camera);
}

void CReplayData::RemoveCamera(int frameIndex)
{
	for(int i = 0; i < m_cameras.numElem(); i++)
	{
		if(frameIndex >= m_cameras[i].startTick)
		{
			m_cameras.removeIndex(i);
			return;
		}
	}
}

CCar* CReplayData::GetCarByReplayIndex(int index)
{
	if(index >= 0 && index < m_vehicles.numElem())
		return m_vehicles[index].obj_car;

	return NULL;
}


void CReplayData::UpdateReplayObject( int replayId )
{
	if(m_state == REPL_NONE)
		return;

	if(replayId == REPLAY_NOT_TRACKED)
		return;

	vehiclereplay_t* rep = &m_vehicles[ replayId ];

	if(m_state == REPL_RECORDING)
	{
		if(!RecordVehicleFrame( rep ))
			PlayVehicleFrame( rep ); // should play already recorded
	}
	else
		PlayVehicleFrame( rep );
}


// returns status the current car playing or not
bool CReplayData::IsCarPlaying( CCar* pCar )
{
	if(!(m_state == REPL_PLAYING || m_state == REPL_RECORDING))
		return false;

	if(pCar->m_replayID == REPLAY_NOT_TRACKED)
		return false;

	vehiclereplay_t& veh = m_vehicles[pCar->m_replayID];

	if(veh.obj_car == pCar)
	{
		if(!(veh.done || veh.curr_frame < veh.replayArray.numElem()-1))
			return false;

		int lastFrameIdx = veh.replayArray.numElem()-1;

		replaycontrol_t& frame = veh.replayArray[lastFrameIdx];

		// wait for our tick
		return (m_tick <= frame.tick);
	}

	return false;
}

ConVar replay_framecorrect("replay_framecorrect", "1", "Correct replay frames", CV_CHEAT);

void CReplayData::PlayVehicleFrame(vehiclereplay_t* rep)
{
	if(rep->obj_car == NULL)
		return;

	if(rep->obj_car->IsLocked())
		return;

	// if replay is complete or it's playback time, play this.
	if(!(rep->done || rep->curr_frame < rep->replayArray.numElem()-1))
		return;

	if(rep->curr_frame >= rep->replayArray.numElem())
		return; // done playing it

	replaycontrol_t& frame = rep->replayArray[rep->curr_frame];

	// wait for our tick
	if(m_tick < frame.tick)
		return;

	// advance frame
	rep->curr_frame++;
	rep->check = true;

	if(rep->check)
	{
		rep->check = false;

		// correct whole frame
		rep->obj_car->GetPhysicsBody()->SetPosition(frame.car_origin);
		rep->obj_car->GetPhysicsBody()->SetOrientation(Quaternion(frame.car_rot.w, frame.car_rot.x, frame.car_rot.y, frame.car_rot.z));
		rep->obj_car->GetPhysicsBody()->SetLinearVelocity(frame.car_vel);
		rep->obj_car->GetPhysicsBody()->SetAngularVelocity(frame.car_angvel);
	}

	// unpack
	short accelControl = (frame.control_vars & 0x3FF);
	short brakeControl = ((frame.control_vars >> 10) & 0x3FF);
	short steerControl = ((frame.control_vars >> 20) & 0x3FF);	// steering sign bit
	short steerSign = (frame.control_vars >> 30) ? -1 : 1;	// steering sign bit

	int control_flags = frame.button_flags;

	if(control_flags & IN_ANALOGSTEER)
		rep->obj_car->m_steerRatio = steerControl * steerSign;
	else
		rep->obj_car->m_steerRatio = 1023;

	rep->obj_car->m_accelRatio = accelControl;
	rep->obj_car->m_brakeRatio = brakeControl;
	
	rep->obj_car->SetControlButtons(control_flags);

	if(m_state == REPL_RECORDING)		// overwrite the vehicle damage (in case of mission that has pre-recorded vehicle frames)
		frame.car_damage = rep->obj_car->GetDamage();
	else if(m_state == REPL_PLAYING)	// replay the damage
		rep->obj_car->SetDamage(frame.car_damage);
}

// records vehicle frame
bool CReplayData::RecordVehicleFrame(vehiclereplay_t* rep)
{
	int nFrame = rep->replayArray.numElem();

	// if replay is done or current frames are loaded and must be played (or rewinded)
	if( rep->done || rep->curr_frame < rep->replayArray.numElem() )
		return false; // done or has future frames

	// position must be set
	if(nFrame == 0)
	{
		Quaternion orient = rep->obj_car->GetPhysicsBody()->GetOrientation();

		rep->car_initial_pos = rep->obj_car->GetPhysicsBody()->GetPosition();
		rep->car_initial_rot = orient;
		rep->car_initial_vel = rep->obj_car->GetPhysicsBody()->GetLinearVelocity();
		rep->car_initial_angvel = rep->obj_car->GetPhysicsBody()->GetAngularVelocity();
	}

	bool addControls = false;
	float prevTime = 0.0f;

	short accelControl = rep->obj_car->m_accelRatio;
	short brakeControl = rep->obj_car->m_brakeRatio;
	short steerControl = rep->obj_car->m_steerRatio;

	uint control_flags = rep->obj_car->GetControlButtons();
	control_flags &= ~IN_MISC; // kill misc buttons, left only needed

	if(nFrame == 0)
	{
		addControls = true;
	}
	else // add only when old buttons has changed
	{
		replaycontrol_t& prevControl = rep->replayArray[nFrame-1];

		// unpack
		short prevAccelControl = (prevControl.control_vars & 0x3FF);
		short prevbrakeControl = ((prevControl.control_vars >> 10) & 0x3FF);
		short prevsteerControl = ((prevControl.control_vars >> 20) & 0x3FF);

		short prevSteerSign = (prevControl.control_vars >> 30) ? -1 : 1;	// steering sign bit

		short prevSteerRatio = 1023;

		if( control_flags & IN_ANALOGSTEER )
			prevSteerRatio = prevsteerControl * prevSteerSign;

		addControls = (prevControl.button_flags != control_flags) || (accelControl != prevAccelControl) || (brakeControl != prevbrakeControl) || (steerControl != prevsteerControl);
	}

	if (rep->skeptFrames < rep->skipFrames)
	{
		rep->skeptFrames++;
		return true;
	}
	else
		rep->skeptFrames = 0;

	if (rep->recordOnlyCollisions)
		addControls = false;

	// TODO: add replay frame if car has collision with objects
	if( addControls || rep->obj_car->GetPhysicsBody()->m_collisionList.numElem() > 0 )
	{
		replaycontrol_t con;
		con.button_flags = control_flags;

		Quaternion orient = rep->obj_car->GetPhysicsBody()->GetOrientation();

		con.car_origin = rep->obj_car->GetPhysicsBody()->GetPosition();
		con.car_rot = TVec4D<half>(orient.x, orient.y, orient.z, orient.w);
		con.car_vel = rep->obj_car->GetPhysicsBody()->GetLinearVelocity();
		con.car_angvel = rep->obj_car->GetPhysicsBody()->GetAngularVelocity();
		con.car_damage = rep->obj_car->GetDamage();

		int steerBit = 0;

		if(steerControl < 0)
			steerBit = 1;

		if( !(control_flags & IN_ANALOGSTEER) )
		{
			steerControl = 1023;
			steerBit = 0;
		}

		con.control_vars = (steerBit << 30) | (abs(steerControl) << 20) | (brakeControl << 10) | accelControl;

		con.tick = m_tick;

		rep->replayArray.append(con);
		rep->curr_frame++;
	}

	return true;
}

extern EqString g_scriptName;

void CReplayData::SaveToFile( const char* filename )
{
	m_filename = filename;

	if(m_filename.Path_Extract_Ext().Length() == 0)
		m_filename = m_filename + ".rdat";

	IFile* pFile = g_fileSystem->Open(m_filename.c_str(), "wb", SP_MOD);

	if(pFile)
	{
		replayhdr_t hdr;
		hdr.version = VEHICLEREPLAY_VERSION;
		strcpy(hdr.levelname, g_pGameWorld->GetLevelName());
		strcpy(hdr.envname, g_pGameWorld->GetEnvironmentName());
		strcpy(hdr.missionscript, g_scriptName.c_str());

		hdr.startRegX = -1;
		hdr.startRegY = -1;	// detect.

		pFile->Write(&hdr, 1, sizeof(replayhdr_t));

		int count = m_vehicles.numElem();
		pFile->Write(&count, 1, sizeof(int));
		
		// write vehicles
		for(int i = 0; i < m_vehicles.numElem(); i++)
			WriteVehicleAndFrames(&m_vehicles[i], pFile);

		// write events
		WriteEvents( pFile );

		g_fileSystem->Close(pFile);

		Msg("Replay '%s' saved\n", filename);
	}

	if(m_cameras.numElem() > 0)
	{
		EqString camsFilename = m_filename.Path_Strip_Ext() + ".rcam";
		IFile* pFile = g_fileSystem->Open(camsFilename.c_str(), "wb", SP_MOD);

		if(pFile)
		{
			replaycamerahdr_t camhdr;
			camhdr.version = CAMERAREPLAY_VERSION;
			camhdr.numCameras = m_cameras.numElem();

			pFile->Write(&camhdr, 1, sizeof(replaycamerahdr_t));

			for(int i = 0; i < m_cameras.numElem(); i++)
			{
				pFile->Write(&m_cameras[i], 1, sizeof(replaycamera_t));
			}

			g_fileSystem->Close(pFile);

			Msg("Replay cameras '%s' saved\n", filename);
		}
	}
}

void CReplayData::WriteVehicleAndFrames(vehiclereplay_t* rep, IVirtualStream* stream )
{
	vehiclereplay_file_t data(*rep);

	stream->Write(&data, 1, sizeof(vehiclereplay_file_t));

	for(int j = 0; j < rep->replayArray.numElem(); j++)
	{
		replaycontrol_t& control = rep->replayArray[j];
		stream->Write(&control, 1, sizeof(replaycontrol_t));
	}
}

void CReplayData::WriteEvents( IVirtualStream* stream, int onlyEvent )
{
	int numEvents = m_events.numElem();

	if(onlyEvent != -1)
	{
		int nEvents = 0;
		for(int i = 0; i < numEvents; i++)
		{
			// if set, do only specific events
			if(onlyEvent != -1)
			{
				if(m_events[i].eventType != onlyEvent)
					continue;
			}

			nEvents++;
		}

		numEvents = nEvents;
	}

	stream->Write(&numEvents, 1, sizeof(int));

	CMemoryStream eventData;

	// write events
	for(int i = 0; i < m_events.numElem(); i++)
	{
		replayevent_t& evt = m_events[i];

		eventData.Open(NULL, VS_OPEN_WRITE, 128);

		// if set, do only specific events
		if( onlyEvent != -1 )
		{
			if( evt.eventType != onlyEvent )
				continue;
		}

		replayevent_file_t fevent( evt );

		// make replay event data
		if( evt.eventData != NULL )
		{
			switch( evt.eventType )
			{
				// boolean event
				case REPLAY_EVENT_CAR_ENABLE:
				case REPLAY_EVENT_CAR_LOCK:
				{
					eventData.Write( evt.eventData, 1, fevent.eventDataSize );

					fevent.eventDataSize = eventData.Tell();
					
					break;
				}
			}
		}

		stream->Write(&fevent, 1, sizeof(replayevent_file_t));

		if( fevent.eventDataSize > 0 )
		{
			Msg("Write event data size=%d\n", fevent.eventDataSize);
			stream->Write(eventData.GetBasePointer(), 1, fevent.eventDataSize);
		}
	}
}

bool CReplayData::SaveVehicleReplay( CCar* target, const char* filename )
{
	vehiclereplay_t* rep = NULL;

	for(int i = 0; i < m_vehicles.numElem(); i++)
	{
		if(m_vehicles[i].obj_car == target)
			rep = &m_vehicles[i];
	}

	if(!rep)
	{
		Msg("Vehicle is not recorded");
		return false;
	}

	IFile* pFile = g_fileSystem->Open((_Es(filename) + ".vr").c_str(), "wb", SP_MOD);

	if(pFile)
	{
		WriteVehicleAndFrames(rep, pFile);

		g_fileSystem->Close(pFile);

		Msg("Car replay '%s' saved\n", filename);
		return true;
	}

	return false;
}

void CReplayData::ReadEvent( replayevent_t& evt, IVirtualStream* stream )
{
	replayevent_file_t fevent;
	stream->Read(&fevent, 1, sizeof(replayevent_file_t));

	evt.frameIndex = fevent.frameIndex;
	evt.replayIndex = fevent.replayIndex;
	evt.eventType = fevent.eventType;
	evt.eventFlags = fevent.eventFlags;

	if( fevent.eventDataSize > 0 )
	{
		switch( evt.eventType )
		{
			// boolean event
			case REPLAY_EVENT_CAR_ENABLE:
			case REPLAY_EVENT_CAR_LOCK:
			{
				bool enabled;
				stream->Read(&enabled, 1, sizeof(bool));

				evt.eventData = (void*)enabled;
				break;
			}
		}
	}
	else
	{
		evt.eventData = NULL;
	}
}

bool CReplayData::LoadVehicleReplay( CCar* target, const char* filename, int& tickCount )
{
	if(	m_state == REPL_PLAYING ||
		m_state == REPL_INIT_PLAYBACK)
	{
		tickCount = 0;
		return true;
	}

	Msg("Loading vehicle frames from '%s'\n", filename);

	IFile* pFile = g_fileSystem->Open((_Es(filename) + ".vr").c_str(), "rb", SP_MOD);

	if(pFile)
	{
		// read replay file header
		vehiclereplay_file_t data;
		pFile->Read(&data, 1, sizeof(vehiclereplay_file_t));

		// try to inject frames
		int repIdx = target->m_replayID;

		bool wereTracked = true;

		// make vehicle tracking if not
		if(repIdx == REPLAY_NOT_TRACKED)
		{
			vehiclereplay_t newRep;
			repIdx = m_vehicles.append(newRep);
			wereTracked = false;
		}

		vehiclereplay_t& veh = m_vehicles[repIdx];

		if(!wereTracked)
		{
			// don't overwrite some data
			veh.obj_car = target;
			veh.name = data.name;

			veh.car_initial_pos = data.car_initial_pos;
			veh.car_initial_rot = Quaternion(data.car_initial_rot.w, data.car_initial_rot.x, data.car_initial_rot.y, data.car_initial_rot.z);
			veh.car_initial_vel = data.car_initial_vel;
			veh.car_initial_angvel = data.car_initial_angvel;

			// set replay id
			veh.obj_car->m_replayID = repIdx;
			veh.curr_frame = 0;

			veh.scriptObjectId = data.scriptObjectId;
		}
		else
			veh.curr_frame = veh.replayArray.numElem();

		veh.check = true;
		//veh.done = true;

		// read frames of vehicle controls
		for(int j = 0; j < data.numFrames; j++)
		{
			replaycontrol_t control;
			pFile->Read(&control, 1, sizeof(replaycontrol_t));

			tickCount = control.tick;

			// add offset to the ticks
			control.tick += m_tick;

			veh.replayArray.append(control);
		}

		// read events if we have some
		int numEvents = 0;
		pFile->Read(&numEvents, 1, sizeof(int));

		for(int i = 0; i < numEvents; i++)
		{
			replayevent_t evt;
			ReadEvent(evt, pFile);

			evt.frameIndex += m_tick;

			m_events.append( evt );
		}

		// events must be sorted again
		m_events.quickSort( _sortEventsFunc, 0, m_events.numElem()-1 );

		// make this vehicle active if not
		m_activeVehicles.addUnique( repIdx );

		g_fileSystem->Close(pFile);

		return true;
	}

	MsgWarning("Cannot load vehicle replay '%s'\n", filename);

	return false;
}

void CReplayData::LoadFromFile(const char* filename)
{
	m_filename = filename;

	if(m_filename.Path_Extract_Ext().Length() == 0)
		m_filename = m_filename + ".rdat";

	IFile* pFile = g_fileSystem->Open(m_filename.c_str(), "rb", SP_MOD);

	if(pFile)
	{
		replayhdr_t hdr;
		pFile->Read(&hdr, 1, sizeof(replayhdr_t));

		if(hdr.version != VEHICLEREPLAY_VERSION)
		{
			MsgError("Invalid replay file version!\n");
			g_fileSystem->Close(pFile);
			return;
		}

		MsgInfo("Replay info:\n");

		MsgInfo("	level: '%s'\n", hdr.levelname);
		MsgInfo("	env: '%s'\n", hdr.envname);
		MsgInfo("	mission: '%s'\n", hdr.missionscript);

		Clear();

		int veh_count = 0;
		pFile->Read(&veh_count, 1, sizeof(int));

		// read vehicle names
		for(int i = 0; i < veh_count; i++)
		{
			vehiclereplay_file_t data;

			pFile->Read(&data, 1, sizeof(vehiclereplay_file_t));

			vehiclereplay_t veh;

			// event will spawn this car
			veh.obj_car = NULL;

			veh.car_initial_pos = data.car_initial_pos;
			veh.car_initial_rot = Quaternion(data.car_initial_rot.w, data.car_initial_rot.x, data.car_initial_rot.y, data.car_initial_rot.z);
			veh.car_initial_vel = data.car_initial_vel;
			veh.car_initial_angvel = data.car_initial_angvel;
			veh.scriptObjectId = data.scriptObjectId;

			veh.name = data.name;

			veh.curr_frame = 0;
			veh.check = false;
			veh.done = true;

			for(int j = 0; j < data.numFrames; j++)
			{
				replaycontrol_t control;
				pFile->Read(&control, 1, sizeof(replaycontrol_t));

				veh.replayArray.append(control);
			}

			m_vehicles.append(veh);
		}

		// read events
		int numEvents = 0;
		pFile->Read(&numEvents, 1, sizeof(int));

		for(int i = 0; i < numEvents; i++)
		{
			replayevent_t evt;
			ReadEvent(evt, pFile);

			// set frame count
			if(evt.eventType == REPLAY_EVENT_END)
				m_numFrames = evt.frameIndex;

			m_events.append( evt );
		}

		Msg("Replay %s loaded\n", filename);

		g_fileSystem->Close(pFile);

		// try loading camera file
		EqString camsFilename = m_filename.Path_Strip_Ext() + ".rcam";
		pFile = g_fileSystem->Open(camsFilename.c_str(), "rb", SP_MOD);
		if(pFile)
		{
			replaycamerahdr_t camhdr;
			pFile->Read( &camhdr,1,sizeof(replaycamerahdr_t) );

			if(camhdr.version == CAMERAREPLAY_VERSION)
			{
				m_currentCamera = 0;

				replaycamera_t cam;

				for(int i = 0; i < camhdr.numCameras; i++)
				{
					pFile->Read( &cam,1,sizeof(replaycamera_t) );

					m_cameras.append(cam);
				}
			}
			else
			{
				MsgError("Unsupported camera replay file version!\n");
			}

			g_fileSystem->Close(pFile);
		}

		// load mission, level, set environment
		m_state = REPL_INIT_PLAYBACK;

		if(!LoadMissionScript( hdr.missionscript ))
		{
			g_pGameWorld->SetLevelName(hdr.levelname);
		}

		g_pGameWorld->SetEnvironmentName(hdr.envname);
	}
}

void CReplayData::PushSpawnOrRemoveEvent( EReplayEventType type, CGameObject* object)
{
	if( m_state != REPL_RECORDING )
		return;

	replayevent_t evt;
	evt.eventType = type;
	evt.frameIndex = m_tick;
	evt.replayIndex = REPLAY_NOT_TRACKED;
	evt.eventData = NULL;
	evt.eventFlags = REPLAY_FLAG_IS_PUSHED;

	// assign replay index
	if( type == REPLAY_EVENT_SPAWN )
	{
		if(object->ObjType() == GO_CAR)
		{
			evt.replayIndex = Record( (CCar*)object );
			object->m_replayID = evt.replayIndex;
		}
		else if (object->ObjType() == GO_CAR_AI)
		{
			CAITrafficCar* pCar = (CAITrafficCar*)object;

			evt.replayIndex = Record(pCar, false);
			m_vehicles[evt.replayIndex].skipFrames = 64;	// skip frames
			m_vehicles[evt.replayIndex].skeptFrames = 0;

			if ( pCar->IsPursuer() )
				evt.eventFlags |= REPLAY_FLAG_CAR_COP_AI;
			else
				evt.eventFlags |= REPLAY_FLAG_CAR_AI;

			object->m_replayID = evt.replayIndex;
		}

		m_activeVehicles.append( evt.replayIndex );
	}
	else if( type == REPLAY_EVENT_REMOVE )
	{
		if ( IsCar(object) )
		{
			// find recording car
			evt.replayIndex = ((CCar*)object)->m_replayID;

			if(evt.replayIndex != REPLAY_NOT_TRACKED)
			{
				// make replay done, and invalidate realtime data
				m_vehicles[evt.replayIndex].done = true;
				m_vehicles[evt.replayIndex].obj_car = NULL;
			}

			// no additional information needed
			m_activeVehicles.fastRemove( evt.replayIndex );
		}
	}

	m_events.append( evt );
}

void CReplayData::PushEvent(EReplayEventType type, int replayId, void* eventData)
{
	if( m_state != REPL_RECORDING )
		return;

	replayevent_t evt;
	evt.eventType = type;
	evt.frameIndex = m_tick;
	evt.replayIndex = replayId;
	evt.eventData = eventData;
	evt.eventFlags = REPLAY_FLAG_IS_PUSHED;

	m_events.append( evt );
}

void CReplayData::ResetEvents()
{
	for(int i = 0; i < m_events.numElem(); i++)
		m_events[i].eventFlags &= ~REPLAY_FLAG_IS_PUSHED;
}

void CReplayData::RaiseTickEvents()
{
	for(int i = m_currentEvent; i < m_events.numElem(); i++)
	{
		replayevent_t& evt = m_events[i];

		if(evt.frameIndex != m_tick)
			break;

		RaiseReplayEvent( evt );

		m_currentEvent++;
	}
}

void CReplayData::RaiseReplayEvent(const replayevent_t& evt)
{
	if(evt.eventFlags & REPLAY_FLAG_IS_PUSHED)
		return;

	switch(evt.eventType)
	{
		// assign replay index
		case REPLAY_EVENT_SPAWN:
		{
			// spawn car
			if(evt.replayIndex == REPLAY_NOT_TRACKED)
				break;

			vehiclereplay_t& rep = m_vehicles[evt.replayIndex];

			m_activeVehicles.append( evt.replayIndex );

			// spawn replay car if not scripted
			if( rep.scriptObjectId == SCRIPT_ID_NOTSCRIPTED )
			{
				int type = CAR_TYPE_NORMAL;

				if (evt.eventFlags == REPLAY_FLAG_CAR_AI)
				{
					type = CAR_TYPE_TRAFFIC_AI;
				}
				else if (evt.eventFlags == REPLAY_FLAG_CAR_COP_AI)
				{
					// pursuer cars aren't random for now
					type = CAR_TYPE_PURSUER_COP_AI;
				}
				else if (evt.eventFlags == REPLAY_FLAG_CAR_GANG_AI)
				{
					type = CAR_TYPE_PURSUER_GANG_AI;
				}

				// create car and spawn
				rep.obj_car = g_pGameSession->CreateCar(rep.name.c_str(), type);

				// assign the replay index so it can play self
				rep.obj_car->m_replayID = evt.replayIndex;
			
				ASSERTMSG(rep.obj_car, varargs("ERROR! Cannot find car '%s'", rep.name.c_str()));
			
				// temp
				if(g_pGameSession->GetPlayerCar() == NULL)
				{
					g_pGameSession->SetPlayerCar(rep.obj_car);
					g_pGameWorld->m_level.QueryNearestRegions(rep.car_initial_pos, true);
				}

				rep.obj_car->Spawn();
				g_pGameWorld->AddObject( rep.obj_car );
			}
			else
			{
				CGameObject* obj = g_pGameSession->FindScriptObjectById( rep.scriptObjectId );

				if( obj && IsCar(obj) )
				{
					rep.obj_car = (CCar*)obj;
					rep.obj_car->m_replayID = evt.replayIndex;
				}
			}

			SetupReplayCar( &rep );

			break;
		}
		case REPLAY_EVENT_REMOVE:
		{
			// remove car
			if( evt.replayIndex == REPLAY_NOT_TRACKED )
				break;

			vehiclereplay_t& rep = m_vehicles[evt.replayIndex];

			m_activeVehicles.fastRemove( evt.replayIndex );

			if( rep.scriptObjectId == SCRIPT_ID_NOTSCRIPTED )
			{
				if(g_pGameSession->GetPlayerCar() == rep.obj_car)
					g_pGameSession->SetPlayerCar(NULL);

				g_pGameWorld->RemoveObject(rep.obj_car);
				rep.obj_car = NULL;
			}

			break;
		}
		case REPLAY_EVENT_END:
		{
			g_pGameSession->SignalMissionStatus(MIS_STATUS_SUCCESS, 0.0f);
			Stop();
			break;
		}
		case REPLAY_EVENT_FORCE_RANDOM:
		{
			g_pGameWorld->m_random.Regenerate();
			break;
		}
		case REPLAY_EVENT_CAR_RANDOMCOLOR:
		{
			// spawn car
			if(evt.replayIndex == REPLAY_NOT_TRACKED)
				break;

			vehiclereplay_t& rep = m_vehicles[evt.replayIndex];

			carConfigEntry_t* conf = rep.obj_car->m_conf;

			if( conf->m_colors.numElem() > 0 )
			{
				int col_idx = g_pGameWorld->m_random.Get(0, conf->m_colors.numElem() - 1);
				rep.obj_car->SetColorScheme(col_idx);
			}
			break;
		}
		case REPLAY_EVENT_CAR_ENABLE:
		{
			if( evt.replayIndex == REPLAY_NOT_TRACKED )
				break;

			vehiclereplay_t& rep = m_vehicles[evt.replayIndex];
			if(rep.obj_car)
				rep.obj_car->Enable( (bool)evt.eventData );

			break;
		}
		case REPLAY_EVENT_CAR_LOCK:
		{
			if( evt.replayIndex == REPLAY_NOT_TRACKED )
				break;

			vehiclereplay_t& rep = m_vehicles[evt.replayIndex];
			if(rep.obj_car)
				rep.obj_car->Lock( (bool)evt.eventData );

			break;
		}
	}
}

void CReplayData::SetupReplayCar( vehiclereplay_t* rep )
{
	rep->curr_frame = 0;
	CCar* pCar = rep->obj_car;

	if(!pCar)
		return;

	if(rep->replayArray.numElem() == 0)
	{
		MsgWarning("Warning: no replay frames for '%s' (rid=%d)\n", rep->name.c_str(), rep->obj_car->m_replayID);

		pCar->GetPhysicsBody()->SetPosition(rep->car_initial_pos);
		pCar->GetPhysicsBody()->SetOrientation(rep->car_initial_rot);
		pCar->GetPhysicsBody()->SetLinearVelocity(rep->car_initial_vel);
		pCar->GetPhysicsBody()->SetAngularVelocity(rep->car_initial_angvel);
	}
	else
	{
		const replaycontrol_t& ctrl = rep->replayArray[0];

		pCar->GetPhysicsBody()->SetPosition(ctrl.car_origin);
		pCar->GetPhysicsBody()->SetOrientation(Quaternion(ctrl.car_rot));
		pCar->GetPhysicsBody()->SetLinearVelocity(ctrl.car_vel);
		pCar->GetPhysicsBody()->SetAngularVelocity(ctrl.car_angvel);
	}
}