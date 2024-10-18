//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Sound Emitter System
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"
#include "eqSoundEmitterCommon.h"

struct SoundScriptDesc;
struct SoundEmitterData;

struct EmitParams;
class ConCommandBase;

class CEmitterObjectSound;
class CSoundScriptEditor;

static constexpr const int s_loopRemainTimeFactorNameHash = StringIdConst24("loopRemainTimeFactor");

// Sound channel entity that controls it's sound sources
class CSoundingObject : public WeakRefObject<CSoundingObject>
{
	friend class CSoundEmitterSystem;
	friend class CEmitterObjectSound;
	friend class CSoundScriptEditor;
public:
	CSoundingObject() = default;
	virtual ~CSoundingObject();

	static constexpr const int ID_RANDOM = -1;		// only used in EmitSound
	static constexpr const int ID_ALL = 0x80000000; // only used for parameter setup

	int			EmitSound(int uniqueId, EmitParams* ep);

	const IEqAudioSource::State GetEmitterState(int uniqueId) const;
	void						SetEmitterState(int uniqueId, IEqAudioSource::State state, bool rewindOnPlay = false);

	bool		HasEmitter(int uniqueId) const;

	// returns sample id that was in EmitParams
	int			GetEmitterSampleId(int uniqueId) const;

	// updates sample id. will Stop and again Play automatically on change
	void		SetEmitterSampleId(int uniqueId, int sampleId);

	void		StopEmitter(int uniqueId, bool destroy = false);
	void		PlayEmitter(int uniqueId, bool rewind = false);
	void		PauseEmitter(int uniqueId);
	void		StopLoop(int uniqueId, float fadeOutTime = 0.0f);
	
	// WARNING: SetPitch and SetVolume changes only the value that was passed through EmitParams

	void		SetPitch(int uniqueId, float pitch);
	void		SetVolume(int uniqueId, float volume);

	void		SetPosition(int uniqueId, const Vector3D& position);
	void		SetVelocity(int uniqueId, const Vector3D& velocity);
	void		SetConeProperties(int uniqueId, const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf = 1.0f);

	void		SetSamplePlaybackPosition(int uniqueId, int waveId, float seconds);
	void		SetSampleVolume(int uniqueId, int waveId, float volume);
	void		SetParams(int uniqueId, const IEqAudioSource::Params& params);

	void		SetInputValue(int uniqueId, const char* name, float value);
	void		SetInputValue(int uniqueId, int inputNameHash, float value);

	int			GetChannelSoundCount(int chan) const { return m_numChannelSounds[chan]; }

	void		SetSoundVolumeScale(float fScale)	{ m_volumeScale = fScale; }
	float		GetSoundVolumeScale() const			{ return m_volumeScale; }

protected:
	SoundEmitterData*	FindEmitter(int uniqueId) const;
	void		AddEmitter(int uniqueId, SoundEmitterData* emitter);

	void		SetEmitterState(SoundEmitterData* emitter, IEqAudioSource::State state, bool rewindOnPlay);

	void		StopEmitter(SoundEmitterData* emitter, bool destroy);
	void		PauseEmitter(SoundEmitterData* emitter);
	void		PlayEmitter(SoundEmitterData* emitter, bool rewind);

	void		StartLoop(SoundEmitterData* emitter, float fadeInTime = 0.0f);
	void		StopLoop(SoundEmitterData* emitter, float fadeOutTime = 0.0f);

	void		SetPosition(SoundEmitterData* emitter, const Vector3D& position);
	void		SetVelocity(SoundEmitterData* emitter, const Vector3D& velocity);
	void		SetConeProperties(SoundEmitterData* emitter, const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf = 1.0f);

	void		SetPitch(SoundEmitterData* emitter, float pitch);
	void		SetVolume(SoundEmitterData* emitter, float volume);

	void		SetSamplePlaybackPosition(SoundEmitterData* emitter, int waveId, float seconds);
	void		SetSampleVolume(SoundEmitterData* emitter, int waveId, float volume);
	void		SetParams(SoundEmitterData* emitter, const IEqAudioSource::Params& params);

	void		SetInputValue(SoundEmitterData* emitter, int inputNameHash, float value);

	bool		UpdateEmitters(const Vector3D& listenerPos);
	void		StopFirstEmitterByChannel(int chan);

	void		FlushOldEmitters();

	// sounds at channel counter
	Map<int, SoundEmitterData*>	m_emitters{ PP_SL };
	SoundEmitterData*	m_deleteList{ nullptr };

	Threading::CEqMutex	m_mutex;
	uint8				m_numChannelSounds[CHAN_MAX]{ 0 };
	float				m_volumeScale{ 1.0f };
};

class CEmitterObjectSound
{
public:
	// TODO: support automatically starting the sound
	//		 by providing EmitParams to constructor

	CEmitterObjectSound(CSoundingObject& soundingObj, int uniqueId);

	// returns sample id that was in EmitParams
	int			GetEmitterSampleId() const;

	// updates sample id. will Stop and again Play automatically on change
	void		SetEmitterSampleId(int sampleId);

	const IEqAudioSource::State GetEmitterState() const;
	void		SetEmitterState(IEqAudioSource::State state, bool rewindOnPlay = false);

	void		StopEmitter();
	void		PlayEmitter(bool rewind = false);
	void		PauseEmitter();

	void		StartLoop(float fadeInTime = 0.0f);
	void		StopLoop(float fadeOutTime = 0.0f);

	void		SetPosition(const Vector3D& position);
	void		SetVelocity(const Vector3D& velocity);
	void		SetConeProperties(const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf = 1.0f); 

	void		SetPitch(float pitch);
	void		SetVolume(float volume);

	void		SetSamplePlaybackPosition(int waveId, float seconds);
	void		SetSampleVolume(int waveId, float volume);
	void		SetParams(const IEqAudioSource::Params& params);

	void		SetInputValue(const char* name, float value);
	void		SetInputValue(int inputNameHash, float value);
private:
	CSoundingObject&			m_soundingObj;
	CWeakPtr<SoundEmitterData>	m_emitter{ nullptr };
};