#pragma once

#include "imgui.h"

namespace Theme
{
    // Accent
    extern const ImVec4 ACCENT_COLOR;
    extern const ImVec4 ACCENT_COLOR_HDR;
    extern const ImVec4 ACCENT_COLOR_ACTIVE;

    // Core surfaces (updated by Apply)
    extern ImVec4 BACKGROUND_COLOR;
    extern ImVec4 SURFACE_COLOR;
    extern ImVec4 SURFACE_HOVER_COLOR;
    extern ImVec4 SURFACE_ACTIVE_COLOR;
    extern ImVec4 TEXT_COLOR;
    extern ImVec4 TEXT_MUTED_COLOR;
    extern ImVec4 BORDER_COLOR;

    void Apply(bool darkMode);
    ImVec4 GetClearColor(bool darkMode);
}
