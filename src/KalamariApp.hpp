#pragma once

#include "AppRenderer.hpp"
#include "Config.hpp"
#include "Editor.hpp"
#include "Sidebar.hpp"
#include "Vault.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Kalamari
{
    // An open tab in the editor
    struct Tab
    {
        std::shared_ptr<Note> note;
        Editor editor; // Each tab has its own editor state
    };

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
        Sidebar m_sidebar;
        Config m_config;
        std::string m_globalConfigPath;  // Path for global (non-vault) config

        // Open tabs
        std::vector<std::unique_ptr<Tab>> m_tabs;
        int m_activeTab = -1;

        // Modal states
        bool m_darkMode = true;
        bool m_showSettings = false;
        bool m_showRename = false;
        bool m_showCommandPalette = false;
        std::shared_ptr<Note> m_noteToRename;
        std::shared_ptr<Note> m_noteToDelete;

        // Input buffers
        char m_renameBuffer[256] = {};
        char m_newVaultBuffer[512] = {};
        char m_searchBuffer[256] = {};
        char m_commandBuf[256] = {};

        // Splitter
        float m_sidebarWidth = 260.0f;

        void DrawAppLayout();
        void DrawTabBar();
        void DrawSettingsModal();
        void DrawRenameModal();
        void DrawCommandPalette();
        void DrawOpenVaultModal();

        void OpenNote(const std::shared_ptr<Note>& note);
        void CloseTab(int index);
        void HandleWikiLinkNav();

        void SaveAllNotes();
        void SaveConfig();
        void LoadConfig();

        // Sidebar callbacks
        SidebarCallbacks MakeSidebarCallbacks();
    };
}
