#pragma once

#include "Vault.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Kalamari
{
    class Editor
    {
    public:
        Editor() = default;
        ~Editor() = default;

        void Draw(const std::shared_ptr<Note>& activeNote);

        // Wiki-link navigation: returns non-empty if a link was clicked this frame
        const std::string& GetWikiLinkTarget() const { return m_wikiLinkTarget; }
        void ClearWikiLinkTarget() { m_wikiLinkTarget.clear(); }

    private:
        const Note* m_lastNote = nullptr;
        bool m_editMode = false;
        std::string m_wikiLinkTarget;

        void DrawReadingMode(const std::vector<std::string>& lines);
        void DrawEditMode(Note& note);
        void SplitLines(const std::string& content, std::vector<std::string>& lines) const;
        void RenderMarkdownLine(const std::string& line, int lineIndex);
        void RenderCodeLine(const std::string& line, int lineIndex);
        void RenderFenceLine(const std::string& line, int lineIndex);
        void RenderInlineFormatted(const std::string& text);
    };
}
