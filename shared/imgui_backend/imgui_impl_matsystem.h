// dear imgui: Renderer Backend for Equilibrium Engine
#pragma once
#include <imgui.h>      // IMGUI_IMPL_API

class IGPURenderPassRecorder;

IMGUI_IMPL_API bool     ImGui_ImplMatSystem_Init();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_RenderDrawData(ImDrawData* draw_data, IGPURenderPassRecorder* rendPassRecorder);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplMatSystem_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_InvalidateDeviceObjects();

// Draw list commands between BeginOpaque and EndOpaque are drawn without blending, so texture alpha
// is ignored (render targets whose alpha is not coverage). Blending is restored for each draw list.
IMGUI_IMPL_API void     ImGui_ImplMatSystem_BeginOpaque(ImDrawList* drawList);
IMGUI_IMPL_API void     ImGui_ImplMatSystem_EndOpaque(ImDrawList* drawList);

// AddImage drawn without blending
IMGUI_IMPL_API void     ImGui_ImplMatSystem_AddImageOpaque(ImDrawList* drawList, ImTextureID texture, const ImVec2& pMin, const ImVec2& pMax, const ImVec2& uvMin = ImVec2(0, 0), const ImVec2& uvMax = ImVec2(1, 1), ImU32 col = IM_COL32_WHITE);
