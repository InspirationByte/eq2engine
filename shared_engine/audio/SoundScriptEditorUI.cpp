//////////////////////////////////////////////////////////////////////////////////
// Copyright © Inspiration Byte
// 2009-2022
//////////////////////////////////////////////////////////////////////////////////
// Description: Sound emitter system script editor
//////////////////////////////////////////////////////////////////////////////////

#pragma optimize("", off)
#include <imgui.h>
#include <imgui_curve_edit.h>
#include "core/core_common.h"
#include "eqSoundEmitterSystem.h"
#include "eqSoundEmitterObject.h"
#include "eqSoundEmitterPrivateTypes.h"

#if !defined(_RETAIL) && !defined(_PROFILE)
#define SOUNDSCRIPT_EDITOR
#endif

class CSoundScriptEditor
{
public:
	static void DrawScriptEditor(bool& open);
};

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

					modified = ImGui::SliderFloat("Volume##script_volume", &script.volume, 0.0f, 1.0f, "%.2f", 1.0f) || modified;
					modified = ImGui::SliderFloat("Pitch##script_pitch", &script.pitch, 0.01f, 4.0f, "%.2f", 1.0f) || modified;
					ImGui::Separator();
					modified = ImGui::SliderFloat("Attenuation##script_atten", &script.atten, 0.01f, 10.0f, "%.2f", 1.0f) || modified;
					modified = ImGui::SliderFloat("Rolloff##script_rolloff", &script.rolloff, 0.01f, 4.0f, "%.2f", 1.0f) || modified;
					ImGui::Separator();
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

				ImGui::EndTabBar();
			}
		}
		else
			g_sounds->m_isolateSound = nullptr;

		ImGui::End();
	}
}

#endif // SOUNDSCRIPT_EDITOR