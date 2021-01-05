//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Sound system
//
//				Ambient sound
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSND_STREAM_H
#define ALSND_STREAM_H

#include "ISoundSystem.h"
#include "IFileSystem.h"

#define AMB_STREAM_BUFFERS 4

class DkSoundAmbient : public ISoundPlayable
{
public:
						DkSoundAmbient();
						~DkSoundAmbient();

	ESoundState			GetState() const;

	void				Play();
	void				Stop();
	void				StopLoop();

	void				Pause();

	void				Update();

	void				SetVolume(float val);
	void				SetPitch(float val);

	void				SetSample(ISoundSample* sample);
	ISoundSample*		GetSample() const;

	DkSoundSampleLocal*	m_sample;

protected:

	void				OpenStreamFile(const char *name);

	void				ResetStream();

	void				UpdateStreaming();

	void				StartStreaming();
	void				StopStreaming();

	bool				UploadStream(ALuint buffer);
	void				EmptyStreamQueue();

//

	OggVorbis_File		m_oggStream;
	ALuint				m_buffers[AMB_STREAM_BUFFERS];

	IFile*				m_oggFile;
	vorbis_info*		m_vorbisInfo;

	ALenum				m_format;
	ALuint				m_alSource;

	float				m_volume;
	float				m_pitch;

	bool				m_playing : 1;
	bool				m_loaded : 1;
	bool				m_ready : 1;
};

#endif