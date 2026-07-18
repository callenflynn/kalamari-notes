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
    // =========================================================================
    // Draw — top-level entry point
    // =========================================================================
    void Editor::Draw(const std::shared_ptr<Note>& activeNote)
    {
        if (!activeNote)
        {
            ImVec2 region = ImGui::GetContentRegionAvail();
            ImGui::SetCursorPos(ImVec2(
                (region.x - ImGui::CalcTextSize("Select or create a note to begin.").x) * 0.5f,
                region.y * 0.35f));
            ImGui::TextDisabled("Select or create a note to begin.");

            ImGui::Spacing();
            ImGui::Spacing();

            ImGui::TextWrapped("Markdown basics:");
            ImGui::BulletText("# Heading / ## Heading / ### Heading");
            ImGui::BulletText("**bold**  *italic*  ~~strikethrough~~  `code`");
            ImGui::BulletText("- list item");
            ImGui::BulletText("> blockquote");
            ImGui::BulletText("--- horizontal rule");
            ImGui::BulletText("[[Wiki Link]] to connect notes");
            ImGui::BulletText("``` code block ```");
            ImGui::BulletText("[link](url)");
            return;
        }

        // Clear wiki-link target each frame (consumed by KalamariApp)
        m_wikiLinkTarget.clear();

        // Reset state when the note changes
        if (activeNote.get() != m_lastNote)
        {
            m_editMode = false;
            m_lastNote = activeNote.get();
        }

        // ---- Title bar with mode toggle ----
        std::string title = activeNote->path.stem().string();
        ImGui::SetWindowFontScale(1.35f);
        ImGui::Text("%s", title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::SameLine();
        float toggleWidth = ImGui::CalcTextSize(m_editMode ? "Preview" : "Edit").x + 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - toggleWidth);
        if (ImGui::SmallButton(m_editMode ? "Preview" : "Edit"))
        {
            m_editMode = !m_editMode;
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- Split content into lines ----
        std::vector<std::string> lines;
        SplitLines(activeNote->content, lines);

        if (lines.empty())
            lines.emplace_back();

        // ---- Editor content area ----
        ImGui::BeginChild("EditorScroll", ImVec2(0, 0), ImGuiChildFlags_None);

        // Keyboard shortcuts
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_E))
            m_editMode = !m_editMode;

        if (ImGui::IsKeyPressed(ImGuiKey_Escape) && m_editMode)
            m_editMode = false;

        if (m_editMode)
        {
            DrawEditMode(*activeNote);
        }
        else
        {
            DrawReadingMode(lines);
        }

        ImGui::EndChild();
    }

    // =========================================================================
    // Reading Mode — beautiful markdown preview, click anywhere to edit
    // =========================================================================
    void Editor::DrawReadingMode(const std::vector<std::string>& lines)
    {
        // Pre-compute code block ranges
        std::vector<bool> inCodeBlock(lines.size(), false);
        std::vector<bool> isFenceLine(lines.size(), false);
        bool insideBlock = false;
        for (size_t i = 0; i < lines.size(); ++i)
        {
            std::string trimmed = lines[i];
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            if (trimmed.size() >= 3 && trimmed.substr(0, 3) == "```")
            {
                isFenceLine[i] = true;
                insideBlock = !insideBlock;
                continue;
            }
            if (insideBlock)
                inCodeBlock[i] = true;
        }

        // Render each line as pure markdown preview (no editing, no invisible buttons)
        for (int i = 0; i < static_cast<int>(lines.size()); ++i)
        {
            if (inCodeBlock[i])
                RenderCodeLine(lines[i], i);
            else if (isFenceLine[i])
                RenderFenceLine(lines[i], i);
            else
                RenderMarkdownLine(lines[i], i);
        }

        // Click-to-edit: clicking content area enters edit mode.
        // Wiki-link clicks set m_wikiLinkTarget, which blocks this.
        // MouseDown+MouseUp within a small threshold avoids scrollbar drags.
        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            if (m_wikiLinkTarget.empty())
                m_editMode = true;
        }
    }

    // =========================================================================
    // Edit Mode — unified InputTextMultiline editor
    // =========================================================================
    void Editor::DrawEditMode(Note& note)
    {
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(ImGuiCol_WindowBg));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f));
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

    // =========================================================================
    // Line splitting / joining
    // =========================================================================
    void Editor::SplitLines(const std::string& content, std::vector<std::string>& lines) const
    {
        lines.clear();
        std::string current;
        for (size_t i = 0; i < content.size(); ++i)
        {
            if (content[i] == '\r')
            {
                lines.push_back(current);
                current.clear();
                if (i + 1 < content.size() && content[i + 1] == '\n')
                    ++i;
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

    // =========================================================================
    // Markdown line rendering (reading mode only — no editing, no invisible buttons)
    // =========================================================================
    void Editor::RenderMarkdownLine(const std::string& line, int lineIndex)
    {
        ImGui::PushID(lineIndex);

        // ---- Heading ----
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
                color.w = 0.85f;
            ImGui::TextColored(color, "%s", heading.text.c_str());
            ImGui::SetWindowFontScale(1.0f);

            ImGui::PopID();
            return;
        }

        // ---- Horizontal rule ----
        {
            std::string trimmed = line;
            trimmed.erase(0, trimmed.find_first_not_of(" \t"));
            if ((trimmed.size() >= 3 && trimmed.substr(0, 3) == "---") ||
                (trimmed.size() >= 3 && trimmed.substr(0, 3) == "***") ||
                (trimmed.size() >= 3 && trimmed.substr(0, 3) == "___"))
            {
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::PopID();
                return;
            }
        }

        // ---- List item ----
        std::string listContent;
        if (IsListItem(line, listContent))
        {
            ImGui::BulletText("%s", listContent.c_str());
            ImGui::PopID();
            return;
        }

        // ---- Blockquote ----
        {
            size_t start = line.find_first_not_of(" \t");
            if (start != std::string::npos && line[start] == '>')
            {
                std::string quoteContent;
                if (start + 1 < line.size() && line[start + 1] == ' ')
                    quoteContent = line.substr(start + 2);
                else
                    quoteContent = line.substr(start + 1);

                ImGui::PushStyleColor(ImGuiCol_Text,
                    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

                ImVec2 cursor = ImGui::GetCursorScreenPos();
                float lineHeight = ImGui::GetTextLineHeight();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(
                    ImVec2(cursor.x, cursor.y),
                    ImVec2(cursor.x + 3.0f, cursor.y + lineHeight),
                    ImGui::GetColorU32(ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]));

                ImGui::SetCursorScreenPos(ImVec2(cursor.x + 8.0f, cursor.y));
                ImGui::TextWrapped("%s", quoteContent.empty() ? " " : quoteContent.c_str());

                ImGui::PopStyleColor();
                ImGui::PopID();
                return;
            }
        }

        // ---- Plain text with inline formatting ----
        if (line.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text,
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::Text("\u00B6");  // pilcrow for empty lines
            ImGui::PopStyleColor();
        }
        else
        {
            RenderInlineFormatted(line);
        }

        ImGui::PopID();
    }

    // =========================================================================
    // Code block line rendering
    // =========================================================================
    void Editor::RenderCodeLine(const std::string& line, int lineIndex)
    {
        ImGui::PushID(lineIndex);

        ImVec2 lineStartPos = ImGui::GetCursorScreenPos();
        float lineHeight = ImGui::GetTextLineHeight();
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // Theme-adaptive code block background
        ImVec4 codeBgColor = ImVec4(
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].x * 0.7f,
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].y * 0.7f,
            ImGui::GetStyle().Colors[ImGuiCol_WindowBg].z * 0.85f,
            0.6f);
        dl->AddRectFilled(
            lineStartPos,
            ImVec2(lineStartPos.x + ImGui::GetContentRegionAvail().x,
                   lineStartPos.y + lineHeight + 2.0f),
            ImGui::GetColorU32(codeBgColor), 2.0f);

        // Left accent border
        dl->AddRectFilled(
            lineStartPos,
            ImVec2(lineStartPos.x + 3.0f, lineStartPos.y + lineHeight + 2.0f),
            ImGui::GetColorU32(Theme::ACCENT_COLOR), 0.0f);

        ImGui::SetCursorScreenPos(ImVec2(lineStartPos.x + 10.0f, lineStartPos.y + 1.0f));
        ImVec4 codeTextColor = ImVec4(
            ImGui::GetStyle().Colors[ImGuiCol_Text].x * 0.85f,
            ImGui::GetStyle().Colors[ImGuiCol_Text].y * 0.85f,
            ImGui::GetStyle().Colors[ImGuiCol_Text].z * 0.9f,
            1.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, codeTextColor);
        ImGui::Text("%s", line.empty() ? " " : line.c_str());
        ImGui::PopStyleColor();

        ImGui::PopID();
    }

    // =========================================================================
    // Code fence line rendering (``` or ```lang)
    // =========================================================================
    void Editor::RenderFenceLine(const std::string& line, int lineIndex)
    {
        ImGui::PushID(lineIndex);

        std::string trimmed = line;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));

        // Extract language specifier (e.g., "python" from "```python")
        std::string display;
        if (trimmed.size() > 3)
            display = trimmed.substr(3);
        if (display.empty())
            display = " ";

        ImGui::PushStyleColor(ImGuiCol_Text,
            ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        ImGui::Text("%s", display.c_str());
        ImGui::PopStyleColor();

        ImGui::PopID();
    }

    // =========================================================================
    // Inline formatted text: **bold**, *italic*, ~~strikethrough~~, `code`, [[Wiki Link]]
    // =========================================================================
    void Editor::RenderInlineFormatted(const std::string& text)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4 textColor = style.Colors[ImGuiCol_Text];
        ImVec4 codeColor = ImVec4(0.929f, 0.314f, 0.004f, 0.85f);
        ImVec4 strikethroughColor = ImVec4(
            textColor.x * 0.7f, textColor.y * 0.7f, textColor.z * 0.7f, textColor.w);

        std::string rendered;
        size_t pos = 0;

        while (pos < text.size())
        {
            // ---- Wiki-link: [[Note Name]] ----
            if (pos + 1 < text.size() && text[pos] == '[' && text[pos + 1] == '[')
            {
                size_t end = text.find("]]", pos + 2);
                if (end != std::string::npos)
                {
                    // Flush accumulated text
                    if (!rendered.empty())
                    {
                        ImGui::Text("%s", rendered.c_str());
                        ImGui::SameLine(0, 0);
                        rendered.clear();
                    }

                    std::string linkName = text.substr(pos + 2, end - pos - 2);

                    // Render clickable link
                    ImVec4 linkColor = ImVec4(0.929f, 0.314f, 0.004f, 0.9f);
                    ImVec4 linkHover = ImVec4(1.0f, 0.45f, 0.05f, 1.0f);

                    ImVec2 linkTextPos = ImGui::GetCursorScreenPos();
                    float linkTextWidth = ImGui::CalcTextSize(linkName.c_str()).x;
                    float linkTextHeight = ImGui::GetTextLineHeight();
                    bool hovered = ImGui::IsMouseHoveringRect(
                        linkTextPos,
                        ImVec2(linkTextPos.x + linkTextWidth, linkTextPos.y + linkTextHeight));

                    ImGui::PushStyleColor(ImGuiCol_Text, hovered ? linkHover : linkColor);
                    ImGui::Text("%s", linkName.c_str());
                    ImGui::PopStyleColor();

                    if (hovered)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        dl->AddLine(
                            ImVec2(linkTextPos.x, linkTextPos.y + linkTextHeight),
                            ImVec2(linkTextPos.x + linkTextWidth, linkTextPos.y + linkTextHeight),
                            ImGui::GetColorU32(hovered ? linkHover : linkColor), 1.0f);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            m_wikiLinkTarget = linkName;
                    }

                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            // ---- Inline code: `code` ----
            if (text[pos] == '`')
            {
                size_t end = text.find('`', pos + 1);
                if (end != std::string::npos)
                {
                    if (!rendered.empty())
                    {
                        ImGui::Text("%s", rendered.c_str());
                        ImGui::SameLine(0, 0);
                        rendered.clear();
                    }

                    std::string code = text.substr(pos + 1, end - pos - 1);
                    ImGui::PushStyleColor(ImGuiCol_Text, codeColor);
                    ImGui::Text("%s", code.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 1;
                    continue;
                }
            }

            // ---- Bold: **text** ----
            if (pos + 1 < text.size() && text[pos] == '*' && text[pos + 1] == '*')
            {
                size_t end = text.find("**", pos + 2);
                if (end != std::string::npos)
                {
                    if (!rendered.empty())
                    {
                        ImGui::Text("%s", rendered.c_str());
                        ImGui::SameLine(0, 0);
                        rendered.clear();
                    }

                    std::string bold = text.substr(pos + 2, end - pos - 2);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(
                        ImMin(textColor.x * 1.15f, 1.0f),
                        ImMin(textColor.y * 1.05f, 1.0f),
                        textColor.z * 0.9f,
                        1.0f));
                    RenderInlineFormatted(bold);  // recursive: wiki-links inside bold work
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            // ---- Italic: *text* (but not **) ----
            if (text[pos] == '*' && (pos == 0 || text[pos - 1] != '*') &&
                (pos + 1 >= text.size() || text[pos + 1] != '*'))
            {
                size_t end = text.find('*', pos + 1);
                if (end != std::string::npos &&
                    (end + 1 >= text.size() || text[end + 1] != '*'))
                {
                    if (!rendered.empty())
                    {
                        ImGui::Text("%s", rendered.c_str());
                        ImGui::SameLine(0, 0);
                        rendered.clear();
                    }

                    std::string italic = text.substr(pos + 1, end - pos - 1);
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImVec4(textColor.x * 0.85f, textColor.y * 0.85f,
                               textColor.z * 0.85f, 0.85f));
                    RenderInlineFormatted(italic);
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 1;
                    continue;
                }
            }

            // ---- Strikethrough: ~~text~~ ----
            if (pos + 1 < text.size() && text[pos] == '~' && text[pos + 1] == '~')
            {
                size_t end = text.find("~~", pos + 2);
                if (end != std::string::npos)
                {
                    if (!rendered.empty())
                    {
                        ImGui::Text("%s", rendered.c_str());
                        ImGui::SameLine(0, 0);
                        rendered.clear();
                    }

                    std::string striked = text.substr(pos + 2, end - pos - 2);
                    ImVec2 cursor = ImGui::GetCursorScreenPos();
                    ImVec2 textSize = ImGui::CalcTextSize(striked.c_str());
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    float y = cursor.y + textSize.y * 0.5f;

                    ImGui::PushStyleColor(ImGuiCol_Text, strikethroughColor);
                    RenderInlineFormatted(striked);
                    ImGui::PopStyleColor();

                    dl->AddLine(ImVec2(cursor.x, y), ImVec2(cursor.x + textSize.x, y),
                                ImGui::GetColorU32(strikethroughColor), 1.0f);

                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            rendered.push_back(text[pos]);
            ++pos;
        }

        // Flush remaining text
        if (!rendered.empty())
            ImGui::Text("%s", rendered.c_str());
    }
}
