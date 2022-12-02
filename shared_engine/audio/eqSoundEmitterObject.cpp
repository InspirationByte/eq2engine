#include "core/core_common.h"
#include "math/Random.h"

#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"
#include "eqSoundEmitterSystem.h"

using namespace Threading;

CSoundingObject::~CSoundingObject()
{
	g_sounds->RemoveSoundingObject(this);

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
	{
		StopEmitter(*it, true);
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
			const bool isAudible = script->is2d || distToSound < maxDistSqr;

			// switch emitter between virtual and real here
			if (g_sounds->SwitchSourceState(emitter, !isAudible))
			{
				needDelete = params.releaseOnStop && emitter->soundSource == nullptr;
			}
		}

		if(needDelete)
		{
			StopEmitter(emitter, true);
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
			m_emitters.remove(it);
			break;
		}
	}
}

int	CSoundingObject::GetEmitterSampleId(int uniqueId) const
{
	const auto it = m_emitters.find(uniqueId);
	if (it == m_emitters.end())
		return -1;

	return it.value()->sampleId;
}

void CSoundingObject::SetEmitterSampleId(int uniqueId, int sampleId)
{
	const auto it = m_emitters.find(uniqueId);
	if (it == m_emitters.end())
		return;

	SoundEmitterData* emitter = *it;

	if (emitter->sampleId != sampleId)
	{
		emitter->sampleId = sampleId;

		if (emitter->virtualParams.state == IEqAudioSource::PLAYING)
		{
			// this will restart emitter safely
			g_sounds->SwitchSourceState(emitter, true);
			g_sounds->SwitchSourceState(emitter, false);
		}
	}
}

const IEqAudioSource::State CSoundingObject::GetEmitterState(int uniqueId) const
{
	const auto it = m_emitters.find(uniqueId);
	if (it == m_emitters.end())
		return IEqAudioSource::STOPPED;

	return it.value()->virtualParams.state;
}

void CSoundingObject::SetEmitterState(int uniqueId, IEqAudioSource::State state, bool rewindOnPlay)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetEmitterState(*it, state, rewindOnPlay);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetEmitterState(*it, state, rewindOnPlay);
}

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

void CSoundingObject::SetParams(int uniqueId, const IEqAudioSource::Params& params)
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

void CSoundingObject::SetEmitterState(SoundEmitterData* emitter, IEqAudioSource::State state, bool rewindOnPlay)
{
	if (!emitter)
		return;

	if (emitter->virtualParams.state == state)
		return;

	IEqAudioSource::Params param;
	param.set_state(state);

	if (rewindOnPlay && state == IEqAudioSource::PLAYING)
		param.updateFlags |= IEqAudioSource::UPDATE_DO_REWIND;

	// update virtual params
	emitter->virtualParams |= param;

	if (emitter->soundSource)
		emitter->soundSource->UpdateParams(param);
}

void CSoundingObject::StopEmitter(SoundEmitterData* emitter, bool destroy)
{
	if (!emitter)
		return;

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
	if (!emitter)
		return;

	// check if not playing already
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
	if (!emitter)
		return;

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
	if (!emitter)
		return;

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
	if (!emitter)
		return;

	// TODO: update virtual params

	// update actual params
	if (emitter->soundSource)
	{
		IEqAudioSource* source = emitter->soundSource;
		source->SetSampleVolume(waveId, volume);
	}
}

void CSoundingObject::SetParams(SoundEmitterData* emitter, const IEqAudioSource::Params& params)
{
	if (!emitter)
		return;

	// update virtual params
	emitter->virtualParams |= params;

	// update actual params
	if (emitter->soundSource)
		emitter->soundSource->UpdateParams(params);
}

//----------------------------------------

CEmitterObjectSound::CEmitterObjectSound(CSoundingObject& soundingObj, int uniqueId)
	: m_soundingObj(soundingObj)
{
	auto it = m_soundingObj.m_emitters.find(uniqueId);
	if (it != m_soundingObj.m_emitters.end())
		m_emitter = *it;
}

int CEmitterObjectSound::GetEmitterSampleId() const
{
	if (!m_emitter)
		return -1;
	return m_emitter->sampleId;
}

void CEmitterObjectSound::SetEmitterSampleId(int sampleId)
{
	if (m_emitter->sampleId == sampleId)
		return;

	m_emitter->sampleId = sampleId;

	if (m_emitter->virtualParams.state == IEqAudioSource::PLAYING)
	{
		// this will restart emitter safely
		g_sounds->SwitchSourceState(m_emitter, true);
		g_sounds->SwitchSourceState(m_emitter, false);
	}
}

const IEqAudioSource::State CEmitterObjectSound::GetEmitterState() const
{
	if (!m_emitter)
		return IEqAudioSource::STOPPED;
	return m_emitter->virtualParams.state;
}

void CEmitterObjectSound::SetEmitterState(IEqAudioSource::State state, bool rewindOnPlay)
{
	m_soundingObj.SetEmitterState(m_emitter, state, rewindOnPlay);
}

void CEmitterObjectSound::StopEmitter(bool destroy)
{
	m_soundingObj.StopEmitter(m_emitter, destroy);
}

void CEmitterObjectSound::PlayEmitter(bool rewind)
{
	m_soundingObj.PlayEmitter(m_emitter, rewind);
}

void CEmitterObjectSound::PauseEmitter()
{
	m_soundingObj.PauseEmitter(m_emitter);
}

void CEmitterObjectSound::StopLoop()
{
	m_soundingObj.StopLoop(m_emitter);
}

void CEmitterObjectSound::SetPosition(const Vector3D& position)
{
	m_soundingObj.SetPosition(m_emitter, position);
}

void CEmitterObjectSound::SetVelocity(const Vector3D& velocity)
{
	m_soundingObj.SetVelocity(m_emitter, velocity);
}

void CEmitterObjectSound::SetPitch(float pitch)
{
	m_soundingObj.SetPitch(m_emitter, pitch);
}

void CEmitterObjectSound::SetVolume(float volume)
{
	m_soundingObj.SetVolume(m_emitter, volume);
}

void CEmitterObjectSound::SetSampleVolume(int waveId, float volume)
{
	m_soundingObj.SetSampleVolume(m_emitter, waveId, volume);
}

void CEmitterObjectSound::SetParams(const IEqAudioSource::Params& params)
{
	m_soundingObj.SetParams(m_emitter, params);
}
