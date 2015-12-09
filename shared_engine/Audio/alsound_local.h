//////////////////////////////////////////////////////////////////////////////////
// Copyright � Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Sound system//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSOUND_LOCAL_H
#define ALSOUND_LOCAL_H

#include "al/al.h"
#include "al/alc.h"
#include "al/alext.h"

#ifdef _WIN32
#include "vorbis/vorbisfile.h"
#else
#include <vorbis/vorbisfile.h>
#endif // _WIN32

#include "soundzero.h"

#include "alsnd_sample.h"
#include "alsnd_emitter.h"
#include "alsnd_stream.h"

//#include "EFX-Util.h"

#define AL_FORMAT_VORBIS_EXT			0x10003

struct sndEffect_t
{
	char		name[128];

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

	bool					IsInitialized();

	//-----------------------------------------------------------------------------------------

	// sets volume (not affecting on ambient sources)
	void					SetVolumeScale(float vol_scale);

	// sets the pause state
	void					SetPauseState(bool pause);
	bool					GetPauseState();

	void					SetListener(const Vector3D& position,
												const Vector3D& forwardVec,
												const Vector3D& upVec,
												const Vector3D& velocity,
												sndEffect_t* pEffect = NULL);

	Vector3D				GetListenerPosition();

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

	ISoundSample*			LoadSample(const char *name, bool streaming, bool looping = false, int nFlags = 0);
	ISoundSample*			FindSampleByName( const char *name );
	void					ReleaseSample(ISoundSample *pSample);

	void					ReleaseEmitters();
	void					ReleaseSamples();

public:

	void					ReloadEffects();

protected:

	void					InitEffects();

	int						RequestChannel(DkSoundEmitterLocal* emitter);

protected:

	Vector3D				m_listenerOrigin;
	Vector3D				m_listenerDir;

	soundParams_t			m_defaultParams;

	bool					m_bSoundInit;

	bool					m_pauseState;

	DkList<ISoundEmitter*>	m_pSoundEmitters;
	DkList<ISoundSample*>	m_pSoundSamples;
	DkList<sndChannel_t*>	m_pChannels;
	DkList<DkSoundAmbient*>	m_pAmbients;
	DkList<sndEffect_t>		m_effects;

	ALCcontext*				m_ctx;
	ALCdevice*				m_dev;

	ALuint					m_nEffectSlots[2];
	int						m_nCurrentSlot;

	sndEffect_t*			m_pCurrentEffect;

	float					m_fVolumeScale;

	CEqMutex				m_Mutex;
};

#endif // ALSOUND_LOCAL_H
