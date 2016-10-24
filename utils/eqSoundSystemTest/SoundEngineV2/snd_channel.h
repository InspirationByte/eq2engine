//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2017
//////////////////////////////////////////////////////////////////////////////////
// Description: Eq sound engine channel
//////////////////////////////////////////////////////////////////////////////////

#ifndef SND_CHANNEL_H
#define SND_CHANNEL_H

#include "soundinterface.h"
#include "snd_defs.h"

class ISoundSource;

class CSoundChannel : public ISoundChannel
{
public:
	CSoundChannel();

	virtual bool        IsPlaying () { return m_bPlaying; }
	virtual bool        IsLooping () { return m_bLooping; }

	virtual int         PlaySound(int nSound, bool bLooping);
	virtual int         PlaySound(int nSound);
	virtual int         PlayLoop(int nSound);

	virtual void        StopSound ();

	virtual void        SetOrigin (const Vector3D& vOrigin)		{ m_vOrigin = vOrigin; }
	virtual void        SetVolume (float flVolume)				{ m_flVolume = flVolume; }
	virtual void        SetPitch(float flPitch)					{ m_flPitch = flPitch; }
	virtual void        SetAttenuation (float flAttn)			{ m_flAttenuation = flAttn; }

	void				SetReserved (bool b) { m_bReserved = b; }
	bool				IsReserved () { return m_bReserved; }

	void				MixChannel(paintbuffer_t *pBuffer, int nSamples);
private:
    ISoundSource*		m_pSound;

    Vector3D			m_vOrigin;
    
    bool				m_bPlaying;
    bool				m_bLooping;
    float				m_flAttenuation;
    float				m_flPitch;
    float				m_flVolume;

    float				m_nSamplePos;
    bool				m_bReserved;

	typedef void (CSoundChannel::*MIXFUNC)(void *pBuffer, float flRate, int nVolume, int nSamples);
	MIXFUNC				m_sourceMixer;

    void				m_mixMono16(void *pBuffer, float flRate, int nVolume, int nSamples);
    void				m_mixStereo16(void *pBuffer, float flRate, int nVolume, int nSamples);

    void				m_SpatializeMono(int in, int *out);
    void				m_SpatializeStereo(int in, int out[2]);
};

#endif // SND_CHANNEL_H