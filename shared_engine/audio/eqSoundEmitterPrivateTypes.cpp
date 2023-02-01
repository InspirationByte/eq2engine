//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system
//////////////////////////////////////////////////////////////////////////////////

#include "core/core_common.h"
#include "math/Random.h"
#include "math/Spline.h"
#include "utils/KeyValues.h"

#include "eqSoundEmitterPrivateTypes.h"

float SoundSplineDesc::splineInterpLinear(float t, int maxPoints, const float* points)
{
	const Vector2D* pts = (const Vector2D*)points;

	if (t < pts[0].x)
		return pts[0].y;

	int left = 0;
	while (left < maxPoints && pts[left].x < t)
		left++;
	if (left)
		left--;

	if (left >= maxPoints - 1)
		return pts[maxPoints - 1].y;

	const Vector2D p = pts[left];
	const Vector2D q = pts[left + 1];

	const float h = (t - p.x) / (q.x - p.x);
	return p.y + (q.y - p.y) * h;
}

float SoundSplineDesc::splineInterp(float t, int maxPoints, const float* points)
{
	float output = 0.0f;
	spline<1>(points, maxPoints, t, &output);
	return output;
}

void SoundSplineDesc::Reset()
{
	values[0] = 0.0f;
	values[1] = 0.0f;
	values[2] = 1.0f;
	values[3] = 1.0f;
	values[4] = -F_INFINITY;
	valueCount = 4;
}

void SoundSplineDesc::Fix()
{
	if (valueCount < MAX_SPLINE_POINTS * 2)
		values[valueCount] = -F_INFINITY;
}

SoundScriptDesc::SoundScriptDesc(const char* name) 
	: name(name)
{
}

const ISoundSource* SoundScriptDesc::GetBestSample(int sampleId /*= -1*/)
{
	const int numSamples = samples.numElem();

	if (!numSamples)
		return nullptr;

	if (!samples.inRange(sampleId))	// if it is out of range, randomize
		sampleId = -1;

	if (sampleId < 0)
	{
		if (numSamples == 1)
			return samples[0];

		if (numBitsSet(sampleRandomizer) >= numSamples)
			sampleRandomizer = 0;

		int sampleId;
		do {
			sampleId = RandomInt(0, numSamples - 1);
		} while (((1 << sampleId) & sampleRandomizer) != 0);
		sampleRandomizer |= (1 << sampleId);

		return samples[sampleId];
	}
	else
		return samples[sampleId];
}

uint8 SoundScriptDesc::FindVariableIndex(const char* varName) const
{
	if (*varName >= '0' && *varName <= '9')
	{
		ASSERT_FAIL("constant value %s passed, use FindVariableOrMakeConst\n", varName);
		return SOUND_VAR_INVALID;
	}

	char tmpName[32]{ 0 };
	strncpy(tmpName, varName, sizeof(tmpName));
	tmpName[sizeof(tmpName) - 1] = 0;

	uint arrayIdx = 0;
	// try parse array index
	char* arrSub = strchr(tmpName, '[');
	if (arrSub)
	{
		*arrSub++ = 0;
		char* numberStart = arrSub;

		// check for numeric
		if (!(*numberStart >= '0' && *numberStart <= '9'))
		{
			MsgError("sound script '%s' mixer: array index is invalid for %s\n", name.ToCString(), tmpName);
			return SOUND_VAR_INVALID;
		}

		// find closing
		arrSub = strchr(arrSub, ']');
		if (!arrSub)
		{
			MsgError("sound script '%s' mixer: missing ']' for %s\n", name.ToCString(), tmpName);
			return SOUND_VAR_INVALID;
		}
		*arrSub = 0;
		arrayIdx = atoi(numberStart);
	}

	const int valIdx = nodeDescs.findIndex([tmpName](const SoundNodeDesc& desc) {
		return !strcmp(desc.name, tmpName);
	});

	if (valIdx == -1)
		return SOUND_VAR_INVALID;

	return SoundNodeDesc::PackInputIdArrIdx((uint)valIdx, arrayIdx);
}

int	SoundScriptDesc::GetInputNodeId(int nameHash) const
{
	auto it = inputNodeMap.find(nameHash);
	if (it == inputNodeMap.end())
		return -1;
	return *it;
}

void SoundScriptDesc::ParseDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection, const KVSection* defaultsSec)
{
	// initialize first
	scriptDesc.randomSample = false;

	for (int paramId = 0; paramId < SOUND_PARAM_COUNT; ++paramId)
		scriptDesc.paramNodeMap[paramId] = SOUND_VAR_INVALID;

	// pick 'rndwave' or 'wave' sections for lists
	KVSection* waveKey = waveKey = scriptSection->FindSection("wave", KV_FLAG_SECTION);

	if (!waveKey)
	{
		waveKey = scriptSection->FindSection("rndwave", KV_FLAG_SECTION);
		scriptDesc.randomSample = true;
	}

	if (waveKey)
	{
		for (int j = 0; j < waveKey->keys.numElem(); j++)
		{
			const KVSection* ent = waveKey->keys[j];

			if (stricmp(ent->name, "wave"))
				continue;

			scriptDesc.soundFileNames.append(KV_GetValueString(ent));
		}
	}
	else
	{
		waveKey = scriptSection->FindSection("wave");

		if (waveKey)
			scriptDesc.soundFileNames.append(KV_GetValueString(waveKey));
	}

	if (scriptDesc.soundFileNames.numElem() == 0)
	{
		MsgWarning("No wave defined for script '%s'!\n", scriptDesc.name.ToCString());
		return;
	}

	Array<SoundNodeDesc>& nodeDescs = scriptDesc.nodeDescs;
	Array<SoundSplineDesc>& splineDescs = scriptDesc.splineDescs;

	// parse constants, inputs, mixers
	for (int i = 0; i < scriptSection->KeyCount(); ++i)
	{
		const KVSection& valKey = *scriptSection->keys[i];
		if (valKey.IsSection())
			continue;

		int nodeType = -1;
		
		if (!stricmp(valKey.name, "input"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' input: name is required\n");
				continue;
			}

			ASSERT_MSG(scriptDesc.FindVariableIndex(nodeName) == 0xff, "Node %s was already declared", nodeName);

			nodeType = SOUND_NODE_INPUT;

			// TODO: SetupNode(SOUND_NODE_INPUT)

			const int nodeIdx = nodeDescs.numElem();
			SoundNodeDesc& inputDesc = nodeDescs.append();
			inputDesc.type = nodeType;
			strncpy(inputDesc.name, nodeName, sizeof(inputDesc.name));
			inputDesc.name[sizeof(inputDesc.name) - 1] = 0;

			// TODO: support array index
			inputDesc.input.valueCount = 1;
			inputDesc.input.rMin = KV_GetValueFloat(&valKey, 1, 0.0f);
			inputDesc.input.rMax = KV_GetValueFloat(&valKey, 2, 1.0f);

			scriptDesc.inputNodeMap.insert(StringToHash(nodeName), nodeIdx);
		}
		else if (!stricmp(valKey.name, "mixer"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' mixer: name is required\n");
				continue;
			}
			ASSERT_MSG(scriptDesc.FindVariableIndex(nodeName) == 0xff, "Node %s was already declared", nodeName);

			// TODO: SetupNode(SOUND_NODE_FUNC)

			nodeType = SOUND_NODE_FUNC;
			const char* funcTypeName = KV_GetValueString(&valKey, 1, "");
			int funcType = GetSoundFuncTypeByString(funcTypeName);
			if (funcType == -1)
			{
				MsgError("sound script '%s' mixer: %s unknown\n", funcTypeName);
				continue;
			}

			// TODO: SetupFunc()

			SoundNodeDesc& funcDesc = nodeDescs.append();
			funcDesc.type = nodeType;
			funcDesc.func.type = funcType;

			strncpy(funcDesc.name, nodeName, sizeof(funcDesc.name));
			funcDesc.name[sizeof(funcDesc.name) - 1] = 0;

			for (int v = 0; v < SoundNodeDesc::MAX_ARRAY_IDX; ++v)
				funcDesc.func.inputIds[v] = SOUND_VAR_INVALID;

			auto findInputVarOrMakeConst = [&scriptDesc](SoundNodeDesc& node, int inputIdx, const char* valName)
			{
				if (*valName >= '0' && *valName <= '9')
				{
					node.inputConst[inputIdx] = atof(valName);
					return SOUND_VAR_INVALID;// SoundNodeDesc::PackInputIdArrIdx(31, inputIdx);
				}

				uint8 varIdx = scriptDesc.FindVariableIndex(valName);

				if (varIdx == SOUND_VAR_INVALID)
					MsgError("sound script '%s': unknown var %s\n", scriptDesc.name.ToCString(), valName);

				return varIdx;

			};

			// TODO: ParseFuncNode()

			// parse format for each type
			switch ((ESoundFuncType)funcType)
			{
				case SOUND_FUNC_ABS:
				{
					// N args
					int nArg = 0;
					for (int v = 2; v < valKey.ValueCount(); ++v)
					{
						const char* valName = KV_GetValueString(&valKey, v, nullptr);
						ASSERT(valName);

						funcDesc.func.inputIds[nArg++] = findInputVarOrMakeConst(funcDesc, v, valName);
					}
					funcDesc.func.inputCount = nArg;

					// same number of N returns
					funcDesc.func.outputCount = nArg;
					break;
				}
				case SOUND_FUNC_ADD:
				case SOUND_FUNC_SUB:
				case SOUND_FUNC_MUL:
				case SOUND_FUNC_DIV:
				case SOUND_FUNC_MIN:
				case SOUND_FUNC_MAX:
				{
					// 2 args
					for (int v = 0; v < 2; ++v)
					{
						const char* valName = KV_GetValueString(&valKey, v + 2, nullptr);
						if (!valName)
						{
							MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), funcDesc.name);
							continue;
						}

						funcDesc.func.inputIds[v] = findInputVarOrMakeConst(funcDesc, v, valName);
					}
					funcDesc.func.inputCount = 2;
					funcDesc.func.outputCount = 1;
					break;
				}
				case SOUND_FUNC_AVERAGE:
				{
					// N args
					int nArg = 0;
					for (int v = 2; v < valKey.ValueCount(); ++v)
					{
						const char* valName = KV_GetValueString(&valKey, v, nullptr);
						ASSERT(valName);

						funcDesc.func.inputIds[nArg++] = findInputVarOrMakeConst(funcDesc, v, valName);
					}
					funcDesc.func.inputCount = nArg;
					funcDesc.func.outputCount = 1;
					break;
				}
				case SOUND_FUNC_SPLINE:
				{
					// input x0 y0 x1 y1 ... xN yN
					const char* inputValName = KV_GetValueString(&valKey, 2, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString());
						continue;
					}

					funcDesc.func.outputCount = 1;
					funcDesc.func.inputCount = 1;

					funcDesc.func.inputIds[0] = findInputVarOrMakeConst(funcDesc, i, inputValName);
					funcDesc.func.inputIds[1] = splineDescs.numElem();
					
					SoundSplineDesc& spline = splineDescs.append();

					int nArg = 0;
					for (int v = 3; v < valKey.ValueCount(); ++v)
					{
						if(nArg < SoundSplineDesc::MAX_SPLINE_POINTS * 2)
							spline.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}
					spline.valueCount = nArg;
					spline.Fix();

					if (nArg & 1)
					{
						MsgError("sound script '%s' mixer %s: uneven curve arguments\n", scriptDesc.name.ToCString(), funcDesc.name);
					}
					break;
				}
				case SOUND_FUNC_FADE:
				{
					// outputCount input x0 y0 x1 y1 ... xN yN
					const int numOutputs = KV_GetValueInt(&valKey, 2, 0);
					if (!numOutputs)
					{
						MsgError("sound script '%s' mixer %s: no outputs for fade\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					const char* inputValName = KV_GetValueString(&valKey, 3, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					funcDesc.func.outputCount = numOutputs;
					funcDesc.func.inputCount = 1;

					funcDesc.func.inputIds[0] = findInputVarOrMakeConst(funcDesc, i, inputValName);
					funcDesc.func.inputIds[1] = splineDescs.numElem();

					SoundSplineDesc& spline = splineDescs.append();

					int nArg = 0;
					for (int v = 4; v < valKey.ValueCount(); ++v)
					{
						if (nArg < SoundSplineDesc::MAX_SPLINE_POINTS * 2)
							spline.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}

					if (nArg < SoundSplineDesc::MAX_SPLINE_POINTS * 2)
						spline.values[nArg] = -F_INFINITY;
					spline.valueCount = nArg;

					if (nArg & 1)
					{
						MsgError("sound script '%s' mixer %s: uneven spline arguments\n", scriptDesc.name.ToCString(), funcDesc.name);
					}
					break;
				}
			} // switch funcType
		} // const, input, mixer
		else if (!stricmp(valKey.name, s_soundParamNames[SOUND_PARAM_SAMPLE_VOLUME]))
		{
			const int sampleIdx = KV_GetValueInt(&valKey);
			const char* valueStr = KV_GetValueString(&valKey, 1, nullptr);

			if (!valueStr)
				continue;

			char nodeNameWithPrefix[32]{ 0 };
			strcpy(nodeNameWithPrefix, "out_");
			strncat(nodeNameWithPrefix, s_soundParamNames[SOUND_PARAM_SAMPLE_VOLUME], sizeof(nodeNameWithPrefix));
			nodeNameWithPrefix[sizeof(nodeNameWithPrefix) - 1] = 0;

			uint8 nodeIdx = scriptDesc.FindVariableIndex(nodeNameWithPrefix);
			if (nodeIdx == SOUND_VAR_INVALID)
			{
				scriptDesc.paramNodeMap[SOUND_PARAM_SAMPLE_VOLUME] = nodeDescs.numElem();

				SoundNodeDesc& nodeDesc = nodeDescs.append();
				nodeDesc.flags = SOUND_NODE_FLAG_AUTOGENERATED | SOUND_NODE_FLAG_OUTPUT;

				strncpy(nodeDesc.name, nodeNameWithPrefix, sizeof(nodeDesc.name));
				nodeDesc.name[sizeof(nodeDesc.name) - 1] = 0;

				nodeDesc.type = SOUND_NODE_FUNC;
				nodeDesc.func.type = SOUND_FUNC_COPY;

				nodeDesc.func.inputCount = scriptDesc.soundFileNames.numElem();
				nodeDesc.func.outputCount = scriptDesc.soundFileNames.numElem();

				nodeDesc.func.inputIds[sampleIdx] = scriptDesc.FindVariableIndex(valueStr);
			}
			else
			{
				SoundNodeDesc& nodeDesc = nodeDescs[nodeIdx];
				nodeDesc.func.inputIds[sampleIdx] = scriptDesc.FindVariableIndex(valueStr);
			}
		}
	} // for

	auto sectionGetOrDefault = [scriptSection, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection->FindSection(name);
		if (!sec && defaultsSec)
			sec = defaultsSec->FindSection(name);
		return sec;
	};

	// load the parameters (either as constants or mixed values)
	for (int paramId = 0; paramId < SOUND_PARAM_COUNT; ++paramId)
	{
		if (scriptDesc.paramNodeMap[paramId] != SOUND_VAR_INVALID)
			continue;

		scriptDesc.paramNodeMap[paramId] = nodeDescs.numElem();

		// add parameters (const or non-const)
		char nodeNameWithPrefix[32]{ 0 };
		strcpy(nodeNameWithPrefix, "out_");
		strncat(nodeNameWithPrefix, s_soundParamNames[paramId], sizeof(nodeNameWithPrefix));
		nodeNameWithPrefix[sizeof(nodeNameWithPrefix) - 1] = 0;

		const char* nodeName = s_soundParamNames[paramId];

		SoundNodeDesc& nodeDesc = nodeDescs.append();
		nodeDesc.type = SOUND_NODE_CONST;
		nodeDesc.flags = SOUND_NODE_FLAG_AUTOGENERATED | SOUND_NODE_FLAG_OUTPUT;
		nodeDesc.c.paramId = paramId;

		strncpy(nodeDesc.name, nodeNameWithPrefix, sizeof(nodeDesc.name));
		nodeDesc.name[sizeof(nodeDesc.name) - 1] = 0;

		const KVSection* constSec = sectionGetOrDefault(nodeName);
		const char* valueStr = KV_GetValueString(constSec, 0, nullptr);

		if (!valueStr)
		{
			nodeDesc.c.value = s_soundParamDefaults[paramId];
			continue;
		}

		if (paramId != SOUND_PARAM_SAMPLE_VOLUME)
		{
			if (*valueStr >= '0' && *valueStr <= '9')
			{
				nodeDesc.c.value = KV_GetValueFloat(constSec);
				continue;
			}
		}

		// re-quialify into function
		nodeDesc.type = SOUND_NODE_FUNC;
		nodeDesc.func.type = SOUND_FUNC_COPY;

		nodeDesc.func.inputIds[0] = scriptDesc.FindVariableIndex(valueStr);
		nodeDesc.func.inputCount = 1;
		nodeDesc.func.outputCount = 1;
	}
}

void SoundScriptDesc::ReloadDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection)
{
	// before clearing out nodeDescs, fill defaults
	KVSection defaultsSec;
	for (int paramId = 0; paramId < SOUND_PARAM_SAMPLE_VOLUME; ++paramId)
	{
		const SoundNodeDesc& nodeDesc = scriptDesc.nodeDescs[scriptDesc.paramNodeMap[paramId]];
		if (nodeDesc.type == SOUND_NODE_CONST)
			defaultsSec.SetKey(s_soundParamNames[paramId], nodeDesc.c.value);
	}

	for (int i = 0; i < scriptDesc.samples.numElem(); ++i)
	{
		// that will stop all sources that playing that sample
		g_audioSystem->OnSampleDeleted(scriptDesc.samples[i]);
	}

	scriptDesc.samples.clear();
	scriptDesc.nodeDescs.clear();
	scriptDesc.soundFileNames.clear();
	scriptDesc.splineDescs.clear();
	scriptDesc.inputNodeMap.clear();

	defaultsSec.SetKey("maxDistance", scriptDesc.maxDistance);
	defaultsSec.SetKey("loop", scriptDesc.loop);
	defaultsSec.SetKey("is2d", scriptDesc.is2d);

	ParseDesc(scriptDesc, scriptSection, &defaultsSec);
}

SoundEmitterData::SoundEmitterData()
{
	for (int i = 0; i < MAX_SOUND_SAMPLES_SCRIPT; ++i)
	{
		sampleVolume[i] = 1.0f;
		samplePos[i] = -1.0f;
		
	}

	for (int i = 0; i < SOUND_PARAM_COUNT; ++i)
		params[i] = 0.0f;
}

void SoundEmitterData::CreateNodeRuntime()
{
	inputs.clear();
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
	const int inputNodeId = script->GetInputNodeId(inputNameHash);
	if (inputNodeId < 0)
		return;

	auto dataIt = inputs.find(inputNodeId);
	if (dataIt == inputs.end())
		return;

	SoundNodeInput& in = *dataIt;
	in.values[arrayIdx] = value;
	nodesNeedUpdate = true;
}

void SoundEmitterData::SetInputValue(uint8 inputId, float value)
{
	uint nodeId, arrayIdx;
	SoundNodeDesc::UnpackInputIdArrIdx(inputId, nodeId, arrayIdx);

	auto dataIt = inputs.find(nodeId);
	if (dataIt == inputs.end())
		return;

	SoundNodeInput& in = *dataIt;
	in.values[arrayIdx] = value;
	nodesNeedUpdate = true;
}

float SoundEmitterData::GetInputValue(int nodeId, int arrayIdx)
{
	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;

	if(nodeDescs[nodeId].type == SOUND_NODE_CONST)
		return nodeDescs[nodeId].c.value;

	auto dataIt = inputs.find(nodeId);
	if (dataIt == inputs.end())
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
	int	values[128];
	int	sp{ 0 };

	template<typename T>
	int Push(T value)
	{
		const int oldSP = sp;
		*(T*)&values[oldSP] = value;
		sp += sizeof(T) / sizeof(int);
		return oldSP;
	}

	template<typename T>
	T Pop()
	{
		sp -= sizeof(T) / sizeof(int);
		return *(T*)&values[sp];
	}

	template<typename T>
	T Get(int pos)
	{
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
	if (!nodesNeedUpdate)
		return;

	PROF_EVENT("Emitter Data Nodes Eval");

	nodesNeedUpdate = false;

	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	const Array<SoundSplineDesc>& splineDescs = script->splineDescs;

	EvalStack stack;
	short nodeValueSp[64];

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
			stack.Push(nodeDesc.c.value);
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

	if (paramMap[SOUND_PARAM_SAMPLE_VOLUME] != SOUND_VAR_INVALID)
	{
		nodeParams.updateFlags |= UPDATE_SAMPLE_VOLUME;

		const SoundNodeDesc& svolumeNodeDesc = nodeDescs[paramMap[SOUND_PARAM_SAMPLE_VOLUME]];
		if (svolumeNodeDesc.type != SOUND_NODE_FUNC)
			return;

		const int startSp = nodeValueSp[paramMap[SOUND_PARAM_SAMPLE_VOLUME]];

		for (int i = 0; i < svolumeNodeDesc.func.outputCount; ++i)
			sampleVolume[i] = stack.Get<float>(startSp+i);
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