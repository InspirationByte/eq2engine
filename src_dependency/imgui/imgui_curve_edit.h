// [src] https://github.com/ocornut/imgui/issues/123
// [src] https://github.com/ocornut/imgui/issues/55

// v1.23 - selection index track, value range, context menu, improve manipulation controls (D.Click to add/delete, drag to add)
// v1.22 - flip button; cosmetic fixes
// v1.21 - oops :)
// v1.20 - add iq's interpolation code
// v1.10 - easing and colors
// v1.00 - jari komppa's original

#pragma once

#include "imgui.h"

/* To use, add this prototype somewhere..
namespace ImGui
{
    int Curve(const char *label, const ImVec2& size, int maxpoints, ImVec2 *points);
    float CurveValue(float p, int maxpoints, const ImVec2 *points);
    float CurveValueSmooth(float p, int maxpoints, const ImVec2 *points);
};
*/
/*
    Example of use:
    ImVec2 foo[10];
    int selectionIdx = -1;
    ...
    foo[0].x = ImGui::CurveTerminator; // init data so editor knows to take it from here
    ...
    if (ImGui::Curve("Das editor", ImVec2(600, 200), 10, foo, &selectionIdx))
    {
        // curve changed
    }
    ...
    float value_you_care_about = ImGui::CurveValue(0.7f, 10, foo); // calculate value at position 0.7
*/

namespace ImGui
{
typedef float (*splineFunc)(float t, int maxPoints, const float* points);

bool    CurveFrame(const char* label, const ImVec2& size,
                const int maxpoints, ImVec2* points, splineFunc splineFn,
                const ImVec2& rangeMin = ImVec2(0, 0), const ImVec2& rangeMax = ImVec2(1, 1));

int     Curve(const char* label, const ImVec2& size, 
                const int maxpoints, ImVec2* points, int* selection, 
                splineFunc splineFn,
                const ImVec2& rangeMin = ImVec2(0, 0), 
                const ImVec2& rangeMax = ImVec2(1, 1));

float   CurveValue(float p, int maxpoints, const ImVec2* points);
float   CurveValueSmooth(float p, int maxpoints, const ImVec2* points, splineFunc splineFn);
}; // namespace ImGui
