//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound system//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSOUND_LOCAL_H
#define ALSOUND_LOCAL_H

#include "soundzero.h"

#include "alsnd_sample.h"
#include "alsnd_emitter.h"
#include "alsnd_stream.h"

#define SOUND_EFX_SLOTS					2

struct sndEffect_t
{
	char		name[32];
	ALuint		nAlEffect;
};

struct sndChannel_t
{
	sndChannel_t()
	{
		onGamePause = false;
		alState = AL_STOPPED;
		lastPauseOffsetBytes = 0;
		emitter = NULL;
	}

	ALuint					alSource;
	ALenum					alState;

	bool					onGamePause;
	int						lastPauseOffsetBytes;

	DkSoundEmitterLocal*	emitter;
};

class DkSoundSystemLocal : public ISoundSystem
{
public:
	friend class			DkSoundEmitterLocal;
	friend class			DkSoundAmbient;

							DkSoundSystemLocal();

	void					Init();
	void					Update();
	void					Shutdown();

	bool					InitContext();
	void					ShutdownContext();

	bool					IsInitialized();

	//-----------------------------------------------------------------------------------------

	// sets the pause state
	void					SetPauseState(bool pause);
	bool					GetPauseState();

	void					SetListener(const Vector3D& position,
												const Vector3D& forwardVec,
												const Vector3D& upVec,
												const Vector3D& velocity,
												sndEffect_t* pEffect = NULL);

	const Vector3D&			GetListenerPosition() const;

	//-----------------------------------------------------------------------------------------

	// Effect host

	// searches for effect
	sndEffect_t*			FindEffect(const char* pszName);

	//-----------------------------------------------------------------------------------------

	// emitters
	ISoundPlayable*			GetStaticStreamChannel( int channel );
	int						GetNumStaticStreamChannels();

	ISoundEmitter*			AllocEmitter();
	void					FreeEmitter(ISoundEmitter* pEmitter);
	bool					IsValidEmitter(ISoundEmitter* pEmitter) const;

	ISoundSample*			LoadSample(const char *name, int nFlags = 0);
	ISoundSample*			FindSampleByName( const char *name );
	void					ReleaseSample(ISoundSample *pSample);

	void					ReleaseEmitters();
	void					ReleaseSamples();
	void					ReleaseEffects();

	void					ReloadEFX();
	void					PrintInfo();

protected:

	void					InitEFX();
	bool					CreateALEffect(const char* pszName, struct kvkeybase_t* pSection, struct sndEffect_t& effect);

	int						RequestChannel(DkSoundEmitterLocal* emitter);

	//------------------

	Vector3D				m_listenerOrigin;
	Vector3D				m_listenerDir;

	soundParams_t			m_defaultParams;

	bool					m_init;
	bool					m_pauseState;

	DkList<ISoundEmitter*>	m_emitters;
	DkList<ISoundSample*>	m_samples;
	DkList<sndChannel_t*>	m_channels;
	DkList<DkSoundAmbient*>	m_ambientStreams;
	DkList<sndEffect_t>		m_effects;

	ALCcontext*				m_ctx;
	ALCdevice*				m_dev;

	ALuint					m_effectSlots[SOUND_EFX_SLOTS];
	int						m_currEffectSlotIdx;

	sndEffect_t*			m_currEffect;

	float					m_masterVolume;
};

#endif // ALSOUND_LOCAL_H
