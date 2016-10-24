//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine
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
    CSoundEngine ()		{ m_Chain.pNext = m_Chain.pPrev = &m_Chain; Init( ); }
    ~CSoundEngine ()	{ Shutdown( ); }

    int						Init();
    int						Shutdown();

    virtual void			InitDevice(void* wndhandle);
    virtual void			DestroyDevice();

    virtual void			Update();

    virtual void			SetListener(const Vector3D& vOrigin, const Vector3D& vForward, const Vector3D& vRight, const Vector3D& vUp);

    virtual void			PlaySound(int nIndex, const Vector3D& vOrigin, float flVolume, float flAttenuation);

    virtual ISoundChannel*	AllocChannel(bool reserve = false);
    virtual void			FreeChannel(ISoundChannel *pChan);

    //  memory

    void*					HeapAlloc (unsigned int size)	{ return alloc(size); }
    void					HeapFree (void *ptr)			{ free(ptr); }

    //  registration

    int						PrecacheSound(const char *szFilename);
    ISoundSource*			GetSound(int nSound) { if (m_Sounds[nSound]) return m_Sounds[nSound]->pSource; return NULL; }

    //  mixing

    void					MixStereo16 (samplepair_t *pInput, stereo16_t *pOutput, int nSamples, int nVolume);
    paintbuffer_t*			GetChannelBuffer () { return &m_channelBuffer; }

    //  spatialization

	struct ListenerData {
		Vector3D			origin, forward, right, up;
	} m_listener;

private:
    bool					m_bInitialized;

    ISoundDevice*			pAudioDevice;
    
    paintbuffer_t*			GetPaintBuffer(int nBytes);
    paintbuffer_t			m_paintBuffer;
    paintbuffer_t			m_channelBuffer;

    //
    //  memory
    //

    void*					m_hHeap;
    void*					alloc (unsigned int size);
    void					free (void *ptr);

    //
    //  sounds
    //

    snd_link_t				m_Chain;
    snd_link_t*				m_Sounds[MAX_SOUNDS];

    snd_link_t*				Create(const char *szFilename);
    snd_link_t*				Find(const char *szFilename);
    void					Delete(snd_link_t *pLink);

    //
    //  channels
    //

    void					m_mixChannels(paintbuffer_t *pBuffer, int nSamples);
    CSoundChannel			m_Channels[MAX_CHANNELS];
};

extern CSoundEngine* gSound;
