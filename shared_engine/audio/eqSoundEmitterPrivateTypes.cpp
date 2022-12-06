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

SoundScriptDesc::SoundScriptDesc(const char* name) 
	: name(name)
{
}

const ISoundSource* SoundScriptDesc::GetBestSample(int sampleId /*= -1*/) const
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
		else
			return samples[RandomInt(0, numSamples - 1)];
	}
	else
		return samples[sampleId];
}

uint8 SoundScriptDesc::FindVariableIndex(const char* varName) const
{
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
			return 0xff;
		}

		// find closing
		arrSub = strchr(arrSub, ']');
		if (!arrSub)
		{
			MsgError("sound script '%s' mixer: missing ']' for %s\n", name.ToCString(), tmpName);
			return 0xff;
		}
		*arrSub = 0;
		arrayIdx = atoi(numberStart);
	}

	const int valIdx = nodeDescs.findIndex([tmpName](const SoundNodeDesc& desc) {
		return !strcmp(desc.name, tmpName);
	});

	if (valIdx == -1)
	{
		MsgError("sound script '%s': unknown var %s\n", name.ToCString(), tmpName);
		return 0xff;
	}

	return SoundNodeDesc::PackInputIdArrIdx((uint)valIdx, arrayIdx);
}

void SoundScriptDesc::ParseDesc(SoundScriptDesc& scriptDesc, const KVSection* scriptSection)
{
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
	Array<SoundCurveDesc>& curveDescs = scriptDesc.curveDescs;

	// parse constants, inputs, mixers
	for (int i = 0; i < scriptSection->KeyCount(); ++i)
	{
		const KVSection& valKey = *scriptSection->keys[i];
		if (valKey.IsSection())
			continue;

		int nodeType = -1;
		if (!stricmp(valKey.name, "const"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' const: name is required\n");
				continue;
			}
			const int hashName = StringToHash(nodeName);
			nodeType = SOUND_NODE_CONST;

			SoundNodeDesc& constDesc = nodeDescs.append();
			constDesc.type = nodeType;
			strncpy(constDesc.name, nodeName, sizeof(constDesc.name));
			constDesc.name[sizeof(constDesc.name) - 1] = 0;

			constDesc.c.value = KV_GetValueFloat(&valKey, 1, 0.0f);
		}
		else if (!stricmp(valKey.name, "input"))
		{
			const char* nodeName = KV_GetValueString(&valKey, 0, nullptr);

			if (nodeName == nullptr || !nodeName[0])
			{
				MsgError("sound script '%s' input: name is required\n");
				continue;
			}
			nodeType = SOUND_NODE_INPUT;

			const int nodeIdx = nodeDescs.numElem();
			SoundNodeDesc& inputDesc = nodeDescs.append();
			inputDesc.type = nodeType;
			strncpy(inputDesc.name, nodeName, sizeof(inputDesc.name));
			inputDesc.name[sizeof(inputDesc.name) - 1] = 0;

			// FIXME: support array index?
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
			nodeType = SOUND_NODE_FUNC;
			const char* funcTypeName = KV_GetValueString(&valKey, 1, "");
			int funcType = GetSoundFuncTypeByString(funcTypeName);
			if (funcType == -1)
			{
				MsgError("sound script '%s' mixer: %s unknown\n", funcTypeName);
				continue;
			}

			SoundNodeDesc& funcDesc = nodeDescs.append();
			funcDesc.type = nodeType;
			funcDesc.func.type = funcType;

			strncpy(funcDesc.name, nodeName, sizeof(funcDesc.name));
			funcDesc.name[sizeof(funcDesc.name) - 1] = 0;

			// parse format for each type
			switch ((ESoundFuncType)funcType)
			{
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

						funcDesc.func.inputIds[v] = scriptDesc.FindVariableIndex(valName);
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
						funcDesc.func.inputIds[nArg++] = scriptDesc.FindVariableIndex(valName);
					}
					funcDesc.func.inputCount = nArg;
					funcDesc.func.outputCount = 1;
					break;
				}
				case SOUND_FUNC_CURVE:
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
					funcDesc.func.inputIds[0] = scriptDesc.FindVariableIndex(inputValName);
					funcDesc.func.inputIds[1] = curveDescs.numElem();
					
					SoundCurveDesc& curve = curveDescs.append();

					int nArg = 0;
					for (int v = 3; v < valKey.ValueCount(); ++v)
					{
						if(nArg < SoundCurveDesc::MAX_CURVE_POINTS * 2)
							curve.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}

					if (nArg < SoundCurveDesc::MAX_CURVE_POINTS * 2)
						curve.values[nArg] = -F_INFINITY;
					curve.valueCount = nArg;

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
					funcDesc.func.inputIds[0] = scriptDesc.FindVariableIndex(inputValName);
					funcDesc.func.inputIds[1] = curveDescs.numElem();

					SoundCurveDesc& curve = curveDescs.append();

					int nArg = 0;
					for (int v = 4; v < valKey.ValueCount(); ++v)
					{
						if (nArg < SoundCurveDesc::MAX_CURVE_POINTS * 2)
							curve.values[nArg++] = KV_GetValueFloat(&valKey, v, 0.5f);
					}

					if (nArg < SoundCurveDesc::MAX_CURVE_POINTS * 2)
						curve.values[nArg] = -F_INFINITY;
					curve.valueCount = nArg;

					if (nArg & 1)
					{
						MsgError("sound script '%s' mixer %s: uneven curve arguments\n", scriptDesc.name.ToCString(), funcDesc.name);
					}
					break;
				}
			} // switch funcType
		} // const, input, mixer
	} // for
}

void SoundEmitterData::CreateNodeRuntime()
{
	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;

	for (int i = 0; i < nodeDescs.numElem(); ++i)
	{
		const SoundNodeDesc& nodeDesc = nodeDescs[i];

		if (nodeDesc.type == SOUND_NODE_CONST)
			continue; // no need, we have desc to access

		nodeData.insert(i);
	}
}

void SoundEmitterData::SetInputValue(int inputNameHash, float value)
{
	auto it = script->inputNodeMap.find(inputNameHash);
	if (it == script->inputNodeMap.end())
		return;

	SetNodeValue(*it, 0, value);	// FIXME: support array index?
	nodesNeedUpdate = true;
}

void SoundEmitterData::SetNodeValue(int nodeId, int arrayIdx, float value)
{
	auto dataIt = nodeData.find(nodeId);
	if (dataIt == nodeData.end())
		return;

	SoundNodeRuntime& runtime = *dataIt;
	runtime.values[arrayIdx] = value;
}

float SoundEmitterData::GetNodeValue(int nodeId, int arrayIdx)
{
	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	if(nodeDescs[nodeId].type == SOUND_NODE_CONST)
		return nodeDescs[nodeId].c.value;

	auto dataIt = nodeData.find(nodeId);
	if (dataIt == nodeData.end())
	{
		// could be a const so retrieve it
		const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;

		if (nodeDescs[nodeId].type == SOUND_NODE_CONST)
			return nodeDescs[nodeId].c.value;

		return 0.0f;
	}

	SoundNodeRuntime& runtime = *dataIt;
	return runtime.values[arrayIdx];
}

float SoundEmitterData::GetNodeValue(uint8 inputId)
{
	if (inputId == 0xff)
		return 0.0f;

	uint nodeId, arrayIdx;
	SoundNodeDesc::UnpackInputIdArrIdx(inputId, nodeId, arrayIdx);

	return GetNodeValue(nodeId, arrayIdx);
}

void SoundEmitterData::UpdateNodes()
{
	if (!nodesNeedUpdate)
		return;
	nodesNeedUpdate = false;

	const Array<SoundNodeDesc>& nodeDescs = script->nodeDescs;
	const Array<SoundCurveDesc>& curveDescs = script->curveDescs;

	// evaluate each function node down
	for (int nodeId = 0; nodeId < nodeDescs.numElem(); ++nodeId)
	{
		const SoundNodeDesc& nodeDesc = nodeDescs[nodeId];

		if (nodeDesc.type != SOUND_NODE_FUNC)
			continue;

		switch ((ESoundFuncType)nodeDesc.func.type)
		{
			case SOUND_FUNC_ADD:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, operandA + operandB);
				break;
			}
			case SOUND_FUNC_SUB:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, operandA - operandB);
				break;
			}
			case SOUND_FUNC_MUL:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, operandA * operandB);
				break;
			}
			case SOUND_FUNC_DIV:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, operandA / operandB);
				break;
			}
			case SOUND_FUNC_MIN:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, min(operandA, operandB));
				break;
			}
			case SOUND_FUNC_MAX:
			{
				const float operandA = GetNodeValue(nodeDesc.func.inputIds[0]);
				const float operandB = GetNodeValue(nodeDesc.func.inputIds[1]);
				SetNodeValue(nodeId, 0, max(operandA, operandB));
				break;
			}
			case SOUND_FUNC_AVERAGE:
			{
				float value = 0.0f;
				for (int i = 0; i < nodeDesc.func.inputCount; ++i)
					value = GetNodeValue(nodeDesc.func.inputIds[i]);

				SetNodeValue(nodeId, 0, value / nodeDesc.func.inputCount);
				break;
			}
			case SOUND_FUNC_CURVE:
			{
				const float input = GetNodeValue(nodeDesc.func.inputIds[0]);
				const SoundCurveDesc& curveDesc = curveDescs[nodeDesc.func.inputIds[1]];

				float output = 0.0f;
				spline<1>(curveDesc.values, curveDesc.valueCount / 2, input, &output);
				SetNodeValue(nodeId, 0, output);
				break;
			}
			case SOUND_FUNC_FADE:
			{
				const float input = GetNodeValue(nodeDesc.func.inputIds[0]);
				const SoundCurveDesc& curveDesc = curveDescs[nodeDesc.func.inputIds[1]];

				float output = 0.0f;
				spline<1>(curveDesc.values, curveDesc.valueCount / 2, input, &output);

				// split one output into the number of outputs
				for (int i = 0; i < nodeDesc.func.outputCount; ++i)
				{
					const float targetValue = (float)i;
					const float fadeValue = clamp(1.0f - fabs(output - targetValue), 0.0f, 1.0f);
					SetNodeValue(nodeId, i, output);
				}

				break;
			}
		}
	}

	// TODO: output value mapping to sound parameters
	// possible values:
	//		svolume N <value>
	//		volume <value>
	// 		airAbsorption <value>
	//		rollOff <value>
	//		pitch <value>
	//		atten <value>
	//		hpf <value>
	//		lpf <value>
}