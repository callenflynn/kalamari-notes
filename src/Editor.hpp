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

        // Draw the editor for the given note. Returns true if content changed.
        bool Draw(const std::shared_ptr<Note>& activeNote);

        // Wiki-link navigation
        const std::string& GetWikiLinkTarget() const { return m_wikiLinkTarget; }
        void ClearWikiLinkTarget() { m_wikiLinkTarget.clear(); }

        bool IsEditing() const { return m_editMode; }
        void SetEditMode(bool edit) { m_editMode = edit; }

    private:
        const Note* m_lastNote = nullptr;
        bool m_editMode = false;
        bool m_wasEditingLastFrame = false;
        std::string m_wikiLinkTarget;
        int m_inlineEditLine = -1;   // Line index being edited inline (-1 = none)
        bool m_inlineFocusSet = false; // Whether focus has been set for inline edit
        std::string m_inlineBuffer;   // Buffer for inline line editing

        // Find in page
        bool m_showFind = false;
        char m_findBuffer[256] = {};
        int m_findIndex = -1;
        int m_findCount = 0;

        void DrawReadingMode(const std::vector<std::string>& lines, Note& note);
        void DrawEditMode(Note& note);
        void DrawInlineEdit(int lineIndex, Note& note);
        void DrawFindBar(const std::string& content);
        static int CountWords(const std::string& text);
    };
}
