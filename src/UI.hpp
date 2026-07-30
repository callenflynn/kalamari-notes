#pragma once

#include "imgui.h"
#include <string>

namespace UI
{
    // Call after fonts have been loaded so helpers can use the heading font.
    void SetFonts(ImFont* bodyFont, ImFont* headingFont);

    void PushHeadingFont();
    void PopHeadingFont();

    void PushBodyFont();
    void PopBodyFont();

    // Accent / primary buttons
    bool ButtonPrimary(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool ButtonSecondary(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool ButtonGhost(const char* label, const ImVec2& size = ImVec2(0, 0));

    // Natural width of an auto-sized ImGui button for the given label.
    float ButtonWidth(const char* label);

    // Search input with an embedded hint and subtle styling.
    bool SearchInput(const char* id, char* buf, int bufSize, const char* hint = nullptr, float width = -1.0f);

    // Card / modal backdrop helpers.
    void PushCardStyle();
    void PopCardStyle();

    // Draw a soft divider line.
    void DrawDivider(float paddingY = 8.0f);

    // Render a subtle shadow under a rectangle.
    void DrawShadow(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding = 8.0f, float alpha = 0.25f);

    // A centered empty-state panel.
    void EmptyState(const char* title, const char* subtitle, const char* hint);

}
