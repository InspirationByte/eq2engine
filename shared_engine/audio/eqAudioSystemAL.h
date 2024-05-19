//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"

// this allows us to mix between samples in single source
// and also eliminates few reallocations to just copy to single AL buffer
#define USE_ALSOFT_BUFFER_CALLBACK		1

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
	ISoundSourcePtr				GetSample(const char* filename);
	void						AddSample(ISoundSource* sample);
	void						OnSampleDeleted(ISoundSource* sample);

	// finds the effect. May return EFFECT_ID_NONE
	AudioEffectId				FindEffect(const char* name) const;

	// sets the new effect
	void						SetEffect(int slot, AudioEffectId effect);
	int							GetEffectSlotCount() const;

	void						UpdateDeviceHRTF();

private:
	struct EffectInfo
	{
		char		name[32];
		ALuint		nAlEffect;
	};

	struct MixerChannel
	{
		// TODO: set lowPass / highPass there too

		float		volume{ 1.0f };
		float		pitch{ 1.0f };
		int			updateFlags{ 0 }; // IAudioSource::Update enum
	};

	using ContextParamsList = FixedArray<int, 32>;
	using MixerChannelList = FixedArray<MixerChannel, EQSND_MIXER_CHANNELS>;
	using EffectSlotList = FixedArray<ALuint, EQSND_EFFECT_SLOTS>;
	using SourceList = Array<CRefPtr<CEqAudioSourceAL>>;
	using SampleList = Map<int, ISoundSource*>;
	using EffectsList = Map<int, EffectInfo>;

	bool			CreateALEffect(const char* pszName, KVSection* pSection, EffectInfo& effect);
	void			SuspendSourcesWithSample(ISoundSource* sample);

	void			GetContextParams(ContextParamsList& paramsList) const;

	bool			InitContext();
	void			InitEffects();

	void			DestroyContext();
	void			DestroyEffects();

	MixerChannelList		m_mixerChannels;

	EffectSlotList			m_effectSlots;
	SourceList				m_sources{ PP_SL };	// tracked sources
	SampleList				m_samples{ PP_SL };
	EffectsList				m_effects{ PP_SL };

	struct Listener
	{
		Vector3D position{ vec3_zero };
		Vector3D velocity{ vec3_zero };
		Vector3D orientF{ vec3_forward };
		Vector3D orientU{ vec3_up };
	} m_listener;

	ALCcontext*				m_ctx{ nullptr };
	ALCdevice*				m_dev{ nullptr };
	bool					m_noSound{ true };
	bool					m_begunUpdate{ false };
};
