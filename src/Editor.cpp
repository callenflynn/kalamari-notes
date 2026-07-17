#include "Editor.hpp"

#include "Theme.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <cctype>
#include <cstdio>

namespace
{
    struct HeadingInfo
    {
        int level = 0;
        std::string text;
    };

    HeadingInfo ParseHeading(const std::string& line)
    {
        size_t i = 0;
        while (i < line.size() && line[i] == ' ')
            ++i;

        int level = 0;
        while (i < line.size() && line[i] == '#' && level < 6)
        {
            ++level;
            ++i;
        }

        if (level == 0 || level > 6)
            return {0, line};

        if (i < line.size() && line[i] != ' ')
            return {0, line};

        while (i < line.size() && line[i] == ' ')
            ++i;

        return {level, line.substr(i)};
    }

    bool IsListItem(const std::string& line, std::string& content)
    {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
            return false;

        if (start + 1 < line.size() && line[start] == '-' && line[start + 1] == ' ')
        {
            content = line.substr(start + 2);
            return true;
        }
        return false;
    }
}

namespace Kalamari
{
    void Editor::Draw(const std::shared_ptr<Note>& activeNote)
    {
        if (!activeNote)
        {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos(ImVec2((region.x - ImGui::CalcTextSize("Select or create a note to begin.").x) * 0.5f,
                                        region.y * 0.35f));
            ImGui::TextDisabled("Select or create a note to begin.");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TextWrapped("Markdown basics:");
            ImGui::BulletText("# Heading");
            ImGui::BulletText("## Heading");
            ImGui::BulletText("### Heading");
            ImGui::BulletText("**bold**");
            ImGui::BulletText("*italic*");
            ImGui::BulletText("- list item");
            ImGui::BulletText("[link](url)");
            return;
        }

        // Reset focus state when the note changes
        if (activeNote.get() != m_lastNote)
        {
            m_focusedLine = -1;
            m_editBuffer.clear();
            m_lastNote = activeNote.get();
        }

        std::string title = activeNote->path.stem().string();
        ImGui::SetWindowFontScale(1.35f);
        ImGui::Text("%s", title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        std::vector<std::string> lines;
        SplitLines(activeNote->content, lines);

        // Ensure at least one editable line
        if (lines.empty())
            lines.emplace_back();

        // Clamp focused line to valid range
        if (m_focusedLine >= static_cast<int>(lines.size()))
            m_focusedLine = static_cast<int>(lines.size()) - 1;

        ImGui::BeginChild("EditorScroll", ImVec2(0, 0), ImGuiChildFlags_None);

        // Defocus on Escape
        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            m_focusedLine = -1;
        }

        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        {
            if (i == m_focusedLine)
            {
                // Editable line
                if (m_editBuffer != lines[i])
                {
                    m_editBuffer = lines[i];
                }

                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_WindowBg));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::PushItemWidth(-1);

                if (m_focusJustChanged)
                {
                    ImGui::SetKeyboardFocusHere();
                    m_focusJustChanged = false;
                }

                ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AllowTabInput;
                bool edited = ImGui::InputText("##Line", &m_editBuffer, flags);

                // Live update while typing
                if (edited || ImGui::IsItemEdited())
                {
                    lines[i] = m_editBuffer;
                    JoinLines(lines, activeNote->content);
                    activeNote->dirty = true;
                }

                if (edited)
                {
                    // Enter pressed: insert new line below
                    lines.insert(lines.begin() + i + 1, std::string());
                    JoinLines(lines, activeNote->content);
                    m_focusedLine = i + 1;
                    m_editBuffer.clear();
                }

                ImGui::PopItemWidth();
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();
            }
            else
            {
                // Render markdown preview
                RenderMarkdownLine(lines[i], i);
            }
        }

        ImGui::EndChild();
    }

    void Editor::SplitLines(const std::string& content, std::vector<std::string>& lines) const
    {
        lines.clear();
        std::string current;
        for (size_t i = 0; i < content.size(); ++i)
        {
            if (content[i] == '\r')
            {
                if (i + 1 < content.size() && content[i + 1] == '\n')
                {
                    lines.push_back(current);
                    current.clear();
                    ++i;
                }
                else
                {
                    current.push_back(content[i]);
                }
            }
            else if (content[i] == '\n')
            {
                lines.push_back(current);
                current.clear();
            }
            else
            {
                current.push_back(content[i]);
            }
        }
        lines.push_back(current);
    }

    void Editor::JoinLines(const std::vector<std::string>& lines, std::string& content) const
    {
        content.clear();
        for (size_t i = 0; i < lines.size(); ++i)
        {
            if (i > 0)
                content.push_back('\n');
            content += lines[i];
        }
    }

    void Editor::RenderMarkdownLine(const std::string& line, int lineIndex)
    {
        ImGui::PushID(lineIndex);

        // Render heading
        HeadingInfo heading = ParseHeading(line);
        if (heading.level > 0)
        {
            float scale = 1.0f;
            switch (heading.level)
            {
                case 1: scale = 1.5f; break;
                case 2: scale = 1.3f; break;
                case 3: scale = 1.15f; break;
                default: scale = 1.05f; break;
            }

            ImGui::SetWindowFontScale(scale);
            ImVec4 color = Theme::ACCENT_COLOR;
            if (heading.level >= 2)
            {
                color.w = 0.85f;
            }
            ImGui::TextColored(color, "%s", heading.text.c_str());
            ImGui::SetWindowFontScale(1.0f);

            if (ImGui::IsItemClicked())
            {
                m_focusedLine = lineIndex;
                m_editBuffer = line;
            }

            ImGui::PopID();
            return;
        }

        // Render list item
        std::string listContent;
        if (IsListItem(line, listContent))
        {
            ImGui::BulletText("%s", listContent.c_str());
            if (ImGui::IsItemClicked())
            {
                m_focusedLine = lineIndex;
                m_editBuffer = line;
            }
            ImGui::PopID();
            return;
        }

        // Render plain text with wrapping
        if (line.empty())
        {
            ImGui::Text(" ");
        }
        else
        {
            ImGui::TextWrapped("%s", line.c_str());
        }

        if (ImGui::IsItemClicked())
        {
            m_focusedLine = lineIndex;
            m_editBuffer = line;
            m_focusJustChanged = true;
        }

        ImGui::PopID();
    }
}
