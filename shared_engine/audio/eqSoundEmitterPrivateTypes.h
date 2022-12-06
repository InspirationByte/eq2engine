//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "audio/IEqAudioSystem.h"
#include "eqSoundEmitterCommon.h"

static Threading::CEqMutex s_soundEmitterSystemMutex;

class CSoundingObject;
struct KVSection;

/*
* 
* ADVANCED SOUND SCRIPT FEATURES
* 
	const "test" 1.0;

	// input NAME MIN_RANGE MAX_RANGE
	input "tractionSlide" 0.0 18.0;
	input "laterialSlide" 0.0 18.0;

	// mixer are basically functions - "add", "sub", "mul", "div", "min", "max", "average", "curve", "fade", "use"
	//	"add", "sub", "mul", "div", "min", "max" - basic functions
	//	"average" - normalized value of sum of N arguments
	//	"curve" - takes value and passes it through curve interpolation
	//	"fade" - same as "curve", but instead it's being split into multiple values (array)
	//	"use" - make a copy of value

	// each "mixer" declares a variable that can be used 
	// in other mixers as an input value or as output

	mixer avgVal max tractionSlide laterialSlide;
	mixer volume curve avgVal 0.0 1.0 0.0 1.0 1.0;
	mixer pitch curve avgVal 0.0 1.0 0.0 1.0 1.0;
	mixer traction curve avgVal 0.0 1.0 1.0 1.0;
	mixer tractionSampleFade fade traction 2 0.0 1.0 1.0 1.0;
	mixer lateralSampleFade fade tractionSampleFade[1] 2 0.0 1.0 1.0 1.0;

	// svolume is individual sample volume (for each "wave")
	svolume 0 "tractionSampleFade[0]";
	svolume 1 "lateralSampleFade[0]";
	svolume 2 "lateralSampleFade[1]";

	// specifying string value makes use of the mixed value or input
	volume "volume";
	pitch "pitch";
*/

enum ESoundNodeType : int
{
	SOUND_NODE_CONST = 0,
	SOUND_NODE_INPUT,
	SOUND_NODE_FUNC
};

enum ESoundFuncType : int
{
	SOUND_FUNC_ADD,
	SOUND_FUNC_SUB,
	SOUND_FUNC_MUL, 
	SOUND_FUNC_DIV,
	SOUND_FUNC_MIN,
	SOUND_FUNC_MAX,
	SOUND_FUNC_AVERAGE,
	SOUND_FUNC_CURVE,
	SOUND_FUNC_FADE,

	SOUND_FUNC_COUNT,
};

static const char* s_soundFuncTypeNames[SOUND_FUNC_COUNT] = {
	"ADD",
	"SUB",
	"MUL",
	"DIV",
	"MIN",
	"MAX",
	"AVERAGE",
	"CURVE",
	"FADE",
};
static_assert(elementsOf(s_soundFuncTypeNames) == SOUND_FUNC_COUNT, "s_soundFuncTypeNames and SOUND_FUNC_COUNT needs to be in sync");

static int GetSoundFuncTypeByString(const char* name)
{
	for (int i = 0; i < SOUND_FUNC_COUNT; ++i)
	{
		if (!stricmp(name, s_soundFuncTypeNames[i]))
			return (ESoundFuncType)i;
	}
	return -1;
}

struct SoundCurveDesc
{
	static constexpr const int MAX_CURVE_POINTS = 10;
	float	values[MAX_CURVE_POINTS * 2];	// for curve
	uint8	valueCount;
};

struct SoundNodeDesc
{
	static constexpr const int MAX_ARRAY_IDX = 1 << 3;

	char			name[31]{ 0 };
	uint8			type{ 0 };
	
	union {
		struct {
			uint8	inputIds[MAX_ARRAY_IDX];		// id :5, index: 3
			uint8	type;
			uint8	outputCount;
			uint8	inputCount;
		} func;
		struct {
			float	rMin;
			float	rMax;
			int		valueCount;
		} input;
		struct {
			float	value;
		} c;
	};

	static void UnpackInputIdArrIdx(uint8 inputId, uint& id, uint& arrayIdx)
	{
		id = inputId & 31;
		arrayIdx = inputId >> 5 & 7;
	}

	static uint8 PackInputIdArrIdx(uint id, uint arrayIdx)
	{
		return id & 31 | ((arrayIdx & 7) << 5);
	}
};

struct SoundNodeInput
{
	float values[SoundNodeDesc::MAX_ARRAY_IDX] = { 0 };	// indexed by inputIds 
};

struct SoundScriptDesc
{
	SoundScriptDesc() = default;
	SoundScriptDesc(const char* name);

	EqString				name;

	Array<ISoundSource*>	samples{ PP_SL };
	Array<EqString>			soundFileNames{ PP_SL };

	Array<SoundNodeDesc>	nodeDescs{ PP_SL };
	Array<SoundCurveDesc>	curveDescs{ PP_SL };
	Map<int, int>			inputNodeMap{ PP_SL };

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
	uint8				FindVariableIndex(const char* varName) const;
	static void			ParseDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection);
};

struct SoundEmitterData
{
	IEqAudioSource::Params		startParams;
	IEqAudioSource::Params		virtualParams;

	Map<int, SoundNodeInput>	inputs{ PP_SL };

	CRefPtr<IEqAudioSource>		soundSource;				// NULL when virtual 
	const SoundScriptDesc*		script{ nullptr };			// sound script which used to start this sound
	CSoundingObject*			soundingObj{ nullptr };
	int							channelType{ CHAN_INVALID };
	int							sampleId{ -1 };				// when randomSample and sampleId == -1, it's random
	bool						nodesNeedUpdate{ true };	// triggers recalc of entire node set

	void	CreateNodeRuntime();

	void	SetInputValue(int inputNameHash, int arrayIdx, float value);
	void	SetInputValue(uint8 inputId, float value);

	float	GetInputValue(int nodeId, int arrayIdx);
	float	GetInputValue(uint8 inputId);

	void	UpdateNodes();
};