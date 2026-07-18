#include "Editor.hpp"

#include "Markdown.hpp"
#include "Theme.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Kalamari
{
    // =========================================================================
    // Helpers
    // =========================================================================
    int Editor::CountWords(const std::string& text)
    {
        int count = 0;
        bool inWord = false;
        for (char c : text)
        {
            if (std::isspace(static_cast<unsigned char>(c)))
            {
                if (inWord) { ++count; inWord = false; }
            }
            else
            {
                inWord = true;
            }
        }
        if (inWord) ++count;
        return count;
    }

    // =========================================================================
    // Draw — top-level entry point
    // =========================================================================
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

        // Reset state on note change
        if (activeNote.get() != m_lastNote)
        {
            m_editMode = false;
            m_inlineEditLine = -1;
            m_lastNote = activeNote.get();
            m_showFind = false;
            m_findIndex = -1;
            m_findCount = 0;
        }

        // ---- Title bar ----
        std::string title = activeNote->path.stem().string();
        if (title.size() > 50) title = title.substr(0, 47) + "...";

        ImGui::SetWindowFontScale(1.35f);
        ImGui::Text("%s", title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        // Dirty indicator
        if (activeNote->dirty)
        {
            ImGui::SameLine();
            ImGui::TextDisabled("\xE2\x97\x8F"); // ●
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Find bar ----
        if (m_showFind)
            DrawFindBar(activeNote->content);

        // ---- Content ----
        std::vector<std::string> lines;
        Markdown::SplitLines(activeNote->content, lines);
        if (lines.empty()) lines.emplace_back();

        ImGui::BeginChild("EditorScroll", ImVec2(0, 0), ImGuiChildFlags_None);

        // Keyboard shortcuts
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

        // Auto-preview: switch to reading mode when editor loses focus.
        // We track focus across frames to avoid timing issues with IsMouseClicked
        // being processed before widget focus updates.
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

        // ---- Word count (bottom bar) ----
        ImGui::Separator();
        int wc = CountWords(activeNote->content);
        int lc = static_cast<int>(lines.size());
        ImGui::TextDisabled("%d words, %d lines", wc, lc);
        if (m_showFind && m_findCount > 0)
        {
            ImGui::SameLine();
            ImGui::TextDisabled(" | %d matches", m_findCount);
        }

        return changed;
    }

    // =========================================================================
    // Find bar
    // =========================================================================
    void Editor::DrawFindBar(const std::string& content)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
        ImGui::SetNextItemWidth(200);
        ImGui::InputTextWithHint("##FindInput", "Find...", m_findBuffer, sizeof(m_findBuffer));

        // Count matches
        std::string needle(m_findBuffer);
        if (needle.size() >= 2)
        {
            // Case-insensitive count
            m_findCount = 0;
            std::string haystackLower = content;
            std::string needleLower = needle;
            std::transform(haystackLower.begin(), haystackLower.end(), haystackLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            std::transform(needleLower.begin(), needleLower.end(), needleLower.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            size_t pos = 0;
            while ((pos = haystackLower.find(needleLower, pos)) != std::string::npos)
            {
                ++m_findCount;
                pos += needleLower.size();
            }
            if (m_findCount > 0 && m_findIndex < 0)
                m_findIndex = 0;
            if (m_findIndex >= m_findCount)
                m_findIndex = m_findCount - 1;
        }
        else
        {
            m_findCount = 0;
            m_findIndex = -1;
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("<") && m_findIndex > 0)
            --m_findIndex;
        ImGui::SameLine();
        if (ImGui::SmallButton(">") && m_findIndex < m_findCount - 1)
            ++m_findIndex;
        ImGui::SameLine();
        if (m_findCount > 0)
            ImGui::TextDisabled("%d/%d", m_findIndex + 1, m_findCount);
        else if (needle.size() >= 2)
            ImGui::TextDisabled("0");
        ImGui::SameLine();
        if (ImGui::SmallButton("X"))
        {
            m_showFind = false;
            m_findBuffer[0] = '\0';
            m_findCount = 0;
            m_findIndex = -1;
        }

        ImGui::PopStyleVar();
        ImGui::Spacing();
    }

    // =========================================================================
    // Reading Mode
    // =========================================================================
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

        // Render each line
        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        {
            if (i == m_inlineEditLine)
            {
                // Inline edit: show InputText for this line
                DrawInlineEdit(i, note);
            }
            else
            {
                std::string wt = Markdown::RenderLine(lines[i], i, inBlock[i], isFence[i]);
                if (!wt.empty()) m_wikiLinkTarget = wt;
            }
        }

        // Click-to-edit: enter full edit mode
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            m_wikiLinkTarget.empty() && m_inlineEditLine < 0)
        {
            m_editMode = true;
        }
    }

    // =========================================================================
    // Inline line editing (for quick edits without leaving reading mode)
    // =========================================================================
    void Editor::DrawInlineEdit(int lineIndex, Note& note)
    {
        ImGui::PushID(lineIndex + 100000);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_WindowBg));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 1));
        ImGui::PushItemWidth(-1);

        if (ImGui::InputText("##inline", &m_inlineBuffer,
                             ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            // Commit the change: rebuild full content
            std::vector<std::string> lines;
            Markdown::SplitLines(note.content, lines);
            if (lineIndex >= 0 && lineIndex < static_cast<int>(lines.size()))
                lines[lineIndex] = m_inlineBuffer;

            // Rejoin
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

        // Auto-focus only once when entering inline edit
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

    // =========================================================================
    // Edit Mode — full InputTextMultiline
    // =========================================================================
    void Editor::DrawEditMode(Note& note)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_WindowBg));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 8));
        ImGui::PushItemWidth(-1);

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
        if (ImGui::InputTextMultiline("##FullEditor", &note.content,
                                      ImVec2(-1, -1), flags))
        {
            note.dirty = true;
        }

        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }
}
