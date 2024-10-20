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

void SoundNodeDesc::UnpackInputIdArrIdx(uint8 inputId, uint& id, uint& arrayIdx)
{
	// TODO: make use of constants like MAX_NODES, MAX_ARRAY_IDX
	id = inputId & (MAX_SOUND_NODES - 1);
	arrayIdx = inputId >> NODE_ID_BITS & (MAX_ARRAY_IDX-1);
}

uint8 SoundNodeDesc::PackInputIdArrIdx(uint id, uint arrayIdx)
{
	ASSERT(id < MAX_SOUND_NODES);
	ASSERT(arrayIdx < MAX_ARRAY_IDX);

	// TODO: make use of constants like MAX_NODES, MAX_ARRAY_IDX
	return (id & (MAX_SOUND_NODES - 1)) | ((arrayIdx & (MAX_ARRAY_IDX - 1)) << NODE_ID_BITS);
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
		ASSERT_FAIL("constant value %s passed, use findInputVarOrMakeConst\n", varName);
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

	const int valIdx = arrayFindIndexF(nodeDescs, [tmpName](const SoundNodeDesc& desc) {
		return !CString::Compare(desc.name, tmpName);
	});

	if (valIdx == -1)
		return SOUND_VAR_INVALID;

	return SoundNodeDesc::PackInputIdArrIdx((uint)valIdx, arrayIdx);
}

int	SoundScriptDesc::GetInputNodeId(int nameHash) const
{
	auto it = inputNodeMap.find(nameHash);
	if (it.atEnd())
		return -1;
	return *it;
}

static void InitSoundNode(SoundNodeDesc& nodeDesc, const ESoundNodeType type, const int paramType = -1, int inputCount = 1, int outputCount = 1, bool initConstants = true)
{
	nodeDesc.type = type;
	if (initConstants)
	{
		for (int v = 0; v < SoundNodeDesc::MAX_ARRAY_IDX; ++v)
			nodeDesc.inputConst[v] = (paramType != -1) ? s_soundParamDefaults[paramType] : 0.0f;
	}

	switch (type)
	{
	case SOUND_NODE_CONST:
		nodeDesc.c.paramId = paramType;
		nodeDesc.c.valueCount = inputCount;
		break;
	case SOUND_NODE_INPUT:
		nodeDesc.input.rMin = 0.0f;
		nodeDesc.input.rMax = 1.0f;
		nodeDesc.input.valueCount = inputCount;
		break;
	case SOUND_NODE_FUNC:
		nodeDesc.func.inputCount = inputCount;
		nodeDesc.func.outputCount = outputCount;
		nodeDesc.func.type = SOUND_FUNC_COPY;
		for (int v = 0; v < SoundNodeDesc::MAX_ARRAY_IDX; ++v)
			nodeDesc.func.inputIds[v] = SOUND_VAR_INVALID;
		break;
	}
}

void SoundScriptDesc::ParseDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection, const KVSection* defaultsSec)
{
	// initialize first
	scriptDesc.randomSample = false;

	for (int paramType = 0; paramType < SOUND_PARAM_COUNT; ++paramType)
		scriptDesc.paramNodeMap[paramType] = SOUND_VAR_INVALID;

	// pick 'rndwave' or 'wave' sections for lists
	KVSection* waveKey = waveKey = scriptSection->FindSection("wave", KV_FLAG_SECTION);

	if (!waveKey)
	{
		waveKey = scriptSection->FindSection("rndwave", KV_FLAG_SECTION);
		scriptDesc.randomSample = true;
	}

	if (waveKey)
	{
		for (const KVSection* ent : waveKey->Keys("wave"))
		{
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

	auto makeSampleParameter = [&](const KVSection& valKey, ESoundParamType paramType) {
		const int sampleIdx = KV_GetValueInt(&valKey);
		const char* valueStr = KV_GetValueString(&valKey, 1, nullptr);

		if (!valueStr)
			return;

		const int inputCount = scriptDesc.soundFileNames.numElem();

		char nodeNameWithPrefix[32]{ 0 };
		snprintf(nodeNameWithPrefix, sizeof(nodeNameWithPrefix), "out_%s", s_soundParamNames[paramType]);

		const uint8 nodeIdx = scriptDesc.FindVariableIndex(nodeNameWithPrefix);
		if (nodeIdx == SOUND_VAR_INVALID)
		{
			// make new node
			scriptDesc.paramNodeMap[paramType] = nodeDescs.numElem();

			SoundNodeDesc& nodeDesc = nodeDescs.append();
			strncpy(nodeDesc.name, nodeNameWithPrefix, sizeof(nodeDesc.name));
			nodeDesc.name[sizeof(nodeDesc.name) - 1] = 0;
			nodeDesc.flags = SOUND_NODE_FLAG_AUTOGENERATED | SOUND_NODE_FLAG_OUTPUT;

			InitSoundNode(nodeDesc, SOUND_NODE_CONST, paramType, inputCount, inputCount);
			const uint8 inputId = findInputVarOrMakeConst(nodeDesc, sampleIdx, valueStr);
			if (inputId != SOUND_VAR_INVALID)
			{
				InitSoundNode(nodeDesc, SOUND_NODE_FUNC, paramType, inputCount, inputCount);
				nodeDesc.func.inputIds[sampleIdx] = inputId;
			}
		}
		else
		{
			SoundNodeDesc& nodeDesc = nodeDescs[nodeIdx];

			const uint8 inputId = findInputVarOrMakeConst(nodeDesc, sampleIdx, valueStr);
			if (inputId != SOUND_VAR_INVALID)
			{
				// re-qualify existing node into function
				if(nodeDesc.type != SOUND_NODE_FUNC)
					InitSoundNode(nodeDesc, SOUND_NODE_FUNC, paramType, inputCount, inputCount, false);
				nodeDesc.func.inputIds[sampleIdx] = inputId;
			}
		}
	};

	// parse inputs, mixers, outputs
	for (const KVSection* valKey : scriptSection->Keys())
	{
		if (valKey->IsSection())
			continue;

		if (!valKey->name.CompareCaseIns("input"))
		{
			const char* nodeName = KV_GetValueString(valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' input %s: name is required\n", scriptDesc.name.ToCString(), valKey->name.ToCString());
				continue;
			}

			ASSERT_MSG(scriptDesc.FindVariableIndex(nodeName) == 0xff, "Node %s was already declared", nodeName);

			const int nodeIdx = nodeDescs.numElem();

			ASSERT_MSG(nodeDescs.numElem()+1 < SoundNodeDesc::MAX_SOUND_NODES, "Too many nodes in %s", scriptDesc.name.ToCString());

			SoundNodeDesc& inputDesc = nodeDescs.append();
			strncpy(inputDesc.name, nodeName, sizeof(inputDesc.name));
			inputDesc.name[sizeof(inputDesc.name) - 1] = 0;

			InitSoundNode(inputDesc, SOUND_NODE_INPUT);

			// TODO: support array index
			inputDesc.input.rMin = KV_GetValueFloat(valKey, 1, 0.0f);
			inputDesc.input.rMax = KV_GetValueFloat(valKey, 2, 1.0f);

			scriptDesc.inputNodeMap.insert(StringToHash(nodeName), nodeIdx);
		}
		else if (!valKey->name.CompareCaseIns("mixer"))
		{
			const char* nodeName = KV_GetValueString(valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' mixer: name is required\n", scriptDesc.name.ToCString());
				continue;
			}
			ASSERT_MSG(scriptDesc.FindVariableIndex(nodeName) == 0xff, "Node %s was already declared", nodeName);

			const char* funcTypeName = KV_GetValueString(valKey, 1, "");
			int funcType = GetSoundFuncTypeByString(funcTypeName);
			if (funcType == -1)
			{
				MsgError("sound script '%s' mixer: %s unknown func type %s\n", scriptDesc.name.ToCString(), valKey->name.ToCString(), funcTypeName);
				continue;
			}

			SoundNodeDesc& funcDesc = nodeDescs.append();
			strncpy(funcDesc.name, nodeName, sizeof(funcDesc.name));
			funcDesc.name[sizeof(funcDesc.name) - 1] = 0;

			InitSoundNode(funcDesc, SOUND_NODE_FUNC);
			funcDesc.func.type = funcType;

			// parse format for each type
			switch ((ESoundFuncType)funcType)
			{
				case SOUND_FUNC_ABS:
				{
					// N args
					int nArg = 0;
					for (int v = 2; v < valKey->ValueCount(); ++v)
					{
						const char* valName = KV_GetValueString(valKey, v, nullptr);
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
						const char* valName = KV_GetValueString(valKey, v + 2, nullptr);
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
					for (int v = 2; v < valKey->ValueCount(); ++v)
					{
						const char* valName = KV_GetValueString(valKey, v, nullptr);
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
					const char* inputValName = KV_GetValueString(valKey, 2, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), valKey->name.ToCString());
						continue;
					}

					funcDesc.func.outputCount = 1;
					funcDesc.func.inputCount = 1;

					funcDesc.func.inputIds[0] = findInputVarOrMakeConst(funcDesc, 0, inputValName);
					funcDesc.func.inputIds[1] = splineDescs.numElem();
					
					SoundSplineDesc& spline = splineDescs.append();

					int nArg = 0;
					for (int v = 3; v < valKey->ValueCount(); ++v)
					{
						if(nArg < SoundSplineDesc::MAX_SPLINE_POINTS * 2)
							spline.values[nArg++] = KV_GetValueFloat(valKey, v, 0.5f);
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
					const int numOutputs = KV_GetValueInt(valKey, 2, 0);
					if (!numOutputs)
					{
						MsgError("sound script '%s' mixer %s: no outputs for fade\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					const char* inputValName = KV_GetValueString(valKey, 3, nullptr);
					if (!inputValName)
					{
						MsgError("sound script '%s' mixer %s: insufficient args\n", scriptDesc.name.ToCString(), funcDesc.name);
						continue;
					}

					funcDesc.func.outputCount = numOutputs;
					funcDesc.func.inputCount = 1;

					funcDesc.func.inputIds[0] = findInputVarOrMakeConst(funcDesc, 0, inputValName);
					funcDesc.func.inputIds[1] = splineDescs.numElem();

					SoundSplineDesc& spline = splineDescs.append();

					int nArg = 0;
					for (int v = 4; v < valKey->ValueCount(); ++v)
					{
						if (nArg < SoundSplineDesc::MAX_SPLINE_POINTS * 2)
							spline.values[nArg++] = KV_GetValueFloat(valKey, v, 0.5f);
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
		} // input, mixer

		if (!valKey->name.CompareCaseIns(s_soundParamNames[SOUND_PARAM_SAMPLE_VOLUME]))
			makeSampleParameter(*valKey, SOUND_PARAM_SAMPLE_VOLUME);

		if (!valKey->name.CompareCaseIns(s_soundParamNames[SOUND_PARAM_SAMPLE_PITCH]))
			makeSampleParameter(*valKey, SOUND_PARAM_SAMPLE_PITCH);

	} // for kv keys

	auto sectionGetOrDefault = [scriptSection, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection->FindSection(name);
		if (!sec && defaultsSec)
			sec = defaultsSec->FindSection(name);
		return sec;
	};

	// parse constants and create outputs with default values
	for (int paramType = 0; paramType < SOUND_PARAM_COUNT; ++paramType)
	{
		if (scriptDesc.paramNodeMap[paramType] != SOUND_VAR_INVALID)
			continue;

		scriptDesc.paramNodeMap[paramType] = nodeDescs.numElem();

		// add parameters (const or non-const)
		SoundNodeDesc& nodeDesc = nodeDescs.append();
		snprintf(nodeDesc.name, sizeof(nodeDesc.name), "out_%s", s_soundParamNames[paramType]);
		nodeDesc.name[sizeof(nodeDesc.name) - 1] = 0;

		InitSoundNode(nodeDesc, SOUND_NODE_CONST, paramType, (paramType >= SOUND_PARAM_SAMPLE_VOLUME) ? scriptDesc.soundFileNames.numElem() : 1);
		nodeDesc.flags = SOUND_NODE_FLAG_AUTOGENERATED | SOUND_NODE_FLAG_OUTPUT;

		const KVSection* constSec = sectionGetOrDefault(s_soundParamNames[paramType]);
		const char* valueStr = KV_GetValueString(constSec, 0, nullptr);
		if (!valueStr)
			continue;

		ASSERT_MSG(paramType < SOUND_PARAM_SAMPLE_VOLUME, "ParseDesc must handle svolume & spitch before constants");

		const uint8 nodeId = findInputVarOrMakeConst(nodeDesc, 0, valueStr);
		if (nodeId != SOUND_VAR_INVALID)
		{
			// re-quialify into function
			InitSoundNode(nodeDesc, SOUND_NODE_FUNC, paramType, 1, 1, false);
			nodeDesc.func.inputIds[0] = nodeId;
		}
	}
}

void SoundScriptDesc::ReloadDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection)
{
	// before clearing out nodeDescs, fill defaults
	KVSection defaultsSec;
	for (int paramType = 0; paramType < SOUND_PARAM_SAMPLE_VOLUME; ++paramType)
	{
		const SoundNodeDesc& nodeDesc = scriptDesc.nodeDescs[scriptDesc.paramNodeMap[paramType]];
		if (nodeDesc.type == SOUND_NODE_CONST)
			defaultsSec.SetKey(s_soundParamNames[paramType], nodeDesc.inputConst[0]);
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
		samplePitch[i] = 1.0f;
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