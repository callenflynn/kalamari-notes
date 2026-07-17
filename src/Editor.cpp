#include "Editor.hpp"

#include "Theme.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include <cstdio>

namespace
{
    void SetHeadingFontScale()
    {
        ImGui::SetWindowFontScale(1.35f);
    }

    void ResetFontScale()
    {
        ImGui::SetWindowFontScale(1.0f);
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
            ImGui::BulletText("**bold**");
            ImGui::BulletText("*italic*");
            ImGui::BulletText("- list item");
            ImGui::BulletText("[link](url)");
            return;
        }

        std::string title = activeNote->path.stem().string();
        ImGui::SetWindowFontScale(1.35f);
        ImGui::Text("%s", title.c_str());
        ImGui::SetWindowFontScale(1.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImVec2 avail = ImGui::GetContentRegionAvail();
        float paneWidth = avail.x * 0.5f - ImGui::GetStyle().ItemSpacing.x * 0.5f;

        // Left pane: source
        {
            ImGui::BeginChild("SourcePane", ImVec2(paneWidth, avail.y), ImGuiChildFlags_Borders);
            ImGui::TextDisabled("Source");
            ImGui::Separator();

            ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
            if (ImGui::InputTextMultiline("##Source", &activeNote->content, ImVec2(-1, -1), flags))
            {
                activeNote->dirty = true;
            }
            ImGui::EndChild();
        }

        ImGui::SameLine();

        // Right pane: preview
        {
            ImGui::BeginChild("PreviewPane", ImVec2(0, avail.y), ImGuiChildFlags_Borders);
            ImGui::TextDisabled("Preview");
            ImGui::Separator();
            RenderMarkdown(activeNote->content);
            ImGui::EndChild();
        }
    }

    void Editor::RenderMarkdown(const std::string& content)
    {
        // Normalize line endings
        std::string normalized;
        normalized.reserve(content.size());
        for (size_t i = 0; i < content.size(); ++i)
        {
            if (content[i] == '\r' && i + 1 < content.size() && content[i + 1] == '\n')
            {
                normalized.push_back('\n');
                ++i;
            }
            else
            {
                normalized.push_back(content[i]);
            }
        }

        size_t start = 0;
        while (start <= normalized.size())
        {
            size_t end = normalized.find('\n', start);
            if (end == std::string::npos)
            {
                RenderMarkdownLine(normalized.substr(start));
                break;
            }
            RenderMarkdownLine(normalized.substr(start, end - start));
            start = end + 1;
        }
    }

    void Editor::RenderMarkdownLine(const std::string& line)
    {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos)
        {
            ImGui::Text(" ");
            return;
        }

        std::string trimmed = line.substr(start);

        if (trimmed.rfind("# ", 0) == 0)
        {
            SetHeadingFontScale();
            ImGui::TextColored(Theme::ACCENT_COLOR, "%s", trimmed.substr(2).c_str());
            ResetFontScale();
            return;
        }

        if (trimmed.rfind("- ", 0) == 0)
        {
            ImGui::BulletText("%s", trimmed.substr(2).c_str());
            return;
        }

        ImGui::TextWrapped("%s", line.c_str());
    }
}
