// dear imgui:  Platform Backend for Equilibrium based on SDL2
#pragma once
#include <imgui.h>      // IMGUI_IMPL_API

struct SDL_Window;
typedef union SDL_Event SDL_Event;

IMGUI_IMPL_API bool     ImGui_ImplEq_InitForSDL(SDL_Window* window);
IMGUI_IMPL_API void     ImGui_ImplEq_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplEq_NewFrame(bool controlsEnabled);

IMGUI_IMPL_API bool     ImGui_ImplEq_AnyWindowInFocus();
IMGUI_IMPL_API void     ImGui_ImplEq_InputKeyPress(int key, bool down);
IMGUI_IMPL_API void     ImGui_ImplEq_InputText(const char* keyInput);
IMGUI_IMPL_API void     ImGui_ImplEq_InputMousePress(int Button, bool pressed);
IMGUI_IMPL_API void     ImGui_ImplEq_InputMouseWheel(int h, int v);
IMGUI_IMPL_API void     ImGui_ImplEq_InputFocus(bool focused);