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
		SoundEmitterData* emitter = *it;
		if (emitter->soundSource)
			g_audioSystem->DestroySource(emitter->soundSource);

		delete emitter;
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
		IEqAudioSource::Params& virtualParams = emitter->virtualParams;

		if (emitter->soundSource != nullptr)
		{
			needDelete = virtualParams.releaseOnStop && (emitter->soundSource->GetState() == IEqAudioSource::STOPPED);
		}
		else
		{
			const SoundScriptDesc* script = emitter->script;

			bool isAudible = true;
			if (!virtualParams.relative)
			{
				const Vector3D listenerPos = g_audioSystem->GetListenerPosition();

				const float distToSound = lengthSqr(virtualParams.position - listenerPos);
				const float maxDistSqr = M_SQR(script->maxDistance);
				isAudible = distToSound < maxDistSqr;
			}

			// switch emitter between virtual and real here
			if (g_sounds->SwitchSourceState(emitter, !isAudible))
			{
				needDelete = virtualParams.releaseOnStop && emitter->soundSource == nullptr;
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

void CSoundingObject::StopLoop(int uniqueId, float fadeOutTime) 
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		StopLoop(*it, fadeOutTime);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		StopLoop(*it, fadeOutTime);
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

void CSoundingObject::SetConeProperties(int uniqueId, const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetConeProperties(*it, direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetConeProperties(*it, direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
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

void CSoundingObject::SetSamplePlaybackPosition(int uniqueId, int waveId, float seconds)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetSamplePlaybackPosition(*it, waveId, seconds);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetSamplePlaybackPosition(*it, waveId, seconds);
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

void CSoundingObject::SetInputValue(int uniqueId, const char* name, float value)
{
	const int inputNameHash = StringToHash(name);
	SetInputValue(uniqueId, inputNameHash, value);
}

void CSoundingObject::SetInputValue(int uniqueId, int inputNameHash, float value)
{
	if (uniqueId != ID_ALL)
	{
		const auto it = m_emitters.find(uniqueId);
		if (it == m_emitters.end())
			return;

		SetInputValue(*it, inputNameHash, value);
		return;
	}

	for (auto it = m_emitters.begin(); it != m_emitters.end(); ++it)
		SetInputValue(*it, inputNameHash, value);
}

void CSoundingObject::SetEmitterState(SoundEmitterData* emitter, IEqAudioSource::State state, bool rewindOnPlay)
{
	if (!emitter)
		return;

	emitter->stopLoopRemainingTime = 0.0f;
	emitter->stopLoopTime = 0.0f;
	emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);

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

	emitter->stopLoopRemainingTime = 0.0f;
	emitter->stopLoopTime = 0.0f;
	emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);

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

void CSoundingObject::StopLoop(SoundEmitterData* emitter, float fadeOutTime)
{
	if (!emitter)
		return;

	if (fadeOutTime < F_EPS)
		fadeOutTime = emitter->script->stopLoopTime;

	if (fadeOutTime > F_EPS && emitter->stopLoopTime <= 0.0f)
	{
		emitter->stopLoopTime = fadeOutTime;
		emitter->stopLoopRemainingTime = fadeOutTime;
	}
	
	if(fadeOutTime <= 0.0f)
	{
		// set it directly
		IEqAudioSource::Params param;
		param.set_looping(false);

		if (emitter->soundSource)
			emitter->soundSource->UpdateParams(param);
	}
}

void CSoundingObject::SetPosition(SoundEmitterData* emitter, const Vector3D& position)
{
	if (!emitter)
		return;
	emitter->virtualParams.set_position(position);
	emitter->nodeParams.set_position(position);
}

void CSoundingObject::SetVelocity(SoundEmitterData* emitter, const Vector3D& velocity)
{
	if (!emitter)
		return;
	emitter->virtualParams.set_velocity(velocity);
	emitter->nodeParams.set_velocity(velocity);
}

void CSoundingObject::SetConeProperties(SoundEmitterData* emitter, const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf)
{
	if (!emitter)
		return;

	IEqAudioSource::Params& nodeParams = emitter->nodeParams;
	IEqAudioSource::Params params;
	params.set_direction(direction);
	params.set_coneAngles(Vector2D(innerRadus, outerRadius));
	params.set_volume(Vector3D(nodeParams.volume.x, outerVolume, outerVolumeHf));

	emitter->nodeParams |= params;
}

void CSoundingObject::SetPitch(SoundEmitterData* emitter, float pitch)
{
	if (!emitter)
		return;

	emitter->epPitch = pitch;
	IEqAudioSource::Params& nodeParams = emitter->nodeParams;
	nodeParams.updateFlags |= IEqAudioSource::UPDATE_PITCH;
}

void CSoundingObject::SetVolume(SoundEmitterData* emitter, float volume)
{
	if (!emitter)
		return;

	emitter->epVolume = volume;
	IEqAudioSource::Params& nodeParams = emitter->nodeParams;
	nodeParams.updateFlags |= IEqAudioSource::UPDATE_VOLUME;
}

void CSoundingObject::SetSamplePlaybackPosition(SoundEmitterData* emitter, int waveId, float seconds)
{
	if (!emitter)
		return;

	// update virtual params
	// see CSoundEmitterSystem::EmitterUpdateCallback
	if (waveId == -1)
	{
		for(int i = 0; i < MAX_SOUND_SAMPLES_SCRIPT; ++i)
			emitter->samplePos[i] = seconds;
	}
	else
		emitter->samplePos[waveId] = seconds;
}

void CSoundingObject::SetSampleVolume(SoundEmitterData* emitter, int waveId, float volume)
{
	if (!emitter)
		return;

	// update virtual params
	// see CSoundEmitterSystem::EmitterUpdateCallback
	if (waveId == -1)
	{
		for (int i = 0; i < MAX_SOUND_SAMPLES_SCRIPT; ++i)
			emitter->sampleVolume[i] = volume;
	}
	else
		emitter->sampleVolume[waveId] = volume;
}

void CSoundingObject::SetParams(SoundEmitterData* emitter, const IEqAudioSource::Params& params)
{
	if (!emitter)
		return;

	emitter->nodeParams |= params;

	const int excludeFlags = (IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_VOLUME | IEqAudioSource::UPDATE_REF_DIST);
	emitter->virtualParams.merge(params, params.updateFlags & ~excludeFlags);
}

void CSoundingObject::SetInputValue(SoundEmitterData* emitter, int inputNameHash, float value)
{
	if (!emitter)
		return;

	emitter->SetInputValue(inputNameHash, 0, value);
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

void CEmitterObjectSound::StopLoop(float fadeOutTime)
{
	m_soundingObj.StopLoop(m_emitter, fadeOutTime);
}

void CEmitterObjectSound::SetPosition(const Vector3D& position)
{
	m_soundingObj.SetPosition(m_emitter, position);
}

void CEmitterObjectSound::SetVelocity(const Vector3D& velocity)
{
	m_soundingObj.SetVelocity(m_emitter, velocity);
}

void CEmitterObjectSound::SetConeProperties(const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf)
{
	m_soundingObj.SetConeProperties(m_emitter, direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
}

void CEmitterObjectSound::SetPitch(float pitch)
{
	m_soundingObj.SetPitch(m_emitter, pitch);
}

void CEmitterObjectSound::SetVolume(float volume)
{
	m_soundingObj.SetVolume(m_emitter, volume);
}

void CEmitterObjectSound::SetSamplePlaybackPosition(int waveId, float seconds)
{
	m_soundingObj.SetSamplePlaybackPosition(m_emitter, waveId, seconds);
}

void CEmitterObjectSound::SetSampleVolume(int waveId, float volume)
{
	m_soundingObj.SetSampleVolume(m_emitter, waveId, volume);
}

void CEmitterObjectSound::SetParams(const IEqAudioSource::Params& params)
{
	m_soundingObj.SetParams(m_emitter, params);
}

void CEmitterObjectSound::SetInputValue(const char* name, float value)
{
	const int inputNameHash = StringToHash(name);
	SetInputValue(inputNameHash, value);
}

void CEmitterObjectSound::SetInputValue(int inputNameHash, float value)
{
	m_soundingObj.SetInputValue(m_emitter, inputNameHash, value);
}