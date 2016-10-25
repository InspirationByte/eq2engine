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
#include "snd_mix.h"

class ISoundSource;

class CSoundChannel : public ISoundChannel
{
public:
	CSoundChannel();

	bool				IsPlaying () { return m_bPlaying; }
	bool				IsLooping () { return m_bLooping; }

	int					PlaySound(int nSound, bool bLooping);
	int					PlaySound(int nSound);
	int					PlayLoop(int nSound);

	void				StopSound();

	void				SetOrigin(const Vector3D& vOrigin)		{ m_vOrigin = vOrigin; }
	void				SetVolume(float flVolume)				{ m_flVolume = flVolume; }
	void				SetPitch(float flPitch)					{ m_flPitch = flPitch; }
	void				SetAttenuation (float flAttn)			{ m_flAttenuation = flAttn; }

	const Vector3D&		GetOrigin() const {return m_vOrigin;}
	float				GetVolume() const {return m_flVolume;}
	float				GetPitch() const {return m_flPitch;}
	float				GetAttenuation() const {return m_flAttenuation;}

	void				SetReserved(bool b) { m_bReserved = b; }
	bool				IsReserved() { return m_bReserved; }

	void				MixChannel(paintbuffer_t* input, paintbuffer_t* output, int numSamples);
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

	S_MIXFUNC			m_sourceMixer;
	S_SPATIALFUNC		m_spatialize;
};

#endif // SND_CHANNEL_H