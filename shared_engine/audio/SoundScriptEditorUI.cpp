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
	static void InitSoundNodesFromScriptDesc(const SoundScriptDesc& script);

	static int InsertConstNode(float value);
	static bool IsConnected(int link);

	static SoundScriptDesc* s_currentSelection;
	static Map<int, UISoundNodeDesc> s_uiNodes;
	static Array<UINodeLink> s_uiNodeLinks;
	static int s_idGenerator;
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
Array<UINodeLink> CSoundScriptEditor::s_uiNodeLinks{ PP_SL };
int CSoundScriptEditor::s_idGenerator{ 0 };

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
	for (int i = 0; i < s_uiNodeLinks.numElem(); ++i)
	{
		const UINodeLink& linkVal = s_uiNodeLinks[i];

		if (link == linkVal.a || link == linkVal.b)
			return true;
	}
	return false;
}

void CSoundScriptEditor::InitSoundNodesFromScriptDesc(const SoundScriptDesc& script)
{
	s_idGenerator = 0;
	s_uiNodes.clear();
	s_uiNodeLinks.clear();

	for (int nodeId = 0; nodeId < script.nodeDescs.numElem(); ++nodeId)
	{
		const SoundNodeDesc& nodeDesc = script.nodeDescs[nodeId];

		const int newId = s_idGenerator++;
		UISoundNodeDesc& uiDesc = s_uiNodes[newId];
		uiDesc.id = newId;

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
			if (nodeDesc.func.type == SOUND_FUNC_COPY)
			{
				// convert to output
				uiDesc.type = SOUND_NODE_CONST;
				uiDesc.flags |= SOUND_NODE_FLAG_OUTPUT;

				uiDesc.c.rhs = MakeAttribId(uiDesc.id, 0, 2);
				uiDesc.c.value = 1.0f;
				uiDesc.c.inputCount = nodeDesc.func.inputCount;

				for (int i = 0; i < nodeDesc.func.inputCount; ++i)
				{
					uiDesc.c.lhs[i] = MakeAttribId(uiDesc.id, i, 1);

					uint inNodeId, inArrayIdx;
					SoundNodeDesc::UnpackInputIdArrIdx(nodeDesc.func.inputIds[i], inNodeId, inArrayIdx);
					s_uiNodeLinks.append({ (short)MakeAttribId(inNodeId, inArrayIdx, 2), (short)uiDesc.c.lhs[i] });
				}
				continue;
			}

			uiDesc.func.type = nodeDesc.func.type;
			uiDesc.func.inputCount = nodeDesc.func.inputCount;
			uiDesc.func.outputCount = nodeDesc.func.outputCount;

			for (int i = 0; i < uiDesc.func.inputCount; ++i)
			{
				uiDesc.func.lhs[i] = MakeAttribId(uiDesc.id, i, 1);

				uint inNodeId, inArrayIdx;
				SoundNodeDesc::UnpackInputIdArrIdx(nodeDesc.func.inputIds[i], inNodeId, inArrayIdx);
				s_uiNodeLinks.append({ (short)MakeAttribId(inNodeId, inArrayIdx, 2), (short)uiDesc.func.lhs[i] });
			}

			for (int i = 0; i < uiDesc.func.outputCount; ++i)
			{
				uiDesc.func.rhs[i] = MakeAttribId(uiDesc.id, i, 2);
			}
		} // if
	} // for

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
		ImGui::Combo("Sound script", &curSelection, SoundScriptDescTextGetter, &allSounds, allSounds.numElem());

		// show sound script properties
		if (allSounds.numElem() > 0 && curSelection != -1)
		{
			SoundScriptDesc& script = *allSounds[curSelection];

			static bool justChanged = false;

			if (s_currentSelection != &script)
			{
				// TODO: prompt to save unsaved settings!!!
				s_currentSelection = &script;
				InitSoundNodesFromScriptDesc(script);
				justChanged = true;
			}

			static CSoundingObject soundTest;// TODO: isolate to chose currently playing sounding object from list
			static bool isolateSound = false;

			static float playbackPitch = 1.0f;
			static float playbackVolume = 1.0f;

			// playback controls
			{
				ImGui::Checkbox("Isolate", &isolateSound);

				ImGui::SameLine();

				if (ImGui::Button("Play"))
				{
					if (soundTest.GetEmitterState(0) == IEqAudioSource::STOPPED)
					{
						EmitParams snd(script.name);
						snd.flags |= EMITSOUND_FLAG_FORCE_CACHED | EMITSOUND_FLAG_FORCE_2D;

						soundTest.EmitSound(0, &snd);
					}

					soundTest.PlayEmitter(0);
				}

				ImGui::SameLine();
				if (ImGui::Button("Pause"))
					soundTest.PauseEmitter(0);

				ImGui::SameLine();
				if (ImGui::Button("Stop"))
					soundTest.StopEmitter(0, true);

				ImGui::SameLine();
				ImGui::Spacing(); ImGui::SameLine();

				ImGui::Separator();
			}

			g_sounds->m_isolateSound = isolateSound ? &script : nullptr;

			ImGui::Separator();

			if (ImGui::BeginTabBar("##tab"))
			{
				if (ImGui::BeginTabItem("Script parameters"))
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

					/*
					modified = ImGui::SliderFloat("Volume##script_volume", &script.volume, 0.0f, 1.0f, "%.2f", 1.0f) || modified;
					modified = ImGui::SliderFloat("Pitch##script_pitch", &script.pitch, 0.01f, 4.0f, "%.2f", 1.0f) || modified;
					ImGui::Separator();
					modified = ImGui::SliderFloat("Attenuation##script_atten", &script.atten, 0.01f, 10.0f, "%.2f", 1.0f) || modified;
					modified = ImGui::SliderFloat("Rolloff##script_rolloff", &script.rolloff, 0.01f, 4.0f, "%.2f", 1.0f) || modified;
					ImGui::Separator();*/
					modified = ImGui::SliderFloat("Max Distance##script_maxDistance", &script.maxDistance, 10.0f, 200.0f, "%.2f", 1.0f) || modified;
					
					if (modified)
						g_sounds->RestartEmittersByScript(&script);

					ImGui::EndTabItem();
				}

				static char inputNames[][64] = {
					"tractionSlide",
					"lateralSlide"
				};

				static char mixerNames[][64] = {
					"pitch",
					"volume",
					"fade"
				};

				static ImVec2 mixerCurves[elementsOf(inputNames)][10];

				static bool init = false;
				if (!init)
				{
					// init data so editor knows to take it from here
					mixerCurves[0][0].x = ImGui::CurveTerminator;
					mixerCurves[1][0].x = ImGui::CurveTerminator;
				}
				init = true;

				if (ImGui::BeginTabItem("Playback"))
				{
					if (soundTest.GetEmitterState(0) != IEqAudioSource::STOPPED)
					{
						SoundEmitterData* emitter = soundTest.m_emitters[0];

						IEqAudioSource* src = emitter->soundSource;
						if (src)
						{
							// TODO: inputs here

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
								ImGui::VSliderFloat(EqString::Format("%d##smpl%dv", i + 1, i), ImVec2(18, 160), &sampleVolume, 0.0f, 1.0f, "");
								if (ImGui::IsItemActive() || ImGui::IsItemHovered())
									ImGui::SetTooltip("Sample %d volume %.3f", i + 1, sampleVolume);
								src->SetSampleVolume(i, sampleVolume);
							}

							if (modified)
							{
								soundTest.SetVolume(emitter, playbackVolume);
								soundTest.SetPitch(emitter, playbackPitch);
							}

							ImGui::SameLine();

							if (ImGui::BeginTable("Inputs##playbackInputs", 2, ImGuiTableFlags_Borders))
							{
								ImGui::TableSetupColumn("name");
								ImGui::TableSetupColumn("value");

								ImGui::PushID("s_inputs");
								for (int row = 0; row < elementsOf(inputNames); ++row)
								{
									ImGui::TableNextRow();

									// Draw our contents
									static float dummy_f = 0.0f;
									ImGui::PushID(row);
									ImGui::TableSetColumnIndex(0);
									ImGui::Text(inputNames[row]);
									ImGui::TableSetColumnIndex(1);

									ImGui::SliderFloat("##v", &dummy_f, 0.0f, 1.0f);

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

					int posCounter = 0;
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
					for (int i = 0; i < s_uiNodeLinks.numElem(); ++i)
					{
						const UINodeLink linkVal = s_uiNodeLinks[i];
						ImNodes::Link(i, linkVal.a, linkVal.b);
					}

					ImNodes::EndNodeEditor();

					int start_attr, end_attr;
					if (ImNodes::IsLinkCreated(&start_attr, &end_attr))
					{
						//links.push_back(std::make_pair(start_attr, end_attr));
					}

					// adjust node positions
					if (justChanged)
					{
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

							ImNodes::SetNodeGridSpacePos(uiNode.id, ImVec2(posCounter * 150.0f, countByType[uiNode.type] * 80.0f));

							if (uiNode.type == SOUND_NODE_FUNC)
								++countByFunc[uiNode.func.type];

							++countByType[uiNode.type];
						}

						justChanged = false;
					}

					ImGui::EndTabItem();
				}
#if 0
				if (ImGui::BeginTabItem("Automation Inputs"))
				{
					if (ImGui::Button("Add##add_input"))
					{

					}
					ImGui::SameLine();
					static int currentInput = -1;
					ImGui::SetNextItemWidth(210);
					if (ImGui::BeginCombo("Inputs", currentInput == -1 ? "-" : inputNames[currentInput]))
					{
						ImGui::PushID("ed_inputs");
						for (int row = 0; row < elementsOf(inputNames); ++row)
						{
							// Draw our contents
							static float dummy_f = 0.0f;
							ImGui::PushID(row);
							if (ImGui::Selectable(inputNames[row], false, 0, ImVec2(180, 0)))
							{
								currentInput = row;
							}
							ImGui::SameLine();
							ImGui::Text("%.2f - %.2f", 0.1f, 2.5f);
							ImGui::SameLine();
							if (ImGui::SmallButton("Del"))
							{

							}

							ImGui::PopID();
						}
						ImGui::PopID();
						ImGui::EndCombo();
					}

					ImGui::Separator();

					if (currentInput != -1)
					{
						ImGui::InputText("Name", inputNames[currentInput], sizeof(inputNames[currentInput]));
						static float rangeMin = 0.1f;
						static float rangeMax = 1.1f;
						ImGui::SetNextItemWidth(180);
						ImGui::InputFloat("Range Min##range_min", &rangeMin, 0.1f, 1.0f, "%.2f");
						ImGui::SameLine();
						ImGui::SetNextItemWidth(180);
						ImGui::InputFloat("Max##range_max", &rangeMax, 0.1f, 1.0f, "%.2f");
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Automation Mixers"))
				{
					if (ImGui::Button("Add##add_mixer"))
					{

					}

					ImGui::SameLine();
					static int currentMixer = -1;
					ImGui::SetNextItemWidth(210);
					if (ImGui::BeginCombo("Mixers", currentMixer == -1 ? "-" : mixerNames[currentMixer]))
					{
						ImGui::PushID("ed_mixers");
						for (int row = 0; row < elementsOf(mixerNames); ++row)
						{
							// Draw our contents
							static float dummy_f = 0.0f;
							ImGui::PushID(row);
							if (ImGui::Selectable(mixerNames[row], false, 0, ImVec2(180, 0)))
							{
								currentMixer = row;
							}
							ImGui::SameLine();
							ImGui::Text("add");
							ImGui::SameLine();
							if (ImGui::SmallButton("Del"))
							{

							}

							ImGui::PopID();
						}
						ImGui::PopID();
						ImGui::EndCombo();
					}

					ImGui::Separator();

					if (currentMixer != -1)
					{
						ImGui::SetNextItemWidth(180);
						ImGui::InputText("Name", mixerNames[currentMixer], sizeof(mixerNames[currentMixer]));

						const char* mixMethods[] = {
							"add", "sub", "mul", "div", "min", "max", "average"
						};

						ImGui::SameLine();
						ImGui::SetNextItemWidth(180);
						static int methodIdx = 0;
						ImGui::Combo("Method", &methodIdx, mixMethods, elementsOf(mixMethods));

						static int selections[elementsOf(inputNames)] = { -1, -1 };

						for (int i = 0; i < elementsOf(inputNames); ++i)
						{
							ImVec2 minRange(0.0f, 0.0f);
							ImVec2 maxRange(4.0f, 2.0f);

							if (ImGui::Curve(inputNames[i], ImVec2(600, 80), 10, mixerCurves[i], &selections[i], minRange, maxRange))
							{
								// curve changed

							}
						}
					}

					ImGui::EndTabItem();
				}
#endif
				ImGui::EndTabBar();
			}
		}
		else
			g_sounds->m_isolateSound = nullptr;

		ImGui::End();
	}
}

#endif // SOUNDSCRIPT_EDITOR