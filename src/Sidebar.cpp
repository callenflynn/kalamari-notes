#include "Sidebar.hpp"

#include "Theme.hpp"
#include "UI.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <functional>

namespace Kalamari
{
    namespace
    {
        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

    }

    void Sidebar::Draw(float width, Vault& vault, const std::shared_ptr<Note>& currentNote,
                       const SidebarCallbacks& cbs, char* searchBuffer, int searchBufferSize)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 14));
        ImGui::BeginChild("Sidebar", ImVec2(width, 0), ImGuiChildFlags_None);

        float availW = ImGui::GetContentRegionAvail().x;

        // ---- App title ----
        UI::PushHeadingFont();
        ImVec2 titleSize = ImGui::CalcTextSize("Kalamari");
        ImGui::SetCursorPosX((availW - titleSize.x) * 0.5f);
        ImGui::TextColored(Theme::ACCENT_COLOR, "Kalamari");
        UI::PopHeadingFont();

        // ---- Vault info ----
        std::string vaultName = vault.GetVaultName();
        if (!vaultName.empty())
        {
            ImGui::SetCursorPosX((availW - ImGui::CalcTextSize(vaultName.c_str()).x) * 0.5f);
            ImGui::TextDisabled("%s", vaultName.c_str());
        }

        ImGui::Dummy(ImVec2(0.0f, 12.0f));

        // ---- New Note ----
        if (UI::ButtonPrimary("+ New Note", ImVec2(-1, 0)))
            cbs.onCreateNote();

        ImGui::Dummy(ImVec2(0.0f, 12.0f));

        // ---- Search ----
        UI::SearchInput("##Search", searchBuffer, searchBufferSize, "Search notes...", -1.0f);

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        // ---- File tree ----
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild("FileTree", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 24), ImGuiChildFlags_None);

        int noteCount = 0;
        int folderCount = 0;
        CountEntries(vault.GetFileTree(), noteCount, folderCount);

        std::string filter(searchBuffer);
        DrawFileTree(vault, currentNote, cbs, vault.GetFileTree(), filter);
        ImGui::EndChild();
        ImGui::PopStyleVar();

        // ---- Footer ----
        ImGui::Dummy(ImVec2(0.0f, 6.0f));
        UI::DrawDivider(6.0f);
        ImGui::TextDisabled("%d notes, %d folders", noteCount, folderCount);
        ImGui::SameLine();
        float settingsW = ImGui::CalcTextSize("Settings").x + 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - settingsW);
        if (UI::ButtonGhost("Settings"))
            cbs.onOpenSettings();

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    void Sidebar::CountEntries(const std::vector<std::shared_ptr<VaultEntry>>& entries,
                               int& noteCount, int& folderCount) const
    {
        for (const auto& entry : entries)
        {
            if (entry->isDirectory)
            {
                ++folderCount;
                CountEntries(entry->children, noteCount, folderCount);
            }
            else
            {
                ++noteCount;
            }
        }
    }

    void Sidebar::DrawFileTree(Vault& vault, const std::shared_ptr<Note>& currentNote,
                               const SidebarCallbacks& cbs,
                               const std::vector<std::shared_ptr<VaultEntry>>& entries,
                               const std::string& filter, int depth)
    {
        for (const auto& entry : entries)
        {
            if (entry->isDirectory)
            {
                // If filtering, check if any child matches
                bool hasMatch = filter.empty();
                if (!hasMatch)
                {
                    std::function<bool(const std::vector<std::shared_ptr<VaultEntry>>&)> anyMatch =
                        [&](const std::vector<std::shared_ptr<VaultEntry>>& e) -> bool {
                            for (const auto& c : e)
                            {
                                if (!c->isDirectory && ToLower(c->name).find(ToLower(filter)) != std::string::npos)
                                    return true;
                                if (c->isDirectory && anyMatch(c->children))
                                    return true;
                            }
                            return false;
                        };
                    hasMatch = anyMatch(entry->children);
                }

                if (!hasMatch) continue;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                                           ImGuiTreeNodeFlags_SpanAvailWidth |
                                           ImGuiTreeNodeFlags_FramePadding;
                if (entry->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
                if (hasMatch && !filter.empty()) flags |= ImGuiTreeNodeFlags_DefaultOpen;

                // Subtle folder row
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(Theme::SURFACE_COLOR));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(Theme::SURFACE_HOVER_COLOR));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetColorU32(Theme::SURFACE_ACTIVE_COLOR));

                bool open = ImGui::TreeNodeEx(entry->name.c_str(), flags);

                ImGui::PopStyleColor(3);

                if (open)
                {
                    DrawFileTree(vault, currentNote, cbs, entry->children, filter, depth + 1);
                    ImGui::TreePop();
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Selectable("New Note Here"))
                        cbs.onCreateNoteInFolder(entry->relativePath);
                    ImGui::EndPopup();
                }
            }
            else
            {
                if (!filter.empty())
                {
                    std::string nameLower = ToLower(entry->name);
                    std::string filterLower = ToLower(filter);
                    if (nameLower.find(filterLower) == std::string::npos)
                        continue;
                }

                bool isSelected = (currentNote && currentNote->relativePath == entry->relativePath);

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                           ImGuiTreeNodeFlags_SpanAvailWidth |
                                           ImGuiTreeNodeFlags_FramePadding |
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (isSelected)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(Theme::ACCENT_COLOR));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(Theme::ACCENT_COLOR_HDR));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetColorU32(Theme::ACCENT_COLOR_ACTIVE));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
                }
                else
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(Theme::SURFACE_COLOR));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::GetColorU32(Theme::SURFACE_HOVER_COLOR));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::GetColorU32(Theme::SURFACE_ACTIVE_COLOR));
                }

                ImGui::PushID(entry->relativePath.c_str());

                std::string display = entry->name;
                if (display.size() > 3 && display.substr(display.size() - 3) == ".md")
                    display = display.substr(0, display.size() - 3);
                if (display.size() > 28) display = display.substr(0, 25) + "...";

                ImGui::TreeNodeEx(display.c_str(), flags);

                if (isSelected)
                    ImGui::PopStyleColor(4);
                else
                    ImGui::PopStyleColor(3);

                if (ImGui::IsItemClicked())
                {
                    auto note = vault.FindNoteByPath(entry->relativePath);
                    if (note) cbs.onSelectNote(note);
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Selectable("Rename"))
                    {
                        auto note = vault.FindNoteByPath(entry->relativePath);
                        if (note) cbs.onRenameNote(note);
                    }
                    if (ImGui::Selectable("Delete"))
                    {
                        auto note = vault.FindNoteByPath(entry->relativePath);
                        if (note) cbs.onDeleteNote(note);
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID();
            }
        }
    }
}
