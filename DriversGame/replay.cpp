//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Replay data
//////////////////////////////////////////////////////////////////////////////////

#include "replay.h"
#include "DrvSynStates.h"
#include "session_stuff.h"
#include "IDebugOverlay.h"

#include "world.h"
#include "input.h"

#define CONTROL_DETAILS_STEP	(0.0025f)
#define CORRECTION_TICK			32
#define COLLISION_MIN_IMPULSE	(0.1f)

DECLARE_CMD(replay_debug_stop, "stops playback (debug)", CV_CHEAT)
{
	g_replayData->m_state = REPL_NONE;
}

ConVar replay_realtime_pursuer("replay_realtime_pursuer", "0", "Cancels replay control for pursuer cars, ideal for AI debugging", CV_CHEAT);

// sort events in right order
int _sortEventsFunc(const replayEvent_t& a, const replayEvent_t& b)
{
	return a.frameIndex - b.frameIndex;
}

CReplayData::CReplayData()
{
	m_state = REPL_NONE;
	m_filename = "";
	m_currentEvent = 0;
	m_numFrames = 0;
	m_currentCamera = -1;
	m_unsaved = true;
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

	replayCarStream_t veh;

	veh.obj_car = pCar;
	veh.recordOnlyCollisions = onlyCollisions;

	veh.scriptObjectId = pCar->GetScriptID();

	veh.name = pCar->m_conf->carName.c_str();

	veh.curr_frame = 0;
	veh.done = false;
	veh.onEvent = true;

	veh.skipFrames = 0;
	veh.skeptFrames = 0;
	veh.first_tick = m_tick;

	return m_vehicles.append(veh);
}

void CReplayData::Clear()
{
	m_currentEvent = 0;
	m_numFrames = 0;
	m_playbackPhysIterations = g_pGameSession->GetPhysicsIterations();

	ClearEvents();
	
	m_vehicles.clear();
	m_cameras.clear();
	m_activeVehicles.clear();

	m_unsaved = true;
}

void CReplayData::ClearEvents()
{
	for (int i = 0; i < m_events.numElem(); i++)
	{
		if(m_events[i].eventType == REPLAY_EVENT_NETWORKSTATE_CHANGED)
			delete ((CNetMessageBuffer*)m_events[i].eventData);
	}

	m_events.clear();
}

void CReplayData::StartRecording()
{
	m_tick = 0;
	m_currentEvent = 0;
	m_state = REPL_RECORDING;
	m_playbackPhysIterations = g_pGameSession->GetPhysicsIterations();
}

void CReplayData::StartPlay()
{
	m_tick = 0;
	m_currentEvent = 0;
	m_currentCamera = -1;

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
		replayCarStream_t& veh = m_vehicles[i];
		veh.done = true;

		if(m_state == REPL_PLAYING)
		{
			veh.obj_car = NULL;	// release
			veh.curr_frame = 0;
			veh.skeptFrames = 0;
			veh.skipFrames = 0;
		}
	}

	ResetEvents();

	m_currentEvent = 0;
	m_tick = 0;
	m_currentCamera = -1;
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
		m_tick++;

		RaiseTickEvents();

		if(m_state == REPL_PLAYING)
		{
			for(int i = m_currentCamera+1; i < m_cameras.numElem(); i++)
			{
				if(m_tick >= m_cameras[i].startTick)
				{
					m_currentCamera = i;
					break;
				}
			}
		}
	}
}

replayCamera_t* CReplayData::GetCurrentCamera()
{
	if(m_cameras.numElem() == 0)
		return NULL;

	if(m_currentCamera == -1)
		return NULL;

	return &m_cameras[m_currentCamera];
}

int CReplayData::AddCamera(const replayCamera_t& camera)
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

CCar* CReplayData::GetCarByScriptId(int id)
{
	for(int i = 0; i < m_vehicles.numElem(); i++)
	{ 
		replayCarStream_t& replay = m_vehicles[i];

		if (replay.scriptObjectId == id)
		{
			if(!replay.obj_car)
				ASSERTMSG(false, varargs("Scripted car did not spawn on tick %d, but it spawns on %d", m_tick, m_vehicles[i].first_tick));

			return replay.obj_car;	// can be null
		}
	}

	

	return nullptr;
}

void CReplayData::UpdateReplayObject( int replayId )
{
	if(m_state == REPL_NONE)
		return;

	if(replayId == REPLAY_NOT_TRACKED)
		return;

	replayCarStream_t* rep = &m_vehicles[ replayId ];

	if(m_state == REPL_RECORDING)
	{
		CCar* car = rep->obj_car;

		if (car && (car->m_changeList_Replay.numElem()))
		{
			// allow netmessage to do this
			CNetMessageBuffer* msgBuffer = new CNetMessageBuffer();

			car->OnPackMessage(msgBuffer, car->m_changeList_Replay);

			PushEvent(REPLAY_EVENT_NETWORKSTATE_CHANGED, replayId, msgBuffer);

			car->m_changeList_Replay.clear();
		}

		if(!RecordVehicleFrame( rep ))
			PlayVehicleFrame( rep ); // should play already recorded
	}
	else
		PlayVehicleFrame( rep );
}

void CReplayData::ForceUpdateReplayObjects()
{
	for(int i = 0; i < m_activeVehicles.numElem(); i++)
	{
		UpdateReplayObject(m_activeVehicles[i]);
	}
}

// returns status the current car playing or not
bool CReplayData::IsCarPlaying( CCar* pCar )
{
	if(!(m_state == REPL_PLAYING || m_state == REPL_RECORDING))
		return false;

	if(pCar->m_replayID == REPLAY_NOT_TRACKED)
		return false;

	replayCarStream_t& veh = m_vehicles[pCar->m_replayID];

	if(veh.obj_car == pCar)
	{
		if(!(veh.done || veh.curr_frame < veh.replayArray.numElem()-1))
			return false;

		int lastFrameIdx = veh.replayArray.numElem()-1;

		replayCarFrame_t& frame = veh.replayArray[lastFrameIdx];

		// wait for our tick
		return (m_tick <= frame.tick);
	}

	return false;
}

void CReplayData::PlayVehicleFrame(replayCarStream_t* rep)
{
	if(rep->obj_car == NULL)
		return;

	// if replay is complete or it's playback time, play this.
	if(!(rep->done || rep->curr_frame < rep->replayArray.numElem()-1))
		return;

	if(rep->curr_frame >= rep->replayArray.numElem())
		return; // done playing it

	replayCarFrame_t& frame = rep->replayArray[rep->curr_frame];

	CCar* car = rep->obj_car;
	CEqRigidBody* body = car->GetPhysicsBody();

	// wait for our tick
	if (m_tick < frame.tick)
	{
		// only interpolate cars that not player/lead/view car
		if (rep->curr_frame > 0 && 
			g_pGameSession->GetViewObject() != car && 
			g_pGameSession->GetLeadCar() != car && 
			g_pGameSession->GetPlayerCar() != car &&
			car->m_conf->flags.isCar)
		{
			replayCarFrame_t& prevFrame = rep->replayArray[rep->curr_frame - 1];

			// interpolate the replay if time difference between frames is huge
			if ((frame.tick - prevFrame.tick) >= CORRECTION_TICK)
			{
				float frameLerpValue = RemapVal((float)m_tick, (float)prevFrame.tick, (float)frame.tick, 0.0f, 1.0f);

				auto frameLerpPos = lerp(prevFrame.car_origin, frame.car_origin, frameLerpValue);
				auto frameLerpOrient = slerp(Quaternion(prevFrame.car_rot), Quaternion(frame.car_rot), frameLerpValue);
				auto frameLerpLinVel = lerp(prevFrame.car_vel, frame.car_vel, frameLerpValue);
				auto frameLerpAngVel = lerp(prevFrame.car_angvel, frame.car_angvel, frameLerpValue);

				body->SetPosition(frameLerpPos);
				body->SetOrientation(frameLerpOrient);
				body->SetLinearVelocity(frameLerpLinVel);
				body->SetAngularVelocity(frameLerpAngVel);
			}
		}
		return;
	}

	// advance frame
	rep->curr_frame++;

	// disable correction for pursuer, debug only
	if (replay_realtime_pursuer.GetBool() && car->ObjType() == GO_CAR_AI)
	{
		CAITrafficCar* trafficCar = (CAITrafficCar*)car;
		if (trafficCar->IsPursuer())
			return;
	}

	// correct whole frame
	body->SetPosition(frame.car_origin);
	body->SetOrientation(Quaternion(frame.car_rot.w, frame.car_rot.x, frame.car_rot.y, frame.car_rot.z));
	body->SetLinearVelocity(frame.car_vel);
	body->SetAngularVelocity(frame.car_angvel);

	// unpack
	uint16 accelControl = frame.control_vars.acceleration;
	uint16 brakeControl = frame.control_vars.brake;
	short steerControl = frame.control_vars.steering * (frame.control_vars.steerSign > 0 ? -1 : 1);
	bool autoHandbrake = (frame.control_vars.autoHandbrake > 0);

	int control_flags = frame.button_flags;

	car->m_steerRatio = steerControl;
	car->m_accelRatio = accelControl;
	car->m_brakeRatio = brakeControl;
	car->m_autohandbrake = autoHandbrake;

	car->SetControlButtons(control_flags);
}

// records vehicle frame
bool CReplayData::RecordVehicleFrame(replayCarStream_t* rep)
{
	int numFrames = rep->replayArray.numElem();

	// if replay is done or current frames are loaded and must be played (or rewinded)
	if( rep->done || numFrames > 0 && rep->curr_frame < numFrames )
		return false; // done or has future frames

	CEqRigidBody* body = rep->obj_car->GetPhysicsBody();

	bool addControls = false;

	uint16 accelControl = rep->obj_car->m_accelRatio;
	uint16 brakeControl = rep->obj_car->m_brakeRatio;
	short steerControl = rep->obj_car->m_steerRatio;
	bool autoHandbrake = rep->obj_car->m_autohandbrake;

	short control_flags = rep->obj_car->GetControlButtons();
	control_flags &= ~IN_MISC; // kill misc buttons, left only needed

	rep->skeptFrames++;

	// correct only non-frozen bodies
	bool forceCorrection = (m_tick % CORRECTION_TICK == 0) && !body->IsFrozen() || (numFrames == 0);

	DkList<CollisionPairData_t>& collisionList = body->m_collisionList;
	bool satisfiesCollisions = collisionList.numElem([=](CollisionPairData_t& pair){ return pair.appliedImpulse > COLLISION_MIN_IMPULSE; });

	// decide to discard frames or not depending on events
	// or record only frames where collision ocurred
	if(!rep->recordOnlyCollisions && !rep->onEvent && !satisfiesCollisions)
	{
		// skip frames if not on event
		if (rep->skeptFrames < rep->skipFrames && !forceCorrection)
			return true;
		else
			rep->skeptFrames = 0;

		replayCarFrame_t& prevControl = rep->replayArray[numFrames-1];

		// unpack
		uint16 prevAccelControl = prevControl.control_vars.acceleration;
		uint16 prevbrakeControl = prevControl.control_vars.brake;
		short prevsteerControl = prevControl.control_vars.steering * (prevControl.control_vars.steerSign > 0 ? -1 : 1);
		bool prevAutoHandbrake = (prevControl.control_vars.autoHandbrake > 0);

		addControls =	(prevControl.button_flags != control_flags) ||
						(accelControl != prevAccelControl) ||
						(brakeControl != prevbrakeControl) ||
						(steerControl != prevsteerControl) ||
						(autoHandbrake != prevAutoHandbrake) ||
						forceCorrection;
	}

	// should not skip frames after collision
	if(rep->skipFrames > 0 && (satisfiesCollisions || !rep->obj_car->IsAlive()))
		rep->skipFrames = 0;

	// add replay frame if car has collision with objects

	if( rep->onEvent || addControls || satisfiesCollisions )
	{
		replayCarFrame_t con;
		con.button_flags = control_flags;

		Quaternion orient = body->GetOrientation();

		con.car_origin = body->GetPosition();
		con.car_rot = TVec4D<half>(orient.x, orient.y, orient.z, orient.w);
		con.car_vel = body->GetLinearVelocity();
		con.car_angvel = body->GetAngularVelocity();

		con.control_vars.acceleration = accelControl;
		con.control_vars.brake = brakeControl;
		con.control_vars.steering = abs(steerControl);
		con.control_vars.steerSign = (steerControl < 0) ? 1 : 0;
		con.control_vars.autoHandbrake = autoHandbrake ? 1 : 0;

		con.tick = m_tick;

		rep->replayArray.append(con);
		rep->curr_frame++;

		rep->onEvent = false;
	}

	return true;
}

extern EqString g_scriptName;

void CReplayData::SaveToFile( const char* filename )
{
	if(g_pGameSession == NULL)
		return;

	m_filename = filename;

	if(m_filename.Path_Extract_Ext().Length() == 0)
		m_filename = m_filename + ".rdat";

	IFile* rplFile = g_fileSystem->Open(m_filename.c_str(), "wb", SP_MOD);

	if(rplFile)
	{
		replayHeader_t hdr;
		hdr.idreplay = VEHICLEREPLAY_IDENT;
		hdr.version = VEHICLEREPLAY_VERSION;
		//hdr.phys_iterations = g_pGameSession->GetPhysicsIterations();

		strcpy(hdr.levelname, g_pGameWorld->GetLevelName());
		strcpy(hdr.envname, g_pGameWorld->GetEnvironmentName());
		strcpy(hdr.missionscript, g_State_Game->GetMissionScriptName());

		rplFile->Write(&hdr, 1, sizeof(replayHeader_t));

		int count = m_vehicles.numElem();
		rplFile->Write(&count, 1, sizeof(int));

		// write vehicles
		for(int i = 0; i < m_vehicles.numElem(); i++)
			WriteVehicleAndFrames(&m_vehicles[i], rplFile);

		// write events
		WriteEvents(rplFile);

		g_fileSystem->Close(rplFile);

		Msg("Replay '%s' saved\n", filename);
		m_unsaved = false;
	}

	if(m_cameras.numElem() > 0)
	{
		EqString camsFilename = m_filename.Path_Strip_Ext() + ".rcam";
		IFile* camFile = g_fileSystem->Open(camsFilename.c_str(), "wb", SP_MOD);

		if(camFile)
		{
			replayCameraHeader_t camhdr;
			camhdr.version = CAMERAREPLAY_VERSION;
			camhdr.numCameras = m_cameras.numElem();

			camFile->Write(&camhdr, 1, sizeof(replayCameraHeader_t));

			for(int i = 0; i < m_cameras.numElem(); i++)
			{
				camFile->Write(&m_cameras[i], 1, sizeof(replayCamera_t));
			}

			g_fileSystem->Close(camFile);

			Msg("Replay cameras '%s' saved\n", filename);
		}
	}
}

void CReplayData::WriteVehicleAndFrames(replayCarStream_t* rep, IVirtualStream* stream )
{
	replayFileCarStream_t data(*rep);

	stream->Write(&data, 1, sizeof(replayFileCarStream_t));

	for(int j = 0; j < rep->replayArray.numElem(); j++)
	{
		replayCarFrame_t& control = rep->replayArray[j];
		stream->Write(&control, 1, sizeof(replayCarFrame_t));
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

	bool hasEndEvent = false;
	int lastEventFrame = m_numFrames > 0 ? m_numFrames : m_tick;
	for(int i = 0; i < m_events.numElem(); i++)
	{
		replayEvent_t& evt = m_events[i];

		if(evt.eventType == REPLAY_EVENT_END)
			hasEndEvent = true;
	}

	if(!hasEndEvent)
		numEvents++;

	// to write end event
	stream->Write(&numEvents, 1, sizeof(int));

	CMemoryStream eventData;

	// write events
	for(int i = 0; i < m_events.numElem(); i++)
	{
		replayEvent_t& evt = m_events[i];

		// if set, do only specific events
		if( onlyEvent != -1 )
		{
			if( evt.eventType != onlyEvent )
				continue;
		}

		replayFileEvent_t fevent( evt );

		// make replay event data
		if( evt.eventData != NULL )
		{
			eventData.Open(NULL, VS_OPEN_WRITE, 128);

			switch( evt.eventType )
			{
				case REPLAY_EVENT_CAR_DEATH:
				{
					int value = (int)(intptr_t)evt.eventData;

					eventData.Write( &value, sizeof(value), 1 );
					fevent.eventDataSize = eventData.Tell();
					break;
				}
				case REPLAY_EVENT_NETWORKSTATE_CHANGED:
				{
					CNetMessageBuffer* buf = (CNetMessageBuffer*)evt.eventData;
					buf->ResetPos();
					buf->WriteToStream(&eventData);
					fevent.eventDataSize = buf->GetMessageLength();
					break;
				}
			}
		}
		else
			fevent.eventDataSize = 0;

		stream->Write(&fevent, 1, sizeof(replayFileEvent_t));

		if( fevent.eventDataSize > 0 )
			stream->Write(eventData.GetBasePointer(), 1, fevent.eventDataSize);
	}

	// write end event
	if(!hasEndEvent)
	{
		replayFileEvent_t endEvent;
		endEvent.frameIndex = lastEventFrame+1;
		endEvent.replayIndex = REPLAY_NOT_TRACKED;
		endEvent.eventType = REPLAY_EVENT_END;
		endEvent.eventFlags = 0;
		endEvent.eventDataSize = 0;

		stream->Write(&endEvent, 1, sizeof(replayFileEvent_t));
	}
}

bool CReplayData::SaveVehicleReplay( CCar* target, const char* filename )
{
	replayCarStream_t* rep = NULL;

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

void CReplayData::ReadEvent( replayEvent_t& evt, IVirtualStream* stream )
{
	replayFileEvent_t fevent;
	stream->Read(&fevent, 1, sizeof(replayFileEvent_t));

	evt.frameIndex = fevent.frameIndex;
	evt.replayIndex = fevent.replayIndex;
	evt.eventType = fevent.eventType;
	evt.eventFlags = fevent.eventFlags;

	// pre-process simple events
	switch (evt.eventType)
	{
		case REPLAY_EVENT_SPAWN:
		{
#define SPAWN_ERROR_CORRECTION_TICKS 4
			evt.frameIndex -= SPAWN_ERROR_CORRECTION_TICKS;

			m_vehicles[evt.replayIndex].first_tick = evt.frameIndex;
			break;
		}
	}

	if( fevent.eventDataSize > 0 )
	{
		switch( evt.eventType )
		{
			case REPLAY_EVENT_CAR_DEATH:
			{
				int value;
				stream->Read(&value, 1, sizeof(value));

				evt.eventData = (void*)(intptr_t)value;
				break;
			}
			case REPLAY_EVENT_NETWORKSTATE_CHANGED:
			{
				void* tempBuf = stackalloc(fevent.eventDataSize+4);
				stream->Read(tempBuf, 1, fevent.eventDataSize);

				CNetMessageBuffer* buf = new CNetMessageBuffer();
				buf->WriteData(tempBuf, fevent.eventDataSize);

				stackfree(tempBuf);

				evt.eventData = buf;
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
		// find target car
		if (target->m_replayID == REPLAY_NOT_TRACKED)
		{
			tickCount = 0;
		}
		else
		{
			int numControls = m_vehicles[target->m_replayID].replayArray.numElem();

			// get last tick and subtract from current
			if (numControls > 0)
				tickCount = m_vehicles[target->m_replayID].replayArray[numControls - 1].tick - m_tick;
		}
		
		return true;
	}

	Msg("Loading vehicle frames from '%s'\n", filename);

	IFile* pFile = g_fileSystem->Open((_Es(filename) + ".vr").c_str(), "rb", SP_MOD);

	if(pFile)
	{
		// read replay file header
		replayFileCarStream_t data;
		pFile->Read(&data, 1, sizeof(replayFileCarStream_t));

		// try to inject frames
		int repIdx = target->m_replayID;

		bool wereTracked = true;

		// make vehicle tracking if not
		if(repIdx == REPLAY_NOT_TRACKED)
		{
			replayCarStream_t newRep;
			repIdx = m_vehicles.append(newRep);
			wereTracked = false;
		}

		replayCarStream_t& veh = m_vehicles[repIdx];

		if(!wereTracked)
		{
			// don't overwrite some data
			veh.obj_car = target;
			veh.name = data.name;

			// set replay id
			veh.obj_car->m_replayID = repIdx;
			veh.curr_frame = 0;

			veh.scriptObjectId = data.scriptObjectId;
			veh.first_tick = m_tick;
		}
		else
			veh.curr_frame = veh.replayArray.numElem();

		// read frames of vehicle controls
		for(int j = 0; j < data.numFrames; j++)
		{
			replayCarFrame_t control;
			pFile->Read(&control, 1, sizeof(replayCarFrame_t));

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
			replayEvent_t evt;
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

void CReplayData::StopVehicleReplay(CCar* pCar)
{
	if(	m_state != REPL_RECORDING)
	{
		return;
	}

	if(pCar->m_replayID == REPLAY_NOT_TRACKED)
		return;

	replayCarStream_t& veh = m_vehicles[pCar->m_replayID];

	// remove future frames to record new ones
	for(int i = veh.curr_frame; i < veh.replayArray.numElem(); )
		veh.replayArray.removeIndex(i);
}

bool CReplayData::LoadFromFile(const char* filename)
{
	m_filename = filename;

	if(m_filename.Path_Extract_Ext().Length() == 0)
		m_filename = m_filename + ".rdat";

	IFile* pFile = g_fileSystem->Open(m_filename.c_str(), "rb", SP_MOD);

	if(!pFile)
	{
		MsgError("Demo '%s' not found!\n", m_filename.c_str());
		return false;
	}

	replayHeader_t hdr;
	pFile->Read(&hdr, 1, sizeof(replayHeader_t));

	if(hdr.idreplay != VEHICLEREPLAY_IDENT)
	{
		MsgError("Error: '%s' is not a valid replay file!\n", m_filename.c_str());
		g_fileSystem->Close(pFile);
		return false;
	}

	if(hdr.version != VEHICLEREPLAY_VERSION)
	{
		MsgError("Error: Replay '%s' has invalid version!\n", m_filename.c_str());
		g_fileSystem->Close(pFile);
		return false;
	}

	Clear();

	//m_playbackPhysIterations = hdr.phys_iterations;

	int veh_count = 0;
	pFile->Read(&veh_count, 1, sizeof(int));

	// read vehicle names
	for(int i = 0; i < veh_count; i++)
	{
		replayFileCarStream_t data;

		pFile->Read(&data, 1, sizeof(replayFileCarStream_t));

		replayCarStream_t veh;

		// event will spawn this car
		veh.obj_car = NULL;

		veh.scriptObjectId = data.scriptObjectId;

		veh.name = data.name;

		veh.done = true; // that's complete replay
		veh.curr_frame = 0;
		veh.onEvent = false;
		veh.first_tick = m_tick;

		// read control array stored after header
		for(int j = 0; j < data.numFrames; j++)
		{
			replayCarFrame_t control;
			pFile->Read(&control, 1, sizeof(replayCarFrame_t));

			veh.replayArray.append(control);
		}

		m_vehicles.append(veh);
	}

	// read events
	int numEvents = 0;
	pFile->Read(&numEvents, 1, sizeof(int));

	for(int i = 0; i < numEvents; i++)
	{
		replayEvent_t evt;
		ReadEvent(evt, pFile);

		// set frame count
		if(evt.eventType == REPLAY_EVENT_END)
			m_numFrames = evt.frameIndex;

		m_events.append( evt );
	}

	MsgAccept("Replay file '%s' loaded successfully\n", filename);

	g_fileSystem->Close(pFile);

	// try loading camera file
	EqString camsFilename = m_filename.Path_Strip_Ext() + ".rcam";
	pFile = g_fileSystem->Open(camsFilename.c_str(), "rb", SP_MOD);

	if(pFile)
	{
		replayCameraHeader_t camhdr;
		pFile->Read( &camhdr,1,sizeof(replayCameraHeader_t) );

		if(camhdr.version == CAMERAREPLAY_VERSION)
		{
			m_currentCamera = 0;

			replayCamera_t cam;

			for(int i = 0; i < camhdr.numCameras; i++)
			{
				pFile->Read( &cam,1,sizeof(replayCamera_t) );
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
	m_unsaved = false;

	g_pGameWorld->SetLevelName(hdr.levelname);
	g_pGameWorld->SetEnvironmentName(hdr.envname);
	
	if(!g_State_Game->SetMissionScript(hdr.missionscript))
	{
		MsgError("ERROR - Invalid mission script '%s' for replay '%s'\n", hdr.missionscript, filename);

		Clear();

		m_state = REPL_NONE;

		return false;
	}

	return true;
}

void CReplayData::PushSpawnOrRemoveEvent( EReplayEventType type, CGameObject* object, int eventFlags)
{
	if( m_state != REPL_RECORDING )
		return;


	replayEvent_t evt;
	evt.eventType = type;
	evt.frameIndex = m_tick;
	evt.replayIndex = REPLAY_NOT_TRACKED;
	evt.eventData = NULL;
	evt.eventFlags = REPLAY_FLAG_IS_PUSHED | eventFlags;

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

			replayCarStream_t& veh = m_vehicles[evt.replayIndex];

			if ( pCar->IsPursuer() )
			{
				CAIPursuerCar* pursuer = (CAIPursuerCar*)pCar;

				if(pursuer->GetPursuerType() == PURSUER_TYPE_COP)
					evt.eventFlags |= REPLAY_FLAG_CAR_COP_AI;
				else if(pursuer->GetPursuerType() == PURSUER_TYPE_GANG)
					evt.eventFlags |= REPLAY_FLAG_CAR_GANG_AI;
			}
			else
			{
				veh.skipFrames = 64;	// skip frames
				veh.skeptFrames = 0;
				evt.eventFlags |= REPLAY_FLAG_CAR_AI;
			}


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
				replayCarStream_t& veh = m_vehicles[evt.replayIndex];

				// make replay done, and invalidate realtime data
				veh.done = true;
				veh.onEvent = true;
				veh.obj_car = NULL;
			}
			else
				return; // you can't do that

			// no additional information needed
			m_activeVehicles.fastRemove( evt.replayIndex );
		}
	}

	m_events.append( evt );
}

void CReplayData::PushEvent(EReplayEventType type, int replayId, void* eventData, int eventFlags)
{
	if( m_state != REPL_RECORDING )
		return;

	replayEvent_t evt;
	evt.eventType = type;
	evt.frameIndex = m_tick;
	evt.replayIndex = replayId;
	evt.eventData = eventData;
	evt.eventFlags = REPLAY_FLAG_IS_PUSHED | eventFlags;

	if(type == REPLAY_EVENT_END)
		m_numFrames = m_tick;

	if(evt.replayIndex != REPLAY_NOT_TRACKED)
		m_vehicles[evt.replayIndex].onEvent = true;

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
		replayEvent_t& evt = m_events[i];

		if(evt.frameIndex > m_tick)
			break;

		RaiseReplayEvent( evt );

		m_currentEvent++;
	}
}

void CReplayData::RaiseReplayEvent(const replayEvent_t& evt)
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

			replayCarStream_t& rep = m_vehicles[evt.replayIndex];

			m_activeVehicles.append( evt.replayIndex );

			// spawn replay car if not scripted
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
				g_pGameSession->SetPlayerCar(rep.obj_car);

			if( rep.scriptObjectId != SCRIPT_ID_NOTSCRIPTED )
			{
				// here is an exception for lua call
				rep.obj_car->m_scriptID = rep.scriptObjectId; //g_pGameSession->GenScriptID();

				//ASSERT(rep.scriptObjectId == rep.obj_car->GetScriptID());
			}

			// spawn
			rep.obj_car->Spawn();
			g_pGameWorld->AddObject(rep.obj_car);

			SetupReplayCar( &rep );

			break;
		}
		case REPLAY_EVENT_REMOVE:
		{
			// remove car
			if( evt.replayIndex == REPLAY_NOT_TRACKED )
				break;

			replayCarStream_t& rep = m_vehicles[evt.replayIndex];

			m_activeVehicles.fastRemove( evt.replayIndex );

			if(g_pGameSession->GetPlayerCar() == rep.obj_car)
				g_pGameSession->SetPlayerCar(NULL);

			g_pGameWorld->RemoveObject(rep.obj_car);
			rep.obj_car = NULL;

			break;
		}
		case REPLAY_EVENT_END:
		{
			g_pGameSession->SignalMissionStatus(MIS_STATUS_REPLAY_END, 0.0f);
			//Stop();
			break;
		}
		case REPLAY_EVENT_FORCE_RANDOM:
		{
			g_replayRandom.Regenerate();
			break;
		}
		case REPLAY_EVENT_RANDOM_SEED:
		{
			g_replayRandom.SetSeed((int)(intptr_t)evt.eventData);
			break;
		}
		case REPLAY_EVENT_CAR_DEATH:
		{
			if( evt.replayIndex == REPLAY_NOT_TRACKED )
				break;

			replayCarStream_t& rep = m_vehicles[evt.replayIndex];

			if(rep.obj_car)
			{
				// set car damage to maximum and fire event
				rep.obj_car->SetDamage( rep.obj_car->GetMaxDamage() );

				int deathByReplayIdx = (int)(intptr_t)evt.eventData;

				if(deathByReplayIdx != REPLAY_NOT_TRACKED)
				{
					replayCarStream_t& deathByRep = m_vehicles[(int)(intptr_t)evt.eventData];
					rep.obj_car->OnDeath( deathByRep.obj_car );
				}
				else
					rep.obj_car->OnDeath( NULL );
			}

			break;
		}
		case REPLAY_EVENT_NETWORKSTATE_CHANGED:
		{
			if (evt.replayIndex == REPLAY_NOT_TRACKED)
				break;

			replayCarStream_t& rep = m_vehicles[evt.replayIndex];

			if (rep.obj_car)
			{
				CNetMessageBuffer* buf = (CNetMessageBuffer*)evt.eventData;
				buf->ResetPos();
				rep.obj_car->OnUnpackMessage(buf);
			}
		}
	}
}

void CReplayData::SetupReplayCar( replayCarStream_t* rep )
{
	rep->curr_frame = 0;
	CCar* pCar = rep->obj_car;

	if(!pCar)
		return;

	CEqRigidBody* body = pCar->GetPhysicsBody();

	const replayCarFrame_t& ctrl = rep->replayArray[0];

	body->SetPosition(ctrl.car_origin);
	body->SetOrientation(Quaternion(ctrl.car_rot));
	body->SetLinearVelocity(ctrl.car_vel);
	body->SetAngularVelocity(ctrl.car_angvel);
}
