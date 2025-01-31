#include "core/core_common.h"
#include "eqSoundScript.h"
#include "math/Random.h"
#include "math/Spline.h"
#include "utils/KeyValues.h"

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

static const char* s_nodeTypeNameStr[] = {
	"const",
	"input",
	"mixer"
};

struct SoundFuncDesc
{
	EqStringRef name;
	uint8 argCount;
	uint8 retCount;
};

constexpr SoundFuncDesc s_soundFuncTypeDesc[] = {
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
		if (!s_soundFuncTypeDesc[i].name.CompareCaseIns(name))
			return (ESoundFuncType)i;
	}
	return -1;
}

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

void SoundScriptDesc::ParseDesc(SoundScriptDesc& scriptDesc, const KVSection& scriptSection, const KVSection* defaultsSec)
{
	// initialize first
	scriptDesc.randomSample = false;

	for (int paramType = 0; paramType < SOUND_PARAM_COUNT; ++paramType)
		scriptDesc.paramNodeMap[paramType] = SOUND_VAR_INVALID;

	// pick 'rndwave' or 'wave' sections for lists
	KVSection* waveKey = waveKey = scriptSection.FindSection("wave", KV_FLAG_SECTION);

	if (!waveKey)
	{
		waveKey = scriptSection.FindSection("rndwave", KV_FLAG_SECTION);
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
		waveKey = scriptSection.FindSection("wave");

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
	for (const KVSection* valKey : scriptSection.Keys())
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

			scriptDesc.inputNodeMap.insert(StringId24(nodeName), nodeIdx);
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
			const int funcType = GetSoundFuncTypeByString(funcTypeName);
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

	auto sectionGetOrDefault = [&, defaultsSec](const char* name) {
		const KVSection* sec = scriptSection.FindSection(name);
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

void SoundScriptDesc::ReloadDesc(SoundScriptDesc& scriptDesc, const KVSection& scriptSection)
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