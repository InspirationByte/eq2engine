//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2015
//////////////////////////////////////////////////////////////////////////////////
// Description: DarkTech Engine Sound system
//
//				Ambient sound
//////////////////////////////////////////////////////////////////////////////////

#ifndef ALSND_STREAM_H
#define ALSND_STREAM_H

#include "ISoundSystem.h"
#include "IFileSystem.h"

class DkSoundAmbient : public ISoundPlayable
{
public:
						DkSoundAmbient();
						~DkSoundAmbient();

	ESoundState			GetState() const;

	void				Play();
	void				Stop();
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

	float				m_volume;
	float				m_pitch;

	ALenum				m_format;
	ALuint				m_alSource;
	ALuint				m_buffer[2];
	
	bool				m_playing : 1;
	bool				m_loaded : 1;
	bool				m_ready : 1;

	IFile*				m_oggFile;
	OggVorbis_File		m_oggStream;

	vorbis_info*		m_vorbisInfo;
	//vorbis_comment*		m_vorbisComment;
	
};

#endif