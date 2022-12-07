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
#include "eqSoundEmitterSystem.h"
#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"

#pragma optimize("", off)

struct UISoundNodeDesc
{
	int				id;
	char			name[30];
	uint8			type{ 0 };
	uint8			flags{ 0 };

	union
	{
		struct {
			SoundCurveDesc curve;
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
			float	value;
		} c;
	};
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
	static void SerializeToKeyValues(KVSection& out);

	static void ShowCurveEditor();

	static int InsertConstNode(float value);
	static bool IsConnected(int link);

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

bool SoundScriptDescTextGetter(void* data, int idx, const char** outStr)
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

int CSoundScriptEditor::InsertConstNode(float value)
{
	const int newId = s_idGenerator++;
	UISoundNodeDesc& uiDesc = s_uiNodes[newId];
	uiDesc.id = newId;
	uiDesc.type = SOUND_NODE_CONST;
	uiDesc.c.value = value;

	return newId;
}

static int MakeAttribId(int id, int arrayIdx, int side)
{
	return (id & 31) | ((arrayIdx & 7) << 5) | ((side & 3) << 8);
}

bool CSoundScriptEditor::IsConnected(int link)
{
	for (auto it = s_uiNodeLinks.begin(); it != s_uiNodeLinks.end(); ++it)
	{
		const UINodeLink& linkVal = *it;

		if (link == linkVal.a || link == linkVal.b)
			return true;
	}
	return false;
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

		strncpy(uiDesc.name, nodeDesc.name, sizeof(uiDesc.name));
		uiDesc.name[sizeof(uiDesc.name) - 1] = 0;

		uiDesc.flags = nodeDesc.flags;
		uiDesc.type = nodeDesc.type;

		if (nodeDesc.type == SOUND_NODE_INPUT)
		{
			uiDesc.input.rMin = nodeDesc.input.rMin;
			uiDesc.input.rMax = nodeDesc.input.rMax;
			uiDesc.input.valueCount = nodeDesc.input.valueCount;

			for (int i = 0; i < uiDesc.input.valueCount; ++i)
				uiDesc.input.rhs[i] = MakeAttribId(uiDesc.id, i, 2);
		}
		else if (nodeDesc.type == SOUND_NODE_CONST)
		{
			uiDesc.c.lhs[0] = MakeAttribId(uiDesc.id, 0, 1);
			uiDesc.c.inputCount = 1;
			uiDesc.c.rhs = MakeAttribId(uiDesc.id, 0, 2);
			uiDesc.c.value = nodeDesc.c.value;
		}
		else if (nodeDesc.type == SOUND_NODE_FUNC)
		{
			// convert to output as it is clearly an output
			if (nodeDesc.func.type == SOUND_FUNC_COPY)
			{
				uiDesc.type = SOUND_NODE_CONST;

				uiDesc.c.rhs = MakeAttribId(uiDesc.id, 0, 2);
				uiDesc.c.value = 1.0f;
				uiDesc.c.inputCount = nodeDesc.func.inputCount;

				for (int i = 0; i < nodeDesc.func.inputCount; ++i)
				{
					uiDesc.c.lhs[i] = MakeAttribId(uiDesc.id, i, 1);

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

			for (int i = 0; i < uiDesc.func.inputCount; ++i)
			{
				uiDesc.func.lhs[i] = MakeAttribId(uiDesc.id, i, 1);

				// also add links
				uint inNodeId, inArrayIdx;
				SoundNodeDesc::UnpackInputIdArrIdx(nodeDesc.func.inputIds[i], inNodeId, inArrayIdx);
				s_uiNodeLinks.insert(s_linkIdGenerator++, { (short)MakeAttribId(inNodeId, inArrayIdx, 2), (short)uiDesc.func.lhs[i] });
			}

			for (int i = 0; i < uiDesc.func.outputCount; ++i)
			{
				uiDesc.func.rhs[i] = MakeAttribId(uiDesc.id, i, 2);
			}

			// copy curve properties
			if (nodeDesc.func.type == SOUND_FUNC_CURVE || nodeDesc.func.type == SOUND_FUNC_FADE)
			{
				const int curveIdx = nodeDesc.func.inputIds[1];
				uiDesc.func.curve = script.curveDescs[curveIdx];
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

void CSoundScriptEditor::SerializeToKeyValues(KVSection& out)
{

}

void CSoundScriptEditor::ShowCurveEditor()
{
	if (!s_currentEditingCurveNode)
		return;

	SoundCurveDesc& curve = s_currentEditingCurveNode->func.curve;
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

			// ImNodes::SetNodeScreenSpacePos(ui_node.id, click_pos);

			if (ImGui::BeginMenu("Add Node"))
			{
				if (ImGui::MenuItem("Input"))
				{

				}
				if (ImGui::BeginMenu("Function"))
				{
					for (int i = SOUND_FUNC_ADD; i < SOUND_FUNC_COUNT; ++i)
					{
						if (ImGui::MenuItem(s_soundFuncTypeNames[i]))
						{

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
				ImNodes::PushColorStyle(ImNodesCol_TitleBar, IM_COL32(11, 109, 191, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarHovered, IM_COL32(45, 126, 194, 255));
				ImNodes::PushColorStyle(ImNodesCol_TitleBarSelected, IM_COL32(81, 148, 204, 255));

				ImNodes::BeginNode(uiNode.id);

				ImNodes::BeginNodeTitleBar();
				ImGui::TextUnformatted(uiNode.name);
				ImNodes::EndNodeTitleBar();

				// draw inputs
				for (int i = 0; i < uiNode.c.inputCount; ++i)
				{
					EqString str = (uiNode.c.inputCount > 1) ? EqString::Format("in[%d]", i) : "in";

					ImNodes::BeginInputAttribute(uiNode.c.lhs[i]);
					const float label_width = ImGui::CalcTextSize(str).x;
					ImGui::TextUnformatted(str);
					if (!IsConnected(uiNode.c.lhs[i]))
					{
						ImGui::SameLine();
						ImGui::PushItemWidth(node_width - label_width);
						ImGui::DragFloat("##hidelabel", &uiNode.c.value, 0.01f);
						ImGui::PopItemWidth();
					}
					ImNodes::EndInputAttribute();
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

			for (int i = 0; i < uiNode.input.valueCount; ++i)
			{
				EqString str = (uiNode.input.valueCount > 1) ? EqString::Format("out[%d]", i) : "out";

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
			ImGui::TextUnformatted(s_soundFuncTypeNames[uiNode.func.type]);
			ImNodes::EndNodeTitleBar();

			// draw inputs
			for (int i = 0; i < uiNode.func.inputCount; ++i)
			{
				EqString str = (uiNode.func.inputCount > 1) ? EqString::Format("in[%d]", i) : "in";

				ImNodes::BeginInputAttribute(uiNode.func.lhs[i]);
				const float label_width = ImGui::CalcTextSize(str).x;
				ImGui::TextUnformatted(str);
				if (!IsConnected(uiNode.func.lhs[i]))
				{
					ImGui::SameLine();
					ImGui::PushItemWidth(node_width - label_width);
					ImGui::DragFloat("##hidelabel", &uiNode.c.value, 0.01f);
					ImGui::PopItemWidth();
				}
				ImNodes::EndInputAttribute();
			}

			// draw curve if it's curve
			if (uiNode.func.type == SOUND_FUNC_CURVE || uiNode.func.type == SOUND_FUNC_FADE)
			{
				SoundCurveDesc& curve = uiNode.func.curve;
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
			for (int i = 0; i < uiNode.func.outputCount; ++i)
			{
				EqString str = (uiNode.func.outputCount > 1) ? EqString::Format("out[%d]", i) : "out";

				{
					ImNodes::BeginOutputAttribute(uiNode.func.rhs[i]);
					const float label_width = ImGui::CalcTextSize(str).x;
					ImGui::Indent(node_width - label_width);
					ImGui::TextUnformatted(str);
					ImNodes::EndOutputAttribute();
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
		int start_attr, end_attr;
		if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
		{
			s_uiNodeLinks.insert(s_linkIdGenerator++, { (short)start_attr, (short)end_attr });
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

			static float playbackPitch = 1.0f;
			static float playbackVolume = 1.0f;

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
				if (ImGui::BeginCombo("Display instance", currentEmitterIdx == -1 ? "<debug one>" : GetEmitterFriendlyName(emittersPlaying[currentEmitterIdx])))
				{
					ImGui::PushID("ed_inputs");
					for (int row = 0; row < emittersPlaying.numElem(); ++row)
					{
						// Draw our contents
						static float dummy_f = 0.0f;
						ImGui::PushID(row);
						if (ImGui::Selectable(GetEmitterFriendlyName(emittersPlaying[row]), false, 0, ImVec2(180, 0)))
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

			if (ImGui::BeginTabBar("##tab"))
			{
				if (ImGui::BeginTabItem("Parameters"))
				{
					bool modified = false;

					if (ImGui::Button("Revert"))
					{

					}
					ImGui::SameLine();
					if (ImGui::Button("Copy to clipboard"))
					{
						// TODO:
					}

					ImGui::Separator();

					ImGui::Text("Sound filenames");
					ImGui::PushID("sound_names");
					for (int i = 0; i < selectedScript->soundFileNames.numElem(); ++i)
					{
						EqString& name = selectedScript->soundFileNames[i];
						char nameStrBuffer[128];
						strncpy(nameStrBuffer, name, sizeof(nameStrBuffer));
						nameStrBuffer[sizeof(nameStrBuffer)-1] = 0;

						ImGui::PushID(i);
						if (ImGui::InputText("##v", nameStrBuffer, sizeof(nameStrBuffer)))
							name.Assign(nameStrBuffer);
						ImGui::SameLine();

						if (ImGui::SmallButton("del"))
							selectedScript->soundFileNames.removeIndex(i--);

						ImGui::PopID();
					}
					ImGui::PopID();
					if (ImGui::Button("Add Sound"))
						selectedScript->soundFileNames.append("sound-file-name.wav");

					bool scriptLoop = selectedScript->loop;
					bool scriptIs2D = selectedScript->is2d;
					bool scriptRandomSample = selectedScript->randomSample;
					modified = ImGui::Checkbox("Randomized sample##script_rndWave", &scriptRandomSample) || modified;
					ImGui::Separator();

					modified = ImGui::Checkbox("Loop##script_loop", &scriptLoop) || modified;
					ImGui::SameLine();
					modified = ImGui::Checkbox("Is 2D##script_is2d", &scriptIs2D) || modified;

					ImGui::PushItemWidth(300.0f);
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

					if (modified)
					{
						selectedScript->randomSample = scriptRandomSample;
						selectedScript->loop = scriptLoop;
						selectedScript->is2d = scriptIs2D;
						g_sounds->RestartEmittersByScript(selectedScript);
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Playback"))
				{
					if (currentEmit.obj->GetEmitterState(currentEmit.emitId) != IEqAudioSource::STOPPED)
					{
						SoundEmitterData* emitter = currentEmit.obj->m_emitters[currentEmit.emitId];

						IEqAudioSource* src = emitter->soundSource;
						if (src)
						{
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
					}
					else
					{
						ImGui::Text("Press play to see parameters here");
					}
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Automation"))
				{
					ImNodes::BeginNodeEditor();

					DrawNodeEditor(justChanged);
					justChanged = false;

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		else
			g_sounds->m_isolateSound = nullptr;

		ImGui::End();
	}
}

#endif // SOUNDSCRIPT_EDITOR