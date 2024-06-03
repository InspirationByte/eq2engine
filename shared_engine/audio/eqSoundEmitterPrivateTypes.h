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

	// mixer are basically functions - "add", "sub", "mul", "div", "min", "max", "average", "spline", "fade", "use"
	//	"add", "sub", "mul", "div", "min", "max" - basic functions
	//	"average" - normalized value of sum of N arguments
	//	"spline" - takes value and passes it through spline interpolation
	//	"fade" - same as "spline", but instead it's being split into multiple values (array)
	//	"use" - make a copy of value

	// each "mixer" declares a variable that can be used 
	// in other mixers as an input value or as output

	mixer avgVal max tractionSlide laterialSlide;
	mixer volume spline avgVal 0.0 1.0 0.0 1.0 1.0;
	mixer pitch spline avgVal 0.0 1.0 0.0 1.0 1.0;
	mixer traction spline avgVal 0.0 1.0 1.0 1.0;
	mixer tractionSampleFade fade traction 2 0.0 1.0 1.0 1.0;
	mixer lateralSampleFade fade tractionSampleFade[1] 2 0.0 1.0 1.0 1.0;

	// svolume is individual sample volume (for each "wave")
	svolume 0 "tractionSampleFade[0]";
	svolume 1 "lateralSampleFade[0]";
	svolume 2 "lateralSampleFade[1]";

	// spitch is individual sample pitch (for each "wave")
	spitch 1 "tractionSampleFade[0]";
	spitch 2 "lateralSampleFade[0]";

	// specifying string value makes use of the mixed value or input
	volume "volume";
	pitch "pitch";
*/

static constexpr const int MAX_SOUND_SAMPLES_SCRIPT = 8;
static constexpr const uint8 SOUND_VAR_INVALID = 0xff;

enum ESoundParamType : int
{
	SOUND_PARAM_VOLUME = 0,
	SOUND_PARAM_PITCH,
	SOUND_PARAM_HPF,
	SOUND_PARAM_LPF,		// TODO: merge LPF/HPF
	SOUND_PARAM_AIRABSORPTION,
	SOUND_PARAM_ROLLOFF,
	SOUND_PARAM_ATTENUATION,
	SOUND_PARAM_SAMPLE_VOLUME,
	SOUND_PARAM_SAMPLE_PITCH,

	SOUND_PARAM_COUNT,
};

enum ELoopCommand : int
{
	LOOPCMD_NONE = 0,
	LOOPCMD_FADE_IN,
	LOOPCMD_FADE_OUT,

	LOOPCMD_FLAG_CHANGED = (1 << 31)
};

static const char* s_soundParamNames[] = {
	"volume",
	"pitch",
	"hpf",
	"lpf",
	"airAbsorption",
	"rollOff",
	"distance",
	"svolume",
	"spitch",
};
static_assert(elementsOf(s_soundParamNames) == SOUND_PARAM_COUNT, "s_soundParamNames and SOUND_PARAM_COUNT needs to be in sync");

static float s_soundParamDefaults[] = {
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	0.0f,
	2.0f,
	1.0f,
	1.0f,
	1.0f,
};
static_assert(elementsOf(s_soundParamNames) == SOUND_PARAM_COUNT, "s_soundParamNames and SOUND_PARAM_COUNT needs to be in sync");

static float s_soundParamLimits[] = {
	1.0f,
	8.0f,
	1.0f,
	1.0f,
	10.0f,
	8.0f,
	10.0f,
	1.0f,
	100.0f,
};
static_assert(elementsOf(s_soundParamNames) == SOUND_PARAM_COUNT, "s_soundParamNames and SOUND_PARAM_COUNT needs to be in sync");

enum ESoundNodeType : int
{
	SOUND_NODE_CONST = 0,
	SOUND_NODE_INPUT,
	SOUND_NODE_FUNC,

	SOUND_NODE_TYPE_COUNT
};

static const char* s_nodeTypeNameStr[] = {
	"const",
	"input",
	"mixer"
};

enum ESoundFuncType : int
{
	SOUND_FUNC_COPY,
	SOUND_FUNC_ADD,
	SOUND_FUNC_SUB,
	SOUND_FUNC_MUL,
	SOUND_FUNC_DIV,
	SOUND_FUNC_MIN,
	SOUND_FUNC_MAX,
	SOUND_FUNC_ABS,
	SOUND_FUNC_AVERAGE,
	SOUND_FUNC_SPLINE,
	SOUND_FUNC_FADE,

	SOUND_FUNC_COUNT
};

struct SoundFuncDesc
{
	char* name;
	uint8 argCount;
	uint8 retCount;
};

enum ESoundFuncArgCount
{
	SOUND_FUNC_ARG_VARIADIC = 0x80
};

static SoundFuncDesc s_soundFuncTypeDesc[] = {
	{ "copy", SOUND_FUNC_ARG_VARIADIC, SOUND_FUNC_ARG_VARIADIC },
	{ "add", 2, 1 },
	{ "sub", 2, 1 },
	{ "mul", 2, 1 },
	{ "div", 2, 1 },
	{ "min", 2, 1 },
	{ "max", 2, 1 },
	{ "abs", SOUND_FUNC_ARG_VARIADIC, SOUND_FUNC_ARG_VARIADIC },
	{ "average", SOUND_FUNC_ARG_VARIADIC, 1 },
	{ "spline", 1, 1 },
	{ "fade", 1, SOUND_FUNC_ARG_VARIADIC },
};
static_assert(elementsOf(s_soundFuncTypeDesc) == SOUND_FUNC_COUNT, "s_soundFuncTypeNames and SOUND_FUNC_COUNT needs to be in sync");

static int GetSoundFuncTypeByString(const char* name)
{
	for (int i = 0; i < SOUND_FUNC_COUNT; ++i)
	{
		if (!stricmp(name, s_soundFuncTypeDesc[i].name))
			return (ESoundFuncType)i;
	}
	return -1;
}

struct SoundSplineDesc
{
	static constexpr const int MAX_SPLINE_POINTS = 10;
	float	values[MAX_SPLINE_POINTS * 2];
	uint8	valueCount{ 0 }; // divide by two pls

	static float splineInterpLinear(float t, int maxPoints, const float* points);
	static float splineInterp(float t, int maxPoints, const float* points);

	void	Reset();
	void	Fix();
};

enum ESoundNodeFlags
{
	SOUND_NODE_FLAG_AUTOGENERATED = (1 << 0),
	SOUND_NODE_FLAG_OUTPUT = (1 << 1),
};

struct SoundNodeDesc
{
	static constexpr int NODE_ID_BITS = 5;
	static constexpr int ARRAY_IDX_BITS = 3;

	static constexpr int MAX_SOUND_NODES = 1 << NODE_ID_BITS;
	static constexpr int MAX_ARRAY_IDX = 1 << ARRAY_IDX_BITS;

	char			name[30]{ 0 };
	uint8			type{ 0 };
	uint8			flags{ 0 };
	float			inputConst[MAX_ARRAY_IDX]{ 0.0f };

	union {
		struct {
			uint8	inputIds[MAX_ARRAY_IDX];
			uint8	type;						// ESoundFuncType
			uint8	outputCount;
			uint8	inputCount;
		} func;
		struct {
			float	rMin;
			float	rMax;
			uint8	valueCount;
		} input;
		struct {
			uint8	paramId;
			uint8	valueCount;
		} c;
	};

	static void UnpackInputIdArrIdx(uint8 inputId, uint& id, uint& arrayIdx);
	static uint8 PackInputIdArrIdx(uint id, uint arrayIdx);
};

struct SoundNodeInput
{
	float values[SoundNodeDesc::MAX_ARRAY_IDX];	// indexed by inputIds 
};

struct SoundScriptDesc
{
	SoundScriptDesc() = default;
	SoundScriptDesc(const char* name);

	EqString				name;
	Array<ISoundSourcePtr>	samples{ PP_SL };
	Array<EqString>			soundFileNames{ PP_SL };

	Array<SoundNodeDesc>	nodeDescs{ PP_SL };
	Array<SoundSplineDesc>	splineDescs{ PP_SL };
	uint8					paramNodeMap[SOUND_PARAM_COUNT];
	Map<int, int>			inputNodeMap{ PP_SL };	// maps nodeName -> nodeIdx

	uint		sampleRandomizer{ 0 };

	int			channelType{ CHAN_INVALID };
	float		maxDistance{ 1.0f };
	float		stopLoopTime{ 0.0f };
	float		startLoopTime{ 0.0f };
	
	bool		loop : 1;
	bool		is2d : 1;
	bool		randomSample : 1;

	const ISoundSource* GetBestSample(int sampleId /*= -1*/);
	uint8				FindVariableIndex(const char* varName) const;
	int					GetInputNodeId(int nameHash) const;

	static void			ParseDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection, const KVSection* defaultsSec);
	static void			ReloadDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection);
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
	int							nodesNeedUpdate{ true };	// triggers recalc of entire node set

	SoundEmitterData();

	void	CreateNodeRuntime();

	void	SetInputValue(int inputNameHash, int arrayIdx, float value);
	void	SetInputValue(uint8 inputId, float value);

	float	GetInputValue(int nodeId, int arrayIdx);
	float	GetInputValue(uint8 inputId);

	void	UpdateNodes();
	void	CalcFinalParameters(float volumeScale, IEqAudioSource::Params& outParams);
};