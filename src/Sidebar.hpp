#pragma once

#include "Vault.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Kalamari
{
    struct SidebarCallbacks
    {
        std::function<void(std::shared_ptr<Note>)> onSelectNote;
        std::function<void()> onCreateNote;
        std::function<void(const std::string&)> onCreateNoteInFolder;
        std::function<void(std::shared_ptr<Note>)> onRenameNote;
        std::function<void(std::shared_ptr<Note>)> onDeleteNote;
        std::function<void()> onOpenSettings;
        std::function<void(const std::filesystem::path&)> onSwitchVault;
    };

    class Sidebar
    {
    public:
        Sidebar() = default;

        void Draw(float width, Vault& vault, const std::shared_ptr<Note>& currentNote,
                  const SidebarCallbacks& cbs, char* searchBuffer, int searchBufferSize);

    private:
        void DrawFileTree(Vault& vault, const std::shared_ptr<Note>& currentNote,
                          const SidebarCallbacks& cbs,
                          const std::vector<std::shared_ptr<VaultEntry>>& entries,
                          const std::string& filter, int depth = 0);
        void CountEntries(const std::vector<std::shared_ptr<VaultEntry>>& entries,
                          int& noteCount, int& folderCount) const;
    };
}
