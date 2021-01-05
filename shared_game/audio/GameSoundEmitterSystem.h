//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

#ifndef GAMESNDEMITSYSTEM_H
#define GAMESNDEMITSYSTEM_H

#include "math/Vector.h"
#include "ISoundSystem.h"
#include "utils/eqstring.h"
#include "utils/DkList.h"
#include "utils/eqthread.h"

class BaseEntity;

// flags
enum EEmitSoundFlags
{
	EMITSOUND_FLAG_OCCLUSION		= (1 << 0),		// occludes source by the world geometry
	EMITSOUND_FLAG_ROOM_OCCLUSION	= (1 << 1),		// uses more expensive occlusion system, use it for ambient sounds
	EMITSOUND_FLAG_FORCE_CACHED		= (1 << 2),		// forces emitted sound to be loaded if not cached by PrecacheScriptSound (not recommended, debug only)
	EMITSOUND_FLAG_FORCE_2D			= (1 << 3),		// force 2D sound (music, etc.)
	EMITSOUND_FLAG_STARTSILENT		= (1 << 4),		// starts silent
	EMITSOUND_FLAG_START_ON_UPDATE	= (1 << 5),		// start playing sound on emitter system update

	EMITSOUND_FLAG_PENDING			= (1 << 6),		// was in pending list
};

// channel type for entity call
enum ESoundChannelType
{
	CHAN_INVALID = -1,

	CHAN_STATIC,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_WEAPON,
	CHAN_SIGNAL,
	CHAN_STREAM,	// streaming channel

	CHAN_COUNT
};

static int s_soundChannelMaxEmitters[CHAN_COUNT] =
{
	16, // 16 static channels for entity
	1,	// 1 voice/speech of human
	3,	// 3 for item (clicks, etc)
	16,	// 3 for body
	1,	// 1 for weapon shoot sounds
	1,	// 1 for signal
	1	// one streaming sound
};

static const char* s_soundChannelNames[CHAN_COUNT] =
{
	"CHAN_STATIC",
	"CHAN_VOICE",
	"CHAN_ITEM",
	"CHAN_BODY",
	"CHAN_WEAPON",
	"CHAN_SIGNAL",
	"CHAN_STREAM",
};

static ESoundChannelType ChannelTypeByName(const char* str)
{
	for(int i = 0; i < CHAN_COUNT; i++)
	{
		if(!stricmp(str, s_soundChannelNames[i]))
			return (ESoundChannelType)i;
	}

	return CHAN_INVALID;
}

//---------------------------------------------------------------------------------

struct EmitSound_t;

class CSoundChannelObject
{
public:
	//PPMEM_MANAGED_OBJECT()

	CSoundChannelObject();
	virtual ~CSoundChannelObject();

	// emit sound
	void		EmitSound(const char* name);

	// emit sound with parameters
	void		EmitSoundWithParams( EmitSound_t* ep );

	int			GetChannelSoundCount( ESoundChannelType chan );
	void		DecrementChannelSoundCount( ESoundChannelType chan );

	void		SetSoundVolumeScale( float fScale ) {m_volumeScale = fScale;}
	float		GetSoundVolumeScale() const { return m_volumeScale;}

protected:
	// sounds at channel counter
	uint8		m_numChannelSounds[CHAN_COUNT];

	float		m_volumeScale;
};

//---------------------------------------------------------------------------------

class ISoundController;

struct EmitSound_t
{
	// the first and biggest
	void Init( const char* pszName, const Vector3D& origin, float volume, float pitch, float radius, CSoundChannelObject* obj, int flags );

	EmitSound_t()
	{
		Init("", vec3_zero, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName )
	{
		Init(pszName, vec3_zero, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, int flags )
	{
		Init(pszName, vec3_zero, 1.0f, 1.0f, 1.0f, NULL, flags);
	}

	EmitSound_t( const char* pszName, float volume, float pitch )
	{
		Init(pszName, vec3_zero, volume, pitch, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, const Vector3D& pos )
	{
		Init(pszName, pos, 1.0f, 1.0f, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	EmitSound_t( const char* pszName, const Vector3D& pos, float volume, float pitch )
	{
		Init(pszName, pos, volume, pitch, 1.0f, NULL, EMITSOUND_FLAG_OCCLUSION);
	}

	const char*				name;
	Vector3D				origin;
	float					fVolume;
	float					fPitch;
	float					fRadiusMultiplier;

	CSoundChannelObject*	pObject;
	ISoundController*		pController;

	int						nFlags;
	int						sampleId;
	int						emitterIndex;
};

typedef EmitSound_t EmitParams;

// Dynamic sound controller
class ISoundController
{
public:
	PPMEM_MANAGED_OBJECT();

	virtual ~ISoundController() {}

	virtual bool			IsStopped() const = 0;
	virtual void			StartSound( const char* newSoundName ) = 0; // starts sound

	virtual void			Play() = 0;
	virtual void			Pause() = 0; // pauses sound
	virtual void			Stop(bool force = false) = 0; // stops sound and detach channel
	virtual void			StopLoop() = 0; // stops sound loop

	virtual EmitSound_t&	GetEmitParams() = 0;

	virtual void			SetPitch(float fPitch) = 0; // pitch in range from 0.0001 ... 5
	virtual void			SetVolume(float fVolume) = 0; // volume in range from 0 .. 1
	virtual void			SetOrigin(const Vector3D& origin) = 0;
	virtual void			SetVelocity(const Vector3D& velocity) = 0;
};

struct EmitterData_t;

// Dynamic sound controller
class CSoundController : public ISoundController
{
	friend class	CSoundEmitterSystem;
public:

	CSoundController() : m_emitData(NULL), m_volume(1.0f)
	{
	}

	bool			IsStopped() const;

	void			StartSound(const char* newSoundName); // starts sound

	void			Play(); // starts sound
	void			Pause(); // pauses sound
	void			Stop(bool force = false); // stops sound

	void			StopLoop(); // stops sound loop

	EmitSound_t&	GetEmitParams() {return m_emitParams;}

	void			SetPitch(float fPitch);					// pitch in range from 0.0001 ... 5
	void			SetVolume(float fVolume);				// volume in range from 0 .. 1
	void			SetOrigin(const Vector3D& origin);
	void			SetVelocity(const Vector3D& velocity);

protected:

	EmitSound_t		m_emitParams;
	EqString		m_soundName;	// hold sound name

	float			m_volume;

	EmitterData_t*	m_emitData;
};

//-------------------------------------------------------------------------------------

struct soundScriptDesc_t
{
	char*		pszName;
	int			namehash;

	DkList<ISoundSample*>	pSamples;
	DkList<EqString>		soundFileNames;

	float		fVolume;
	float		fAtten;
	float		fRolloff;
	float		fPitch;
	float		fAirAbsorption;
	float		fMaxDistance;

	bool		extraStreaming : 1;
	bool		loop : 1;
	bool		stopLoop : 1;
	bool		is2d : 1;

	ESoundChannelType	channelType;
};

struct EmitterData_t
{
	EmitterData_t()
	{
		soundSource = nullptr;
		channelObj = nullptr;
		controller = nullptr;
		script = nullptr;
		velocity = vec3_zero;
	}

	ESoundChannelType		channelType;

	CSoundChannelObject*	channelObj;
	ISoundController*		controller;
	ISoundEmitter*			soundSource;

	Vector3D				origin;
	Vector3D				interpolatedOrigin;
	Vector3D				velocity;

	float					startVolume;
	soundScriptDesc_t*		script;			// sound script which used to start this sound
	EmitParams				emitParams;		// parameters which used to start this sound
};

typedef float (*fnSoundEmitterUpdate)(soundParams_t& params, EmitterData_t* emit, bool bForceNoInterp);

// the sound emitter system
class CSoundEmitterSystem
{
	friend class				CSoundController;

public:
	CSoundEmitterSystem();

	void						Init(float maxDistance, fnSoundEmitterUpdate updFunc = nullptr);
	void						Shutdown();

	void						LoadScriptSoundFile(const char* fileName);

	void						SetPaused(bool paused);
	bool						IsPaused();

	void						PrecacheSound(const char* pszName);							// precaches sound

	void						GetAllSoundNames(DkList<EqString>& soundNames) const;

	// emits new sound. returns channel type
	int							EmitSound( EmitSound_t* emit );								// emits sound with specified parameters
	void						Emit2DSound( EmitSound_t* emit, int channelType = -1 );

	void						StopAllSounds();

	void						StopAllEmitters();
	void						StopAll2DSounds();

	void						Set2DChannelsVolume(ESoundChannelType channelType, float volume);

	void						Update(float pitchScale = 1.0f, bool force = false);													// updates sound emitter system
	bool						UpdateEmitter( EmitterData_t* emit, soundParams_t &params, bool bForceNoInterp = false );

	soundScriptDesc_t*			FindSound(const char* soundName) const;						// searches for loaded script sound

	ISoundController*			CreateSoundController(EmitSound_t* ep);						// creates new sound controller
	void						RemoveSoundController(ISoundController* cont);

	void						InvalidateSoundChannelObject(CSoundChannelObject* pEnt);
protected:

	static void					JobSchedulePlayStaticChannel(void* data, int i);

	ISoundSample*				FindBestSample(soundScriptDesc_t* script, int sampleId = -1);

	int							GetEmitterIndexByEntityAndChannel(CSoundChannelObject* pEnt, ESoundChannelType chan);

#ifdef EDITOR
public:
#else
private:
#endif

	DkList<soundScriptDesc_t*>	m_allSounds;

#ifdef EDITOR
private:
#endif

	float						m_2dChannelVolume[CHAN_COUNT];
	float						m_2dScriptVolume[CHAN_COUNT];

	float						m_defaultMaxDistance;

	int							m_totalErrors;

	fnSoundEmitterUpdate		m_fnEmitterProcess;

	DkList<EmitterData_t*>		m_emitters;
	DkList<ISoundController*>	m_controllers;

	DkList<EmitSound_t>			m_pendingStartSounds;
	DkList<EmitSound_t>			m_pendingStartSounds2D;

	Threading::CEqMutex&		m_mutex;

	bool						m_isInit;

	bool						m_isPaused;	
};

extern CSoundEmitterSystem* g_sounds;

#endif // GAMESNDEMITSYSTEM_H
