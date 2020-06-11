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

typedef int VoiceHandle_t;
#define VOICE_INVALID_HANDLE (-1)

enum EVoiceUpdateFlags
{
	VOICE_UPDATE_POSITION		= (1 << 0),
	VOICE_UPDATE_VELOCITY		= (1 << 1),
	VOICE_UPDATE_VOLUME			= (1 << 2),
	VOICE_UPDATE_PITCH			= (1 << 3),
	VOICE_UPDATE_REF_DIST		= (1 << 4),
	VOICE_UPDATE_AIRABSORPTION	= (1 << 5),
	VOICE_UPDATE_RELATIVE		= (1 << 6),
	VOICE_UPDATE_STATE			= (1 << 7),
	VOICE_UPDATE_LOOPING		= (1 << 8),

	VOICE_UPDATE_DO_REWIND			= (1 << 16),
	VOICE_UPDATE_RELEASE_ON_STOP	= (1 << 17)
};

enum EVoiceState
{
	VOICE_STATE_STOPPED = 0,
	VOICE_STATE_PLAYING,
	VOICE_STATE_PAUSED
};

struct voiceParams_t
{
	Vector3D			position;
	Vector3D			velocity;
	float				volume;					// [0.0, 1.0]
	float				pitch;					// [0.0, 100.0]
	float				referenceDistance;
	float				rolloff;
	float				airAbsorption;
	EVoiceState			state;
	bool				relative;
	bool				looping;
	bool				releaseOnStop;
	VoiceHandle_t		id;						// read-only
};

typedef int (*VoiceUpdateCallback)(void* obj, voiceParams_t& params);		// returns EVoiceUpdateFlags

//-----------------------------------------------------------------

#define STREAM_BUFFER_COUNT		(4)
#define STREAM_BUFFER_SIZE		(1024*8) // 8 kb

struct AudioVoice_t
{
	ALuint					m_buffers[STREAM_BUFFER_COUNT];
	ISoundSource*			m_sample;
	VoiceUpdateCallback		m_callback;
	void*					m_callbackObject;
	ALuint					m_source;
	int						m_streamPos;
	EVoiceState				m_state;

	VoiceHandle_t			m_id;
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

	VoiceHandle_t			GetFreeVoice(ISoundSource* sample, void* callbackObject, VoiceUpdateCallback fnCallback);
	void					ReleaseVoice(VoiceHandle_t voice);

	void					GetVoiceParams(VoiceHandle_t handle, voiceParams_t& params);
	void					UpdateVoice(VoiceHandle_t handle, voiceParams_t params, int mask);

private:
	void					SuspendVoicesWithSample(ISoundSource* sample);

	bool					InitContext();
	void					DestroyContext();

	bool					QueueStreamVoice(AudioVoice_t& voice, ALuint buffer);
	void					SetupVoice(AudioVoice_t& voice, ISoundSource* sample);

	void					EmptyBuffers(AudioVoice_t& voice);
	void					DoVoiceUpdate(AudioVoice_t& voice);

	DkList<AudioVoice_t>	m_voices;
	DkList<ISoundSource*>	m_samples;

	ALCcontext*				m_ctx;
	ALCdevice*				m_dev;

	bool					m_noSound;
};

#endif EQSOUNDSYSTEMAL_H