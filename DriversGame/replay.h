//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: Replay data
//////////////////////////////////////////////////////////////////////////////////

/*
TODO:
		- [ok] Record all cars
		- [ok] Record car spawns
		- [ok] Cameras
		- Camera keyFrames ()
		- [ok] Record spawn params
		- [ok] Stream recording of the controls and events
		- Record networked cars
*/

#ifndef REPLAY_H
#define REPLAY_H

#include "car.h"
#include "in_keys_ident.h"

#define REPLAY_NOT_TRACKED (-1)

#define VEHICLEREPLAY_IDENT MCHAR4('D','S','R','P')

#define VEHICLEREPLAY_VERSION	4
#define CAMERAREPLAY_VERSION	1

#define USERREPLAYS_PATH "UserReplays/"

struct replaycamera_s
{
	int				startTick;	// camera start tick
	int8			type;		// ECameraMode

	int				targetIdx;	// target replay index. Used to target at object

	FVector3D		origin;
	TVec3D<half>	rotation;
	half			fov;
};

ALIGNED_TYPE(replaycamera_s,4) replaycamera_t;

enum EReplayFlags
{
	REPLAY_FLAG_CAR_AI		= (1 << 0),
	REPLAY_FLAG_CAR_COP_AI	= (1 << 1),
	REPLAY_FLAG_CAR_GANG_AI = (1 << 2),

	REPLAY_FLAG_SCRIPT_CALL	= (1 << 3),

	// this is a marker
	REPLAY_FLAG_IS_PUSHED	= (1 << 14)
};

// replay event opcodes
enum EReplayEventType
{
	// demo end indicator
	REPLAY_EVENT_END = 0,

	// object spawn/remove
	REPLAY_EVENT_SPAWN,
	REPLAY_EVENT_REMOVE,

	REPLAY_EVENT_FORCE_RANDOM,

	REPLAY_EVENT_SET_PLAYERCAR,
	REPLAY_EVENT_CAR_SETCOLOR,
	REPLAY_EVENT_CAR_ENABLE,
	REPLAY_EVENT_CAR_LOCK,
	REPLAY_EVENT_CAR_DAMAGE,
	REPLAY_EVENT_CAR_SETMAXDAMAGE,
	REPLAY_EVENT_CAR_SETMAXSPEED,
	REPLAY_EVENT_CAR_DEATH,
};

struct replaycontrol_s
{
	int				tick;			// time since last update

	// 3 packed 10 bit values
	struct 
	{
		uint		acceleration : 10;
		uint		brake : 10;
		uint		steering : 10;
		uint		steerSign : 1;
		uint		autoHandbrake : 1;
	} control_vars;

	FVector3D		car_origin;
	TVec4D<half>	car_rot;

	TVec3D<half>	car_vel;
	TVec3D<half>	car_angvel;

	short			button_flags;

	ubyte			lights_enabled;
	ubyte			reserved;
};

ALIGNED_TYPE(replaycontrol_s,4) replaycontrol_t;

struct vehiclereplay_t
{
	FVector3D	car_initial_pos;
	Quaternion	car_initial_rot;
	Vector3D	car_initial_vel;
	Vector3D	car_initial_angvel;

	int			scriptObjectId;

	int64		curr_frame; // play only

	uint16		skipFrames;
	uint16		skeptFrames;

	bool		recordOnlyCollisions : 1;
	bool		done		: 1;
	bool		onEvent		: 1;

	CCar*		obj_car;
	EqString	name;

	DkList<replaycontrol_t> replayArray;
};

struct replayevent_t
{
	int 		frameIndex;		// physics frame index
	int			replayIndex;
	
	uint8		eventType;

	short		eventFlags;

	void*		eventData;
};

struct vehiclereplay_file_s
{
	vehiclereplay_file_s() {}

	vehiclereplay_file_s(const vehiclereplay_t& rep)
	{
		car_initial_pos = rep.car_initial_pos;
		car_initial_rot = TVec4D<half>(	rep.car_initial_rot.x, 
										rep.car_initial_rot.y,
										rep.car_initial_rot.z,
										rep.car_initial_rot.w);
		car_initial_vel = rep.car_initial_vel;
		car_initial_angvel = rep.car_initial_angvel;

		scriptObjectId = rep.scriptObjectId;

		strcpy(name, rep.name.GetData());

		numFrames = rep.replayArray.numElem();
	}

	FVector3D		car_initial_pos;
	TVec4D<half>	car_initial_rot;
	TVec3D<half>	car_initial_vel;
	TVec3D<half>	car_initial_angvel;

	int			scriptObjectId;

	char		name[128];

	int			numFrames;

	// float	startTime;	// start time when actually car spawned
};

ALIGNED_TYPE(vehiclereplay_file_s,4) vehiclereplay_file_t;

struct replayevent_file_s
{
	replayevent_file_s() {}

	replayevent_file_s(const replayevent_t& evt)
	{
		frameIndex = evt.frameIndex;
		replayIndex = evt.replayIndex;
		eventType = evt.eventType;
		eventFlags = evt.eventFlags;

		eventFlags &= ~REPLAY_FLAG_IS_PUSHED;
		
		eventDataSize = 0;
	}

	int64 		frameIndex;		// physics frame index
	int			replayIndex;
	uint8		eventType;
	short		eventFlags;

	int			eventDataSize;
};

ALIGNED_TYPE(replayevent_file_s,4) replayevent_file_t;

struct replayhdr_s
{
	int		idreplay;		// VEHICLEREPLAY_IDENT

	int		version;		// VEHICLEREPLAY_VERSION

	char	levelname[64];
	char	envname[64];
	char	missionscript[64];
};

ALIGNED_TYPE(replayhdr_s,4) replayhdr_t;

struct replaycamerahdr_s
{
	int		version;		// CAMERAREPLAY_VERSION

	int		numCameras;
};

ALIGNED_TYPE(replaycamerahdr_s,4) replaycamerahdr_t;

enum EReplayState
{
	REPL_NONE = 0,

	REPL_RECORDING,

	REPL_INIT_PLAYBACK,
	REPL_PLAYING,
};

class CReplayData
{
public:
							CReplayData();

							~CReplayData();

	// adds vehicle to record, returns replay index
	int						Record(CCar* pCar, bool onlyCollisions = false);

	// clear all data
	void					Clear();

	// starts recording
	void					StartRecording();

	// initializes replay for playing
	//void					SetupObjects();

	void					StartPlay();

	// stop play or recording
	void					Stop();


	// returns status the current car playing or not
	bool					IsCarPlaying(CCar* pCar);

	void					UpdatePlayback( float fDt );
	void					UpdateReplayObject( int replayId );

	void					ForceUpdateReplayObjects();

	void					SaveToFile( const char* filename );
	bool					SaveVehicleReplay( CCar* target, const char* filename );

	bool					LoadFromFile(const char* filename);
	bool					LoadVehicleReplay( CCar* target, const char* filename, int& tickCount );

	void					StopVehicleReplay(CCar* pCar);

	//------------------------------------------------------

	// replay events
	void					PushSpawnOrRemoveEvent(EReplayEventType type, CGameObject* object, int eventFlags = 0);
	void					PushEvent(EReplayEventType type, int replayId = REPLAY_NOT_TRACKED, void* eventData = NULL, int eventFlags = 0);

	replaycamera_t*			GetCurrentCamera();
	int						AddCamera(const replaycamera_t& camera);
	void					RemoveCamera(int frameIndex);

	CCar*					GetCarByReplayIndex(int index);

	EReplayState			m_state;
	int						m_tick;
	int						m_numFrames;

	DkList<replaycamera_t>	m_cameras;

	int						m_currentCamera;

protected:

	void					ResetEvents();

	void					WriteEvents( IVirtualStream* stream, int onlyEvent = -1 );
	void					WriteVehicleAndFrames(vehiclereplay_t* rep, IVirtualStream* stream );

	void					ReadEvent( replayevent_t& evt, IVirtualStream* stream );

	void					PlayVehicleFrame(vehiclereplay_t* rep);

	// records vehicle frame
	bool					RecordVehicleFrame(vehiclereplay_t* rep);

	void					RaiseTickEvents();
	void					RaiseReplayEvent(const replayevent_t& evt);

	void					SetupReplayCar( vehiclereplay_t* rep );
	

private:

	DkList<int>					m_activeVehicles;

	DkList<vehiclereplay_t>		m_vehicles;
	DkList<replayevent_t>		m_events;
	
	int							m_currentEvent;

	EqString					m_filename;
};

extern CReplayData* g_replayData;

#endif // REPLAY_H