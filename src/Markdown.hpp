#pragma once

#include <string>
#include <vector>

namespace Kalamari
{
    // Standalone markdown parser/renderer.
    // Call RenderLine() for each line of a note to render it as formatted markdown.
    // Handles: headings, bold, italic, strikethrough, inline code, wiki-links,
    //          task lists, numbered lists, blockquotes, code blocks, horizontal rules,
    //          markdown links, images.
    namespace Markdown
    {
        // Split content into lines (handles \r\n, \r, \n)
        void SplitLines(const std::string& content, std::vector<std::string>& lines);

        // Render a single line as markdown. Requires lineIndex for ImGui ID uniqueness.
        // Returns a string if a wiki-link [[target]] was clicked this frame.
        // Pass existing wikiLinkTarget to accumulate across lines.
        std::string RenderLine(const std::string& line, int lineIndex, bool inCodeBlock, bool isFenceLine);

        // Render inline formatting within a single line of text.
        // Returns the clicked wiki-link target, or empty string.
        std::string RenderInline(const std::string& text);

        // ---- Helpers exposed for Editor integration ----
        struct HeadingInfo { int level; std::string text; };
        HeadingInfo ParseHeading(const std::string& line);
        bool IsListItem(const std::string& line, std::string& content, bool& isTask, bool& checked);
        bool IsHorizontalRule(const std::string& line);
        bool IsBlockquote(const std::string& line, std::string& content);
        bool IsCodeFence(const std::string& line, std::string& lang);
        bool IsOrderedList(const std::string& line, std::string& content);
        bool IsImageLink(const std::string& line, std::string& alt, std::string& url);
    }
}
