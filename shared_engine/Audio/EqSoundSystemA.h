//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2014
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium engine sound system
//				OpenAL version
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQSOUNDSYSTEMA_H
#define EQSOUNDSYSTEMA_H

#include "IEqSoundSystem.h"

#include "utils/DkList.h"
#include "utils/eqthread.h"

#include "al/al.h"
#include "al/alc.h"
#include "al/alext.h"
#include "vorbis/vorbisfile.h"
#include "EFX-Util.h"

using namespace Threading;

class CEqSoundEmitterA;

struct sndchannel_t
{
	unsigned int		alSource;
	ALenum				alState;

	// other params
	bool				onGamePause;
	int					lastPauseOffsetBytes;

	CEqSoundEmitterA*	emitter;
};

// sound engine
class CEqSoundSystemA : public IEqSoundSystem
{
	friend class CEqSoundEmitterA;
	friend class CEqSoundSampleA;
public:
							CEqSoundSystemA();
							~CEqSoundSystemA();

	void					Initialize();
	void					Shutdown();

	void					ReleaseEmitters();
	void					ReleaseSamples();

	bool					IsInitialized() const;

	//---------------------

	// samples
	IEqSoundSample*			LoadSample(const char* name, int nFlags = 0);
	void					FreeSample(IEqSoundSample* pSample);

	// emitters
	IEqSoundEmitter*		AllocSoundEmitter();
	void					FreeSoundEmitter(IEqSoundEmitter* pEmitter);

	// searches for effect
	IEqSoundEffect*			FindEffect( const char* pszName ) const;
	
	//---------------------

	// updates sound system
	void					Update(float fDt, float fGlobalPitch = 1.0f);

	// listener's
	void					SetListener(const Vector3D& origin, 
										const Vector3D& zForward, 
										const Vector3D& yUp, 
										const Vector3D& velocity, 
										IEqSoundEffect* pEffect = NULL);

	Vector3D				GetListenerPosition();

	// sets volume (not affecting on ambient sources)
	void					SetVolumeScale( float vol_scale );

protected:

	Vector3D					m_listenerOrigin;
	emitterparams_t				m_defaultParams;

	bool						m_soundInit;

	DkList<IEqSoundEmitter*>	m_emitters;
	DkList<IEqSoundSample*>		m_samples;
	DkList<sndchannel_t*>		m_channels;

	DkList<IEqSoundEffect*>		m_effects;

	ALCcontext*					m_alContext;
	ALCdevice*					m_alDevice;

	ALuint						m_nEffectSlots[2];
	int							m_nCurrentSlot;

	IEqSoundEffect*				m_pCurrentEffect;

	float						m_fVolumeScale;

	CEqMutex					m_Mutex;
};

#endif // EQSOUNDSYSTEMA_H