#include "Editor.hpp"

#include "Markdown.hpp"
#include "Theme.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <algorithm>
#include <cctype>

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

    void Editor::InsertMarkdownWrap(Note& note, const char* before, const char* after)
    {
        ImGui::SetClipboardText((std::string(before) + "text" + after).c_str());
        // The user will paste — this is a simple approach for the toolbar
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

        // Mode toggle button (right-aligned)
        ImGui::SameLine();
        const char* modeLabel = m_editMode ? "Preview" : "Edit";
        float btnW = ImGui::CalcTextSize(modeLabel).x + 20;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - btnW);
        if (ImGui::SmallButton(modeLabel))
            m_editMode = !m_editMode;

        ImGui::Spacing();

        // ---- Toolbar (edit mode only) ----
        if (m_editMode)
            DrawToolbar(*activeNote);

        ImGui::Separator();
        ImGui::Spacing();

        // ---- Find bar ----
        if (m_showFind)
            DrawFindBar();

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
    // Toolbar
    // =========================================================================
    void Editor::DrawToolbar(Note& note)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));

        if (ImGui::SmallButton("H1")) { note.content += "\n# "; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("H2")) { note.content += "\n## "; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("H3")) { note.content += "\n### "; note.dirty = true; }
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        if (ImGui::SmallButton("B")) { note.content += "**bold**"; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("I")) { note.content += "*italic*"; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("S")) { note.content += "~~strike~~"; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("`")) { note.content += "`code`"; note.dirty = true; }
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        if (ImGui::SmallButton("Link")) { note.content += "[label](url)"; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Task")) { note.content += "\n- [ ] "; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Quote")) { note.content += "\n> "; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("Code")) { note.content += "\n```\n\n```\n"; note.dirty = true; }
        ImGui::SameLine();
        if (ImGui::SmallButton("HR")) { note.content += "\n---\n"; note.dirty = true; }

        ImGui::PopStyleVar();
        ImGui::Spacing();
    }

    // =========================================================================
    // Find bar
    // =========================================================================
    void Editor::DrawFindBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
        ImGui::SetNextItemWidth(250);
        ImGui::InputTextWithHint("##FindInput", "Find in note...", m_findBuffer, sizeof(m_findBuffer));

        if (!m_findBuffer[0] || std::strlen(m_findBuffer) < 2)
        {
            m_findCount = 0;
            m_findIndex = -1;
        }

        // Count matches in current note content
        if (m_findBuffer[0] && std::strlen(m_findBuffer) >= 2)
        {
            // Find count is computed but highlighting is done via ImGui's built-in selection
            std::string needle(m_findBuffer);
            std::string haystack;
            // Get current note content from the active tab - we'll count from what we have
            m_findCount = 0;
            // We can't easily get the note content here, so we'll count on next frame
        }

        ImGui::SameLine();
        if (ImGui::SmallButton("Find"))
        {
            // Focus the InputTextMultiline to trigger ImGui's built-in find
            // For now, the highlight happens via ImGui's text selection
        }
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
