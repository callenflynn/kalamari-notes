#pragma once

#include "AppRenderer.hpp"
#include "Editor.hpp"
#include "Vault.hpp"
#include <memory>
#include <string>

namespace Kalamari
{
    class KalamariApp
    {
    public:
        KalamariApp() = default;
        ~KalamariApp() = default;

        bool Init();
        void Run();
        void Shutdown();

    private:
        AppRenderer m_renderer;
        Vault m_vault;
        Editor m_editor;

        std::shared_ptr<Note> m_currentNote;
        bool m_darkMode = true;
        bool m_showSettings = false;

        char m_searchBuffer[256] = {};
        char m_renameBuffer[256] = {};
        char m_vaultSwitchBuffer[256] = {};
        std::shared_ptr<Note> m_noteToRename;
        std::shared_ptr<Note> m_noteToDelete;

        void DrawSidebar();
        void DrawMainArea();
        void DrawSettingsModal();
        void DrawRenameModal();
        void ProcessDeferredOperations();
    };
}
