#include "core/core_common.h"
#include "math/Random.h"

#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"
#include "eqSoundEmitterSystem.h"

using namespace Threading;

CSoundingObject::~CSoundingObject()
{
	g_sounds->OnRemoveSoundingObject(this);

	{
		CScopedMutex m(m_mutex);
		for (auto emitter: m_emitters)
		{
			g_audioSystem->DestroySource(emitter->soundSource);
			delete emitter;
		}
		FlushOldEmitters();
	}
}

void CSoundingObject::FlushOldEmitters()
{
	SoundEmitterData* del = m_deleteList;
	while(del)
	{
		SoundEmitterData* tmp = del;
		del = del->delNext;

		delete tmp;
	}
	m_deleteList = nullptr;
}

int CSoundingObject::EmitSound(int uniqueId, EmitParams* ep)
{
	const bool isRandom = uniqueId == -1;
	if (isRandom)
	{
		CScopedMutex m(m_mutex);
		// ensure that no collisions happen
		do {
			uniqueId = RandomInt(0, StringHashMask);
		} while (m_emitters.contains(uniqueId));
	}

	ep->flags |= isRandom ? EMITSOUND_FLAG_RELEASE_ON_STOP : 0;

	return g_sounds->EmitSoundInternal(ep, uniqueId & StringHashMask, this);
}

bool CSoundingObject::UpdateEmitters(const Vector3D& listenerPos)
{
	CScopedMutex m(m_mutex);

	// update emitters manually if they are in virtual state
	for (auto it = m_emitters.begin(); !it.atEnd(); ++it)
	{
		bool needDelete = false;
		SoundEmitterData* emitter = *it;
		IEqAudioSource::Params& virtualParams = emitter->virtualParams;
		const SoundScriptDesc* script = emitter->script;

		if (emitter->soundSource != nullptr)
		{
			needDelete = virtualParams.releaseOnStop && (emitter->soundSource->GetState() == IEqAudioSource::STOPPED);
		}
		else
		{
			bool isAudible = true;
			if (!virtualParams.relative)
			{
				const float distToSound = lengthSqr(virtualParams.position - listenerPos);
				const float maxDistSqr = M_SQR(script->maxDistance);
				isAudible = distToSound < maxDistSqr;
			}

			if (!isAudible && virtualParams.releaseOnStop)
			{
				needDelete = true;
			}
			else
			{
				// switch emitter between virtual and real here
				if (g_sounds->SwitchSourceState(emitter, !isAudible))
					needDelete = emitter->soundSource == nullptr;
			}
		}

		if(needDelete)
		{
			StopEmitter(emitter, true);
			m_emitters.remove(it);
		}
	}

	FlushOldEmitters();

	return m_emitters.size() > 0;
}

void CSoundingObject::StopFirstEmitterByChannel(int chan)
{
	if (chan == CHAN_INVALID)
		return;

	CScopedMutex m(m_mutex);

	// find first sound with the specific channel and kill it
	for (auto it = m_emitters.begin(); !it.atEnd(); ++it)
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

bool CSoundingObject::HasEmitter(int uniqueId) const 
{
	SoundEmitterData* emitter = FindEmitter(uniqueId);
	return emitter;
}

int	CSoundingObject::GetEmitterSampleId(int uniqueId) const
{
	SoundEmitterData* emitter = FindEmitter(uniqueId);

	if (!emitter)
		return -1;

	return emitter->sampleId;
}

void CSoundingObject::SetEmitterSampleId(int uniqueId, int sampleId)
{
	SoundEmitterData* emitter = FindEmitter(uniqueId);

	if (emitter && emitter->sampleId != sampleId)
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
	SoundEmitterData* emitter = FindEmitter(uniqueId);
	if (!emitter)
		return IEqAudioSource::STOPPED;

	return emitter->virtualParams.state;
}

void CSoundingObject::SetEmitterState(int uniqueId, IEqAudioSource::State state, bool rewindOnPlay)
{
	if (uniqueId != ID_ALL)
	{
		SetEmitterState(FindEmitter(uniqueId), state, rewindOnPlay);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetEmitterState(emitter, state, rewindOnPlay);
	}
}

void CSoundingObject::StopEmitter(int uniqueId, bool destroy /*= false*/)
{
	if (uniqueId != ID_ALL)
	{
		CScopedMutex m(m_mutex);
		const auto it = m_emitters.find(uniqueId);
		if (it.atEnd())
			return;

		SoundEmitterData* emitter = *it;

		if (destroy)
			m_emitters.remove(it);

		StopEmitter(emitter, destroy);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			StopEmitter(emitter, destroy);
	}

	if (destroy)
	{
		CScopedMutex m(m_mutex);
		m_emitters.clear();
	}
}

void CSoundingObject::PauseEmitter(int uniqueId)
{
	if (uniqueId != ID_ALL)
	{
		PauseEmitter(FindEmitter(uniqueId));
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			PauseEmitter(emitter);
	}
}

void CSoundingObject::PlayEmitter(int uniqueId, bool rewind /*= false*/)
{
	if (uniqueId != ID_ALL)
	{
		PlayEmitter(FindEmitter(uniqueId), rewind);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			PlayEmitter(emitter, rewind);
	}
}

void CSoundingObject::StopLoop(int uniqueId, float fadeOutTime) 
{
	if (uniqueId != ID_ALL)
	{
		StopLoop(FindEmitter(uniqueId), fadeOutTime);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			StopLoop(emitter, fadeOutTime);
	}
}

void CSoundingObject::SetPosition(int uniqueId, const Vector3D& position)
{
	if (uniqueId != ID_ALL)
	{
		SetPosition(FindEmitter(uniqueId), position);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetPosition(emitter, position);
	}
}

void CSoundingObject::SetVelocity(int uniqueId, const Vector3D& velocity)
{
	if (uniqueId != ID_ALL)
	{
		SetVelocity(FindEmitter(uniqueId), velocity);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetVelocity(emitter, velocity);
	}
}

void CSoundingObject::SetConeProperties(int uniqueId, const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf)
{
	if (uniqueId != ID_ALL)
	{
		SetConeProperties(FindEmitter(uniqueId), direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetConeProperties(emitter, direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
	}
}

void CSoundingObject::SetPitch(int uniqueId, float pitch)
{
	if (uniqueId != ID_ALL)
	{
		CScopedMutex m(m_mutex);
		const auto it = m_emitters.find(uniqueId);
		if (it.atEnd())
			return;

		SetPitch(*it, pitch);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetPitch(emitter, pitch);
	}
}

void CSoundingObject::SetVolume(int uniqueId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		SetVolume(FindEmitter(uniqueId), volume);
		return;
	}

	for (auto emitter : m_emitters)
		SetVolume(emitter, volume);
}

void CSoundingObject::SetSampleVolume(int uniqueId, int waveId, float volume)
{
	if (uniqueId != ID_ALL)
	{
		SetSampleVolume(FindEmitter(uniqueId), waveId, volume);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetSampleVolume(emitter, waveId, volume);
	}
}

void CSoundingObject::SetSamplePlaybackPosition(int uniqueId, int waveId, float seconds)
{
	if (uniqueId != ID_ALL)
	{
		SetSamplePlaybackPosition(FindEmitter(uniqueId), waveId, seconds);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetSamplePlaybackPosition(emitter, waveId, seconds);
	}
}

void CSoundingObject::SetParams(int uniqueId, const IEqAudioSource::Params& params)
{
	if (uniqueId != ID_ALL)
	{
		SetParams(FindEmitter(uniqueId), params);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetParams(emitter, params);
	}
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
		SetInputValue(FindEmitter(uniqueId), inputNameHash, value);
		return;
	}

	{
		CScopedMutex m(m_mutex);
		for (auto emitter : m_emitters)
			SetInputValue(emitter, inputNameHash, value);
	}
}

SoundEmitterData* CSoundingObject::FindEmitter(int uniqueId) const
{
	{
		CScopedMutex m(*const_cast<CEqMutex*>(&m_mutex));

		const auto it = m_emitters.find(uniqueId);
		if (!it.atEnd())
			return *it;
	}
	return nullptr;
}

void CSoundingObject::AddEmitter(int uniqueId, SoundEmitterData* emitter)
{
	CScopedMutex m(m_mutex);
	auto itOld = m_emitters.find(uniqueId);
	if(!itOld.atEnd())
		StopEmitter(*itOld, true);

	m_emitters.insert(uniqueId, emitter);
}

void CSoundingObject::SetEmitterState(SoundEmitterData* emitter, IEqAudioSource::State state, bool rewindOnPlay)
{
	if (!emitter)
		return;

	if (emitter->virtualParams.state == state)
		return;

	if((emitter->loopCommand & 31) != LOOPCMD_NONE)
	{
		emitter->loopCommand = LOOPCMD_NONE | LOOPCMD_FLAG_CHANGED;
		emitter->loopCommandRatePerSecond = 0.0f;
		emitter->loopCommandTimeFactor = 1.0f;
		emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);
	}


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

		g_audioSystem->DestroySource(emitter->soundSource);

		{
			CScopedMutex m(m_mutex);

			emitter->delNext = m_deleteList;
			m_deleteList = emitter;
		}
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

	IEqAudioSource::Params param;

	bool isNotPlaying = emitter->virtualParams.state != IEqAudioSource::PLAYING;
	if (isNotPlaying || (emitter->loopCommand & 31) != LOOPCMD_NONE)
	{
		emitter->loopCommand = LOOPCMD_NONE | LOOPCMD_FLAG_CHANGED;
		emitter->loopCommandRatePerSecond = 0.0f;
		emitter->loopCommandTimeFactor = 1.0f;
		emitter->SetInputValue(s_loopRemainTimeFactorNameHash, 0, 1.0f);

		param.set_looping(emitter->script->loop);
	}

	// check if not playing already
	if (isNotPlaying)
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

void CSoundingObject::StartLoop(SoundEmitterData* emitter, float fadeInTime)
{
	if (!emitter)
		return;

	if (fadeInTime < F_EPS)
		fadeInTime = emitter->script->startLoopTime;

	const bool wasStopped = emitter->virtualParams.state == IEqAudioSource::STOPPED;
	const bool loopCmdChanged = (emitter->loopCommand & 31) != LOOPCMD_FADE_IN;

	if(wasStopped)
		PlayEmitter(emitter, false);

	if(loopCmdChanged)
	{
		if(emitter->soundSource)
		{
			IEqAudioSource::Params param;
			param.set_looping(emitter->script->loop);
			emitter->soundSource->UpdateParams(param);
		}

		emitter->loopCommandRatePerSecond = 1.0f / fadeInTime;
		emitter->loopCommand = LOOPCMD_FADE_IN | LOOPCMD_FLAG_CHANGED;

		if(wasStopped)
			emitter->loopCommandTimeFactor = 0.0f;
	}
}

void CSoundingObject::StopLoop(SoundEmitterData* emitter, float fadeOutTime)
{
	if (!emitter)
		return;

	if((emitter->loopCommand & 31) != LOOPCMD_FADE_OUT)
	{
		if (fadeOutTime < F_EPS)
			fadeOutTime = emitter->script->stopLoopTime;

		emitter->loopCommandRatePerSecond = 1.0f / fadeOutTime;
		emitter->loopCommand = LOOPCMD_FADE_OUT | LOOPCMD_FLAG_CHANGED;
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
	m_emitter = CWeakPtr(m_soundingObj.FindEmitter(uniqueId));
}

int CEmitterObjectSound::GetEmitterSampleId() const
{
	if (!m_emitter)
		return -1;

	return m_emitter->sampleId;
}

void CEmitterObjectSound::SetEmitterSampleId(int sampleId)
{
	if (!m_emitter)
		return;

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
	m_soundingObj.SetEmitterState(m_emitter.Ptr(), state, rewindOnPlay);
}

void CEmitterObjectSound::StopEmitter()
{
	m_soundingObj.StopEmitter(m_emitter.Ptr(), false);
}

void CEmitterObjectSound::PlayEmitter(bool rewind)
{
	m_soundingObj.PlayEmitter(m_emitter.Ptr(), rewind);
}

void CEmitterObjectSound::PauseEmitter()
{
	m_soundingObj.PauseEmitter(m_emitter.Ptr());
}

void CEmitterObjectSound::StartLoop(float fadeInTime)
{
	m_soundingObj.StartLoop(m_emitter.Ptr(), fadeInTime);
}

void CEmitterObjectSound::StopLoop(float fadeOutTime)
{
	m_soundingObj.StopLoop(m_emitter.Ptr(), fadeOutTime);
}

void CEmitterObjectSound::SetPosition(const Vector3D& position)
{
	m_soundingObj.SetPosition(m_emitter.Ptr(), position);
}

void CEmitterObjectSound::SetVelocity(const Vector3D& velocity)
{
	m_soundingObj.SetVelocity(m_emitter.Ptr(), velocity);
}

void CEmitterObjectSound::SetConeProperties(const Vector3D& direction, float innerRadus, float outerRadius, float outerVolume, float outerVolumeHf)
{
	m_soundingObj.SetConeProperties(m_emitter.Ptr(), direction, innerRadus, outerRadius, outerVolume, outerVolumeHf);
}

void CEmitterObjectSound::SetPitch(float pitch)
{
	m_soundingObj.SetPitch(m_emitter.Ptr(), pitch);
}

void CEmitterObjectSound::SetVolume(float volume)
{
	m_soundingObj.SetVolume(m_emitter.Ptr(), volume);
}

void CEmitterObjectSound::SetSamplePlaybackPosition(int waveId, float seconds)
{
	m_soundingObj.SetSamplePlaybackPosition(m_emitter.Ptr(), waveId, seconds);
}

void CEmitterObjectSound::SetSampleVolume(int waveId, float volume)
{
	m_soundingObj.SetSampleVolume(m_emitter.Ptr(), waveId, volume);
}

void CEmitterObjectSound::SetParams(const IEqAudioSource::Params& params)
{
	m_soundingObj.SetParams(m_emitter.Ptr(), params);
}

void CEmitterObjectSound::SetInputValue(const char* name, float value)
{
	const int inputNameHash = StringToHash(name);
	SetInputValue(inputNameHash, value);
}

void CEmitterObjectSound::SetInputValue(int inputNameHash, float value)
{
	m_soundingObj.SetInputValue(m_emitter.Ptr(), inputNameHash, value);
}
