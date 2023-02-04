//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"

//-----------------------------------------------------------------

#define EQSND_MIXER_CHANNELS			16
#define EQSND_STREAM_BUFFER_COUNT		4
#define EQSND_SAMPLE_COUNT				8
#define EQSND_STREAM_BUFFER_SIZE		(1024*8) // 8 kb

//-----------------------------------------------------------------

struct KVSection;
class CEqAudioSourceAL;

// Audio system, controls voices
class CEqAudioSystemAL : public IEqAudioSystem
{
	friend class CEqAudioSourceAL;
public:
	CEqAudioSystemAL();
	~CEqAudioSystemAL();

	void						Init();
	void						Shutdown();

	CRefPtr<IEqAudioSource>		CreateSource();
	void						DestroySource(IEqAudioSource* source);

	void						BeginUpdate();
	void						EndUpdate();

	void						StopAllSounds(int chanId = -1);
	void						PauseAllSounds(int chanId = -1);
	void						ResumeAllSounds(int chanId = -1);

	void						ResetMixer(int chanId);
	void						SetChannelVolume(int chanId, float value);
	void						SetChannelPitch(int chanId, float value);

	void						SetMasterVolume(float value);

	// sets listener properties
	void						SetListener(const Vector3D& position,
											const Vector3D& velocity,
											const Vector3D& forwardVec,
											const Vector3D& upVec);

	const Vector3D&				GetListenerPosition() const;

	// loads sample source data
	CRefPtr<ISoundSource>		GetSample(const char* filename);
	void						OnSampleDeleted(ISoundSource* sample);

	// finds the effect. May return EFFECT_ID_NONE
	audioEffectId_t					FindEffect(const char* name) const;

	// sets the new effect
	void						SetEffect(int slot, audioEffectId_t effect);
	int							GetEffectSlotCount() const;

private:
	struct sndEffect_t
	{
		char		name[32];
		ALuint		nAlEffect;
	};

	struct MixerChannel_t
	{
		float		volume{ 1.0f };
		float		pitch{ 1.0f };

		// TODO: set lowPass / highPass there too

		int			updateFlags{ 0 }; // IAudioSource::Update enum
	};

	bool			CreateALEffect(const char* pszName, KVSection* pSection, sndEffect_t& effect);
	void			SuspendSourcesWithSample(ISoundSource* sample);

	bool			InitContext();
	void			InitEffects();

	void			DestroyContext();
	void			DestroyEffects();

	FixedArray<MixerChannel_t, EQSND_MIXER_CHANNELS>	m_mixerChannels;

	FixedArray<ALuint, EQSND_EFFECT_SLOTS>	m_effectSlots;
	Array<CRefPtr<CEqAudioSourceAL>>		m_sources{ PP_SL };	// tracked sources
	Map<int, ISoundSource*>					m_samples{ PP_SL };
	Map<int, sndEffect_t>					m_effects{ PP_SL };

	struct Listener
	{
		Vector3D position{ vec3_zero };
		Vector3D velocity{ vec3_zero };
		Vector3D orientF{ vec3_forward };
		Vector3D orientU{ vec3_up };
	} m_listener;

	ALCcontext*								m_ctx{ nullptr };
	ALCdevice*								m_dev{ nullptr };
	bool									m_noSound{ true };
	bool									m_begunUpdate{ false };
};

//-----------------------------------------------------------------
// Sound source

class CEqAudioSourceAL : public IEqAudioSource
{
	friend class CEqAudioSystemAL;
public:
	CEqAudioSourceAL(CEqAudioSystemAL* owner);
	~CEqAudioSourceAL();

	void					Ref_DeleteObject();

	void					Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback = nullptr);
	void					Setup(int chanId, ArrayCRef<const ISoundSource*> samples, UpdateCallback fnCallback = nullptr);
	void					Release();

	// full scale
	void					GetParams(Params& params) const;
	void					UpdateParams(const Params& params, int overrideUpdateFlags = -1);

	void					SetSamplePlaybackPosition(int sourceIdx, float seconds);
	float					GetSamplePlaybackPosition(int sourceIdx);
	void					SetSampleVolume(int sourceIdx, float volume);
	float					GetSampleVolume(int sourceIdx);
	int						GetSampleCount() const;

	// atomic
	State					GetState() const { return m_state; }
	bool					IsLooping() const { return m_looping; }

	ALsizei					GetSampleBuffer(void* data, ALsizei size);

protected:

	struct SourceStream
	{
		ISoundSource*	sample{ nullptr };
		int				curPos{ 0 };
		float			volume{ 1.0f };
	};

	bool					QueueStreamChannel(ALuint buffer);
	void					SetupSample(const ISoundSource* sample);
	void					SetupSamples(ArrayCRef<const ISoundSource*> samples);

	SourceStream&			GetSourceStream() { return m_streams.front(); }
	bool					IsStreamed() const;

	void					InitSource();

	void					EmptyBuffers();
	bool					DoUpdate();

	CEqAudioSystemAL*		m_owner{ nullptr };

	FixedArray<SourceStream, EQSND_SAMPLE_COUNT> m_streams;

	ALuint					m_buffers[EQSND_STREAM_BUFFER_COUNT]{ 0 };
	ALuint					m_bufferChannels{ 0 };
	ALuint					m_source{ 0 };
	ALuint					m_filter{ 0 };

	UpdateCallback			m_callback;

	State					m_state{ State::STOPPED };

	Vector3D				m_volume{ 1.0f };			// need them raw and unaffected by mixer params
	float					m_pitch{ 1.0f };

	int						m_channel{ -1 };			// mixer channel index
	bool					m_releaseOnStop{ true };
	bool					m_forceStop{ false };
	bool					m_looping{ false };
};
