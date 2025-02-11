//////////////////////////////////////////////////////////////////////////////////
// Copyright (C) Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "math/Random.h"
#include "math/Spline.h"
#include "utils/KeyValues.h"

#include "eqSoundEmitterPrivateTypes.h"

SoundEmitterData::SoundEmitterData()
{
	for (int i = 0; i < MAX_SOUND_SAMPLES_SCRIPT; ++i)
	{
		sampleVolume[i] = 1.0f;
		samplePitch[i] = 1.0f;
		samplePos[i] = -1.0f;
	}

	for (int i = 0; i < SOUND_PARAM_COUNT; ++i)
		params[i] = 0.0f;
}

void SoundEmitterData::CreateNodeRuntime()
{
	inputs.clear();
	if (!script)
		return;

	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	for (int i = 0; i < nodeDescs.numElem(); ++i)
	{
		const SoundNodeDesc& nodeDesc = nodeDescs[i];

		if (nodeDesc.type != SOUND_NODE_INPUT)
			continue; // no need, we have desc to access

		SoundNodeInput& input = *inputs.insert(i);
		memset(&input, 0, sizeof(input));
	}
}

void SoundEmitterData::SetInputValue(int inputNameHash, int arrayIdx, float value)
{
	if (!script)
		return;

	const int inputNodeId = script->GetInputNodeId(inputNameHash);
	if (inputNodeId < 0)
		return;

	auto dataIt = inputs.find(inputNodeId);
	if (dataIt.atEnd())
		return;

	SoundNodeInput& in = *dataIt;
	in.values[arrayIdx] = value;
	Atomic::Exchange(nodesNeedUpdate, 1);
}

void SoundEmitterData::SetInputValue(uint8 inputId, float value)
{
	uint nodeId, arrayIdx;
	SoundNodeDesc::UnpackInputIdArrIdx(inputId, nodeId, arrayIdx);

	auto dataIt = inputs.find(nodeId);
	if (dataIt.atEnd())
		return;

	SoundNodeInput& in = *dataIt;
	in.values[arrayIdx] = value;
	Atomic::Exchange(nodesNeedUpdate, 1);
}

float SoundEmitterData::GetInputValue(int nodeId, int arrayIdx)
{
	if (!script)
		return -1.0f;

	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	if(nodeDescs[nodeId].type == SOUND_NODE_CONST)
		return nodeDescs[nodeId].inputConst[arrayIdx];

	auto dataIt = inputs.find(nodeId);
	if (dataIt.atEnd())
		return 0.0f;

	SoundNodeInput& in = *dataIt;
	return in.values[arrayIdx];
}

float SoundEmitterData::GetInputValue(uint8 inputId)
{
	if (inputId == SOUND_VAR_INVALID)
		return 0.0f;

	uint nodeId, arrayIdx;
	SoundNodeDesc::UnpackInputIdArrIdx(inputId, nodeId, arrayIdx);

	return GetInputValue(nodeId, arrayIdx);
}

//---------------------------------------
// Simple evaluator for sound scripts
// with it's own runtime memory
//---------------------------------------

struct EvalStack
{
	static constexpr int STACK_SIZE = 128;

	int	values[STACK_SIZE];
	int	sp{ 0 };

	template<typename T>
	int Push(T value)
	{
		ASSERT_MSG(sp >= 0, "Stack Pop - sp out of range (%d)", sp);
		ASSERT_MSG(sp < STACK_SIZE, "Stack Pop - sp out of range (%d)", sp);

		const int oldSP = sp;
		*(T*)&values[oldSP] = value;
		sp += sizeof(T) / sizeof(int);
		return oldSP;
	}

	template<typename T>
	T Pop()
	{
		sp -= sizeof(T) / sizeof(int);
		ASSERT_MSG(sp >= 0, "Stack Pop - sp out of range (%d)", sp);

		return *(T*)&values[sp];
	}

	template<typename T>
	T Get(int pos)
	{
		ASSERT_MSG(pos >= 0, "Stack Get - pos out of range (%d)", pos);
		ASSERT_MSG(pos < STACK_SIZE, "Stack Get - pos out of range (%d)", pos);

		return *(T*)&values[pos];
	}
};

typedef void (*SoundEmitEvalFn)(EvalStack& stack, int argc, int nret);

static void evalCopy(EvalStack& stack, int argc, int nret)
{
	// copy need to do nothing as argument was already pushed on stack
}

static void evalAdd(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(operandA + operandB);
}

static void evalSub(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(operandA - operandB);
}

static void evalMul(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(operandA * operandB);
}

static void evalDiv(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(operandA / operandB);
}

static void evalMin(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(min(operandA, operandB));
}

static void evalMax(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 2);
	ASSERT(nret == 1);
	const float operandB = stack.Pop<float>();
	const float operandA = stack.Pop<float>();
	stack.Push(max(operandA, operandB));
}

static void evalAverage(EvalStack& stack, int argc, int nret)
{
	ASSERT(nret == 1);

	// order doesn't matter
	float value = 0.0f;
	for (int i = 0; i < argc; ++i)
		value += stack.Pop<float>();

	stack.Push(value / (float)argc);
}

static void evalAbs(EvalStack& stack, int argc, int nret)
{
	float value[SoundNodeDesc::MAX_ARRAY_IDX];

	for (int i = 0; i < argc; ++i)
		value[argc - i - 1] = fabsf(stack.Pop<float>());

	for (int i = 0; i < nret; ++i)
		stack.Push(value[i]);
}

static void evalSpline(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 1);
	ASSERT(nret == 1);

	const float input = stack.Pop<float>();
	const SoundSplineDesc& splineDesc = *stack.Pop<SoundSplineDesc*>();

	const float output = SoundSplineDesc::splineInterpLinear(input, splineDesc.valueCount / 2, splineDesc.values);
	stack.Push(output);
}

static void evalFaderSpline(EvalStack& stack, int argc, int nret)
{
	ASSERT(argc == 1);

	const float input = stack.Pop<float>();
	const SoundSplineDesc& splineDesc = *stack.Pop<SoundSplineDesc*>();

	const float output = SoundSplineDesc::splineInterpLinear(input, splineDesc.valueCount / 2, splineDesc.values);

	// split one output into the number of outputs
	for (int i = 0; i < nret; ++i)
	{
		const float targetValue = (float)i;
		const float fadeValue = clamp(1.0f - fabsf(targetValue - output), 0.0f, 1.0f);
		stack.Push(fadeValue);
	}
}

static SoundEmitEvalFn s_soundFuncTypeEvFn[] = {
	evalCopy, // COPY
	evalAdd, // ADD
	evalSub, // SUB
	evalMul, // MUL
	evalDiv, // DIV
	evalMin, // MIN
	evalMax, // MAX
	evalAbs, // ABS
	evalAverage, // AVERAGE
	evalSpline, // SPLINE
	evalFaderSpline, // FADE
};
static_assert(elementsOf(s_soundFuncTypeEvFn) == SOUND_FUNC_COUNT, "s_soundFuncTypeFuncs and SOUND_FUNC_COUNT needs to be in sync");

void SoundEmitterData::UpdateNodes()
{
	if (Atomic::CompareExchange(nodesNeedUpdate, TRUE, FALSE) != TRUE)
		return;

	if (!script)
		return;

	PROF_EVENT("Emitter Data Nodes Eval");

	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	const Array<SoundSplineDesc>& splineDescs = script->splineDescs;

	EvalStack stack;
	short nodeValueSp[SoundNodeDesc::MAX_SOUND_NODES];

	// evaluate each function node down
	for (int nodeId = 0; nodeId < nodeDescs.numElem(); ++nodeId)
	{
		const SoundNodeDesc& nodeDesc = nodeDescs[nodeId];

		// remember stack pointer of each node for further processing
		nodeValueSp[nodeId] = stack.sp;

		if (nodeDesc.type == SOUND_NODE_INPUT)
		{
			// add input values to stack
			for (int i = 0; i < nodeDesc.input.valueCount; ++i)
				stack.Push(GetInputValue(nodeId, i));

			continue;
		}
		else if (nodeDesc.type == SOUND_NODE_CONST)
		{
			// FIXME: remove const in favor of input only?
			for(int i = 0; i < nodeDesc.c.valueCount; ++i)
				stack.Push(nodeDesc.inputConst[i]);
			continue;
		}
		else if (nodeDesc.type == SOUND_NODE_FUNC)
		{
			// NOTE: stack would be inverse to read!!!
			
			if (nodeDesc.func.type == SOUND_FUNC_SPLINE || nodeDesc.func.type == SOUND_FUNC_FADE)
			{
				// push spline ptr to the stack
				const SoundSplineDesc& splineDesc = script->splineDescs[nodeDesc.func.inputIds[1]];
				stack.Push(&splineDesc);
			}

			for (int i = 0; i < nodeDesc.func.inputCount; ++i)
			{
				const uint8 inputId = nodeDesc.func.inputIds[i];

				if (inputId != SOUND_VAR_INVALID)
				{
					uint inNodeId, inArrayIdx;
					SoundNodeDesc::UnpackInputIdArrIdx(inputId, inNodeId, inArrayIdx);

					ASSERT(inNodeId <= SoundNodeDesc::MAX_SOUND_NODES);

					// retrieve operands of previous nodes and put them to back of stack
					const float operand = stack.Get<float>(nodeValueSp[inNodeId] + inArrayIdx);
					stack.Push(operand);
				}
				else
				{
					// since no value was pushed to stack, we use const
					stack.Push(nodeDesc.inputConst[i]);
				}
			}
			
			s_soundFuncTypeEvFn[nodeDesc.func.type](stack, nodeDesc.func.inputCount, nodeDesc.func.outputCount);
		}
	}

	// output value mapping to sound parameters
	const uint8* paramMap = script->paramNodeMap;
	{
		const float volume = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_VOLUME]]);
		if (memcmp(&nodeParams.volume.x, &volume, sizeof(float)))
			nodeParams.set_volume(Vector3D(volume, nodeParams.volume.yz()));
	}

	{
		const float pitch = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_PITCH]]);
		if (memcmp(&nodeParams.pitch, &pitch, sizeof(float)))
			nodeParams.set_pitch(pitch);
	}

	{
		const float hpf = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_HPF]]);
		if (memcmp(&nodeParams.bandPass.y, &hpf, sizeof(float)))
			nodeParams.set_bandPass(Vector2D(nodeParams.bandPass.x, hpf));
	}

	{
		const float lpf = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_LPF]]);
		if (memcmp(&nodeParams.bandPass.x, &lpf, sizeof(float)))
			nodeParams.set_bandPass(Vector2D(lpf, nodeParams.bandPass.y));
	}

	{
		const float airAbsorption = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_AIRABSORPTION]]);
		if (memcmp(&nodeParams.airAbsorption, &airAbsorption, sizeof(float)))
			nodeParams.set_airAbsorption(airAbsorption);
	}

	{
		const float rollOff = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_ROLLOFF]]);
		if (memcmp(&nodeParams.rolloff, &rollOff, sizeof(float)))
			nodeParams.set_rolloff(rollOff);
	}

	{
		const float attenuation = stack.Get<float>(nodeValueSp[paramMap[SOUND_PARAM_ATTENUATION]]);
		if (memcmp(&nodeParams.referenceDistance, &attenuation, sizeof(float)))
			nodeParams.set_referenceDistance(attenuation);
	}

	// svolume
	if (paramMap[SOUND_PARAM_SAMPLE_VOLUME] != SOUND_VAR_INVALID)
	{
		nodeParams.updateFlags |= UPDATE_SAMPLE_VOLUME;

		const int startSp = nodeValueSp[paramMap[SOUND_PARAM_SAMPLE_VOLUME]];
		const SoundNodeDesc& svolumeNodeDesc = nodeDescs[paramMap[SOUND_PARAM_SAMPLE_VOLUME]];
		if (svolumeNodeDesc.type == SOUND_NODE_FUNC)
		{
			for (int i = 0; i < svolumeNodeDesc.func.outputCount; ++i)
				sampleVolume[i] = stack.Get<float>(startSp + i);
		}
		else if (svolumeNodeDesc.type == SOUND_NODE_CONST)
		{
			for (int i = 0; i < svolumeNodeDesc.c.valueCount; ++i)
				sampleVolume[i] = stack.Get<float>(startSp + i);
		}
	}

	// spitch
	if (paramMap[SOUND_PARAM_SAMPLE_PITCH] != SOUND_VAR_INVALID)
	{
		nodeParams.updateFlags |= UPDATE_SAMPLE_PITCH;

		const int startSp = nodeValueSp[paramMap[SOUND_PARAM_SAMPLE_PITCH]];
		const SoundNodeDesc& spitchNodeDesc = nodeDescs[paramMap[SOUND_PARAM_SAMPLE_PITCH]];
		if (spitchNodeDesc.type == SOUND_NODE_FUNC)
		{
			for (int i = 0; i < spitchNodeDesc.func.outputCount; ++i)
				samplePitch[i] = stack.Get<float>(startSp + i);
		}
		else if (spitchNodeDesc.type == SOUND_NODE_CONST)
		{
			for (int i = 0; i < spitchNodeDesc.c.valueCount; ++i)
				samplePitch[i] = stack.Get<float>(startSp + i);
		}
	}
}

void SoundEmitterData::CalcFinalParameters(float volumeScale, IEqAudioSource::Params& outParams)
{
	PROF_EVENT("Emitter Calc Final Params");

	// update pitch and volume individually
	if (nodeParams.updateFlags & IEqAudioSource::UPDATE_VOLUME)
	{
		const Vector3D finalVolume(max(nodeParams.volume.x * epVolume, 0.0f), nodeParams.volume.yz());
		virtualParams.set_volume(finalVolume);

		outParams.set_volume(Vector3D(finalVolume.x * volumeScale, finalVolume.yz()));
	}

	if (nodeParams.updateFlags & IEqAudioSource::UPDATE_PITCH)
	{
		const float finalPitch = max(nodeParams.pitch * epPitch, 0.0f);
		virtualParams.set_pitch(finalPitch);

		outParams.set_pitch(finalPitch);
	}

	if (nodeParams.updateFlags & IEqAudioSource::UPDATE_REF_DIST)
	{
		const float finalRefDist = max(nodeParams.referenceDistance * epRadiusMultiplier, 0.0f);
		virtualParams.set_referenceDistance(finalRefDist);

		outParams.set_referenceDistance(finalRefDist);
	}

	// merge other params as usual
	const int excludeFlags = (IEqAudioSource::UPDATE_PITCH | IEqAudioSource::UPDATE_VOLUME | IEqAudioSource::UPDATE_REF_DIST);
	virtualParams.merge(nodeParams, nodeParams.updateFlags & ~excludeFlags);
	outParams.merge(nodeParams, nodeParams.updateFlags & ~excludeFlags);
}