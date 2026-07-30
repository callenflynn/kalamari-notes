#include "Editor.hpp"

#include "Markdown.hpp"
#include "Theme.hpp"
#include "UI.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Kalamari
{
    namespace
    {
        int CountWords(const std::string& text)
        {
            int count = 0;
            bool inWord = false;
            for (char c : text)
            {
                if (std::isspace(static_cast<unsigned char>(c)))
                {
                    if (inWord) { ++count; inWord = false; }
                }
                else inWord = true;
            }
            if (inWord) ++count;
            return count;
        }    
    }

    bool Editor::Draw(const std::shared_ptr<Note>& activeNote)
    {
        if (!activeNote)
        {
            ImVec2 r = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos(ImVec2((r.x - ImGui::CalcTextSize("No note selected").x) * 0.5f, r.y * 0.4f));
            ImGui::TextDisabled("No note selected");
            ImGui::SetCursorPosX((r.x - ImGui::CalcTextSize("Ctrl+N to create one").x) * 0.5f);
            ImGui::TextDisabled("Ctrl+N to create one");
            return false;
        }

        m_wikiLinkTarget.clear();
        bool changed = false;

        if (activeNote.get() != m_lastNote)
        {
            m_editMode = false;
            m_inlineEditLine = -1;
            m_lastNote = activeNote.get();
            m_showFind = false;
            m_findIndex = -1;
            m_findCount = 0;
        }

        // ---- Header toolbar ----
        DrawToolbar(*activeNote);
        ImGui::Spacing();

        // ---- Find bar ----
        if (m_showFind)
            DrawFindBar(activeNote->content);

        // ---- Content ----
        std::vector<std::string> lines;
        Markdown::SplitLines(activeNote->content, lines);
        if (lines.empty()) lines.emplace_back();

        ImGui::BeginChild("EditorScroll", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 8), ImGuiChildFlags_None);

        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_E))
            m_editMode = !m_editMode;
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_F))
        {
            m_showFind = !m_showFind;
            if (!m_showFind) { m_findIndex = -1; m_findCount = 0; }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            if (m_showFind)
            {
                m_showFind = false;
                m_findIndex = -1;
                m_findCount = 0;
            }
            else if (m_inlineEditLine >= 0)
                m_inlineEditLine = -1;
            else if (m_editMode)
                m_editMode = false;
        }

        if (m_editMode)
        {
            DrawEditMode(*activeNote);
            if (activeNote->dirty) changed = true;
        }
        else
        {
            DrawReadingMode(lines, *activeNote);
        }

        ImGui::EndChild();

        if (m_editMode)
        {
            if (m_wasEditingLastFrame && !ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows))
                m_editMode = false;
            m_wasEditingLastFrame = true;
        }
        else
        {
            m_wasEditingLastFrame = false;
        }

        // ---- Status bar ----
        ImGui::Separator();
        int wc = CountWords(activeNote->content);
        int lc = static_cast<int>(lines.size());
        ImGui::TextDisabled("%d words  \xc2\xb7  %d lines", wc, lc);
        if (m_showFind && m_findCount > 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("  |  %d matches", m_findCount);
        }

        return changed;
    }

    void Editor::DrawToolbar(Note& note)
    {
        std::string title = note.path.stem().string();
        if (title.size() > 50) title = title.substr(0, 47) + "...";

        // Title
        ImGui::AlignTextToFramePadding();
        UI::PushHeadingFont();
        ImGui::Text("%s", title.c_str());
        UI::PopHeadingFont();

        // Dirty indicator
        if (note.dirty)
        {
            ImGui::SameLine();
            ImGui::TextColored(Theme::ACCENT_COLOR, "\xE2\x97\x8F"); // ●
        }

        // Right-align the Read/Edit/Find button group
        const char* findLabel = m_showFind ? "Close Find" : "Find";
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float groupW = UI::ButtonWidth("Read") + UI::ButtonWidth("Edit") + UI::ButtonWidth(findLabel)
                           + spacing * 2.0f;
        const float rightEdge = ImGui::GetWindowContentRegionMax().x;
        float x = rightEdge - groupW;
        const float minX = ImGui::GetCursorPosX() + spacing;
        if (x < minX) x = minX;
        ImGui::SameLine();
        ImGui::SetCursorPosX(x);

        // Read / Edit toggle
        if (m_editMode)
        {
            if (UI::ButtonGhost("Read", ImVec2(0, 0))) m_editMode = false;
            ImGui::SameLine(0, spacing);
            ImGui::BeginDisabled();
            UI::ButtonPrimary("Edit", ImVec2(0, 0));
            ImGui::EndDisabled();
        }
        else
        {
            ImGui::BeginDisabled();
            UI::ButtonPrimary("Read", ImVec2(0, 0));
            ImGui::EndDisabled();
            ImGui::SameLine(0, spacing);
            if (UI::ButtonGhost("Edit", ImVec2(0, 0))) m_editMode = true;
        }
        ImGui::SameLine(0, spacing);
        if (UI::ButtonGhost(findLabel, ImVec2(0, 0)))
        {
            m_showFind = !m_showFind;
            if (!m_showFind) { m_findIndex = -1; m_findCount = 0; }
        }
    }

    void Editor::DrawFindBar(const std::string& content)
    {
        ImGui::Spacing();
        ImVec2 avail = ImGui::GetContentRegionAvail();

        // Search input
        ImGui::SetNextItemWidth((std::max)(120.0f, (std::min)(300.0f, avail.x - 160.0f)));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(Theme::SURFACE_COLOR));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
        ImGui::InputTextWithHint("##FindInput", "Find...", m_findBuffer, sizeof(m_findBuffer));
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        // Count matches
        std::string needle(m_findBuffer);
        if (needle.size() >= 2)
        {
            std::string haystackLower = content;
            std::string needleLower = needle;
            std::transform(haystackLower.begin(), haystackLower.end(), haystackLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(needleLower.begin(), needleLower.end(), needleLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            size_t pos = 0;
            m_findCount = 0;
            while ((pos = haystackLower.find(needleLower, pos)) != std::string::npos)
            {
                ++m_findCount;
                pos += needleLower.size();
            }
            if (m_findCount > 0 && m_findIndex < 0) m_findIndex = 0;
            if (m_findIndex >= m_findCount) m_findIndex = m_findCount - 1;
        }
        else
        {
            m_findCount = 0;
            m_findIndex = -1;
        }

        ImGui::SameLine();
        if (UI::ButtonSecondary("<", ImVec2(28, 0)) && m_findIndex > 0)
            --m_findIndex;
        ImGui::SameLine();
        if (UI::ButtonSecondary(">", ImVec2(28, 0)) && m_findIndex < m_findCount - 1)
            ++m_findIndex;
        ImGui::SameLine();
        if (m_findCount > 0)
            ImGui::TextDisabled("%d/%d", m_findIndex + 1, m_findCount);
        else if (needle.size() >= 2)
            ImGui::TextDisabled("0");
        ImGui::SameLine();
        if (UI::ButtonGhost("X", ImVec2(28, 0)))
        {
            m_showFind = false;
            m_findBuffer[0] = '\0';
            m_findCount = 0;
            m_findIndex = -1;
        }
        ImGui::Spacing();
    }

    void Editor::DrawReadingMode(const std::vector<std::string>& lines, Note& note)
    {
        // Pre-compute code block state
        std::vector<bool> inBlock(lines.size(), false);
        std::vector<bool> isFence(lines.size(), false);
        bool inside = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string lang;
            if (Markdown::IsCodeFence(lines[i], lang))
            {
                isFence[i] = true;
                inside = !inside;
            }
            else if (inside)
            {
                inBlock[i] = true;
            }
        }

        // Enable text wrapping for reading mode
        ImGui::PushTextWrapPos(0.0f);

        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        {
            if (i == m_inlineEditLine)
            {
                DrawInlineEdit(i, note);
            }
            else
            {
                std::string wt = Markdown::RenderLine(lines[i], i, inBlock[i], isFence[i]);
                if (!wt.empty()) m_wikiLinkTarget = wt;
            }
        }

        ImGui::PopTextWrapPos();

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            m_wikiLinkTarget.empty() && m_inlineEditLine < 0)
        {
            m_editMode = true;
        }
    }

    void Editor::DrawInlineEdit(int lineIndex, Note& note)
    {
        ImGui::PushID(lineIndex + 100000);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(Theme::SURFACE_COLOR));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
        ImGui::PushItemWidth(-1);

        if (ImGui::InputText("##inline", &m_inlineBuffer,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            std::vector<std::string> lines;
            Markdown::SplitLines(note.content, lines);
            if (lineIndex >= 0 && lineIndex < static_cast<int>(lines.size()))
                lines[lineIndex] = m_inlineBuffer;

            note.content.clear();
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (i > 0) note.content += '\n';
                note.content += lines[i];
            }
            note.dirty = true;
            m_inlineEditLine = -1;
            m_inlineFocusSet = false;
        }

        if (!m_inlineFocusSet && ImGui::IsItemVisible())
        {
            ImGui::SetKeyboardFocusHere(-1);
            m_inlineFocusSet = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    void Editor::DrawEditMode(Note& note)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(Theme::SURFACE_COLOR));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetColorU32(Theme::BORDER_COLOR));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 12));
        ImGui::PushItemWidth(-1);

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        if (ImGui::InputTextMultiline("##FullEditor", &note.content,
                                      ImVec2(-1, -1), flags))
        {
            note.dirty = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }
}
