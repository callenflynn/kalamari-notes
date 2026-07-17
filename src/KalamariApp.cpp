#include "KalamariApp.hpp"

#include "Theme.hpp"
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
    namespace
    {
        constexpr Uint32 AUTO_SAVE_INTERVAL_MS = 30000;

        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }
    }

    bool KalamariApp::Init()
    {
        // Sentry crash monitoring
        {
            sentry_options_t* options = sentry_options_new();
            sentry_options_set_dsn(options,
                "https://d93df2fd5b1f23837e7fde7246198213@o4511748121886720.ingest.us.sentry.io/4511748130078720");
            sentry_options_set_database_path(options, ".sentry-native");
#ifdef KALAMARI_VERSION_SHA
            sentry_options_set_release(options, "kalamari@" KALAMARI_VERSION_SHA);
#else
            sentry_options_set_release(options, "kalamari@1.0.0");
#endif
            sentry_options_set_debug(options, 0);
            sentry_options_set_enable_logs(options, 1);
            sentry_init(options);
        }

        if (!m_renderer.Init())
        {
            return false;
        }

        Theme::Apply(m_darkMode);

        // Load font from executable-relative path
        m_renderer.LoadFont("assets/Kameron/static/Kameron-Regular.ttf", 18.0f);

        m_vault.Refresh();
        const auto& notes = m_vault.GetNotes();
        if (!notes.empty())
        {
            m_currentNote = notes.front();
        }

        return true;
    }

    void KalamariApp::Shutdown()
    {
        if (m_currentNote && m_currentNote->dirty)
        {
            m_vault.SaveNote(m_currentNote);
        }

        m_renderer.Shutdown();
        sentry_close();
    }

    void KalamariApp::Run()
    {
        bool done = false;
        Uint64 lastAutoSaveTicks = SDL_GetTicks();

        while (!done)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
                if (event.type == SDL_EVENT_QUIT)
                    done = true;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                    && event.window.windowID == SDL_GetWindowID(m_renderer.GetWindow()))
                    done = true;
            }

            if (SDL_GetWindowFlags(m_renderer.GetWindow()) & SDL_WINDOW_MINIMIZED)
            {
                SDL_Delay(10);
                continue;
            }

            m_renderer.NewFrame();

            ImGuiIO& io = ImGui::GetIO();
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::Begin("##KalamariApp", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
                | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
                | ImGuiWindowFlags_NoBringToFrontOnFocus
                | ImGuiWindowFlags_NoScrollWithMouse);
            ImGui::PopStyleVar();

            DrawSidebar();
            ImGui::SameLine();
            DrawMainArea();

            ImGui::End();

            DrawSettingsModal();
            DrawRenameModal();
            ProcessDeferredOperations();

            Uint64 currentTicks = SDL_GetTicks();
            if (m_currentNote && m_currentNote->dirty &&
                currentTicks - lastAutoSaveTicks >= AUTO_SAVE_INTERVAL_MS)
            {
                m_vault.SaveNote(m_currentNote);
                lastAutoSaveTicks = currentTicks;
            }

            m_renderer.Render(Theme::GetClearColor(m_darkMode));
        }
    }

    void KalamariApp::DrawSidebar()
    {
        ImGuiStyle& style = ImGui::GetStyle();
        float sidebarWidth = 260.0f * m_renderer.GetScale();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, m_darkMode ? Theme::DARK_FRAME_BG : Theme::LIGHT_FRAME_BG);
        ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), ImGuiChildFlags_Borders);

        ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize("Kalamari").x) * 0.5f);
        ImGui::TextColored(Theme::ACCENT_COLOR, "Kalamari");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("+ New Note", ImVec2(-1, 0)))
        {
            if (m_currentNote && m_currentNote->dirty)
            {
                m_vault.SaveNote(m_currentNote);
            }
            m_currentNote = m_vault.CreateNote();
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Vault: %s", m_vault.GetVaultName().c_str());
        ImGui::Spacing();
        ImGui::TextDisabled("Notes");
        ImGui::Spacing();            ImGui::PushStyleColor(ImGuiCol_FrameBg, m_darkMode ? Theme::DARK_BG : Theme::LIGHT_BG);
        ImGui::InputTextWithHint("##Search", "Search notes...", m_searchBuffer, sizeof(m_searchBuffer));
        ImGui::PopStyleColor();
        ImGui::Spacing();

        ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 8.0f));

        std::string query(m_searchBuffer);
        auto notes = query.empty() ? m_vault.GetNotes() : m_vault.Search(query);

        for (const auto& note : notes)
        {
            bool isSelected = (note == m_currentNote);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, Theme::ACCENT_COLOR);
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Theme::ACCENT_COLOR_HDR);
            }

            if (ImGui::Selectable(note->fileName.c_str(), isSelected))
            {
                if (m_currentNote && m_currentNote->dirty)
                {
                    m_vault.SaveNote(m_currentNote);
                }
                m_currentNote = note;
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
            }

            if (ImGui::BeginPopupContextItem(note->fileName.c_str()))
            {
                if (ImGui::Selectable("Rename"))
                {
                    m_noteToRename = note;
                    std::snprintf(m_renameBuffer, sizeof(m_renameBuffer), "%s", note->fileName.c_str());
                }
                if (ImGui::Selectable("Delete"))
                {
                    m_noteToDelete = note;
                }
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();

        if (ImGui::Button("Settings", ImVec2(-1, 0)))
        {
            m_showSettings = true;
            std::snprintf(m_vaultSwitchBuffer, sizeof(m_vaultSwitchBuffer), "%s", m_vault.GetVaultName().c_str());
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    void KalamariApp::DrawMainArea()
    {
        ImGui::BeginChild("EditorContainer", ImVec2(0, 0), ImGuiChildFlags_None);
        m_editor.Draw(m_currentNote);
        ImGui::EndChild();
    }

    void KalamariApp::DrawSettingsModal()
    {
        if (m_showSettings)
        {
            ImGui::OpenPopup("Settings");
            m_showSettings = false;
        }

        if (ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Appearance");
            ImGui::Separator();

            if (ImGui::Button(m_darkMode ? "Switch to Light Mode" : "Switch to Dark Mode", ImVec2(-1, 0)))
            {
                m_darkMode = !m_darkMode;
                Theme::Apply(m_darkMode);
            }

            ImGui::Spacing();
            ImGui::Text("Vault");
            ImGui::Separator();

            ImGui::InputText("Vault name", m_vaultSwitchBuffer, sizeof(m_vaultSwitchBuffer));
            if (ImGui::Button("Switch Vault", ImVec2(120, 0)))
            {
                std::string newVaultName(m_vaultSwitchBuffer);
                if (!newVaultName.empty() && newVaultName != m_vault.GetVaultName())
                {
                    if (m_currentNote && m_currentNote->dirty)
                    {
                        m_vault.SaveNote(m_currentNote);
                    }

                    m_vault.SetVault(newVaultName);
                    m_currentNote.reset();
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Close", ImVec2(120, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    void KalamariApp::DrawRenameModal()
    {
        if (m_noteToRename)
        {
            ImGui::OpenPopup("Rename Note");
        }

        bool open = m_noteToRename != nullptr;
        if (ImGui::BeginPopupModal("Rename Note", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("New name:");
            ImGui::InputText("##NewName", m_renameBuffer, sizeof(m_renameBuffer));
            if (ImGui::Button("OK", ImVec2(120, 0)))
            {
                std::string newFileName(m_renameBuffer);
                if (!newFileName.empty() && m_noteToRename)
                {
                    if (m_currentNote == m_noteToRename && m_currentNote->dirty)
                    {
                        m_vault.SaveNote(m_currentNote);
                    }
                    m_vault.RenameNote(m_noteToRename, newFileName);
                }
                m_noteToRename.reset();
                std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0)))
            {
                m_noteToRename.reset();
                std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        else if (open)
        {
            // Popup failed to open; clear stale state
            m_noteToRename.reset();
            std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
        }
    }

    void KalamariApp::ProcessDeferredOperations()
    {
        if (m_noteToDelete)
        {
            if (m_currentNote == m_noteToDelete && m_currentNote->dirty)
            {
                m_vault.SaveNote(m_currentNote);
            }
            m_vault.DeleteNote(m_noteToDelete);
            if (m_currentNote == m_noteToDelete)
            {
                m_currentNote.reset();
            }
            m_noteToDelete.reset();
        }
    }
}
