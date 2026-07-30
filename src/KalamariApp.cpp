#include "KalamariApp.hpp"

#include "Markdown.hpp"
#include "Theme.hpp"
#include "UI.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include <SDL3/SDL.h>
#include <sentry.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace Kalamari
{
    namespace Layout
    {
        constexpr Uint32 AUTO_SAVE_MS = 30000;
        constexpr Sint32 EVENT_TIMEOUT_MS = 100;
        constexpr float SIDEBAR_MIN = 180;
        constexpr float SIDEBAR_MAX = 500;
        constexpr float TOPBAR_H = 52.0f;
        constexpr float TOPBAR_ITEM_SPACING = 8.0f;
        constexpr float SPLITTER_W = 6.0f;
    }

    namespace
    {
        void AddBreadcrumb(const char* cat, const char* msg)
        {
            sentry_value_t c = sentry_value_new_breadcrumb(nullptr, msg);
            sentry_value_set_by_key(c, "category", sentry_value_new_string(cat));
            sentry_value_set_by_key(c, "level", sentry_value_new_string("info"));
            sentry_add_breadcrumb(c);
        }
    }

    // =========================================================================
    // Init / Shutdown
    // =========================================================================
    bool KalamariApp::Init()
    {
        sentry_options_t* opts = sentry_options_new();
        sentry_options_set_dsn(opts,
            "https://d93df2fd5b1f23837e7fde7246198213@o4511748121886720.ingest.us.sentry.io/4511748130078720");
        sentry_options_set_database_path(opts, ".sentry-native");
#ifdef KALAMARI_VERSION_SHA
        sentry_options_set_release(opts, "kalamari@" KALAMARI_VERSION_SHA);
#else
        sentry_options_set_release(opts, "kalamari@1.0.0");
#endif
        sentry_options_set_debug(opts, 0);
        sentry_options_set_enable_logs(opts, 1);
        sentry_init(opts);

        {
            const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
            std::string configDir = std::string(docs ? docs : ".") + "/kalamari";
            m_globalConfigPath = configDir + "/.config.ini";
            m_config = Config::LoadFromPath(m_globalConfigPath);
        }
        m_darkMode = m_config.darkMode;
        m_sidebarWidth = m_config.sidebarWidth;

        if (!m_renderer.Init(m_config.windowW, m_config.windowH))
            return false;

        Theme::Apply(m_darkMode);

        if (!m_config.lastVaultPath.empty() && std::filesystem::exists(m_config.lastVaultPath))
        {
            m_vault.OpenVault(m_config.lastVaultPath);
            m_config = Config::Load(m_config.lastVaultPath);
            m_darkMode = m_config.darkMode;
            Theme::Apply(m_darkMode);
        }

        return true;
    }

    void KalamariApp::Shutdown()
    {
        SaveAllNotes();
        SaveConfig();
        m_renderer.Shutdown();
        sentry_close();
    }

    bool KalamariApp::SaveAllNotes()
    {
        bool allSaved = true;
        if (m_vault.IsOpen())
        {
            for (auto& tab : m_tabs)
            {
                if (tab->note && tab->note->dirty)
                {
                    if (!m_vault.SaveNote(tab->note))
                        allSaved = false;
                }
            }
        }
        return allSaved;
    }

    void KalamariApp::SaveConfig()
    {
        m_config.darkMode = m_darkMode;
        m_config.sidebarWidth = m_sidebarWidth;
        m_config.lastVaultPath = m_vault.IsOpen() ? m_vault.GetVaultPath().string() : "";

        int w, h;
        SDL_GetWindowSize(m_renderer.GetWindow(), &w, &h);
        m_config.windowW = w;
        m_config.windowH = h;

        m_config.SaveToPath(m_globalConfigPath);
        if (m_vault.IsOpen())
            m_config.Save(m_vault.GetVaultPath().string());
    }

    void KalamariApp::LoadConfig()
    {
        if (m_vault.IsOpen())
            m_config = Config::Load(m_vault.GetVaultPath().string());
    }

    // =========================================================================
    // Run loop
    // =========================================================================
    void KalamariApp::Run()
    {
        bool done = false;
        Uint64 lastSave = SDL_GetTicks();

        while (!done)
        {
            SDL_Event evt;
            bool hasEvent = SDL_WaitEventTimeout(&evt, Layout::EVENT_TIMEOUT_MS);

            if (hasEvent)
            {
                do {
                    ImGui_ImplSDL3_ProcessEvent(&evt);
                    if (evt.type == SDL_EVENT_QUIT) done = true;
                    if (evt.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                        evt.window.windowID == SDL_GetWindowID(m_renderer.GetWindow()))
                        done = true;
                } while (SDL_PollEvent(&evt));
            }

            if (done) break;

            if (SDL_GetWindowFlags(m_renderer.GetWindow()) & SDL_WINDOW_MINIMIZED)
                continue;

            m_renderer.NewFrame();

            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
            ImGui::Begin("##App", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::PopStyleVar(2);

            if (!m_vault.IsOpen())
            {
                DrawOpenVaultModal();
            }
            else
            {
                DrawTopBar();
                DrawAppLayout();
            }

            ImGui::End();

            // Global keyboard shortcuts
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_P) && m_vault.IsOpen())
                m_showCommandPalette = true;
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_N) && m_vault.IsOpen())
            {
                auto n = m_vault.CreateNote();
                if (n) { AddBreadcrumb("note.create", n->fileName.c_str()); OpenNote(n); }
            }
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_W) && m_vault.IsOpen())
            {
                if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size()))
                    static_cast<void>(CloseTab(m_activeTab));
            }
            if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Tab) && m_tabs.size() > 1)
            {
                if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
                    m_activeTab = (m_activeTab - 1 + static_cast<int>(m_tabs.size())) % static_cast<int>(m_tabs.size());
                else
                    m_activeTab = (m_activeTab + 1) % static_cast<int>(m_tabs.size());
            }

            DrawSettingsModal();
            DrawRenameModal();
            DrawCommandPalette();

            if (m_noteToDelete)
            {
                for (int i = static_cast<int>(m_tabs.size()) - 1; i >= 0; --i)
                {
                    if (m_tabs[i]->note == m_noteToDelete)
                        CloseTab(i);
                }
                if (m_vault.DeleteNote(m_noteToDelete))
                    AddBreadcrumb("note.delete", m_noteToDelete->fileName.c_str());
                m_noteToDelete.reset();
            }

            Uint64 now = SDL_GetTicks();
            if (now - lastSave >= Layout::AUTO_SAVE_MS)
            {
                SaveAllNotes();
                lastSave = now;
            }

            m_renderer.Render(Theme::GetClearColor(m_darkMode));
        }
    }

    // =========================================================================
    // Top app bar
    // =========================================================================
    void KalamariApp::DrawTopBar()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
        ImGui::BeginChild("TopBar", ImVec2(-1, Layout::TOPBAR_H), ImGuiChildFlags_None);

        UI::PushHeadingFont();
        ImGui::TextColored(Theme::ACCENT_COLOR, "Kalamari");
        UI::PopHeadingFont();

        ImGui::SameLine();
        ImGui::TextDisabled("|  %s", m_vault.GetVaultName().c_str());

        const char* themeLabel = m_darkMode ? "Light" : "Dark";
        const float groupW = UI::ButtonWidth("New Note")
                           + UI::ButtonWidth("Command Palette")
                           + UI::ButtonWidth(themeLabel)
                           + Layout::TOPBAR_ITEM_SPACING * 2.0f;

        ImGui::SameLine();
        float rightEdge = ImGui::GetWindowContentRegionMax().x;
        float x = rightEdge - groupW;
        if (x > ImGui::GetCursorPosX())
            ImGui::SetCursorPosX(x);

        if (UI::ButtonPrimary("New Note", ImVec2(0, 0)))
        {
            auto n = m_vault.CreateNote();
            if (n) { AddBreadcrumb("note.create", n->fileName.c_str()); OpenNote(n); }
        }
        ImGui::SameLine(0, Layout::TOPBAR_ITEM_SPACING);
        if (UI::ButtonSecondary("Command Palette", ImVec2(0, 0)))
            m_showCommandPalette = true;
        ImGui::SameLine(0, Layout::TOPBAR_ITEM_SPACING);
        if (UI::ButtonSecondary(themeLabel, ImVec2(0, 0)))
        {
            m_darkMode = !m_darkMode;
            Theme::Apply(m_darkMode);
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    // =========================================================================
    // App layout
    // =========================================================================
    void KalamariApp::DrawAppLayout()
    {
        ImGuiIO& io = ImGui::GetIO();

        // ---- Sidebar ----
        SidebarCallbacks cbs = MakeSidebarCallbacks();
        m_sidebar.Draw(m_sidebarWidth, m_vault,
                       m_activeTab >= 0 ? m_tabs[m_activeTab]->note : nullptr,
                       cbs, m_searchBuffer, sizeof(m_searchBuffer));

        // ---- Splitter ----
        ImGui::SameLine(0, 0);
        ImGui::InvisibleButton("##Splitter", ImVec2(Layout::SPLITTER_W, -1));
        if (ImGui::IsItemActive())
        {
            m_sidebarWidth += io.MouseDelta.x;
            m_sidebarWidth = (std::max)(Layout::SIDEBAR_MIN, (std::min)(m_sidebarWidth, Layout::SIDEBAR_MAX));
        }
        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        {
            ImVec2 sMin = ImGui::GetItemRectMin();
            ImVec2 sMax = ImGui::GetItemRectMax();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec4 sepCol = Theme::BORDER_COLOR;
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
                sepCol = Theme::ACCENT_COLOR;
            dl->AddLine(
                ImVec2(sMin.x + Layout::SPLITTER_W * 0.5f, sMin.y),
                ImVec2(sMin.x + Layout::SPLITTER_W * 0.5f, sMax.y),
                ImGui::GetColorU32(sepCol), 1.0f);
        }

        // ---- Main area ----
        ImGui::SameLine(0, 0);
        ImGui::BeginChild("MainArea", ImVec2(0, 0), ImGuiChildFlags_None);

        DrawTabBar();

        ImGui::BeginChild("EditorContainer", ImVec2(0, 0), ImGuiChildFlags_None);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24, 16));
        ImGui::BeginChild("EditorInner", ImVec2(0, 0), ImGuiChildFlags_None);

        if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size()))
        {
            auto& tab = m_tabs[m_activeTab];
            tab->editor.Draw(tab->note);
            HandleWikiLinkNav();
        }
        else
        {
            UI::EmptyState("Kalamari", "Open a note or create a new one", "Ctrl+N  |  Ctrl+P");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::EndChild();
    }

    void KalamariApp::DrawTabBar()
    {
        if (ImGui::BeginTabBar("Tabs", ImGuiTabBarFlags_AutoSelectNewTabs |
                                       ImGuiTabBarFlags_Reorderable |
                                       ImGuiTabBarFlags_FittingPolicyScroll))
        {
            for (int i = 0; i < static_cast<int>(m_tabs.size()); )
            {
                bool open = true;
                std::string title = m_tabs[i]->note->path.stem().string();
                if (title.size() > 25) title = title.substr(0, 22) + "...";

                ImGuiTabItemFlags flags = 0;
                if (m_tabs[i]->note->dirty) flags |= ImGuiTabItemFlags_UnsavedDocument;

                if (ImGui::BeginTabItem(title.c_str(), &open, flags))
                {
                    m_activeTab = i;
                    ImGui::EndTabItem();
                }

                if (!open)
                {
                    if (!CloseTab(i))
                        ++i; // keep tab open if save failed
                }
                else
                    ++i;
            }
            ImGui::EndTabBar();
        }
    }

    void KalamariApp::OpenNote(const std::shared_ptr<Note>& note)
    {
        if (!note) return;

        for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i)
        {
            if (m_tabs[i]->note == note)
            {
                m_activeTab = i;
                return;
            }
        }

        auto tab = std::make_unique<Tab>();
        tab->note = note;
        m_tabs.push_back(std::move(tab));
        m_activeTab = static_cast<int>(m_tabs.size()) - 1;
        AddBreadcrumb("note.open", note->fileName.c_str());
    }

    bool KalamariApp::CloseTab(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_tabs.size())) return false;
        if (m_tabs[index]->note->dirty)
        {
            if (!m_vault.SaveNote(m_tabs[index]->note))
            {
                AddBreadcrumb("note.save_failed", m_tabs[index]->note->fileName.c_str());
                return false;
            }
        }
        m_tabs.erase(m_tabs.begin() + index);
        if (m_activeTab >= static_cast<int>(m_tabs.size()))
            m_activeTab = static_cast<int>(m_tabs.size()) - 1;
        return true;
    }

    void KalamariApp::HandleWikiLinkNav()
    {
        if (m_activeTab < 0 || m_activeTab >= static_cast<int>(m_tabs.size())) return;
        const std::string& target = m_tabs[m_activeTab]->editor.GetWikiLinkTarget();
        if (target.empty()) return;

        auto linked = m_vault.FindOrCreateNote(target);
        if (linked)
        {
            OpenNote(linked);
            AddBreadcrumb("note.wikilink", target.c_str());
        }
        m_tabs[m_activeTab]->editor.ClearWikiLinkTarget();
    }

    // =========================================================================
    // Modals
    // =========================================================================
    void KalamariApp::DrawSettingsModal()
    {
        if (m_showSettings)
        {
            ImGui::OpenPopup("Settings");
            m_showSettings = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        UI::PushCardStyle();
        if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::PushHeadingFont();
            ImGui::Text("Settings");
            UI::PopHeadingFont();
            ImGui::Spacing();
            UI::DrawDivider(6.0f);

            // Appearance
            ImGui::Text("Appearance");
            if (UI::ButtonSecondary(m_darkMode ? "Switch to Light Theme" : "Switch to Dark Theme", ImVec2(-1, 0)))
            {
                m_darkMode = !m_darkMode;
                Theme::Apply(m_darkMode);
            }

            ImGui::Spacing();
            UI::DrawDivider(6.0f);

            // Vault
            ImGui::Text("Vault");
            ImGui::TextDisabled("%s", m_vault.GetVaultPath().string().c_str());

            if (UI::ButtonSecondary("Switch Vault...", ImVec2(-1, 0)))
            {
                SaveAllNotes();
                SaveConfig();
                m_tabs.clear();
                m_activeTab = -1;
                m_vault.CloseVault();
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                UI::PopCardStyle();
                return;
            }

            ImGui::Spacing();
            UI::DrawDivider(6.0f);

            // About
            ImGui::Text("About");
            ImGui::TextDisabled("Kalamari v1.0.0");
            ImGui::TextDisabled("A native markdown notebook");

            ImGui::Spacing();
            if (UI::ButtonPrimary("Close", ImVec2(-1, 0)))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        UI::PopCardStyle();
    }

    void KalamariApp::DrawRenameModal()
    {
        if (m_noteToRename && !m_showRename)
        {
            m_showRename = true;
            std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s",
                          m_noteToRename->fileName.c_str());
            ImGui::OpenPopup("Rename");
        }

        UI::PushCardStyle();
        if (ImGui::BeginPopupModal("Rename", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::PushHeadingFont();
            ImGui::Text("Rename Note");
            UI::PopHeadingFont();
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(Theme::SURFACE_COLOR));
            ImGui::SetNextItemWidth(320.0f);
            ImGui::InputText("##RenameBuf", m_renameBuffer, sizeof(m_renameBuffer));
            ImGui::PopStyleColor();
            ImGui::SetItemDefaultFocus();

            ImGui::Spacing();
            if (UI::ButtonPrimary("OK", ImVec2(120, 0)))
            {
                std::string name(m_renameBuffer);
                if (!name.empty() && m_noteToRename)
                {
                    if (m_vault.RenameNote(m_noteToRename, name))
                        AddBreadcrumb("note.rename", name.c_str());
                }
                m_noteToRename.reset();
                m_showRename = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (UI::ButtonSecondary("Cancel", ImVec2(120, 0)))
            {
                m_noteToRename.reset();
                m_showRename = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        UI::PopCardStyle();
    }

    void KalamariApp::DrawCommandPalette()
    {
        if (m_showCommandPalette)
        {
            ImGui::OpenPopup("##CmdPalette");
            m_showCommandPalette = false;
            std::memset(m_commandBuf, 0, sizeof(m_commandBuf));
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.3f));
        ImGui::SetNextWindowSize(ImVec2(500, 0));

        UI::PushCardStyle();
        if (ImGui::BeginPopup("##CmdPalette", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            UI::PushHeadingFont();
            ImGui::Text("Command Palette");
            UI::PopHeadingFont();
            ImGui::Spacing();

            UI::SearchInput("##CmdInput", m_commandBuf, sizeof(m_commandBuf), "Type to search notes or actions...", -1.0f);

            if (ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                UI::PopCardStyle();
                return;
            }

            std::string q(m_commandBuf);
            std::vector<std::shared_ptr<Note>> results;
            if (!q.empty())
                results = m_vault.Search(q);

            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !q.empty() && !results.empty())
            {
                OpenNote(results[0]);
                ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
                UI::PopCardStyle();
                return;
            }

            ImGui::Spacing();
            ImGui::BeginChild("CmdResults", ImVec2(0, 320), ImGuiChildFlags_None);

            if (q.empty())
            {
                if (ImGui::Selectable("New Note"))
                {
                    auto n = m_vault.CreateNote();
                    if (n) { AddBreadcrumb("note.create", n->fileName.c_str()); OpenNote(n); }
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Selectable(m_darkMode ? "Switch to Light Theme" : "Switch to Dark Theme"))
                {
                    m_darkMode = !m_darkMode;
                    Theme::Apply(m_darkMode);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Selectable("Open Vault..."))
                {
                    SaveAllNotes();
                    SaveConfig();
                    m_tabs.clear();
                    m_activeTab = -1;
                    m_vault.CloseVault();
                    ImGui::CloseCurrentPopup();
                }
                UI::DrawDivider(4.0f);
                ImGui::TextDisabled("  Type to search notes...");
            }
            else
            {
                for (const auto& n : results)
                {
                    std::string display = n->relativePath;
                    if (display.size() > 3 && display.substr(display.size() - 3) == ".md")
                        display = display.substr(0, display.size() - 3);
                    if (ImGui::Selectable(display.c_str()))
                    {
                        OpenNote(n);
                        ImGui::CloseCurrentPopup();
                    }
                }
                if (results.empty())
                    ImGui::TextDisabled("  No results");
            }

            ImGui::EndChild();
            ImGui::EndPopup();
        }
        UI::PopCardStyle();
    }

    void KalamariApp::DrawOpenVaultModal()
    {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        UI::PushCardStyle();
        if (ImGui::Begin("Open Vault", nullptr,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_AlwaysAutoResize))
        {
            UI::PushHeadingFont();
            ImGui::TextColored(Theme::ACCENT_COLOR, "Welcome to Kalamari");
            UI::PopHeadingFont();
            ImGui::Spacing();

            ImGui::TextWrapped("Enter a name for your vault. It will be created in your Documents folder.");
            ImGui::Spacing();
            UI::DrawDivider(6.0f);
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImGui::GetColorU32(Theme::SURFACE_COLOR));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputTextWithHint("##VaultName", "My Vault", m_newVaultBuffer, sizeof(m_newVaultBuffer));
            ImGui::PopStyleColor();

            if (std::strlen(m_newVaultBuffer) > 0)
            {
                const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
                std::string checkPath = (docs ? docs : ".") + std::string("/kalamari/") + m_newVaultBuffer;
                if (std::filesystem::exists(checkPath))
                    ImGui::TextDisabled("Vault already exists — will open it");
            }

            ImGui::Spacing();
            if (UI::ButtonPrimary("Create & Open", ImVec2(-1, 0)))
            {
                std::string name(m_newVaultBuffer);
                if (!name.empty())
                {
                    const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
                    std::string fullPath = (docs ? docs : ".") + std::string("/kalamari/") + name;

                    if (m_vault.OpenVault(fullPath))
                    {
                        m_config.lastVaultPath = fullPath;
                        LoadConfig();
                        m_darkMode = m_config.darkMode;
                        m_sidebarWidth = m_config.sidebarWidth;
                        Theme::Apply(m_darkMode);
                    }
                }
            }

            if (!m_config.lastVaultPath.empty())
            {
                ImGui::Spacing();
                UI::DrawDivider(6.0f);
                ImGui::Spacing();
                ImGui::TextDisabled("Recent vault:");
                if (ImGui::Selectable(m_config.lastVaultPath.c_str()))
                {
                    if (m_vault.OpenVault(m_config.lastVaultPath))
                    {
                        LoadConfig();
                        m_darkMode = m_config.darkMode;
                        m_sidebarWidth = m_config.sidebarWidth;
                        Theme::Apply(m_darkMode);
                    }
                }
            }

            ImGui::End();
        }
        UI::PopCardStyle();
    }

    // =========================================================================
    // Sidebar callbacks
    // =========================================================================
    SidebarCallbacks KalamariApp::MakeSidebarCallbacks()
    {
        SidebarCallbacks cbs;
        cbs.onSelectNote = [this](std::shared_ptr<Note> n) {
            OpenNote(n);
        };
        cbs.onCreateNote = [this]() {
            auto n = m_vault.CreateNote();
            if (n) { AddBreadcrumb("note.create", n->fileName.c_str()); OpenNote(n); }
        };
        cbs.onCreateNoteInFolder = [this](const std::string& folder) {
            auto n = m_vault.CreateNote(folder);
            if (n) { AddBreadcrumb("note.create", n->fileName.c_str()); OpenNote(n); }
        };
        cbs.onRenameNote = [this](std::shared_ptr<Note> n) {
            m_noteToRename = n;
            m_showRename = false;
        };
        cbs.onDeleteNote = [this](std::shared_ptr<Note> n) {
            m_noteToDelete = n;
        };
        cbs.onOpenSettings = [this]() {
            m_showSettings = true;
        };
        cbs.onSwitchVault = [this](const std::filesystem::path&) {
            SaveAllNotes();
            SaveConfig();
            m_tabs.clear();
            m_activeTab = -1;
            m_vault.CloseVault();
        };
        return cbs;
    }
}
