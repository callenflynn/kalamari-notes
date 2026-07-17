#pragma once

#include "imgui.h"

namespace Theme
{
    // Accent
    extern const ImVec4 ACCENT_COLOR;
    extern const ImVec4 ACCENT_COLOR_HDR;

    // Light theme
    extern const ImVec4 LIGHT_BG;
    extern const ImVec4 LIGHT_TEXT;
    extern const ImVec4 LIGHT_BORDER;
    extern const ImVec4 LIGHT_FRAME_BG;
    extern const ImVec4 LIGHT_SCROLLBAR;

    // Dark theme
    extern const ImVec4 DARK_BG;
    extern const ImVec4 DARK_TEXT;
    extern const ImVec4 DARK_BORDER;
    extern const ImVec4 DARK_FRAME_BG;
    extern const ImVec4 DARK_SCROLLBAR;

    void Apply(bool darkMode);
    ImVec4 GetClearColor(bool darkMode);
}
