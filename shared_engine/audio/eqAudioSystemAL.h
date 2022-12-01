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
#define EQSND_STREAM_BUFFER_SIZE		(1024*8) // 8 kb

//-----------------------------------------------------------------

struct KVSection;

// Audio system, controls voices
class CEqAudioSystemAL : public IEqAudioSystem
{
	friend class CEqAudioSourceAL;
public:
	CEqAudioSystemAL();
	~CEqAudioSystemAL();

	void						Init();
	void						Shutdown();

	IEqAudioSource*				CreateSource();
	void						DestroySource(IEqAudioSource* source);

	void						BeginUpdate();
	void						EndUpdate();

	void						StopAllSounds(int chanId = -1, void* callbackObject = nullptr);
	void						PauseAllSounds(int chanId = -1, void* callbackObject = nullptr);
	void						ResumeAllSounds(int chanId = -1, void* callbackObject = nullptr);

	void						SetChannelVolume(int chanId, float value);
	void						SetChannelPitch(int chanId, float value);

	void						SetMasterVolume(float value);

	// sets listener properties
	void						SetListener(const Vector3D& position,
											const Vector3D& velocity,
											const Vector3D& forwardVec,
											const Vector3D& upVec);

	// gets listener properties
	void						GetListener(Vector3D& position, Vector3D& velocity);

	// loads sample source data
	ISoundSource*				LoadSample(const char* filename);
	void						FreeSample(ISoundSource* sample);

	// finds the effect. May return EFFECT_ID_NONE
	effectId_t					FindEffect(const char* name) const;

	// sets the new effect
	void						SetEffect(int slot, effectId_t effect);

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
	CEqAudioSourceAL(int typeId, ISoundSource* sample, UpdateCallback fnCallback, void* callbackObject);
	~CEqAudioSourceAL();

	void					Setup(int chanId, const ISoundSource* sample, UpdateCallback fnCallback = nullptr, void* callbackObject = nullptr);
	void					Release();

	// full scale
	void					GetParams(Params& params) const;
	void					UpdateParams(const Params& params, int mask = 0);

	// atomic
	State					GetState() const { return m_state; }
	bool					IsLooping() const { return m_looping; }

	ALsizei					GetSampleBuffer(void* data, ALsizei size);

protected:

	bool					QueueStreamChannel(ALuint buffer);
	void					SetupSample(const ISoundSource* sample);

	void					InitSource();

	void					EmptyBuffers();
	bool					DoUpdate();

	CEqAudioSystemAL*		m_owner{ nullptr };

	ALuint					m_buffers[EQSND_STREAM_BUFFER_COUNT]{ 0 };
	ISoundSource*			m_sample{ nullptr };
	ALuint					m_source{ 0 };
	int						m_streamPos{ 0 };

	UpdateCallback			m_callback;
	void*					m_callbackObject{ nullptr };

	State					m_state{ State::STOPPED };

	float					m_volume{ 1.0f };			// need them raw and unaffected by mixer params
	float					m_pitch{ 1.0f };

	int						m_channel{ -1 };			// mixer channel index
	bool					m_releaseOnStop{ true };
	bool					m_forceStop{ false };
	bool					m_looping{ false };
};
