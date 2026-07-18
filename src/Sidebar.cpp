#include "Sidebar.hpp"

#include "Theme.hpp"
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
        ImGuiStyle& style = ImGui::GetStyle();
        bool isDark = (style.Colors[ImGuiCol_WindowBg].x < 0.5f);

        ImGui::PushStyleColor(ImGuiCol_ChildBg, isDark ? Theme::DARK_FRAME_BG : Theme::LIGHT_FRAME_BG);
        ImGui::BeginChild("Sidebar", ImVec2(width, 0), ImGuiChildFlags_Borders);

        float availW = ImGui::GetContentRegionAvail().x;

        // ---- App title ----
        ImGui::SetCursorPosX((availW - ImGui::CalcTextSize("Kalamari").x) * 0.5f);
        ImGui::TextColored(Theme::ACCENT_COLOR, "Kalamari");
        ImGui::Spacing();

        // ---- Vault info ----
        std::string vaultName = vault.GetVaultName();
        if (!vaultName.empty())
        {
            ImGui::SetCursorPosX((availW - ImGui::CalcTextSize(vaultName.c_str()).x) * 0.5f);
            ImGui::TextDisabled("%s", vaultName.c_str());
        }
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ---- New Note button ----
        if (ImGui::Button("+ New Note", ImVec2(-1, 0)))
            cbs.onCreateNote();
        ImGui::Spacing();

        // ---- Search ----
        ImGui::PushStyleColor(ImGuiCol_FrameBg, isDark ? Theme::DARK_BG : Theme::LIGHT_BG);
        ImGui::InputTextWithHint("##Search", "Search notes...", searchBuffer, searchBufferSize);
        ImGui::PopStyleColor();
        ImGui::Spacing();

        // ---- File tree ----
        ImGui::BeginChild("FileTree", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 24));

        int noteCount = 0;
        int folderCount = 0;
        CountEntries(vault.GetFileTree(), noteCount, folderCount);

        std::string filter(searchBuffer);
        DrawFileTree(vault, currentNote, cbs, vault.GetFileTree(), filter);
        ImGui::EndChild();

        // ---- Bottom bar: note count + settings ----
        ImGui::Separator();
        ImGui::TextDisabled("%d notes, %d folders", noteCount, folderCount);
        ImGui::SameLine();
        float settingsW = ImGui::CalcTextSize("Settings").x + 16;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - settingsW);
        if (ImGui::SmallButton("Settings"))
            cbs.onOpenSettings();

        ImGui::EndChild();
        ImGui::PopStyleColor();
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
                    // Check children recursively for a match
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
                                           ImGuiTreeNodeFlags_SpanAvailWidth;
                if (entry->children.empty()) flags |= ImGuiTreeNodeFlags_Leaf;
                if (hasMatch && !filter.empty()) flags |= ImGuiTreeNodeFlags_DefaultOpen;

                bool open = ImGui::TreeNodeEx(entry->name.c_str(), flags);

                if (open)
                {
                    DrawFileTree(vault, currentNote, cbs, entry->children, filter, depth + 1);
                    ImGui::TreePop();
                }

                // Folder context menu
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::Selectable("New Note Here"))
                    {
                        cbs.onCreateNoteInFolder(entry->relativePath);
                    }
                    ImGui::EndPopup();
                }
            }
            else
            {
                // Filter non-matching files
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
                                           ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

                ImGui::PushID(entry->relativePath.c_str());

                std::string display = entry->name;
                if (display.size() > 3 && display.substr(display.size() - 3) == ".md")
                    display = display.substr(0, display.size() - 3);
                if (display.size() > 28) display = display.substr(0, 25) + "...";

                if (isSelected)
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

                ImGui::TreeNodeEx(display.c_str(), flags);

                if (isSelected)
                    ImGui::PopStyleColor();

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
