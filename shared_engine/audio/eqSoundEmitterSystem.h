//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve'Source)
//////////////////////////////////////////////////////////////////////////////////

/*
	TODO:
		Virtual emitters in SES instead of in audio system
		Sound channel specifics for DrvSyn
		Sound controller interface specifics for DrvSyn
		Spatial optimizations to query with less distance checks
*/

#pragma once

#include "audio/IEqAudioSystem.h"

struct soundScriptDesc_t;
struct SoundEmitterData;
class CSoundingObject;

// flags
enum EEmitSoundFlags
{
	EMITSOUND_FLAG_OCCLUSION		= (1 << 0),		// occludes source by the world geometry
	EMITSOUND_FLAG_ROOM_OCCLUSION	= (1 << 1),		// uses more expensive occlusion system, use it for ambient sounds
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
	// the first and biggest
	EmitParams() = default;

	EmitParams( const char* pszName ) :
		name(pszName),
		flags(EMITSOUND_FLAG_OCCLUSION)
	{
	}

	EmitParams( const char* pszName, int flags ) :
		name(pszName),
		flags(flags)
	{
	}

	EmitParams( const char* pszName, float volume, float pitch ) :
		name(pszName),
		volume(volume), pitch(pitch),
		flags(EMITSOUND_FLAG_OCCLUSION)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos ) :
		name(pszName),
		origin(pos),
		flags(EMITSOUND_FLAG_OCCLUSION)
	{
	}

	EmitParams( const char* pszName, const Vector3D& pos, float volume, float pitch ) :
		name(pszName),
		origin(pos),
		volume(volume), pitch(pitch),
		flags(EMITSOUND_FLAG_OCCLUSION)
	{
	}

	EqString				name;
	Vector3D				origin{ 0.0f };
	float					volume{ 1.0f };
	float					pitch{ 1.0f };
	float					radiusMultiplier{ 1.0f };

	CSoundingObject*		object{ nullptr };

	int						flags{ 0 };
	int						sampleId{ -1 };
	ESoundChannelType		channelType{ CHAN_INVALID };
};


// Sound channel entity that controls it's sound sources
class CSoundingObject
{
	friend class CSoundEmitterSystem;
public:
	CSoundingObject();
	virtual ~CSoundingObject();

	// emit sound with parameters
	void		EmitSound(EmitParams* ep);

	int			GetChannelSoundCount(ESoundChannelType chan);

	void		SetSoundVolumeScale(float fScale) { m_volumeScale = fScale; }
	float		GetSoundVolumeScale() const { return m_volumeScale; }

protected:
	int			FirstEmitterIdxByChannelType(ESoundChannelType chan) const;
	void		StopFirstEmitter(ESoundChannelType chan);

	// sounds at channel counter
	Array<SoundEmitterData*>	m_emitters{ PP_SL };
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

	void				SetPaused(bool paused);
	bool				IsPaused();

	void				PrecacheSound(const char* pszName);							// precaches sound

	// emits new sound. returns channel type
	int					EmitSound(EmitParams* emit );								// emits sound with specified parameters

	void				StopAllSounds();

	void				StopAllEmitters();

	void				Update(float pitchScale = 1.0f, bool force = false);													// updates sound emitter system

private:
	soundScriptDesc_t*	FindSound(const char* soundName) const;						// searches for loaded script sound
	const ISoundSource*	GetBestSample(const soundScriptDesc_t* script, int sampleId = -1) const;

	void				InvalidateSoundChannelObject(CSoundingObject* pEnt);

	static int			EmitterUpdateCallback(void* obj, IEqAudioSource::Params& params);
	static int			LoopSourceUpdateCallback(void* obj, IEqAudioSource::Params& params);

	bool				SwitchSourceState(SoundEmitterData* emit, bool isVirtual);

	Map<int, soundScriptDesc_t*>	m_allSounds{ PP_SL };

	float							m_defaultMaxDistance{ 100.0f };

	Set<CSoundingObject*>			m_soundingObjects{ PP_SL };

	Array<EmitParams>				m_pendingStartSounds{ PP_SL };

	bool							m_isInit{ false };
	bool							m_isPaused{ false };
};

extern CSoundEmitterSystem* g_sounds;

#define PrecacheScriptSound(snd) g_sounds->PrecacheSound(snd)