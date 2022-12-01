//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

/*
	TODO:
		Sound source direction (for CSoundingObject only)
		Spatial optimizations to query with less distance checks
		Basic cue envelope control support for CSoundingObject
*/

#pragma once

#include "audio/IEqAudioSystem.h"

struct SoundScriptDesc;
struct SoundEmitterData;
class CSoundingObject;

// flags
enum EEmitSoundFlags
{
	EMITSOUND_FLAG_FORCE_CACHED		= (1 << 2),		// forces emitted sound to be loaded if not cached by PrecacheScriptSound (not recommended, debug only)
	EMITSOUND_FLAG_FORCE_2D			= (1 << 3),		// force 2D sound (music, etc.)
	EMITSOUND_FLAG_STARTSILENT		= (1 << 4),		// starts silent
	EMITSOUND_FLAG_START_ON_UPDATE	= (1 << 5),		// start playing sound on emitter system update
	EMITSOUND_FLAG_RANDOM_PITCH		= (1 << 6),		// apply slightly random pitch (best for static hit sounds)

	EMITSOUND_FLAG_PENDING			= (1 << 7),		// was in pending list
};

// channel type for entity call
enum ESoundChannelType : int
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

struct EmitParams
{
	EmitParams() = default;

	EmitParams( const char* pszName ) :
		name(pszName)
	{
	}

	EmitParams( const char* pszName, int flags ) :
		name(pszName),
		flags(flags)
	{
	}

	EmitParams( const char* pszName, float volume, float pitch ) :
		name(pszName),
		volume(volume), pitch(pitch)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos ) :
		name(pszName),
		origin(pos)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos, float volume, float pitch ) :
		name(pszName),
		origin(pos),
		volume(volume), pitch(pitch)
	{
	}

	EqString				name;
	Vector3D				origin{ vec3_zero };
	float					volume{ 1.0f };
	float					pitch{ 1.0f };
	float					radiusMultiplier{ 1.0f };

	int						flags{ 0 };
	int						sampleId{ -1 };
	ESoundChannelType		channelType{ CHAN_INVALID };
};

// Sound channel entity that controls it's sound sources
class CSoundingObject
{
	friend class CSoundEmitterSystem;
public:
	static int DecodeId(int idWaveId, int& waveId);
	static int EncodeId(int id, int waveId);

	CSoundingObject() = default;
	virtual ~CSoundingObject();

	int			EmitSound(int id, EmitParams* ep);

	void		StopEmitter(int idWaveId);
	void		PauseEmitter(int idWaveId);
	void		PlayEmitter(int idWaveId, bool rewind = false);

	void		StopLoop(int idWaveId);

	void		SetPosition(int idWaveId, const Vector3D& position);
	void		SetVelocity(int idWaveId, const Vector3D& velocity);
	void		SetPitch(int idWaveId, float pitch);
	void		SetVolume(int idWaveId, float volume);

	void		SetParams(int idWaveId, IEqAudioSource::Params& params);

	int			GetChannelSoundCount(ESoundChannelType chan) const { return m_numChannelSounds[chan]; }

	void		SetSoundVolumeScale(float fScale)	{ m_volumeScale = fScale; }
	float		GetSoundVolumeScale() const			{ return m_volumeScale; }

protected:
	bool		UpdateEmitters(const Vector3D& listenerPos);
	void		StopFirstEmitterByChannel(ESoundChannelType chan);

	// sounds at channel counter
	Map<int, SoundEmitterData*>	m_emitters{ PP_SL };

	uint8						m_numChannelSounds[CHAN_COUNT]{ 0 };
	float						m_volumeScale{ 1.0f };
};

//-------------------------------------------------------------------------------------


// the sound emitter system
class CSoundEmitterSystem
{
	friend class CSoundingObject;
public:
	static void cmd_vars_sounds_list(const ConCommandBase* base, Array<EqString>& list, const char* query);

	CSoundEmitterSystem();
	~CSoundEmitterSystem();

	void				Init(float maxDistance);
	void				Shutdown();

	void				LoadScriptSoundFile(const char* fileName);
	void				CreateSoundScript(const KVSection* scriptSection);

	void				PrecacheSound(const char* pszName);
	int					EmitSound(EmitParams* emit);

	void				StopAllSounds();

	void				StopAllEmitters();

	void				Update();

private:
	int					EmitSound(EmitParams* emit, CSoundingObject* soundingObj, int objUniqueId);

	SoundScriptDesc*	FindSound(const char* soundName) const;
	void				RemoveSoundingObject(CSoundingObject* obj);

	static int			EmitterUpdateCallback(void* obj, IEqAudioSource::Params& params);
	static int			LoopSourceUpdateCallback(void* obj, IEqAudioSource::Params& params);

	bool				SwitchSourceState(SoundEmitterData* emit, bool isVirtual);

	Map<int, SoundScriptDesc*>		m_allSounds{ PP_SL };
	Set<CSoundingObject*>			m_soundingObjects{ PP_SL };
	Array<EmitParams>				m_pendingStartSounds{ PP_SL };

	float							m_defaultMaxDistance{ 100.0f };

	bool							m_isInit{ false };
};

extern CSoundEmitterSystem* g_sounds;

#define PrecacheScriptSound(snd) g_sounds->PrecacheSound(snd)