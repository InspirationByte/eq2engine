//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system script editor
//////////////////////////////////////////////////////////////////////////////////

#if !defined(_RETAIL) && !defined(_PROFILE)
#define SOUNDSCRIPT_EDITOR
#endif

#ifdef SOUNDSCRIPT_EDITOR
#include <imgui.h>
#include <imgui_curve_edit.h>
#include <imnodes.h>

#include "core/core_common.h"
#include "utils/KeyValues.h"
#include "eqSoundEmitterSystem.h"
#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"

#pragma optimize("", off)

struct UISoundNodeDesc
{
	enum Side : int
	{
		LHS = 1,
		RHS = 2
	};

	SoundCurveDesc	curve;
	int				id;
	char			name[30];
	uint8			type{ 0 };
	uint8			flags{ 0 };

	float			lhsValue[SoundNodeDesc::MAX_ARRAY_IDX]{ 0.0f };

	union
	{
		struct {
			int	lhs[SoundNodeDesc::MAX_ARRAY_IDX];
			int	rhs[SoundNodeDesc::MAX_ARRAY_IDX];
			int	inputCount;
			int	outputCount;
			
			int	type;
		} func;
		struct {
			int		rhs[SoundNodeDesc::MAX_ARRAY_IDX];
			int		valueCount;

			float	rMin;
			float	rMax;
		} input;
		struct {
			int		lhs[SoundNodeDesc::MAX_ARRAY_IDX]; // in case of SOUND_NODE_FLAG_OUTPUT
			int		inputCount;

			int		rhs;
			uint8	paramId;
		} c;
	};

	void	SetupNode(ESoundNodeType type);
	void	SetupFuncNode(ESoundFuncType funcType);
	int		GetOutputCount(bool variadicConnections) const;
	int		GetInputCount(bool variadicConnections) const;
};

struct UINodeLink
{
	short a;
	short b;
};

class CSoundScriptEditor
{
public:
	static void DrawScriptEditor(bool& open);

	static void DrawNodeEditor(bool initializePositions);
	static void InitNodesFromScriptDesc(const SoundScriptDesc& script);

	static void GetUsedNodes(Set<int>& usedNodes);
	static void SerializeToKeyValues(KVSection& out);
	static void SerializeNodesToKeyValues(KVSection& out);

	static void ShowCurveEditor();

	static int GetConnectionCount(int attribId);
	static void GetConnections(int attribId, Array<UINodeLink>& links);
	static void DisconnectLHSAttrib(int attribId);

	static UISoundNodeDesc& SpawnNode();

	static SoundScriptDesc* s_currentSelection;
	static Map<int, UISoundNodeDesc> s_uiNodes;

	static Map<int, UINodeLink> s_uiNodeLinks;
	static int s_linkIdGenerator;
	static int s_idGenerator;

	static UISoundNodeDesc* s_currentEditingCurveNode;
};

#endif

void SoundScriptEditorUIDraw(bool& open)
{
#ifdef SOUNDSCRIPT_EDITOR
	CSoundScriptEditor::DrawScriptEditor(open);
#endif // SOUNDSCRIPT_EDITOR
}

#ifdef SOUNDSCRIPT_EDITOR

static bool SoundScriptDescTextGetter(void* data, int idx, const char** outStr)
{
	Array<SoundScriptDesc*>& allSounds = *reinterpret_cast<Array<SoundScriptDesc*>*>(data);
	*outStr = allSounds[idx]->name.ToCString();
	return true;
}

SoundScriptDesc* CSoundScriptEditor::s_currentSelection = nullptr;
Map<int, UISoundNodeDesc> CSoundScriptEditor::s_uiNodes{ PP_SL };
Map<int, UINodeLink> CSoundScriptEditor::s_uiNodeLinks{ PP_SL };
int CSoundScriptEditor::s_linkIdGenerator{ 0 };
int CSoundScriptEditor::s_idGenerator{ 0 };

UISoundNodeDesc* CSoundScriptEditor::s_currentEditingCurveNode = nullptr;

static int MakeAttribId(int nodeId, int arrayIdx, int side)
{
	// TODO: make use of constants like MAX_NODES, MAX_ARRAY_IDX
	return (nodeId & 31) | ((arrayIdx & 7) << 5) | ((side & 3) << 8);
}

static void UnpackAttribId(int attribId, int& nodeId, int& arrayIdx, int& side)
{
	// TODO: make use of constants like MAX_NODES, MAX_ARRAY_IDX
	nodeId = attribId & 31;
	arrayIdx = attribId >> 5 & 7;
	side = (UISoundNodeDesc::Side)(attribId >> 8 & 3);
}

static UISoundNodeDesc::Side GetAttribSide(int attribId)
{
	// TODO: make use of constants like MAX_NODES, MAX_ARRAY_IDX
	return (UISoundNodeDesc::Side)(attribId >> 8 & 3);
}

UISoundNodeDesc& CSoundScriptEditor::SpawnNode()
{
	const int newId = s_idGenerator++;
	UISoundNodeDesc& uiDesc = s_uiNodes[newId];
	uiDesc.id = newId;

	// make default name
	snprintf(uiDesc.name, sizeof(uiDesc.name), "node%d", newId);
	uiDesc.name[sizeof(uiDesc.name) - 1] = 0;

	return uiDesc;
}

int CSoundScriptEditor::GetConnectionCount(int attribId)
{
	int numCon = 0;
	for (auto it = s_uiNodeLinks.begin(); it != s_uiNodeLinks.end(); ++it)
	{
		const UINodeLink& linkVal = *it;

		if (attribId == linkVal.a || attribId == linkVal.b)
			++numCon;
	}
	return numCon;
}

void CSoundScriptEditor::GetConnections(int attribId, Array<UINodeLink>& links)
{
	int numCon = 0;
	for (auto it = s_uiNodeLinks.begin(); it != s_uiNodeLinks.end(); ++it)
	{
		const UINodeLink& linkVal = *it;

		if (attribId == linkVal.a || attribId == linkVal.b)
			links.append(linkVal);
	}
}

void CSoundScriptEditor::DisconnectLHSAttrib(int attribId)
{
	if (GetAttribSide(attribId) != UISoundNodeDesc::LHS)
		return;

	for (auto it = s_uiNodeLinks.begin(); it != s_uiNodeLinks.end(); ++it)
	{
		const UINodeLink linkVal = *it;
		if (linkVal.a == attribId || linkVal.b == attribId)
		{
			s_uiNodeLinks.remove(it);
			break;
		}
	}
}

void UISoundNodeDesc::SetupNode(ESoundNodeType setupType)
{
	curve.Reset();
	type = setupType;
	if (type == SOUND_NODE_INPUT)
	{
		input.rMin = 0.0f;
		input.rMax = 1.0f;
		input.valueCount = 1;

		for (int i = 0; i < SoundNodeDesc::MAX_ARRAY_IDX; ++i)
			input.rhs[i] = MakeAttribId(id, i, RHS);
	}
	else if (type == SOUND_NODE_CONST)
	{
		for (int i = 0; i < SoundNodeDesc::MAX_ARRAY_IDX; ++i)
			c.lhs[i] = MakeAttribId(id, i, LHS);

		c.inputCount = 1;
		c.rhs = MakeAttribId(id, 0, RHS);
	}
	else if (type == SOUND_NODE_FUNC)
	{
		// depends on function, need further setup
		func.type = -1;
		func.inputCount = 1;
		func.outputCount = 1;

		for (int i = 0; i < SoundNodeDesc::MAX_ARRAY_IDX; ++i)
			func.lhs[i] = MakeAttribId(id, i, LHS);

		for (int i = 0; i < SoundNodeDesc::MAX_ARRAY_IDX; ++i)
			func.rhs[i] = MakeAttribId(id, i, RHS);
	}
}

void UISoundNodeDesc::SetupFuncNode(ESoundFuncType funcType)
{
	SetupNode(SOUND_NODE_FUNC);
	func.type = funcType;
	func.inputCount = s_soundFuncTypeDesc[funcType].argCount;
	func.outputCount = s_soundFuncTypeDesc[funcType].retCount;
}

int UISoundNodeDesc::GetOutputCount(bool variadicConnections) const
{
	if (type == SOUND_NODE_INPUT)
	{
		return input.valueCount;
	}
	else if (type == SOUND_NODE_CONST)
	{
		return 1;
	}
	else if (type == SOUND_NODE_FUNC)
	{
		int outputCount = func.outputCount;
		if (outputCount == SOUND_FUNC_ARG_VARIADIC)
		{
			outputCount = SoundNodeDesc::MAX_ARRAY_IDX;
			while (variadicConnections && outputCount > 1 && CSoundScriptEditor::GetConnectionCount(func.rhs[outputCount - 1]) == 0)
				outputCount--;
		}
		return outputCount;
	}
	return 0;
}

int UISoundNodeDesc::GetInputCount(bool variadicConnections) const
{
	if (type == SOUND_NODE_INPUT)
	{
		return 0;
	}
	else if (type == SOUND_NODE_CONST)
	{
		return c.inputCount;
	}
	else if (type == SOUND_NODE_FUNC)
	{
		int inputCount = func.inputCount;
		if (inputCount == SOUND_FUNC_ARG_VARIADIC)
		{
			inputCount = SoundNodeDesc::MAX_ARRAY_IDX;
			while (variadicConnections &&inputCount > 1 && CSoundScriptEditor::GetConnectionCount(func.lhs[inputCount - 1]) == 0)
				inputCount--;
		}
		return inputCount;
	}
	return 0;
}

void CSoundScriptEditor::InitNodesFromScriptDesc(const SoundScriptDesc& script)
{
	s_linkIdGenerator = 0;
	s_idGenerator = script.nodeDescs.numElem();
	s_uiNodes.clear();
	s_uiNodeLinks.clear();

	// NOTE: node ID when reading existing script is made by script execution order

	for (int nodeId = 0; nodeId < script.nodeDescs.numElem(); ++nodeId)
	{
		const SoundNodeDesc& nodeDesc = script.nodeDescs[nodeId];

		UISoundNodeDesc& uiDesc = s_uiNodes[nodeId];
		uiDesc.id = nodeId;
		uiDesc.SetupNode((ESoundNodeType)nodeDesc.type);

		strncpy(uiDesc.name, nodeDesc.name, sizeof(uiDesc.name));
		uiDesc.name[sizeof(uiDesc.name) - 1] = 0;

		uiDesc.flags = nodeDesc.flags;

		if (nodeDesc.type == SOUND_NODE_INPUT)
		{
			uiDesc.input.rMin = nodeDesc.input.rMin;
			uiDesc.input.rMax = nodeDesc.input.rMax;
			uiDesc.input.valueCount = nodeDesc.input.valueCount;
		}
		else if (nodeDesc.type == SOUND_NODE_CONST)
		{
			uiDesc.c.inputCount = 1;

			for (int i = 0; i < SOUND_PARAM_COUNT; ++i)
			{
				if (nodeId == script.paramNodeMap[i])
				{
					uiDesc.c.paramId = i;
					break;
				}
			}

			if (uiDesc.c.paramId == SOUND_PARAM_SAMPLE_VOLUME)
				uiDesc.c.inputCount = script.soundFileNames.numElem();

			uiDesc.lhsValue[0] = nodeDesc.c.value;
		}
		else if (nodeDesc.type == SOUND_NODE_FUNC)
		{
			// convert to output as it is clearly an output
			// it will be converted back to COPY
			// once deserialized from KVS
			if (nodeDesc.func.type == SOUND_FUNC_COPY)
			{
				uiDesc.type = SOUND_NODE_CONST;

				uiDesc.c.rhs = MakeAttribId(uiDesc.id, 0, UISoundNodeDesc::RHS);
				uiDesc.c.inputCount = nodeDesc.func.inputCount;

				for (int i = 0; i < SOUND_PARAM_COUNT; ++i)
				{
					if (nodeId == script.paramNodeMap[i])
					{
						uiDesc.c.paramId = i;
						break;
					}
				}

				if (uiDesc.c.paramId == SOUND_PARAM_SAMPLE_VOLUME)
					uiDesc.c.inputCount = script.soundFileNames.numElem();

				for (int i = 0; i < nodeDesc.func.inputCount; ++i)
				{
					uiDesc.c.lhs[i] = MakeAttribId(uiDesc.id, i, UISoundNodeDesc::LHS);

					// also add links
					uint inNodeId, inArrayIdx;
					SoundNodeDesc::UnpackInputIdArrIdx(nodeDesc.func.inputIds[i], inNodeId, inArrayIdx);
					s_uiNodeLinks.insert(s_linkIdGenerator++, { (short)MakeAttribId(inNodeId, inArrayIdx, 2), (short)uiDesc.c.lhs[i] });
				}
				continue;
			}

			uiDesc.func.type = nodeDesc.func.type;
			uiDesc.func.inputCount = nodeDesc.func.inputCount;
			uiDesc.func.outputCount = nodeDesc.func.outputCount;

			// add links
			for (int i = 0; i < uiDesc.func.inputCount; ++i)
			{
				uint inNodeId, inArrayIdx;
				SoundNodeDesc::UnpackInputIdArrIdx(nodeDesc.func.inputIds[i], inNodeId, inArrayIdx);
				s_uiNodeLinks.insert(s_linkIdGenerator++, { (short)MakeAttribId(inNodeId, inArrayIdx, 2), (short)uiDesc.func.lhs[i] });
			}

			// copy curve properties
			if (nodeDesc.func.type == SOUND_FUNC_CURVE || nodeDesc.func.type == SOUND_FUNC_FADE)
			{
				const int curveIdx = nodeDesc.func.inputIds[1];
				uiDesc.curve = script.curveDescs[curveIdx];
			}

		} // if
	} // for

	// remove "out_" prefix from output names
	for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
	{
		UISoundNodeDesc& uiNode = *it;
		if (uiNode.flags & SOUND_NODE_FLAG_OUTPUT)
		{
			const int outLen = strlen("out_");
			memmove(uiNode.name, uiNode.name + outLen, strlen(uiNode.name) - outLen + 1);
		}
	}
}

void CSoundScriptEditor::GetUsedNodes(Set<int>& usedNodes)
{
	Array<const UISoundNodeDesc*> nodeQueue{ PP_SL };
	Set<int>& visited = usedNodes;

	// backward iterate from outputs
	for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
	{
		UISoundNodeDesc& uiNode = *it;
		if (uiNode.flags & SOUND_NODE_FLAG_OUTPUT)
		{
			nodeQueue.append(&uiNode);
			visited.insert(uiNode.id);
			continue;
		}
	}

	Array<UINodeLink> foundLinks{ PP_SL };

	while (nodeQueue.numElem())
	{
		const UISoundNodeDesc& uiNode = *nodeQueue.popBack();

		visited.insert(uiNode.id);

		const int* attribIds = nullptr;
		int valCount = 0;

		if (uiNode.type == SOUND_NODE_CONST)
		{
			attribIds = uiNode.c.lhs;
			valCount = uiNode.c.inputCount;
		}
		else if (uiNode.type == SOUND_NODE_FUNC)
		{
			attribIds = uiNode.func.lhs;
			valCount = uiNode.func.inputCount == SOUND_FUNC_ARG_VARIADIC ? SoundNodeDesc::MAX_ARRAY_IDX : uiNode.func.inputCount;
		}

		for (int i = 0; i < valCount; ++i)
		{
			foundLinks.clear();

			// find link
			GetConnections(attribIds[i], foundLinks);
			if (!foundLinks.numElem())
				continue;

			UINodeLink link = foundLinks[0];
			const int oppositeAttribId = (link.a == attribIds[i]) ? link.b : link.a;

			const int nextNodeid = oppositeAttribId & 31;
			if (!visited.contains(nextNodeid))
				nodeQueue.append(&s_uiNodes[nextNodeid]);
		}
	}
}

void CSoundScriptEditor::SerializeToKeyValues(KVSection& out)
{
	SerializeNodesToKeyValues(out);
}

void CSoundScriptEditor::SerializeNodesToKeyValues(KVSection& out)
{
	Set<int> usedNodes{ PP_SL };
	GetUsedNodes(usedNodes);

	Array<const UISoundNodeDesc*> nodeQueue{ PP_SL };
	Set<int> visited{ PP_SL };
	Map<int, int> nodeDistances{ PP_SL };
	Array<UINodeLink> foundLinks{ PP_SL };

	static char* typeNameStr[] = {
		"const",
		"input",
		"mixer,"
	};

	for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
	{
		const UISoundNodeDesc& uiNode = *it;
		if (uiNode.type == SOUND_NODE_INPUT)
		{
			nodeQueue.append(&uiNode);
			visited.insert(uiNode.id);
			nodeDistances[uiNode.id] = 0;
			continue;
		}	
	}


	// start from outputs
	while (nodeQueue.numElem())
	{
		const UISoundNodeDesc& uiNode = *nodeQueue.popFront();

		if (!usedNodes.contains(uiNode.id))
		{
			MsgWarning("Node %d %s won't make effect and will be deleted\n", uiNode.id, uiNode.name);
			continue;
		}

		const int* attribIds = nullptr;
		int valCount = 0;

		if (uiNode.type == SOUND_NODE_INPUT)
		{
			attribIds = uiNode.input.rhs;
			valCount = (uiNode.input.valueCount == SOUND_FUNC_ARG_VARIADIC) ? SoundNodeDesc::MAX_ARRAY_IDX : uiNode.input.valueCount;
		}
		else if (uiNode.type == SOUND_NODE_FUNC)
		{
			attribIds = uiNode.func.rhs;
			valCount = (uiNode.func.outputCount == SOUND_FUNC_ARG_VARIADIC) ? SoundNodeDesc::MAX_ARRAY_IDX : uiNode.func.outputCount;
		}

		for (int i = 0; i < valCount; ++i)
		{
			foundLinks.clear();
			GetConnections(attribIds[i], foundLinks);

			for (int j = 0; j < foundLinks.numElem(); ++j)
			{
				UINodeLink link = foundLinks[j];
				const int oppositeAttribId = (link.a == attribIds[i]) ? link.b : link.a;

				const int nextNodeid = oppositeAttribId & 31;
				nodeDistances[nextNodeid] = nodeDistances[uiNode.id] + 1;

				if (!visited.contains(nextNodeid))
				{
					visited.insert(nextNodeid);
					nodeQueue.append(&s_uiNodes[nextNodeid]);
				}
			}
		}

		if (uiNode.type == SOUND_NODE_INPUT)
		{
			KVSection* sec = out.CreateSection("input");
			sec->AddValue(uiNode.name);
		}
		else if (uiNode.type == SOUND_NODE_FUNC)
		{
			KVSection* sec = out.CreateSection("mixer");
			sec->AddValue(uiNode.name);
			sec->AddValue(s_soundFuncTypeDesc[uiNode.func.type].name);

			switch (uiNode.func.type)
			{
				case SOUND_FUNC_ADD:
				case SOUND_FUNC_SUB:
				case SOUND_FUNC_MUL:
				case SOUND_FUNC_DIV:
				case SOUND_FUNC_MIN:
				case SOUND_FUNC_MAX:
				case SOUND_FUNC_AVERAGE:
				{
					int inputCount = uiNode.GetInputCount(true);

					// add inputs
					for(int i = 0; i < inputCount; ++i)
					{
						foundLinks.clear();
						GetConnections(uiNode.func.lhs[i], foundLinks);

						if (!foundLinks.numElem())
						{
							// FIXME: this is actually unsupported in runtime!
							sec->AddValue(uiNode.lhsValue[i]);
							continue;
						}

						const int oppositeAttribId = (foundLinks[0].a == uiNode.func.lhs[i]) ? foundLinks[0].b : foundLinks[0].a;

						int nodeId, arrayIdx, side;
						UnpackAttribId(oppositeAttribId, nodeId, arrayIdx, side);

						const UISoundNodeDesc& conNode = s_uiNodes[nodeId];
						if (conNode.GetOutputCount(false) > 1 || arrayIdx > 0)
							sec->AddValue(EqString::Format("%s[%d]", s_uiNodes[nodeId].name, arrayIdx));
						else
							sec->AddValue(EqString::Format("%s", s_uiNodes[nodeId].name, arrayIdx));
					}

					break;
				}
				case SOUND_FUNC_FADE:
				{
					// add number of outputs
					sec->AddValue(valCount);
				}
				case SOUND_FUNC_CURVE:
				{
					// add input
					{
						foundLinks.clear();
						GetConnections(uiNode.func.lhs[0], foundLinks);

						ASSERT_MSG(foundLinks.numElem() > 0, "message to programmer: consider using usedNodes pls");
						const int oppositeAttribId = (foundLinks[0].a == uiNode.func.lhs[0]) ? foundLinks[0].b : foundLinks[0].a;

						int nodeId, arrayIdx, side;
						UnpackAttribId(oppositeAttribId, nodeId, arrayIdx, side);

						const UISoundNodeDesc& conNode = s_uiNodes[nodeId];
						if(conNode.GetOutputCount(false) > 1 || arrayIdx > 0)
							sec->AddValue(EqString::Format("%s[%d]", s_uiNodes[nodeId].name, arrayIdx));
						else
							sec->AddValue(EqString::Format("%s", s_uiNodes[nodeId].name, arrayIdx));
					}

					// add curve values
					for (int i = 0; i < uiNode.curve.valueCount; ++i)
						sec->AddValue(uiNode.curve.values[i]);

					break;
				}
			}
		}

		if (uiNode.type == SOUND_NODE_CONST)
		{
			visited.clear();
		}
	}

	// last to process are always outputs
	for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
	{
		UISoundNodeDesc& uiNode = *it;
		if (!(uiNode.flags & SOUND_NODE_FLAG_OUTPUT))
			continue;

		// add inputs
		for (int i = 0; i < uiNode.c.inputCount; ++i)
		{
			foundLinks.clear();
			GetConnections(uiNode.c.lhs[i], foundLinks);

			if (!foundLinks.numElem())
			{
				// add a numeric constant and we're done
				if (uiNode.c.paramId != SOUND_PARAM_SAMPLE_VOLUME && !fsimilar(s_soundParamDefaults[uiNode.c.paramId], uiNode.lhsValue[i]))
				{
					KVSection* sec = out.CreateSection(uiNode.name);
					sec->AddValue(uiNode.lhsValue[i]);
				}
				continue;
			}

			KVSection* sec = out.CreateSection(uiNode.name);

			if (uiNode.c.inputCount > 1)
				sec->AddValue(i);

			const int oppositeAttribId = (foundLinks[0].a == uiNode.c.lhs[i]) ? foundLinks[0].b : foundLinks[0].a;

			int nodeId, arrayIdx, side;
			UnpackAttribId(oppositeAttribId, nodeId, arrayIdx, side);

			const UISoundNodeDesc& conNode = s_uiNodes[nodeId];
			if (conNode.GetOutputCount(false) > 1 || arrayIdx > 0)
				sec->AddValue(EqString::Format("%s[%d]", s_uiNodes[nodeId].name, arrayIdx));
			else
				sec->AddValue(EqString::Format("%s", s_uiNodes[nodeId].name, arrayIdx));
		}
	}
}

//------------------------------------------------------------------------

void CSoundScriptEditor::ShowCurveEditor()
{
	if (!s_currentEditingCurveNode)
		return;

	SoundCurveDesc& curve = s_currentEditingCurveNode->curve;
	static ImVec2 curveRangeMin(F_INFINITY, 0);
	static ImVec2 curveRangeMax(-F_INFINITY, 0);
	if(curveRangeMin.x >= F_INFINITY * 0.5f)
	{
		// auto-detect range on initialization
		ImVec2* curvePoints = (ImVec2*)curve.values;
		Rectangle_t rct;
		for (int i = 0; i < curve.valueCount / 2; ++i)
		{
			rct.AddVertex(Vector2D(curvePoints[i].x, curvePoints[i].y));
			rct.AddVertex(Vector2D(curvePoints[i].x, curvePoints[i].y));
		}
		curveRangeMin = ImVec2(rct.vleftTop.x, rct.vleftTop.y);
		curveRangeMax = ImVec2(rct.vrightBottom.x, rct.vrightBottom.y);
	}

	bool open = true;
	if (ImGui::Begin("Curve Editor", &open))
	{
		static int curveSelection = -1;

		
		ImGui::Text("Range X");
		ImGui::PushItemWidth(150.0f);
		ImGui::SameLine();
		ImGui::InputFloat("##rx_min", &curveRangeMin.x, 0.1f, 0.25f, "%.2f");
		ImGui::SameLine();
		ImGui::InputFloat("##rx_max", &curveRangeMax.x, 0.1f, 0.25f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::Text("Range Y");
		ImGui::PushItemWidth(150.0f);
		ImGui::SameLine();
		ImGui::InputFloat("##ry_min", &curveRangeMin.y, 0.1f, 0.25f, "%.2f");
		ImGui::SameLine();
		ImGui::InputFloat("##ry_max", &curveRangeMax.y, 0.1f, 0.25f, "%.2f");
		ImGui::PopItemWidth();

		if (ImGui::Curve(s_currentEditingCurveNode->name, ImVec2(600, 200), SoundCurveDesc::MAX_CURVE_POINTS, (ImVec2*)curve.values, &curveSelection, curveRangeMin, curveRangeMax))
		{
			// Recalc point count
			int pointCount = 0;
			while (pointCount < SoundCurveDesc::MAX_CURVE_POINTS && curve.values[pointCount*2] >= curveRangeMin.x)
				pointCount++;

			curve.valueCount = pointCount * 2;
		}
		ImGui::End();
	}

	if (!open)
	{
		s_currentEditingCurveNode = nullptr;
		curveRangeMin = ImVec2(F_INFINITY, 0);
		curveRangeMax = ImVec2(-F_INFINITY, 0);
	}
}

void CSoundScriptEditor::DrawNodeEditor(bool initializePositions)
{
	// adjust node positions
	if (initializePositions)
	{
		int posCounter = 0;

		int countByType[SOUND_NODE_TYPE_COUNT]{ 0 };
		int countByFunc[SOUND_FUNC_COUNT]{ 0 };
		for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
		{
			UISoundNodeDesc& uiNode = *it;

			if (countByType[uiNode.type] == 0)
			{
				++posCounter;
			}
			else
			{
				if (uiNode.type == SOUND_NODE_FUNC && countByFunc[uiNode.func.type] == 0)
					++posCounter;
			}

			ImNodes::SetNodeGridSpacePos(uiNode.id, ImVec2(posCounter * 170.0f, countByType[uiNode.type] * 80.0f));

			if (uiNode.type == SOUND_NODE_FUNC)
				++countByFunc[uiNode.func.type];

			++countByType[uiNode.type];
		}
	}

	// Handle new nodes
	// These are driven by the user, so we place this code before rendering the nodes
	{
		const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
			ImNodes::IsEditorHovered() &&
			ImGui::IsMouseClicked(1);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
		if (!ImGui::IsAnyItemHovered() && open_popup)
		{
			ImGui::OpenPopup("NodeContextMenu");
		}

		if (ImGui::BeginPopup("NodeContextMenu"))
		{
			const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

			if (ImGui::BeginMenu("Add Node"))
			{
				if (ImGui::MenuItem("Input"))
				{
					UISoundNodeDesc& nodeDesc = SpawnNode();
					ImNodes::SetNodeScreenSpacePos(nodeDesc.id, click_pos);
					nodeDesc.SetupNode(SOUND_NODE_INPUT);
				}
				if (ImGui::BeginMenu("Function"))
				{
					for (int i = SOUND_FUNC_ADD; i < SOUND_FUNC_COUNT; ++i)
					{
						if (ImGui::MenuItem(s_soundFuncTypeDesc[i].name))
						{
							UISoundNodeDesc& nodeDesc = SpawnNode();
							ImNodes::SetNodeScreenSpacePos(nodeDesc.id, click_pos);

							nodeDesc.SetupFuncNode((ESoundFuncType)i);
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();

			// handle links
			{
				const int numSelected = ImNodes::NumSelectedLinks();

				if (numSelected > 0 && ImGui::MenuItem(EqString::Format("Break %d link(s)", numSelected)))
				{
					Array<int> selectedLinks{ PP_SL };
					selectedLinks.setNum(numSelected);
					ImNodes::GetSelectedLinks(selectedLinks.ptr());

					for(int i = 0; i < selectedLinks.numElem(); ++i)
						s_uiNodeLinks.remove(selectedLinks[i]);

					ImNodes::ClearLinkSelection();
				}
			}

			// handle nodes
			{
				const int numSelected = ImNodes::NumSelectedNodes();

				if (numSelected > 0 && ImGui::MenuItem(EqString::Format("Delete %d node(s)", numSelected)))
				{
					Array<int> selectedNodes{ PP_SL };
					selectedNodes.setNum(numSelected);
					ImNodes::GetSelectedNodes(selectedNodes.ptr());

					for (int i = 0; i < selectedNodes.numElem(); ++i)
					{
						const int nodeId = selectedNodes[i];

						// block deletion of output nodes
						// TODO: somehow represent this in the counter above
						if (s_uiNodes[nodeId].flags & SOUND_NODE_FLAG_OUTPUT)
							continue;

						// delete links associated with node we about to delete
						for (auto linkIt = s_uiNodeLinks.begin(); linkIt != s_uiNodeLinks.end(); ++linkIt)
						{
							UINodeLink& nodeLink = *linkIt;
							if ((nodeLink.a & 31) == nodeId || (nodeLink.b & 31) == nodeId)
							{
								s_uiNodeLinks.remove(linkIt);
							}
						}

						s_uiNodes.remove(nodeId);
					}

					ImNodes::ClearNodeSelection();
				}
			}

			ImGui::EndPopup();
		}
		ImGui::PopStyleVar();
	}

	for (auto it = s_uiNodes.begin(); it != s_uiNodes.end(); ++it)
	{
		UISoundNodeDesc& uiNode = *it;

		const float node_width = 100.0f;

		if (uiNode.type == SOUND_NODE_CONST)
		{
			// it is a output into the sound source
			if (uiNode.flags & SOUND_NODE_FLAG_OUTPUT)
			{
				ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(200, 45, 45, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(220, 65, 65, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(240, 85, 85, 255));

				ImNodes::BeginNode(uiNode.id);

				ImNodes::BeginNodeTitleBar();
				ImGui::TextUnformatted(uiNode.name);
				ImNodes::EndNodeTitleBar();

				// draw inputs
				{
					const int inputCount = uiNode.c.inputCount;
					for (int i = 0; i < inputCount; ++i)
					{
						EqString str = (inputCount > 1) ? EqString::Format("in[%d]", i) : "in";

						ImNodes::BeginInputAttribute(uiNode.c.lhs[i]);
						const float label_width = ImGui::CalcTextSize(str).x;
						ImGui::TextUnformatted(str);
						if (!GetConnectionCount(uiNode.c.lhs[i]))
						{
							ImGui::SameLine();
							ImGui::PushItemWidth(node_width - label_width);
							ImGui::DragFloat("##hidelabel", &uiNode.lhsValue[i], 0.01f);
							ImGui::PopItemWidth();
						}
						ImNodes::EndInputAttribute();
					}
				}

				ImNodes::EndNode();

				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();
				ImNodes::PopColorStyle();
			}
		}
		else if (uiNode.type == SOUND_NODE_INPUT)
		{
			ImNodes::BeginNode(uiNode.id);

			ImNodes::BeginNodeTitleBar();
			ImGui::SetNextItemWidth(node_width);
			ImGui::InputText("##hid", uiNode.name, sizeof(uiNode.name));
			ImNodes::EndNodeTitleBar();

			const int inputCount = uiNode.c.inputCount;
			for (int i = 0; i < inputCount; ++i)
			{
				EqString str = (inputCount > 1) ? EqString::Format("out[%d]", i) : "out";

				ImNodes::BeginOutputAttribute(uiNode.input.rhs[i]);
				const float label_width = ImGui::CalcTextSize(str).x;
				ImGui::Indent(node_width - label_width);

				ImGui::TextUnformatted(str);

				ImNodes::EndOutputAttribute();
			}

			ImNodes::EndNode();
		}
		else if (uiNode.type == SOUND_NODE_FUNC)
		{
			ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(255, 109, 72, 255));
			ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(255, 126, 72, 255));
			ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(255, 148, 72, 255));

			ImNodes::BeginNode(uiNode.id);

			ImNodes::BeginNodeTitleBar();
			ImGui::TextUnformatted(s_soundFuncTypeDesc[uiNode.func.type].name);
			ImNodes::EndNodeTitleBar();

			// draw inputs
			{
				int inputCount = uiNode.func.inputCount;
				if (inputCount == SOUND_FUNC_ARG_VARIADIC)
				{
					inputCount = SoundNodeDesc::MAX_ARRAY_IDX;
					while (inputCount > 1 && GetConnectionCount(uiNode.func.lhs[inputCount - 1]) == 0)
						inputCount--;
					if (inputCount < SoundNodeDesc::MAX_ARRAY_IDX)
						inputCount++;
				}

				for (int i = 0; i < inputCount; ++i)
				{
					EqString str = (inputCount > 1) ? EqString::Format("in[%d]", i) : "in";

					ImNodes::BeginInputAttribute(uiNode.func.lhs[i]);
					const float label_width = ImGui::CalcTextSize(str).x;
					ImGui::TextUnformatted(str);

					if (uiNode.func.inputCount != SOUND_FUNC_ARG_VARIADIC && !GetConnectionCount(uiNode.func.lhs[i]))
					{
						ImGui::SameLine();
						ImGui::PushItemWidth(node_width - label_width);
						ImGui::DragFloat("##hidelabel", &uiNode.lhsValue[i], 0.01f);
						ImGui::PopItemWidth();
					}
					ImNodes::EndInputAttribute();
				}
			}

			// draw curve if it's curve
			if (uiNode.func.type == SOUND_FUNC_CURVE || uiNode.func.type == SOUND_FUNC_FADE)
			{
				SoundCurveDesc& curve = uiNode.curve;
				ImVec2* curvePoints = (ImVec2*)curve.values;
				Rectangle_t rct;
				for (int i = 0; i < curve.valueCount / 2; ++i)
				{
					rct.AddVertex(Vector2D(curvePoints[i].x, curvePoints[i].y + 0.25f));
					rct.AddVertex(Vector2D(curvePoints[i].x, curvePoints[i].y - 0.25f));
				}

				ImVec2 curveRangeMin(rct.vleftTop.x, rct.vleftTop.y);
				ImVec2 curveRangeMax(rct.vrightBottom.x, rct.vrightBottom.y);
				if (ImGui::CurveFrame(uiNode.name, ImVec2(120, 50), 10, curvePoints, curveRangeMin, curveRangeMax))
				{
					s_currentEditingCurveNode = &uiNode;
				}
			}

			// draw outputs
			{
				int outputCount = uiNode.func.outputCount;
				if (outputCount == SOUND_FUNC_ARG_VARIADIC)
				{
					outputCount = SoundNodeDesc::MAX_ARRAY_IDX;
					while (outputCount > 1 && GetConnectionCount(uiNode.func.rhs[outputCount - 1]) == 0)
						outputCount--;
					if (outputCount < SoundNodeDesc::MAX_ARRAY_IDX)
						outputCount++;
				}

				for (int i = 0; i < outputCount; ++i)
				{
					EqString str = (outputCount > 1) ? EqString::Format("out[%d]", i) : "out";

					{
						ImNodes::BeginOutputAttribute(uiNode.func.rhs[i]);
						const float label_width = ImGui::CalcTextSize(str).x;
						ImGui::Indent(node_width - label_width);
						ImGui::TextUnformatted(str);
						ImNodes::EndOutputAttribute();
					}
				}
			}

			ImNodes::EndNode();

			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();
			ImNodes::PopColorStyle();
		}
	}

	// link nodes
	for (auto it = s_uiNodeLinks.begin(); it != s_uiNodeLinks.end(); ++it)
	{
		const UINodeLink linkVal = *it;
		ImNodes::Link(it.key(), linkVal.a, linkVal.b);
	}

	ImNodes::EndNodeEditor();

	// process events

	{
		int startAttr, endAttr;
		if (ImNodes::IsLinkCreated(&startAttr, &endAttr))
		{
			// remove existing connection that is on LHS
			DisconnectLHSAttrib(startAttr);
			DisconnectLHSAttrib(endAttr);

			s_uiNodeLinks.insert(s_linkIdGenerator++, { (short)startAttr, (short)endAttr });
		}
	}

	{
		int link_id;
		if (ImNodes::IsLinkDestroyed(&link_id))
		{
			s_uiNodeLinks.remove(link_id);
		}
	}

	ShowCurveEditor();
}

void CSoundScriptEditor::DrawScriptEditor(bool& open)
{
	if (open && ImGui::Begin("Sound Script Editor", &open))
	{
		static char filterText[64]{ 0 };
		ImGui::SetNextItemWidth(120);
		ImGui::InputText("Filter", filterText, sizeof(filterText));
		ImGui::SameLine();
		Array<SoundScriptDesc*> allSounds(PP_SL);
		g_sounds->GetAllSoundsList(allSounds);

		if (strlen(filterText))
		{
			for (int i = 0; i < allSounds.numElem(); ++i)
			{
				if (allSounds[i]->name.Find(filterText) == -1)
					allSounds.fastRemoveIndex(i--);
			}
		}

		allSounds.sort([](SoundScriptDesc* a, SoundScriptDesc* b) {
			return a->name.Compare(b->name);
		});

		static int curSelection = 0;
		ImGui::SetNextItemWidth(300);
		ImGui::Combo("Script", &curSelection, SoundScriptDescTextGetter, &allSounds, allSounds.numElem());

		// show sound script properties
		if (allSounds.inRange(curSelection))
		{
			SoundScriptDesc* selectedScript = allSounds[curSelection];
			static bool justChanged = false;

			if (s_currentSelection != selectedScript)
			{
				// TODO: prompt to save unsaved settings!!!
				s_currentSelection = selectedScript;
				InitNodesFromScriptDesc(*selectedScript);
				justChanged = true;
			}

			struct EmitPair
			{
				int objId;
				int emitId;
				CSoundingObject* obj;
				SoundEmitterData* emitter;
			};

			static CSoundingObject soundTest;
			static EmitPair currentEmit{ 0, 0, &soundTest, nullptr };
			static bool isolateSound = false;

			// playback controls
			{
				ImGui::Separator();

				if (ImGui::Button("Play"))
				{
					if (currentEmit.obj->GetEmitterState(0) == IEqAudioSource::STOPPED)
					{
						EmitParams snd(selectedScript->name);
						snd.flags |= EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D;

						currentEmit.obj->EmitSound(currentEmit.emitId, &snd);
					}

					currentEmit.obj->PlayEmitter(currentEmit.emitId);
				}

				ImGui::SameLine();
				if (ImGui::Button("Pause"))
					currentEmit.obj->PauseEmitter(currentEmit.emitId);

				ImGui::SameLine();
				if (ImGui::Button("Stop"))
					currentEmit.obj->StopEmitter(currentEmit.emitId);
				ImGui::SameLine();

				ImGui::Checkbox("Isolate", &isolateSound);

				ImGui::SameLine();

				// collect instance list
				// and also validate selection
				static int currentEmitterIdx = -1;
				Array<EmitPair> emittersPlaying{ PP_SL };
				{
					bool isValidEmitter = false;
					int objId = 0;
					for (auto sObjIt = g_sounds->m_soundingObjects.begin(); sObjIt != g_sounds->m_soundingObjects.end(); ++sObjIt, ++objId)
					{
						CSoundingObject* sObj = sObjIt.key();
						for (auto emitterIt = sObj->m_emitters.begin(); emitterIt != sObj->m_emitters.end(); ++emitterIt)
						{
							if (selectedScript != (*emitterIt)->script)
								continue;

							if (currentEmit.emitter && currentEmit.emitter == *emitterIt)
							{
								currentEmitterIdx = emittersPlaying.numElem();
								isValidEmitter = true;
							}

							emittersPlaying.append({ objId, emitterIt.key(), sObj, *emitterIt });
						}
					}

					if (!isValidEmitter)
					{
						currentEmit.objId = 0;
						currentEmit.emitId = 0;
						currentEmit.obj = &soundTest;
						currentEmit.emitter = nullptr;

						currentEmitterIdx = -1;
					}
				}

				auto GetEmitterFriendlyName = [](EmitPair& emit) {
					return EqString::Format("[%d-%x] %s", emit.objId, emit.emitId, emit.emitter->script->name.ToCString());
				};

				ImGui::SetNextItemWidth(210);
				if (ImGui::BeginCombo("Display instance", currentEmitterIdx == -1 ? "" : GetEmitterFriendlyName(emittersPlaying[currentEmitterIdx])))
				{
					ImGui::PushID("ed_inputs");
					for (int row = 0; row < emittersPlaying.numElem(); ++row)
					{
						ImGui::PushID(row);
						if (ImGui::Selectable(GetEmitterFriendlyName(emittersPlaying[row]), row == currentEmitterIdx, 0, ImVec2(180, 0)))
						{
							currentEmit = emittersPlaying[row];
							currentEmitterIdx = row;
						}

						ImGui::PopID();
					}
					ImGui::PopID();
					ImGui::EndCombo();
				}
			}

			g_sounds->m_isolateSound = isolateSound ? selectedScript : nullptr;

			ImGui::Separator();

			if (ImGui::BeginChild("params_view", ImVec2(0, -ImGui::GetTextLineHeightWithSpacing() * 2)))
			{
				if (ImGui::BeginTabBar("##tab"))
				{
					// Simple parameters section
					{
						bool scriptRandomSample = selectedScript->randomSample;
						bool scriptLoop = selectedScript->loop;
						bool scriptIs2D = selectedScript->is2d;
						bool modified = false;

						if (ImGui::BeginTabItem("Sources"))
						{
							ImGui::Text("Sound filenames");
							ImGui::PushID("sound_names");
							ImGui::PushItemWidth(300.0f);
							for (int i = 0; i < selectedScript->soundFileNames.numElem(); ++i)
							{
								EqString& name = selectedScript->soundFileNames[i];
								char nameStrBuffer[128];
								strncpy(nameStrBuffer, name, sizeof(nameStrBuffer));
								nameStrBuffer[sizeof(nameStrBuffer) - 1] = 0;

								ImGui::PushID(i);
								if (ImGui::InputText("##v", nameStrBuffer, sizeof(nameStrBuffer)))
									name.Assign(nameStrBuffer);
								ImGui::SameLine();

								if (ImGui::SmallButton("del"))
									selectedScript->soundFileNames.removeIndex(i--);

								ImGui::PopID();
							}
							ImGui::PopID();
							ImGui::PopItemWidth();

							if (ImGui::Button("Add Sound"))
								selectedScript->soundFileNames.append("sound-file-name.wav");

							modified = ImGui::Checkbox("Randomized sample##script_rndWave", &scriptRandomSample) || modified;

							ImGui::EndTabItem();
						}

						if (ImGui::BeginTabItem("Parameters"))
						{
							{
								modified = ImGui::Checkbox("Loop##script_loop", &scriptLoop) || modified;
								ImGui::SameLine();
								modified = ImGui::Checkbox("Is 2D##script_is2d", &scriptIs2D) || modified;

								ImGui::PushItemWidth(300.0f);
								ArrayCRef<ChannelDef> channelTypes(g_sounds->m_channelTypes);
								if (ImGui::BeginCombo("Channel Type", selectedScript->channelType == -1 ? "CHAN_INVALID" : channelTypes[selectedScript->channelType].name))
								{
									ImGui::PushID("channelTypes");
									for (int row = 0; row < channelTypes.numElem(); ++row)
									{
										ImGui::PushID(row);
										if (ImGui::Selectable(channelTypes[row].name, row == selectedScript->channelType))
										{
											selectedScript->channelType = row;
											modified = true;
										}

										ImGui::PopID();
									}
									ImGui::PopID();
									ImGui::EndCombo();
								}

								modified = ImGui::SliderFloat("Max Distance##script_maxDistance", &selectedScript->maxDistance, 10.0f, 200.0f, "%.2f", 1.0f) || modified;

								ImGui::PushID("paramIds");
								for (int paramId = 0; paramId < SOUND_PARAM_SAMPLE_VOLUME; ++paramId)
								{
									const uint8 nodeId = selectedScript->paramNodeMap[paramId];
									SoundNodeDesc& desc = selectedScript->nodeDescs[nodeId];
									if (desc.type != SOUND_NODE_CONST)
										continue;

									ImGui::PushID(paramId);
									modified = ImGui::SliderFloat(s_soundParamNames[paramId], &desc.c.value, 0.0f, s_soundParamLimits[paramId], "%.3f", 1.0f) || modified;
									ImGui::PopID();
								}
								ImGui::PopItemWidth();
								ImGui::PopID();
							}

							ImGui::EndTabItem();
						}

						if (modified)
						{
							selectedScript->randomSample = scriptRandomSample;
							selectedScript->loop = scriptLoop;
							selectedScript->is2d = scriptIs2D;
							g_sounds->RestartEmittersByScript(selectedScript);
						}
					} // End of simple parameters

					if (ImGui::BeginTabItem("Automation"))
					{
						ImNodes::BeginNodeEditor();

						DrawNodeEditor(justChanged);
						justChanged = false;

						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Playback"))
					{
						if (currentEmit.obj->GetEmitterState(currentEmit.emitId) != IEqAudioSource::STOPPED)
						{
							SoundEmitterData* emitter = currentEmit.obj->m_emitters[currentEmit.emitId];

							IEqAudioSource* src = emitter->soundSource;
							ASSERT(src);

							static float playbackPitch = 1.0f;
							static float playbackVolume = 1.0f;

							// inputs here
							{
								playbackPitch = emitter->epPitch;
								playbackVolume = emitter->epVolume;
							}

							bool modified = false;
							{
								ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(0 / 7.0f, 0.5f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(0 / 7.0f, 0.6f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(0 / 7.0f, 0.7f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(0 / 7.0f, 0.9f, 0.9f));
								modified = ImGui::VSliderFloat("P##play_pitch", ImVec2(18, 160), &playbackPitch, 0.01f, 4.0f, "") || modified;
								ImGui::SameLine();
								if (ImGui::IsItemActive() || ImGui::IsItemHovered())
									ImGui::SetTooltip("Pitch %.3f", playbackPitch);
								ImGui::PopStyleColor(4);
							}
							{
								ImGui::PushStyleColor(ImGuiCol_FrameBg, (ImVec4)ImColor::HSV(1 / 7.0f, 0.5f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, (ImVec4)ImColor::HSV(1 / 7.0f, 0.6f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_FrameBgActive, (ImVec4)ImColor::HSV(1 / 7.0f, 0.7f, 0.5f));
								ImGui::PushStyleColor(ImGuiCol_SliderGrab, (ImVec4)ImColor::HSV(1 / 7.0f, 0.9f, 0.9f));
								modified = ImGui::VSliderFloat("M##play_volume", ImVec2(18, 160), &playbackVolume, 0.0f, 1.0f, "") || modified;

								if (ImGui::IsItemActive() || ImGui::IsItemHovered())
									ImGui::SetTooltip("Volume %.3f", playbackVolume);
								ImGui::PopStyleColor(4);
							}
							for (int i = 0; i < src->GetSampleCount(); ++i)
							{
								ImGui::SameLine();

								float sampleVolume = src->GetSampleVolume(i);
								if (ImGui::VSliderFloat(EqString::Format("%d##smpl%dv", i + 1, i), ImVec2(18, 160), &sampleVolume, 0.0f, 1.0f, ""))
								{
									src->SetSampleVolume(i, sampleVolume);
								}
								if (ImGui::IsItemActive() || ImGui::IsItemHovered())
									ImGui::SetTooltip("Sample %d volume %.3f", i + 1, sampleVolume);

							}

							if (modified)
							{
								currentEmit.obj->SetVolume(emitter, playbackVolume);
								currentEmit.obj->SetPitch(emitter, playbackPitch);
							}

							ImGui::SameLine();

							if (ImGui::BeginTable("Inputs##playbackInputs", 2, ImGuiTableFlags_Borders))
							{
								ImGui::TableSetupColumn("name");
								ImGui::TableSetupColumn("value");

								ImGui::PushID("inputs");
								Array<SoundNodeDesc>& nodeDescs = selectedScript->nodeDescs;
								for (int nodeId = 0; nodeId < nodeDescs.numElem(); ++nodeId)
								{
									SoundNodeDesc& desc = selectedScript->nodeDescs[nodeId];
									if (desc.type != SOUND_NODE_INPUT)
										continue;

									ImGui::PushID(nodeId);

									for (int i = 0; i < desc.input.valueCount; ++i)
									{
										ImGui::TableNextRow();
										ImGui::PushID(i);

										ImGui::TableSetColumnIndex(0);
										ImGui::Text(desc.name);
										ImGui::TableSetColumnIndex(1);

										float inputVal = emitter->GetInputValue(nodeId, i);
										if (ImGui::SliderFloat("##v", &inputVal, desc.input.rMin, desc.input.rMax, "%.3f", 1.0f))
										{
											uint8 inputId = SoundNodeDesc::PackInputIdArrIdx(nodeId, i);
											emitter->SetInputValue(inputId, inputVal);
										}
										ImGui::PopID();
									}

									ImGui::PopID();
								}
								ImGui::PopID();

								ImGui::EndTable();
							}
						}
						else
						{
							ImGui::Text("Press play to see parameters here");
						}

						ImGui::EndTabItem();
					}

					ImGui::EndTabBar();
				}
				ImGui::EndChild();
			}


			{
				if (ImGui::Button("Revert"))
				{

				}
				ImGui::SameLine();
				if (ImGui::Button("Copy to clipboard"))
				{
					KVSection testSec;
					SerializeToKeyValues(testSec);

					KV_PrintSection(&testSec);

					// TODO:
				}
			}
		}
		else
			g_sounds->m_isolateSound = nullptr;

		ImGui::End();
	}
}

#endif // SOUNDSCRIPT_EDITOR