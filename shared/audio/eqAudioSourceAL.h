#pragma once

#include "audio/IEqAudioSystem.h"

class CEqAudioSystemAL;
using SoundMixFunc = int (*)(const void* in, int numInSamples, void* out, int numOutSamples, float volume, float rate);

//-----------------------------------------------------------------
// Sound source

class CEqAudioSourceAL : public IEqAudioSource
{
	friend class CEqAudioSystemAL;
public:
	CEqAudioSourceAL(CEqAudioSystemAL* owner);
	~CEqAudioSourceAL();

	void				Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback = nullptr);
	void				Setup(int chanId, ArrayCRef<const ISoundSource*> samples, UpdateCallback fnCallback = nullptr);
	void				Release();

	// full scale
	void				GetParams(Params& params) const;
	void				UpdateParams(const Params& params, int overrideUpdateFlags = -1);

	void				SetSamplePlaybackPosition(int sourceIdx, float seconds);
	float				GetSamplePlaybackPosition(int sourceIdx) const;

	void				SetSampleVolume(int sourceIdx, float volume);
	float				GetSampleVolume(int sourceIdx) const;

	void				SetSamplePitch(int sourceIdx, float pitch);
	float				GetSamplePitch(int sourceIdx) const;

	int					GetSampleCount() const;

	// atomic
	State				GetState() const { return m_state; }
	bool				IsLooping() const { return m_looping; }

	ALsizei				GetSampleBuffer(void* data, ALsizei size);

protected:

	struct SourceStream
	{
		ISoundSource*	sample{ nullptr };
		SoundMixFunc	mixFunc{ nullptr };
		int				curPos{ 0 };
		float			volume{ 1.0f };
		float			pitch{ 1.0f };
	};
	using SourceStreamList = FixedArray<SourceStream, EQSND_SAMPLE_COUNT>;

	void				SetupStreamMixer(SourceStream& stream) const;

	bool				QueueStreamChannel(ALuint buffer);
	void				SetupSample(const ISoundSource* sample);
	void				SetupSamples(ArrayCRef<const ISoundSource*> samples);

	bool				IsStreamed() const;

	bool				InitSource();

	void				EmptyBuffers();
	bool				DoUpdate();

	CEqAudioSystemAL*	m_owner{ nullptr };

	SourceStreamList	m_streams;

	ALuint				m_buffers[EQSND_STREAM_BUFFER_COUNT]{ 0 };
	ALuint				m_source{ 0 };
	ALuint				m_filter{ 0 };

	UpdateCallback		m_callback;

	State				m_state{ State::STOPPED };

	Vector3D			m_volume{ 1.0f };			// need them raw and unaffected by mixer params
	float				m_pitch{ 1.0f };

	int					m_channel{ -1 };			// mixer channel index
	bool				m_releaseOnStop{ true };
	bool				m_forceStop{ false };
	bool				m_looping{ false };
};
