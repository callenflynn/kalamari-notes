#include "Theme.hpp"

namespace Theme
{
    // Modern indigo accent
    const ImVec4 ACCENT_COLOR       = ImVec4(0.388f, 0.404f, 0.945f, 1.0f); // #6366f1
    const ImVec4 ACCENT_COLOR_HDR   = ImVec4(0.510f, 0.549f, 0.973f, 1.0f); // #828cff
    const ImVec4 ACCENT_COLOR_ACTIVE= ImVec4(0.310f, 0.329f, 0.800f, 1.0f); // #4f54cc

    // Theme palette constants (internal to this translation unit)
    static const ImVec4 DARK_BG         = ImVec4(0.047f, 0.047f, 0.055f, 1.0f); // #0c0c0e
    static const ImVec4 DARK_TEXT       = ImVec4(0.957f, 0.957f, 0.961f, 1.0f); // #f4f4f5
    static const ImVec4 DARK_BORDER     = ImVec4(0.165f, 0.165f, 0.196f, 1.0f); // #2a2a32
    static const ImVec4 DARK_FRAME_BG   = ImVec4(0.090f, 0.090f, 0.110f, 1.0f); // #17171c
    static const ImVec4 LIGHT_BG        = ImVec4(1.000f, 1.000f, 1.000f, 1.0f);
    static const ImVec4 LIGHT_TEXT      = ImVec4(0.098f, 0.098f, 0.106f, 1.0f); // #18181b
    static const ImVec4 LIGHT_BORDER    = ImVec4(0.894f, 0.894f, 0.906f, 1.0f); // #e4e4e7
    static const ImVec4 LIGHT_FRAME_BG  = ImVec4(0.957f, 0.957f, 0.961f, 1.0f); // #f4f4f5

    // Runtime-resolved helpers (filled by Apply)
    ImVec4 BACKGROUND_COLOR     = DARK_BG;
    ImVec4 SURFACE_COLOR        = DARK_FRAME_BG;
    ImVec4 SURFACE_HOVER_COLOR  = ImVec4(0.122f, 0.122f, 0.145f, 1.0f);
    ImVec4 SURFACE_ACTIVE_COLOR = ImVec4(0.153f, 0.153f, 0.184f, 1.0f);
    ImVec4 TEXT_COLOR           = DARK_TEXT;
    ImVec4 TEXT_MUTED_COLOR     = ImVec4(0.612f, 0.643f, 0.686f, 1.0f); // #9ca3af
    ImVec4 BORDER_COLOR         = DARK_BORDER;

    void Apply(bool darkMode)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        if (darkMode)
        {
            ImGui::StyleColorsDark();
            BACKGROUND_COLOR     = DARK_BG;
            SURFACE_COLOR        = DARK_FRAME_BG;
            SURFACE_HOVER_COLOR  = ImVec4(0.122f, 0.122f, 0.145f, 1.0f);
            SURFACE_ACTIVE_COLOR = ImVec4(0.153f, 0.153f, 0.184f, 1.0f);
            TEXT_COLOR           = DARK_TEXT;
            TEXT_MUTED_COLOR     = ImVec4(0.612f, 0.643f, 0.686f, 1.0f);
            BORDER_COLOR         = DARK_BORDER;

            c[ImGuiCol_WindowBg]             = BACKGROUND_COLOR;
            c[ImGuiCol_ChildBg]              = SURFACE_COLOR;
            c[ImGuiCol_PopupBg]              = ImVec4(0.078f, 0.078f, 0.086f, 0.98f);
            c[ImGuiCol_Text]                 = TEXT_COLOR;
            c[ImGuiCol_TextDisabled]         = TEXT_MUTED_COLOR;
            c[ImGuiCol_Border]               = BORDER_COLOR;
            c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
            c[ImGuiCol_FrameBg]              = SURFACE_COLOR;
            c[ImGuiCol_FrameBgHovered]     = SURFACE_HOVER_COLOR;
            c[ImGuiCol_FrameBgActive]       = SURFACE_ACTIVE_COLOR;
            c[ImGuiCol_TitleBg]              = BACKGROUND_COLOR;
            c[ImGuiCol_TitleBgActive]        = SURFACE_COLOR;
            c[ImGuiCol_ScrollbarBg]          = ImVec4(0.047f, 0.047f, 0.055f, 0.6f);
            c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.220f, 0.224f, 0.255f, 1.0f);
            c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.294f, 0.298f, 0.341f, 1.0f);
            c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.369f, 0.376f, 0.424f, 1.0f);
            c[ImGuiCol_Separator]            = BORDER_COLOR;
            c[ImGuiCol_SeparatorHovered]     = ImVec4(0.310f, 0.314f, 0.361f, 1.0f);
            c[ImGuiCol_SeparatorActive]      = ACCENT_COLOR;
            c[ImGuiCol_Header]               = SURFACE_HOVER_COLOR;
            c[ImGuiCol_HeaderHovered]       = SURFACE_ACTIVE_COLOR;
            c[ImGuiCol_HeaderActive]        = ACCENT_COLOR;
            c[ImGuiCol_Button]               = ACCENT_COLOR;
            c[ImGuiCol_ButtonHovered]        = ACCENT_COLOR_HDR;
            c[ImGuiCol_ButtonActive]         = ACCENT_COLOR_ACTIVE;
            c[ImGuiCol_SliderGrab]           = ACCENT_COLOR;
            c[ImGuiCol_SliderGrabActive]     = ACCENT_COLOR_HDR;
            c[ImGuiCol_CheckMark]            = ACCENT_COLOR;
            c[ImGuiCol_TextSelectedBg]       = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.35f);
            c[ImGuiCol_Tab]                  = SURFACE_COLOR;
            c[ImGuiCol_TabHovered]           = ACCENT_COLOR_HDR;
            c[ImGuiCol_TabSelected]          = SURFACE_HOVER_COLOR;
            c[ImGuiCol_TabSelectedOverline]  = ACCENT_COLOR;
            c[ImGuiCol_ResizeGrip]           = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.20f);
            c[ImGuiCol_ResizeGripHovered]    = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.45f);
            c[ImGuiCol_ResizeGripActive]     = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.75f);
            c[ImGuiCol_MenuBarBg]              = SURFACE_COLOR;
            c[ImGuiCol_PlotLines]            = ACCENT_COLOR;
            c[ImGuiCol_PlotLinesHovered]     = ACCENT_COLOR_HDR;
            c[ImGuiCol_PlotHistogram]        = ACCENT_COLOR;
            c[ImGuiCol_PlotHistogramHovered]  = ACCENT_COLOR_HDR;
        }
        else
        {
            ImGui::StyleColorsLight();
            BACKGROUND_COLOR     = LIGHT_BG;
            SURFACE_COLOR        = LIGHT_FRAME_BG;
            SURFACE_HOVER_COLOR  = ImVec4(0.898f, 0.898f, 0.906f, 1.0f); // #e5e5e7
            SURFACE_ACTIVE_COLOR = ImVec4(0.831f, 0.831f, 0.847f, 1.0f); // #d4d4d8
            TEXT_COLOR           = LIGHT_TEXT;
            TEXT_MUTED_COLOR     = ImVec4(0.420f, 0.443f, 0.502f, 1.0f); // #6b7280
            BORDER_COLOR         = LIGHT_BORDER;

            c[ImGuiCol_WindowBg]             = BACKGROUND_COLOR;
            c[ImGuiCol_ChildBg]              = SURFACE_COLOR;
            c[ImGuiCol_PopupBg]              = ImVec4(1.0f, 1.0f, 1.0f, 0.98f);
            c[ImGuiCol_Text]                 = TEXT_COLOR;
            c[ImGuiCol_TextDisabled]         = TEXT_MUTED_COLOR;
            c[ImGuiCol_Border]               = BORDER_COLOR;
            c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
            c[ImGuiCol_FrameBg]              = SURFACE_COLOR;
            c[ImGuiCol_FrameBgHovered]       = SURFACE_HOVER_COLOR;
            c[ImGuiCol_FrameBgActive]        = SURFACE_ACTIVE_COLOR;
            c[ImGuiCol_TitleBg]              = BACKGROUND_COLOR;
            c[ImGuiCol_TitleBgActive]        = SURFACE_COLOR;
            c[ImGuiCol_ScrollbarBg]          = ImVec4(0.961f, 0.961f, 0.969f, 0.6f);
            c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.741f, 0.741f, 0.776f, 1.0f);
            c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.678f, 0.678f, 0.710f, 1.0f);
            c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.612f, 0.612f, 0.643f, 1.0f);
            c[ImGuiCol_Separator]            = BORDER_COLOR;
            c[ImGuiCol_SeparatorHovered]     = ImVec4(0.741f, 0.741f, 0.776f, 1.0f);
            c[ImGuiCol_SeparatorActive]      = ACCENT_COLOR;
            c[ImGuiCol_Header]               = SURFACE_HOVER_COLOR;
            c[ImGuiCol_HeaderHovered]        = SURFACE_ACTIVE_COLOR;
            c[ImGuiCol_HeaderActive]         = ACCENT_COLOR;
            c[ImGuiCol_Button]               = ACCENT_COLOR;
            c[ImGuiCol_ButtonHovered]        = ACCENT_COLOR_HDR;
            c[ImGuiCol_ButtonActive]           = ACCENT_COLOR_ACTIVE;
            c[ImGuiCol_SliderGrab]           = ACCENT_COLOR;
            c[ImGuiCol_SliderGrabActive]     = ACCENT_COLOR_HDR;
            c[ImGuiCol_CheckMark]            = ACCENT_COLOR;
            c[ImGuiCol_TextSelectedBg]       = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.25f);
            c[ImGuiCol_Tab]                  = SURFACE_COLOR;
            c[ImGuiCol_TabHovered]           = ACCENT_COLOR_HDR;
            c[ImGuiCol_TabSelected]          = SURFACE_HOVER_COLOR;
            c[ImGuiCol_TabSelectedOverline]  = ACCENT_COLOR;
            c[ImGuiCol_ResizeGrip]           = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.12f);
            c[ImGuiCol_ResizeGripHovered]    = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.35f);
            c[ImGuiCol_ResizeGripActive]     = ImVec4(ACCENT_COLOR.x, ACCENT_COLOR.y, ACCENT_COLOR.z, 0.60f);
            c[ImGuiCol_MenuBarBg]            = SURFACE_COLOR;
            c[ImGuiCol_PlotLines]            = ACCENT_COLOR;
            c[ImGuiCol_PlotLinesHovered]     = ACCENT_COLOR_HDR;
            c[ImGuiCol_PlotHistogram]        = ACCENT_COLOR;
            c[ImGuiCol_PlotHistogramHovered] = ACCENT_COLOR_HDR;
        }

        // Common modern spacing / rounding
        style.WindowRounding    = 10.0f;
        style.ChildRounding     = 8.0f;
        style.FrameRounding     = 8.0f;
        style.PopupRounding     = 10.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 6.0f;
        style.TabRounding       = 6.0f;
        style.WindowBorderSize  = 0.0f;
        style.ChildBorderSize   = 0.0f;        style.FrameBorderSize  = 1.0f;
        style.PopupBorderSize   = 0.0f;
        style.TabBorderSize     = 0.0f;
        style.WindowPadding     = ImVec2(10, 10);
        style.FramePadding      = ImVec2(10, 6);
        style.ItemSpacing       = ImVec2(8, 8);
        style.ItemInnerSpacing  = ImVec2(6, 6);
        style.ScrollbarSize     = 8.0f;
    }

    ImVec4 GetClearColor(bool darkMode)
    {
        return darkMode ? DARK_BG : LIGHT_BG;
    }
}
