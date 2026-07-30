#include "UI.hpp"
#include "Theme.hpp"
#include "imgui.h"
#include <algorithm>

namespace UI
{
    namespace
    {
        ImFont* g_bodyFont = nullptr;
        ImFont* g_headingFont = nullptr;
    }

    void SetFonts(ImFont* bodyFont, ImFont* headingFont)
    {
        g_bodyFont = bodyFont;
        g_headingFont = headingFont;
    }

    void PushHeadingFont()
    {
        if (g_headingFont)
            ImGui::PushFont(g_headingFont);
    }

    void PopHeadingFont()
    {
        if (g_headingFont)
            ImGui::PopFont();
    }

    void PushBodyFont()
    {
        if (g_bodyFont)
            ImGui::PushFont(g_bodyFont);
    }

    void PopBodyFont()
    {
        if (g_bodyFont)
            ImGui::PopFont();
    }

    bool ButtonPrimary(const char* label, const ImVec2& size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::ACCENT_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::ACCENT_COLOR_HDR);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::ACCENT_COLOR_ACTIVE);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(4);
        return pressed;
    }

    bool ButtonSecondary(const char* label, const ImVec2& size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, Theme::SURFACE_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::SURFACE_HOVER_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::SURFACE_ACTIVE_COLOR);
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return pressed;
    }

    bool ButtonGhost(const char* label, const ImVec2& size)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Theme::SURFACE_HOVER_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, Theme::SURFACE_ACTIVE_COLOR);
        bool pressed = ImGui::Button(label, size);
        ImGui::PopStyleColor(3);
        return pressed;
    }

    float ButtonWidth(const char* label)
    {
        const ImGuiStyle& style = ImGui::GetStyle();
        return ImGui::CalcTextSize(label).x
             + style.FramePadding.x * 2.0f
             + style.FrameBorderSize * 2.0f;
    }

    bool SearchInput(const char* id, char* buf, int bufSize, const char* hint, float width)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Theme::SURFACE_COLOR);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Theme::SURFACE_HOVER_COLOR);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Theme::SURFACE_ACTIVE_COLOR);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));

        if (width > 0.0f)
            ImGui::SetNextItemWidth(width);

        bool changed = ImGui::InputTextWithHint(id, hint ? hint : "Search...", buf, bufSize);

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
        return changed;
    }

    void PushCardStyle()
    {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Theme::SURFACE_COLOR);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, Theme::SURFACE_COLOR);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 20));
    }

    void PopCardStyle()
    {
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);
    }

    void DrawDivider(float paddingY)
    {
        ImVec2 min = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p1(min.x, min.y + paddingY);
        ImVec2 p2(min.x + avail.x, min.y + paddingY);
        dl->AddLine(p1, p2, ImGui::GetColorU32(Theme::BORDER_COLOR), 1.0f);
        ImGui::Dummy(ImVec2(0.0f, paddingY * 2.0f));
    }

    void DrawShadow(ImDrawList* dl, const ImVec2& min, const ImVec2& max, float rounding, float alpha)
    {
        ImU32 col = IM_COL32(0, 0, 0, static_cast<int>(alpha * 255));
        dl->AddRectFilled(min, max, col, rounding);
    }

    void EmptyState(const char* title, const char* subtitle, const char* hint)
    {
        float scale = ImGui::GetFontSize() / 16.0f;
        ImVec2 cardSize = ImVec2(240.0f * scale, 160.0f * scale);
        ImVec2 shadowSize = ImVec2(cardSize.x + 24.0f * scale, cardSize.y + 20.0f * scale);

        ImVec2 region = ImGui::GetContentRegionAvail();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float centerX = pos.x + region.x * 0.5f;
        float centerY = pos.y + region.y * 0.4f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        DrawShadow(dl, ImVec2(centerX - shadowSize.x * 0.5f, centerY - shadowSize.y * 0.5f),
                   ImVec2(centerX + shadowSize.x * 0.5f, centerY + shadowSize.y * 0.5f), 16.0f * scale, 0.15f);

        UI::PushCardStyle();
        ImGui::SetCursorPos(ImVec2((region.x - cardSize.x) * 0.5f, (region.y - cardSize.y) * 0.5f));
        if (ImGui::BeginChild("##EmptyState", cardSize, ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(title).x) * 0.5f);
            UI::PushHeadingFont();
            ImGui::TextColored(Theme::ACCENT_COLOR, "%s", title);
            UI::PopHeadingFont();

            ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(subtitle).x) * 0.5f);
            ImGui::TextDisabled("%s", subtitle);

            ImGui::SetCursorPosX((avail.x - ImGui::CalcTextSize(hint).x) * 0.5f);
            ImGui::TextDisabled("%s", hint);
        }
        ImGui::EndChild();
        UI::PopCardStyle();
    }

}
