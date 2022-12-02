#include "core/core_common.h"
#include "math/Random.h"

#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"
#include "eqSoundEmitterSystem.h"

using namespace Threading;

#pragma optimize("", off)

CSoundingObject::~CSoundingObject()
{
	g_sounds->RemoveSoundingObject(this);

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		StopEmitter(*it, true);
		/*
		SoundEmitterData* emitter = *it;
		if (emitter->soundSource)
		{
			g_audioSystem->DestroySource(emitter->soundSource);
			emitter->soundSource = nullptr;
		}
		delete emitter;
		*/
	}
}

int CSoundingObject::EmitSound(int uniqueId, EmitParams* ep)
{
	const bool isRandom = uniqueId == -1;
	if (isRandom)
	{
		// ensure that no collisions happen
		do {
			uniqueId = RandomInt(0, StringHashMask);
		} while (m_emitters.contains(uniqueId));
	}

	return g_sounds->EmitSound(ep, this, uniqueId & StringHashMask, isRandom);
}

bool CSoundingObject::UpdateEmitters(const Vector3D& listenerPos)
{
	// update emitters manually if they are in virtual state
	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		bool needDelete = false;
		SoundEmitterData* emitter = *it;
		IEqAudioSource::Params& params = emitter->virtualParams;

		if (emitter->soundSource != nullptr)
		{
			needDelete = params.releaseOnStop && (emitter->soundSource->GetState() == IEqAudioSource::STOPPED);
		}
		else
		{
			const SoundScriptDesc* script = emitter->script;

			const float distToSound = lengthSqr(params.position - listenerPos);
			const float maxDistSqr = M_SQR(script->maxDistance);

			// switch emitter between virtual and real here
			if (g_sounds->SwitchSourceState(emitter, distToSound > maxDistSqr))
			{
				needDelete = params.releaseOnStop && emitter->soundSource == nullptr;
			}
		}

		if(needDelete)
		{
			StopEmitter(emitter, true);
			/*
			g_audioSystem->DestroySource(emitter->soundSource);
			emitter->soundSource = nullptr;

			if(emitter->channelType != CHAN_INVALID)
				--m_numChannelSounds[emitter->channelType];

			delete emitter;
			*/
			m_emitters.remove(it);
		}
	}

	return m_emitters.size() > 0;
}

void CSoundingObject::StopFirstEmitterByChannel(int chan)
{
	if (chan == CHAN_INVALID)
		return;

	CScopedMutex m(s_soundEmitterSystemMutex);

	// find first sound with the specific channel and kill it
	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		SoundEmitterData* emitter = *it;
		if (emitter->channelType == chan)
		{
			StopEmitter(emitter, true);
			/*
			if (chan != CHAN_INVALID)
				--m_numChannelSounds[chan];

			if (emitter->soundSource)
			{
				g_audioSystem->DestroySource(emitter->soundSource);
				emitter->soundSource = nullptr;
			}
			delete emitter;
			*/
			m_emitters.remove(it);
			break;
		}
	}
}

#if 0
int CSoundingObject::DecodeId(int idWaveId, int& waveId)
{
	if (waveId & 0x80000000) // needs a 'SET' flag
		waveId = idWaveId >> StringHashBits & 127;

	return idWaveId & StringHashMask;
}

int CSoundingObject::EncodeId(int id, int waveId)
{
	if (waveId == -1)
		return id;

	return (id & StringHashMask) | (waveId << StringHashBits) | 0x80000000;
}
#endif

void CSoundingObject::StopEmitter(int uniqueId, bool destroy /*= false*/)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		{
			CScopedMutex m(s_soundEmitterSystemMutex);
			StopEmitter(*it, destroy);

			if (destroy)
				m_emitters.remove(it);
		}
		return;
	}

	{
		CScopedMutex m(s_soundEmitterSystemMutex);
		for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
			StopEmitter(*it, destroy);

		if(destroy)
			m_emitters.clear();
	}
}

void CSoundingObject::PauseEmitter(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		PauseEmitter(*it);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		PauseEmitter(*it);
}

void CSoundingObject::PlayEmitter(int uniqueId, bool rewind /*= false*/)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		PlayEmitter(*it, rewind);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		PlayEmitter(*it, rewind);
}

void CSoundingObject::StopLoop(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		StopLoop(*it);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		StopLoop(*it);
}

void CSoundingObject::SetPosition(int uniqueId, const Vector3D& position)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetPosition(*it, position);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetPosition(*it, position);
}

void CSoundingObject::SetVelocity(int uniqueId, const Vector3D& velocity)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetVelocity(*it, velocity);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetVelocity(*it, velocity);
}

void CSoundingObject::SetPitch(int uniqueId, float pitch)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetPitch(*it, pitch);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetPitch(*it, pitch);
}

void CSoundingObject::SetVolume(int uniqueId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetVolume(*it, volume);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetVolume(*it, volume);
}

void CSoundingObject::SetSampleVolume(int uniqueId, int waveId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetSampleVolume(*it, waveId, volume);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetSampleVolume(*it, waveId, volume);
}

void CSoundingObject::SetParams(int uniqueId, IEqAudioSource::Params& params)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetParams(*it, params);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetParams(*it, params);
}

void CSoundingObject::StopEmitter(SoundEmitterData* emitter, bool destroy)
{
	if (destroy)
	{
		if (emitter->channelType != CHAN_INVALID)
			--m_numChannelSounds[emitter->channelType];

		if (emitter->soundSource)
		{
			g_audioSystem->DestroySource(emitter->soundSource);
			emitter->soundSource = nullptr;
		}
		delete emitter;

		return;
	}

	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::STOPPED);
	SetParams(emitter, param);
}

void CSoundingObject::PauseEmitter(SoundEmitterData* emitter)
{
	IEqAudioSource::Params param;
	param.set_state(IEqAudioSource::PAUSED);
	SetParams(emitter, param);
}

void CSoundingObject::PlayEmitter(SoundEmitterData* emitter, bool rewind)
{
	// TODO: check if not playing already

	IEqAudioSource::Params param;
	if (emitter->virtualParams.state != IEqAudioSource::PLAYING)
		param.set_state(IEqAudioSource::PLAYING);

	// update virtual params
	emitter->virtualParams |= param;

	// update actual params
	if (emitter->soundSource)
	{
		if(rewind)
			param.updateFlags |= IEqAudioSource::UPDATE_DO_REWIND;

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::StopLoop(SoundEmitterData* emitter)
{
	IEqAudioSource::Params param;
	param.set_looping(false);
	SetParams(emitter, param);
}

void CSoundingObject::SetPosition(SoundEmitterData* emitter, const Vector3D& position)
{
	IEqAudioSource::Params param;
	param.set_position(position);
	SetParams(emitter, param);
}

void CSoundingObject::SetVelocity(SoundEmitterData* emitter, const Vector3D& velocity)
{
	IEqAudioSource::Params param;
	param.set_velocity(velocity);
	SetParams(emitter, param);
}

void CSoundingObject::SetPitch(SoundEmitterData* emitter, float pitch)
{
	const float pitchLevel = emitter->startParams.pitch * pitch;

	// update virtual params
	emitter->virtualParams.set_pitch(pitchLevel);

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource::Params param;
		param.set_pitch(pitchLevel);

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::SetVolume(SoundEmitterData* emitter, float volume)
{
	const float volumeLevel = emitter->startParams.volume * volume;

	// update virtual params
	emitter->virtualParams.set_volume(volumeLevel);

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource::Params param;
		param.set_volume(volumeLevel * GetSoundVolumeScale());

		emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::SetSampleVolume(SoundEmitterData* emitter, int waveId, float volume)
{
	// TODO: update virtual params

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource* source = emitter->soundSource;
		source->SetSampleVolume(waveId, volume);
	}
}

void CSoundingObject::SetParams(SoundEmitterData* emitter, IEqAudioSource::Params& params)
{
	// update virtual params
	emitter->virtualParams |= params;

	// update actual params
	if (emitter->soundSource)
		emitter->soundSource->UpdateParams(params);
}