//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: main control for any streaming sound output device
//////////////////////////////////////////////////////////////////////////////////

#include "math/Vector.h"
#include "dktypes.h"

#include "soundinterface.h"

#include "snd_dev.h"
#include "snd_channel.h"

//----------------------------------------------------------

typedef struct snd_link_s
{
	snd_link_s		*pNext, *pPrev;
	int				nSequence;      // registration sequence
	char			szFilename[128];
	int				nNumber;

	ISoundSource*	pSource;
} snd_link_t;

class CSoundEngine : public ISoundEngine
{
public:
	CSoundEngine();
	~CSoundEngine();

	void					Initialize(void* wndhandle);
	void					Shutdown();

	void					Update();

	void					SetListener(const Vector3D& origin, const Matrix3x3& orient);

	const ListenerInfo_t&	GetListener() const;

	void					PlaySound(int soundId, const Vector3D& origin, float volume, float attenuation);

	ISoundChannel*			AllocChannel(bool reserve = false);
	void					FreeChannel(ISoundChannel *pChan);

	void					StopAllSounds();
	void					ReleaseCache();

	//  registration
	int						PrecacheSound(const char* fileName);
	ISoundSource*			GetSound(int soundId);
	ISoundSource*			FindSound(const char* fileName);

private:
	//  mixing
	paintbuffer_t*			GetChannelBuffer() { return &m_channelBuffer; }

	void					MixChannels(paintbuffer_t* buffer, int numSamples);
	paintbuffer_t*			GetPaintBuffer(int size);

	//  spatialization
	ListenerInfo_t			m_listener;

	bool					m_initialized;
	ISoundDevice*			m_device;
    
	paintbuffer_t			m_paintBuffer;
	paintbuffer_t			m_channelBuffer;

	//
	//  sounds
	//

	snd_link_t				m_sampleChain;
	snd_link_t*				m_samples[MAX_SOUNDS];

	snd_link_t*				CreateSample(const char* fileName);
	snd_link_t*				FindSample(const char* fileName);
	void					DeleteSample(snd_link_t* link);

	//
	//  channels
	//

	CSoundChannel			m_channels[MAX_CHANNELS];
};