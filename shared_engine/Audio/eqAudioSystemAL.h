//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2020
//////////////////////////////////////////////////////////////////////////////////
// Description: Equilibrium Engine Audio system
//////////////////////////////////////////////////////////////////////////////////

#ifndef EQSOUNDSYSTEMAL_H
#define EQSOUNDSYSTEMAL_H

#include <AL/al.h>
#include <AL/alc.h>
#include "utils/DkList.h"

#include "math/Vector.h"
#include "source/snd_source.h"

//-----------------------------------------------------------------
// TODO: interface code

typedef int ChannelHandle_t;
#define CHANNEL_INVALID_HANDLE (-1)

enum EVoiceUpdateFlags
{
	AUDIO_CHAN_UPDATE_POSITION			= (1 << 0),
	AUDIO_CHAN_UPDATE_VELOCITY			= (1 << 1),
	AUDIO_CHAN_UPDATE_VOLUME			= (1 << 2),
	AUDIO_CHAN_UPDATE_PITCH				= (1 << 3),
	AUDIO_CHAN_UPDATE_REF_DIST			= (1 << 4),
	AUDIO_CHAN_UPDATE_AIRABSORPTION		= (1 << 5),
	AUDIO_CHAN_UPDATE_RELATIVE			= (1 << 6),
	AUDIO_CHAN_UPDATE_STATE				= (1 << 7),
	AUDIO_CHAN_UPDATE_LOOPING			= (1 << 8),

	AUDIO_CHAN_UPDATE_DO_REWIND			= (1 << 16),
	AUDIO_CHAN_UPDATE_RELEASE_ON_STOP	= (1 << 17)
};

enum EChannelState
{
	CHANNEL_STATE_STOPPED = 0,
	CHANNEL_STATE_PLAYING,
	CHANNEL_STATE_PAUSED
};

struct channelParams_t
{
	Vector3D			position;
	Vector3D			velocity;
	float				volume;					// [0.0, 1.0]
	float				pitch;					// [0.0, 100.0]
	float				referenceDistance;
	float				rolloff;
	float				airAbsorption;
	EChannelState		state;
	bool				relative;
	bool				looping;
	bool				releaseOnStop;
	ChannelHandle_t		id;						// read-only
};

typedef int (*ChannelUpdateCallback)(void* obj, channelParams_t& params);		// returns EVoiceUpdateFlags

//-----------------------------------------------------------------

#define STREAM_BUFFER_COUNT		(4)
#define STREAM_BUFFER_SIZE		(1024*8) // 8 kb

struct AudioChannel_t
{
	ALuint					m_buffers[STREAM_BUFFER_COUNT];
	ISoundSource*			m_sample;
	ChannelUpdateCallback	m_callback;
	void*					m_callbackObject;
	ALuint					m_source;
	int						m_streamPos;
	EChannelState			m_state;

	ChannelHandle_t			m_id;
	bool					m_releaseOnStop;
	bool					m_looping;
	
};

//-----------------------------------------------------------------

// Audio system, controls voices
class CEqAudioSystemAL
{
public:
	CEqAudioSystemAL();
	~CEqAudioSystemAL();

	void					Init();
	void					Shutdown();

	void					Update();

	void					SetMasterVolume(float value);

	// sets listener properties
	void					SetListener(const Vector3D& position,
										const Vector3D& velocity,
										const Vector3D& forwardVec,
										const Vector3D& upVec);

	// gets listener properties
	void					GetListener(Vector3D& position, Vector3D& velocity);

	// loads sample source data
	ISoundSource*			LoadSample(const char* filename);
	void					FreeSample(ISoundSource* sample);

	ChannelHandle_t			GetFreeChannel(ISoundSource* sample, void* callbackObject, ChannelUpdateCallback fnCallback);
	void					ReleaseChannel(ChannelHandle_t channel);

	void					GetChannelParams(ChannelHandle_t handle, channelParams_t& params);
	void					UpdateChannel(ChannelHandle_t handle, channelParams_t params, int mask);

private:
	void					SuspendChannelsWithSample(ISoundSource* sample);

	bool					InitContext();
	void					DestroyContext();

	bool					QueueStreamChannel(AudioChannel_t& channel, ALuint buffer);
	void					SetupChannel(AudioChannel_t& channel, ISoundSource* sample);

	void					EmptyBuffers(AudioChannel_t& channel);
	void					DoChannelUpdate(AudioChannel_t& channel);

	DkList<AudioChannel_t>	m_channels;
	DkList<ISoundSource*>	m_samples;

	ALCcontext*				m_ctx;
	ALCdevice*				m_dev;

	bool					m_noSound;
};

#endif EQSOUNDSYSTEMAL_H