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

    private:
        int m_focusedLine = -1;
        bool m_focusJustChanged = false;
        std::string m_editBuffer;
        const Note* m_lastNote = nullptr;

        void SplitLines(const std::string& content, std::vector<std::string>& lines) const;
        void JoinLines(const std::vector<std::string>& lines, std::string& content) const;
        void RenderMarkdownLine(const std::string& line, int lineIndex);
    };
}
