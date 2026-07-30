#include "Markdown.hpp"

#include "Theme.hpp"
#include "UI.hpp"
#include "imgui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Kalamari { namespace Markdown
{
    // =========================================================================
    // Line splitting
    // =========================================================================
    void SplitLines(const std::string& content, std::vector<std::string>& lines)
    {
        lines.clear();
        std::string cur;
        for (size_t i = 0; i < content.size(); ++i)
        {
            if (content[i] == '\r')
            {
                lines.push_back(cur); cur.clear();
                if (i + 1 < content.size() && content[i + 1] == '\n') ++i;
            }
            else if (content[i] == '\n')
            {
                lines.push_back(cur); cur.clear();
            }
            else cur.push_back(content[i]);
        }
        lines.push_back(cur);
    }

    // =========================================================================
    // Parsing helpers
    // =========================================================================
    HeadingInfo ParseHeading(const std::string& line)
    {
        size_t i = 0;
        while (i < line.size() && line[i] == ' ') ++i;
        int lvl = 0;
        while (i < line.size() && line[i] == '#' && lvl < 6) { ++lvl; ++i; }
        if (lvl == 0 || (i < line.size() && line[i] != ' ')) return {0, line};
        while (i < line.size() && line[i] == ' ') ++i;
        return {lvl, line.substr(i)};
    }

    bool IsListItem(const std::string& line, std::string& content, bool& isTask, bool& checked)
    {
        isTask = false; checked = false;
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) return false;
        if (s + 1 >= line.size() || line[s] != '-' || line[s + 1] != ' ') return false;

        size_t contentStart = s + 2;
        if (line.size() >= contentStart + 3 &&
            line[contentStart] == '[' && line[contentStart + 2] == ']')
        {
            isTask = true;
            checked = (line[contentStart + 1] == 'x' || line[contentStart + 1] == 'X');
            content = (contentStart + 4 <= line.size()) ? line.substr(contentStart + 4) : "";
        }
        else
        {
            content = line.substr(contentStart);
        }
        return true;
    }

    bool IsHorizontalRule(const std::string& line)
    {
        std::string t = line;
        t.erase(0, t.find_first_not_of(" \t"));
        t.erase(t.find_last_not_of(" \t") + 1);
        if (t.size() < 3) return false;
        char c = t[0];
        if (c != '-' && c != '*' && c != '_') return false;
        for (char ch : t) if (ch != c) return false;
        return true;
    }

    bool IsBlockquote(const std::string& line, std::string& content)
    {
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos || line[s] != '>') return false;
        if (s + 1 < line.size() && line[s + 1] == ' ')
            content = line.substr(s + 2);
        else
            content = line.substr(s + 1);
        return true;
    }

    bool IsCodeFence(const std::string& line, std::string& lang)
    {
        std::string t = line;
        t.erase(0, t.find_first_not_of(" \t"));
        if (t.size() < 3 || t.substr(0, 3) != "```") return false;
        lang = (t.size() > 3) ? t.substr(3) : "";
        return true;
    }

    bool IsOrderedList(const std::string& line, std::string& content)
    {
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos) return false;
        size_t i = s;
        while (i < line.size() && std::isdigit(static_cast<unsigned char>(line[i]))) ++i;
        if (i > s && i < line.size() && line[i] == '.' && i + 1 < line.size() && line[i + 1] == ' ')
        {
            content = line.substr(i + 2);
            return true;
        }
        return false;
    }

    bool IsImageLink(const std::string& line, std::string& alt, std::string& url)
    {
        size_t s = line.find_first_not_of(" \t");
        if (s == std::string::npos || s + 1 >= line.size()) return false;
        if (line[s] != '!' || line[s + 1] != '[') return false;
        size_t altEnd = line.find("](", s + 2);
        if (altEnd == std::string::npos) return false;
        size_t urlEnd = line.find(')', altEnd + 2);
        if (urlEnd == std::string::npos) return false;
        alt = line.substr(s + 2, altEnd - s - 2);
        url = line.substr(altEnd + 2, urlEnd - altEnd - 2);
        return true;
    }

    // =========================================================================
    // Main line renderer
    // =========================================================================
    std::string RenderLine(const std::string& line, int lineIndex, bool inCodeBlock, bool isFenceLine)
    {
        ImGui::PushID(lineIndex);
        std::string wikiTarget;

        if (inCodeBlock)
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float availW = ImGui::GetContentRegionAvail().x;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImU32 bgCol = ImGui::GetColorU32(Theme::SURFACE_COLOR);
            ImU32 barCol = ImGui::GetColorU32(Theme::ACCENT_COLOR);

            // Wrapped text height approximation
            ImVec2 textSize = ImGui::CalcTextSize(line.empty() ? " " : line.c_str());
            int wrapLines = (int)std::ceil(textSize.x / (availW - 32));
            if (wrapLines < 1) wrapLines = 1;
            float blockH = ImGui::GetTextLineHeight() * wrapLines + 12;

            dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + blockH), bgCol, 8.0f);
            dl->AddRectFilled(pos, ImVec2(pos.x + 4, pos.y + blockH), barCol, 8.0f, ImDrawFlags_RoundCornersLeft);

            ImGui::SetCursorScreenPos(ImVec2(pos.x + 16, pos.y + 6));
            ImVec4 tc = ImGui::GetStyle().Colors[ImGuiCol_Text];
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(tc.x * 0.85f, tc.y * 0.85f, tc.z * 0.9f, 1));
            ImGui::Text("%s", line.empty() ? " " : line.c_str());
            ImGui::PopStyleColor();

            ImGui::PopID();
            return "";
        }

        if (isFenceLine)
        {
            std::string lang;
            IsCodeFence(line, lang);
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::Text("%s", lang.empty() ? "```" : lang.c_str());
            ImGui::PopStyleColor();
            ImGui::PopID();
            return "";
        }

        HeadingInfo h = ParseHeading(line);
        if (h.level > 0)
        {
            float scales[] = {0, 1.70f, 1.45f, 1.25f, 1.12f, 1.06f, 1.02f};
            UI::PushHeadingFont();
            ImGui::SetWindowFontScale(scales[h.level]);
            ImGui::Spacing();
            ImVec4 c = Theme::ACCENT_COLOR;
            if (h.level >= 2) c.w = 0.90f;
            ImGui::TextColored(c, "%s", h.text.c_str());
            ImGui::SetWindowFontScale(1.0f);
            UI::PopHeadingFont();
            if (h.level <= 2) ImGui::Spacing();
            ImGui::PopID();
            return "";
        }

        if (IsHorizontalRule(line))
        {
            ImGui::Spacing();
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float w = ImGui::GetContentRegionAvail().x;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec4 c = Theme::BORDER_COLOR;
            dl->AddLine(ImVec2(pos.x, pos.y + 4), ImVec2(pos.x + w, pos.y + 4),
                        ImGui::GetColorU32(c), 1.0f);
            ImGui::Dummy(ImVec2(0, 9));
            ImGui::PopID();
            return "";
        }

        std::string qc;
        if (IsBlockquote(line, qc))
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            float h = ImGui::GetTextLineHeight();
            float availW = ImGui::GetContentRegionAvail().x;
            ImDrawList* dl = ImGui::GetWindowDrawList();
            // Subtle left bar + background
            ImU32 bg = ImGui::GetColorU32(Theme::SURFACE_COLOR);
            dl->AddRectFilled(pos, ImVec2(pos.x + availW, pos.y + h + 8), bg, 4.0f);
            dl->AddRectFilled(pos, ImVec2(pos.x + 3, pos.y + h + 8), ImGui::GetColorU32(Theme::ACCENT_COLOR), 4.0f, ImDrawFlags_RoundCornersLeft);
            ImGui::SetCursorScreenPos(ImVec2(pos.x + 12, pos.y + 4));
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            if (qc.empty()) ImGui::Text(" ");
            else wikiTarget = RenderInline(qc);
            ImGui::PopStyleColor();
            ImGui::PopID();
            return wikiTarget;
        }

        std::string lc; bool isTask, checked;
        if (IsListItem(line, lc, isTask, checked))
        {
            if (isTask)
            {
                bool chk = checked;
                ImGui::Checkbox(("##task" + std::to_string(lineIndex)).c_str(), &chk);
                ImGui::SameLine();
                if (checked)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    wikiTarget = RenderInline(lc);
                    ImGui::PopStyleColor();
                }
                else wikiTarget = RenderInline(lc);
            }
            else
            {
                ImGui::BulletText("%s", "");
                ImGui::SameLine();
                wikiTarget = RenderInline(lc);
            }
            ImGui::PopID();
            return wikiTarget;
        }

        std::string ol;
        if (IsOrderedList(line, ol))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            size_t s = line.find_first_not_of(" \t");
            size_t e = line.find('.', s);
            ImGui::Text("%s.", line.substr(s, e - s).c_str());
            ImGui::SameLine();
            ImGui::PopStyleColor();
            wikiTarget = RenderInline(ol);
            ImGui::PopID();
            return wikiTarget;
        }

        std::string alt, url;
        if (IsImageLink(line, alt, url))
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
            ImGui::Text("[Image: %s]", alt.empty() ? "image" : alt.c_str());
            ImGui::PopStyleColor();
            ImGui::PopID();
            return "";
        }

        if (line.empty())
        {
            ImGui::Spacing();
            ImGui::PopID();
            return "";
        }

        wikiTarget = RenderInline(line);
        ImGui::PopID();
        return wikiTarget;
    }

    // =========================================================================
    // Inline formatting renderer
    // =========================================================================
    std::string RenderInline(const std::string& text, int maxDepth)
    {
        if (maxDepth <= 0)
        {
            if (!text.empty())
            {
                ImGui::Text("%s", text.c_str());
            }
            return "";
        }

        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4 textColor = style.Colors[ImGuiCol_Text];
        ImVec4 linkColor = Theme::ACCENT_COLOR;
        ImVec4 linkHover = Theme::ACCENT_COLOR_HDR;
        ImVec4 codeBg = Theme::SURFACE_COLOR;
        ImVec4 strikeColor = ImVec4(textColor.x * 0.6f, textColor.y * 0.6f, textColor.z * 0.6f, textColor.w);

        std::string wikiTarget;
        std::string buf;
        size_t pos = 0;

        auto flush = [&]() {
            if (!buf.empty())
            {
                ImGui::Text("%s", buf.c_str());
                ImGui::SameLine(0, 0);
                buf.clear();
            }
        };

        while (pos < text.size())
        {
            // Wiki-link [[Name]]
            if (pos + 1 < text.size() && text[pos] == '[' && text[pos + 1] == '[' )
            {
                size_t end = text.find("]]", pos + 2);
                if (end != std::string::npos)
                {
                    flush();
                    std::string name = text.substr(pos + 2, end - pos - 2);

                    ImVec2 linkPos = ImGui::GetCursorScreenPos();
                    float w = ImGui::CalcTextSize(name.c_str()).x;
                    float h = ImGui::GetTextLineHeight();
                    bool hover = ImGui::IsMouseHoveringRect(linkPos, ImVec2(linkPos.x + w, linkPos.y + h));

                    ImGui::PushStyleColor(ImGuiCol_Text, hover ? linkHover : linkColor);
                    ImGui::Text("%s", name.c_str());
                    ImGui::PopStyleColor();

                    if (hover)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        dl->AddLine(ImVec2(linkPos.x, linkPos.y + h),
                                    ImVec2(linkPos.x + w, linkPos.y + h),
                                    ImGui::GetColorU32(hover ? linkHover : linkColor), 1);
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                            wikiTarget = name;
                    }
                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            // Markdown link [label](url)
            if (text[pos] == '[' && pos > 0 && text[pos - 1] != '!')
            {
                size_t br = text.find("](", pos + 1);
                if (br != std::string::npos)
                {
                    size_t rp = text.find(')', br + 2);
                    if (rp != std::string::npos)
                    {
                        flush();
                        std::string label = text.substr(pos + 1, br - pos - 1);
                        std::string linkUrl = text.substr(br + 2, rp - br - 2);
                        ImVec2 lp = ImGui::GetCursorScreenPos();
                        float lw = ImGui::CalcTextSize(label.c_str()).x;
                        float lh = ImGui::GetTextLineHeight();
                        bool lhvr = ImGui::IsMouseHoveringRect(lp, ImVec2(lp.x + lw, lp.y + lh));
                        ImGui::PushStyleColor(ImGuiCol_Text, lhvr ? linkHover : linkColor);
                        ImGui::Text("%s", label.c_str());
                        ImGui::PopStyleColor();
                        if (lhvr)
                        {
                            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            dl->AddLine(ImVec2(lp.x, lp.y + lh), ImVec2(lp.x + lw, lp.y + lh),
                                        ImGui::GetColorU32(lhvr ? linkHover : linkColor), 1);
                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                                SDL_OpenURL(linkUrl.c_str());
                        }
                        ImGui::SameLine(0, 0);
                        pos = rp + 1;
                        continue;
                    }
                }
            }

            // `code`
            if (text[pos] == '`')
            {
                size_t end = text.find('`', pos + 1);
                if (end != std::string::npos)
                {
                    flush();
                    std::string inner = text.substr(pos + 1, end - pos - 1);
                    ImVec2 cur = ImGui::GetCursorScreenPos();
                    ImVec2 sz = ImGui::CalcTextSize(inner.c_str());
                    float padX = 4, padY = 1;
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddRectFilled(ImVec2(cur.x - padX, cur.y - padY),
                                      ImVec2(cur.x + sz.x + padX, cur.y + sz.y + padY),
                                      ImGui::GetColorU32(codeBg), 4.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, Theme::ACCENT_COLOR);
                    ImGui::Text("%s", inner.c_str());
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 1;
                    continue;
                }
            }

            // **bold**
            if (pos + 1 < text.size() && text[pos] == '*' && text[pos + 1] == '*')
            {
                size_t end = text.find("**", pos + 2);
                if (end != std::string::npos)
                {
                    flush();
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImVec4((std::min)(textColor.x * 1.15f, 1.0f),
                               (std::min)(textColor.y * 1.05f, 1.0f),
                               textColor.z * 0.9f, 1));
                    std::string sub = RenderInline(text.substr(pos + 2, end - pos - 2), maxDepth - 1);
                    if (!sub.empty()) wikiTarget = sub;
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            // *italic* (not **)
            if (text[pos] == '*' && (pos == 0 || text[pos - 1] != '*') &&
                (pos + 1 >= text.size() || text[pos + 1] != '*'))
            {
                size_t end = text.find('*', pos + 1);
                if (end != std::string::npos && (end + 1 >= text.size() || text[end + 1] != '*'))
                {
                    flush();
                    ImGui::PushStyleColor(ImGuiCol_Text,
                        ImVec4(textColor.x * 0.85f, textColor.y * 0.85f, textColor.z * 0.85f, 0.85f));
                    std::string sub = RenderInline(text.substr(pos + 1, end - pos - 1), maxDepth - 1);
                    if (!sub.empty()) wikiTarget = sub;
                    ImGui::PopStyleColor();
                    ImGui::SameLine(0, 0);
                    pos = end + 1;
                    continue;
                }
            }

            // ~~strikethrough~~
            if (pos + 1 < text.size() && text[pos] == '~' && text[pos + 1] == '~')
            {
                size_t end = text.find("~~", pos + 2);
                if (end != std::string::npos)
                {
                    flush();
                    ImVec2 cur = ImGui::GetCursorScreenPos();
                    ImVec2 sz = ImGui::CalcTextSize(text.substr(pos + 2, end - pos - 2).c_str());
                    ImGui::PushStyleColor(ImGuiCol_Text, strikeColor);
                    std::string sub = RenderInline(text.substr(pos + 2, end - pos - 2), maxDepth - 1);
                    if (!sub.empty()) wikiTarget = sub;
                    ImGui::PopStyleColor();
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    dl->AddLine(ImVec2(cur.x, cur.y + sz.y * 0.5f),
                                ImVec2(cur.x + sz.x, cur.y + sz.y * 0.5f),
                                ImGui::GetColorU32(strikeColor), 1);
                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            // ==highlight==
            if (pos + 1 < text.size() && text[pos] == '=' && text[pos + 1] == '=' )
            {
                size_t end = text.find("==", pos + 2);
                if (end != std::string::npos)
                {
                    flush();
                    ImVec2 cur = ImGui::GetCursorScreenPos();
                    std::string inner = text.substr(pos + 2, end - pos - 2);
                    ImVec2 sz = ImGui::CalcTextSize(inner.c_str());
                    ImDrawList* dl = ImGui::GetWindowDrawList();
                    ImVec4 hlBg = ImVec4(Theme::ACCENT_COLOR.x, Theme::ACCENT_COLOR.y, Theme::ACCENT_COLOR.z, 0.18f);
                    dl->AddRectFilled(
                        ImVec2(cur.x - 3, cur.y - 2),
                        ImVec2(cur.x + sz.x + 3, cur.y + sz.y + 2),
                        ImGui::GetColorU32(hlBg), 4.0f);
                    std::string sub = RenderInline(inner, maxDepth - 1);
                    if (!sub.empty()) wikiTarget = sub;
                    ImGui::SameLine(0, 0);
                    pos = end + 2;
                    continue;
                }
            }

            buf.push_back(text[pos++]);
        }

        if (!buf.empty())
            ImGui::Text("%s", buf.c_str());

        return wikiTarget;
    }
}}
