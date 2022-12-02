//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system (similar to Valve's Source)
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"
#include "eqSoundEmitterCommon.h"

struct SoundScriptDesc;
struct SoundEmitterData;

struct EmitParams;
class CSoundingObject;
class ConCommandBase;

// Sound channel entity that controls it's sound sources
class CSoundingObject
{
	friend class CSoundEmitterSystem;
public:
	CSoundingObject() = default;
	virtual ~CSoundingObject();

	static constexpr const int ID_RANDOM = -1;		// only used in EmitSound
	static constexpr const int ID_ALL = 0x80000000; // only used for parameter setup

	int			EmitSound(int uniqueId, EmitParams* ep);

	void		StopEmitter(int uniqueId);
	void		PauseEmitter(int uniqueId);
	void		PlayEmitter(int uniqueId, bool rewind = false);
	void		StopLoop(int uniqueId);

	void		SetPosition(int uniqueId, const Vector3D& position);
	void		SetVelocity(int uniqueId, const Vector3D& velocity);
	void		SetPitch(int uniqueId, float pitch);
	void		SetVolume(int uniqueId, float volume);

	void		SetSampleVolume(int uniqueId, int waveId, float volume);
	void		SetParams(int uniqueId, IEqAudioSource::Params& params);

	int			GetChannelSoundCount(int chan) const { return m_numChannelSounds[chan]; }

	void		SetSoundVolumeScale(float fScale)	{ m_volumeScale = fScale; }
	float		GetSoundVolumeScale() const			{ return m_volumeScale; }

protected:
	void		StopEmitter(SoundEmitterData* emitter);
	void		PauseEmitter(SoundEmitterData* emitter);
	void		PlayEmitter(SoundEmitterData* emitter, bool rewind = false);
	void		StopLoop(SoundEmitterData* emitter);

	void		SetPosition(SoundEmitterData* emitter, const Vector3D& position);
	void		SetVelocity(SoundEmitterData* emitter, const Vector3D& velocity);
	void		SetPitch(SoundEmitterData* emitter, float pitch);
	void		SetVolume(SoundEmitterData* emitter, float volume);

	void		SetSampleVolume(SoundEmitterData* emitter, int waveId, float volume);
	void		SetParams(SoundEmitterData* emitter, IEqAudioSource::Params& params);

	bool		UpdateEmitters(const Vector3D& listenerPos);
	void		StopFirstEmitterByChannel(int chan);

	// sounds at channel counter
	Map<int, SoundEmitterData*>	m_emitters{ PP_SL };

	uint8						m_numChannelSounds[CHAN_MAX]{ 0 };
	float						m_volumeScale{ 1.0f };
};