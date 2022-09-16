#include <stdio.h>
#include <imgui.h>
#include <deque>
#include <vector>

extern "C" {
    struct lua_State;
}

extern lua_State* imGuilState;
void LoadImguiBindings();