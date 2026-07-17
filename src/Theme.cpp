#include "Theme.hpp"

namespace Theme
{
    // Orange accent shared by both themes: #ED5001
    const ImVec4 ACCENT_COLOR      = ImVec4(0.929f, 0.314f, 0.004f, 1.0f);
    const ImVec4 ACCENT_COLOR_HDR  = ImVec4(1.0f, 0.35f, 0.02f, 1.0f);

    // Light theme
    const ImVec4 LIGHT_BG          = ImVec4(1.000f, 0.937f, 0.878f, 1.0f);
    const ImVec4 LIGHT_TEXT        = ImVec4(0.141f, 0.141f, 0.141f, 1.0f);
    const ImVec4 LIGHT_BORDER      = ImVec4(0.800f, 0.780f, 0.750f, 1.0f);
    const ImVec4 LIGHT_FRAME_BG    = ImVec4(0.960f, 0.910f, 0.850f, 1.0f);
    const ImVec4 LIGHT_SCROLLBAR   = ImVec4(0.900f, 0.870f, 0.820f, 1.0f);

    // Dark theme
    const ImVec4 DARK_BG           = ImVec4(0.118f, 0.118f, 0.118f, 1.0f);
    const ImVec4 DARK_TEXT         = ImVec4(1.000f, 0.992f, 0.969f, 1.0f);
    const ImVec4 DARK_BORDER       = ImVec4(0.300f, 0.300f, 0.300f, 1.0f);
    const ImVec4 DARK_FRAME_BG     = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);
    const ImVec4 DARK_SCROLLBAR    = ImVec4(0.250f, 0.250f, 0.250f, 1.0f);

    void Apply(bool darkMode)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* c = style.Colors;

        if (darkMode)
        {
            ImGui::StyleColorsDark();
            c[ImGuiCol_WindowBg]           = DARK_BG;
            c[ImGuiCol_ChildBg]            = DARK_BG;
            c[ImGuiCol_PopupBg]            = DARK_BG;
            c[ImGuiCol_Text]               = DARK_TEXT;
            c[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            c[ImGuiCol_Border]             = DARK_BORDER;
            c[ImGuiCol_FrameBg]            = DARK_FRAME_BG;
            c[ImGuiCol_FrameBgHovered]     = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
            c[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
            c[ImGuiCol_TitleBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
            c[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
            c[ImGuiCol_ScrollbarBg]        = DARK_SCROLLBAR;
            c[ImGuiCol_ScrollbarGrab]      = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
            c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
            c[ImGuiCol_Separator]          = DARK_BORDER;
            c[ImGuiCol_SeparatorHovered]   = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
            c[ImGuiCol_SeparatorActive]    = ACCENT_COLOR;
            c[ImGuiCol_Header]             = ImVec4(0.28f, 0.16f, 0.10f, 1.0f);
            c[ImGuiCol_HeaderHovered]      = ImVec4(0.36f, 0.22f, 0.13f, 1.0f);
            c[ImGuiCol_HeaderActive]       = ImVec4(0.44f, 0.28f, 0.16f, 1.0f);
            c[ImGuiCol_Button]             = ACCENT_COLOR;
            c[ImGuiCol_ButtonHovered]      = ACCENT_COLOR_HDR;
            c[ImGuiCol_ButtonActive]       = ImVec4(0.80f, 0.26f, 0.00f, 1.0f);
            c[ImGuiCol_SliderGrab]         = ACCENT_COLOR;
            c[ImGuiCol_SliderGrabActive]   = ACCENT_COLOR_HDR;
            c[ImGuiCol_CheckMark]          = ACCENT_COLOR;
            c[ImGuiCol_TextSelectedBg]     = ImVec4(0.929f, 0.314f, 0.004f, 0.35f);
            c[ImGuiCol_Tab]                = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
            c[ImGuiCol_TabHovered]         = ACCENT_COLOR_HDR;
            c[ImGuiCol_TabSelected]        = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
            c[ImGuiCol_ResizeGrip]         = ImVec4(0.929f, 0.314f, 0.004f, 0.20f);
            c[ImGuiCol_ResizeGripHovered]  = ImVec4(0.929f, 0.314f, 0.004f, 0.50f);
            c[ImGuiCol_ResizeGripActive]   = ImVec4(0.929f, 0.314f, 0.004f, 0.80f);
            c[ImGuiCol_MenuBarBg]          = DARK_FRAME_BG;
            c[ImGuiCol_PlotLines]          = ACCENT_COLOR;
            c[ImGuiCol_PlotLinesHovered]   = ACCENT_COLOR_HDR;
            c[ImGuiCol_PlotHistogram]      = ACCENT_COLOR;
            c[ImGuiCol_PlotHistogramHovered] = ACCENT_COLOR_HDR;
        }
        else
        {
            ImGui::StyleColorsLight();
            c[ImGuiCol_WindowBg]           = LIGHT_BG;
            c[ImGuiCol_ChildBg]            = LIGHT_BG;
            c[ImGuiCol_PopupBg]            = ImVec4(1.0f, 0.96f, 0.91f, 1.0f);
            c[ImGuiCol_Text]               = LIGHT_TEXT;
            c[ImGuiCol_TextDisabled]       = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
            c[ImGuiCol_Border]             = LIGHT_BORDER;
            c[ImGuiCol_FrameBg]            = LIGHT_FRAME_BG;
            c[ImGuiCol_FrameBgHovered]     = ImVec4(0.94f, 0.88f, 0.82f, 1.0f);
            c[ImGuiCol_FrameBgActive]      = ImVec4(0.90f, 0.84f, 0.78f, 1.0f);
            c[ImGuiCol_TitleBg]            = ImVec4(0.93f, 0.86f, 0.80f, 1.0f);
            c[ImGuiCol_TitleBgActive]      = ImVec4(0.90f, 0.83f, 0.77f, 1.0f);
            c[ImGuiCol_ScrollbarBg]        = LIGHT_SCROLLBAR;
            c[ImGuiCol_ScrollbarGrab]      = ImVec4(0.80f, 0.76f, 0.70f, 1.0f);
            c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.72f, 0.68f, 0.62f, 1.0f);
            c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.65f, 0.61f, 0.55f, 1.0f);
            c[ImGuiCol_Separator]          = LIGHT_BORDER;
            c[ImGuiCol_SeparatorHovered]   = ImVec4(0.70f, 0.70f, 0.70f, 1.0f);
            c[ImGuiCol_SeparatorActive]    = ACCENT_COLOR;
            c[ImGuiCol_Header]             = ImVec4(0.92f, 0.80f, 0.72f, 1.0f);
            c[ImGuiCol_HeaderHovered]      = ImVec4(0.90f, 0.75f, 0.65f, 1.0f);
            c[ImGuiCol_HeaderActive]       = ImVec4(0.88f, 0.72f, 0.60f, 1.0f);
            c[ImGuiCol_Button]             = ACCENT_COLOR;
            c[ImGuiCol_ButtonHovered]      = ACCENT_COLOR_HDR;
            c[ImGuiCol_ButtonActive]       = ImVec4(0.80f, 0.26f, 0.00f, 1.0f);
            c[ImGuiCol_SliderGrab]         = ACCENT_COLOR;
            c[ImGuiCol_SliderGrabActive]   = ACCENT_COLOR_HDR;
            c[ImGuiCol_CheckMark]          = ACCENT_COLOR;
            c[ImGuiCol_TextSelectedBg]     = ImVec4(0.929f, 0.314f, 0.004f, 0.30f);
            c[ImGuiCol_Tab]                = ImVec4(0.92f, 0.86f, 0.80f, 1.0f);
            c[ImGuiCol_TabHovered]         = ACCENT_COLOR_HDR;
            c[ImGuiCol_TabSelected]        = ImVec4(0.88f, 0.80f, 0.74f, 1.0f);
            c[ImGuiCol_ResizeGrip]         = ImVec4(0.929f, 0.314f, 0.004f, 0.15f);
            c[ImGuiCol_ResizeGripHovered]  = ImVec4(0.929f, 0.314f, 0.004f, 0.40f);
            c[ImGuiCol_ResizeGripActive]   = ImVec4(0.929f, 0.314f, 0.004f, 0.65f);
            c[ImGuiCol_MenuBarBg]          = LIGHT_FRAME_BG;
            c[ImGuiCol_PlotLines]          = ACCENT_COLOR;
            c[ImGuiCol_PlotLinesHovered]   = ACCENT_COLOR_HDR;
            c[ImGuiCol_PlotHistogram]      = ACCENT_COLOR;
            c[ImGuiCol_PlotHistogramHovered] = ACCENT_COLOR_HDR;
        }
    }

    ImVec4 GetClearColor(bool darkMode)
    {
        return darkMode ? DARK_BG : LIGHT_BG;
    }
}
