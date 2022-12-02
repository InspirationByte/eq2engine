#pragma once
#include "audio/IEqAudioSystem.h"
#include "eqSoundEmitterCommon.h"

static Threading::CEqMutex s_soundEmitterSystemMutex;

class CSoundingObject;

struct SoundScriptDesc
{
	EqString				name;

	Array<ISoundSource*>	samples{ PP_SL };
	Array<EqString>			soundFileNames{ PP_SL };

	int						channelType{ CHAN_INVALID };

	float		volume{ 1.0f };
	float		atten{ 1.0f };
	float		rolloff{ 1.0f };
	float		pitch{ 1.0f };
	float		airAbsorption{ 0.0f };
	float		maxDistance{ 1.0f };

	bool		loop : 1;
	bool		is2d : 1;
	bool		randomSample : 1;

	const ISoundSource* GetBestSample(int sampleId /*= -1*/) const;
};

struct SoundEmitterData
{
	IEqAudioSource::Params	startParams;
	IEqAudioSource::Params	virtualParams;
	CRefPtr<IEqAudioSource>	soundSource;			// NULL when virtual 
	const SoundScriptDesc*	script{ nullptr };		// sound script which used to start this sound
	CSoundingObject*		soundingObj{ nullptr };
	int						channelType{ CHAN_INVALID };
	int						sampleId{ -1 };			// when randomSample and sampleId == -1, it's random
};