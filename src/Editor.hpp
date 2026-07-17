#pragma once

#include "Vault.hpp"
#include <memory>

namespace Kalamari
{
    class Editor
    {
    public:
        Editor() = default;
        ~Editor() = default;

        void Draw(const std::shared_ptr<Note>& activeNote);

    private:
        void RenderMarkdown(const std::string& content);
        void RenderMarkdownLine(const std::string& line);
    };
}
