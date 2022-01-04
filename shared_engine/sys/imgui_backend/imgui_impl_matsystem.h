// dear imgui: Renderer Backend for Equilibrium Engine
#pragma once
#include "imgui.h"      // IMGUI_IMPL_API

IMGUI_IMPL_API bool     ImGui_ImplMatSystem_Init();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_RenderDrawData(ImDrawData* draw_data);

// Use if you want to reset your rendering device without losing Dear ImGui state.
IMGUI_IMPL_API bool     ImGui_ImplMatSystem_CreateDeviceObjects();
IMGUI_IMPL_API void     ImGui_ImplMatSystem_InvalidateDeviceObjects();
