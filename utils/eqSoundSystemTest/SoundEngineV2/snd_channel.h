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
	friend class CSoundEngine;

public:
	CSoundChannel();

	bool				IsPlaying () { return m_playbackState; }
	bool				IsLooping () { return m_looping; }

	int					PlaySound(int nSound, bool bLooping);
	int					PlaySound(int nSound);
	int					PlayLoop(int nSound);

	void				StopSound();

	void				SetOrigin(const Vector3D& vOrigin)		{ m_origin = vOrigin; }
	void				SetVolume(float flVolume)				{ m_volume = flVolume; }
	void				SetPitch(float flPitch)					{ m_pitch = flPitch; }
	void				SetAttenuation (float flAttn)			{ m_attenuation = flAttn; }

	const Vector3D&		GetOrigin() const {return m_origin;}
	float				GetVolume() const {return m_volume;}
	float				GetPitch() const {return m_pitch;}
	float				GetAttenuation() const {return m_attenuation;}

	void				SetReserved(bool b) { m_reserved = b; }
	bool				IsReserved() { return m_reserved; }

	void				MixChannel(paintbuffer_t* input, paintbuffer_t* output, int numSamples);
private:
	bool				SetupSource(ISoundSource* source);

	ISoundSource*		m_source;

	ISoundEngine*		m_owner;

	Vector3D			m_origin;
    
	bool				m_playbackState;
	bool				m_looping;
	float				m_attenuation;
	float				m_pitch;
	float				m_volume;

	float				m_playbackSamplePos;
	bool				m_reserved;

	S_MIXFUNC			m_sourceMixer;
	S_SPATIALFUNC		m_spatialize;
};

#endif // SND_CHANNEL_H