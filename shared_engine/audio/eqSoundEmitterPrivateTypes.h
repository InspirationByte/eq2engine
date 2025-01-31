//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"
#include "source/snd_source.h"
#include "eqSoundEmitterCommon.h"
#include "eqSoundScript.h"

class CSoundingObject;

enum ELoopCommand : int
{
	LOOPCMD_NONE = 0,
	LOOPCMD_FADE_IN,
	LOOPCMD_FADE_OUT,

	LOOPCMD_FLAG_CHANGED = (1 << 31)
};

struct SoundNodeInput
{
	float values[SoundNodeDesc::MAX_ARRAY_IDX];	// indexed by inputIds 
};

struct SoundEmitterData : public WeakRefObject<SoundEmitterData>
{
	// additional enum
	enum ExtraSourceUpdateFlags
	{
		UPDATE_SAMPLE_VOLUME = (1 << 30),
		UPDATE_SAMPLE_PITCH = (1 << 31)
	};

	IEqAudioSource::Params		nodeParams;
	IEqAudioSource::Params		virtualParams;
	float						sampleVolume[MAX_SOUND_SAMPLES_SCRIPT];
	float						samplePitch[MAX_SOUND_SAMPLES_SCRIPT];
	float						samplePos[MAX_SOUND_SAMPLES_SCRIPT];
	float						params[SOUND_PARAM_COUNT];
	int							nodesNeedUpdate{ true };	// triggers recalc of entire node set

	Map<int, SoundNodeInput>	inputs{ PP_SL };

	SoundEmitterData*			delNext{ nullptr };

	CRefPtr<IEqAudioSource>		soundSource;				// NULL when virtual 
	SoundScriptDesc*			script{ nullptr };			// sound script which used to start this sound
	CSoundingObject*			soundingObj{ nullptr };
	int							channelType{ CHAN_INVALID };

	// emit params data
	float						epVolume{ 1.0f };
	float						epPitch{ 1.0f };
	float						epRadiusMultiplier{ 1.0f };

	float						loopCommandTimeFactor{ 1.0f };		// 0..1
	float						loopCommandRatePerSecond{ 0.0f };
	int							loopCommand{ LOOPCMD_NONE };

	int							sampleId{ -1 };				// when randomSample and sampleId == -1, it's random

	SoundEmitterData();

	void	CreateNodeRuntime();

	void	SetInputValue(int inputNameHash, int arrayIdx, float value);
	void	SetInputValue(uint8 inputId, float value);

	float	GetInputValue(int nodeId, int arrayIdx);
	float	GetInputValue(uint8 inputId);

	void	UpdateNodes();
	void	CalcFinalParameters(float volumeScale, IEqAudioSource::Params& outParams);
};