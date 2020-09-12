//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Replay data
//////////////////////////////////////////////////////////////////////////////////

/*
TODO:
		- [ok] RecordVehicle all cars
		- [ok] RecordVehicle car spawns
		- [ok] Cameras
		- Camera keyFrames ()
		- [ok] RecordVehicle spawn params
		- [ok] Stream recording of the controls and events
		- RecordVehicle networked cars
*/

#ifndef REPLAY_H
#define REPLAY_H

#include "car.h"
#include "in_keys_ident.h"

#define REPLAY_NOT_TRACKED (-1)

#define VEHICLEREPLAY_IDENT MCHAR4('D','S','R','P')

#define VEHICLEREPLAY_VERSION	6
#define CAMERAREPLAY_VERSION	2

#define USERREPLAYS_PATH "UserReplays/"

struct replayCamera_s
{
	FVector3D		origin;
	TVec3D<half>	rotation;
	int				startTick;	// camera start tick
	int				targetIdx;	// target replay index. Used to target at object
	half			fov;
	int8			type;		// ECameraMode
};

ALIGNED_TYPE(replayCamera_s,4) replayCamera_t;

enum EReplayFlags
{
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
	REPLAY_EVENT_RANDOM_SEED,

	REPLAY_EVENT_NETWORKSTATE_CHANGED,

	REPLAY_EVENT_CAR_DEATH,	
};

struct replayCarFrame_s
{
	FVector3D		car_origin;
	TVec4D<half>	car_rot;

	TVec3D<half>	car_vel;
	TVec3D<half>	car_angvel;

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

	short			button_flags;
};

ALIGNED_TYPE(replayCarFrame_s,4) replayCarFrame_t;

struct replayCarStream_t
{
	int			scriptObjectId;
	int			type;
	int			first_tick;

	int			curr_frame; // play only

	uint16		skipFrames;
	uint16		skeptFrames;

	bool		recordOnlyCollisions : 1;
	bool		done		: 1;
	bool		onEvent		: 1;

	CCar*		obj_car;
	EqString	name;

	DkList<replayCarFrame_t> replayArray;
};

struct replayEvent_t
{
	int 		frameIndex;		// physics frame index
	int			replayIndex;
	
	uint8		eventType;

	short		eventFlags;

	void*		eventData;
};

struct replayFileCarStream_s
{
	replayFileCarStream_s() {}

	replayFileCarStream_s(const replayCarStream_t& rep)
	{
		scriptObjectId = rep.scriptObjectId;
		strcpy(name, rep.name.GetData());
		numFrames = rep.replayArray.numElem();
	}

	char		name[48];
	int			scriptObjectId;
	int			numFrames;
};

ALIGNED_TYPE(replayFileCarStream_s,4) replayFileCarStream_t;

struct replayFileEvent_s
{
	replayFileEvent_s() {}

	replayFileEvent_s(const replayEvent_t& evt)
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

	int			eventDataSize;
	short		eventFlags;
	uint8		eventType;
};

ALIGNED_TYPE(replayFileEvent_s,4) replayFileEvent_t;

struct replayHeader_s
{
	int		idreplay;			// VEHICLEREPLAY_IDENT

	int		version;			// VEHICLEREPLAY_VERSION
	int		physIterations;		// number of physics iteration

	char	levelname[64];
	char	envname[64];
	char	missionscript[60];

	int		extraDataSize;		// KeyValues mission data
};

ALIGNED_TYPE(replayHeader_s,4) replayHeader_t;

struct replayCameraHeader_s
{
	int		version;		// CAMERAREPLAY_VERSION
	int		numCameras;
};

ALIGNED_TYPE(replayCameraHeader_s,4) replayCameraHeader_t;

enum EReplayState
{
	REPL_NONE = 0,

	REPL_RECORDING,

	REPL_INIT_PLAYBACK,
	REPL_PLAYING,
};

class CReplayTracker
{
public:
							CReplayTracker();

							~CReplayTracker();

	// adds vehicle to record, returns replay index
	int						RecordVehicle(CCar* pCar, bool onlyCollisions = false);

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

	replayCamera_t*			GetCurrentCamera();
	int						AddCamera(const replayCamera_t& camera);
	void					RemoveCamera(int frameIndex);

	CCar*					GetCarByReplayIndex(int index);
	CCar*					GetCarByScriptId(int id);

	EReplayState			m_state;
	bool					m_unsaved;

	int						m_tick;
	int						m_numFrames;

	DkList<replayCamera_t>	m_cameras;
	int						m_currentCamera;

protected:

	void					ResetEvents();

	void					WriteEvents( IVirtualStream* stream, int onlyEvent = -1 );
	void					WriteVehicleAndFrames(replayCarStream_t* rep, IVirtualStream* stream );

	void					ReadEvent( replayEvent_t& evt, IVirtualStream* stream );

	void					PlayVehicleFrame(replayCarStream_t* rep);

	// records vehicle frame
	bool					RecordVehicleFrame(replayCarStream_t* rep);

	void					RaiseTickEvents();
	void					RaiseReplayEvent(const replayEvent_t& evt);

	bool					SetupReplayCar( int index );
	void					ClearEvents();


private:
	int							GetPlaybackTick(int tick) const;

	kvkeybase_t					m_missionData;
	kvkeybase_t					m_missionSettings;

	EqString					m_filename;

	DkList<int>					m_activeVehicles;

	DkList<replayCarStream_t>	m_vehicles;
	DkList<replayEvent_t>		m_events;
	
	int							m_currentEvent;
	int							m_playbackPhysIterations;

	
};

extern CReplayTracker* g_replayTracker;

#endif // REPLAY_H