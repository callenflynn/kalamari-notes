#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace Kalamari
{
    struct Note
    {
        std::filesystem::path path;      // Full path on disk
        std::string fileName;            // "note.md"
        std::string relativePath;        // "folder/note.md" (from vault root, for tree display)
        std::string content;
        bool dirty = false;
    };

    // A node in the vault file tree
    struct VaultEntry
    {
        std::string name;                           // Display name
        std::string relativePath;                   // Relative path from vault root
        bool isDirectory = false;
        bool isOpen = false;                        // Tree open state (persistent across frames)
        std::vector<std::shared_ptr<VaultEntry>> children;
    };

    class Vault
    {
    public:
        Vault() = default;
        ~Vault() = default;

        // Open a vault at an arbitrary path. Creates it if it doesn't exist.
        bool OpenVault(const std::filesystem::path& vaultPath);
        void CloseVault();

        const std::string& GetVaultName() const { return m_vaultName; }
        const std::filesystem::path& GetVaultPath() const { return m_vaultPath; }
        bool IsOpen() const { return m_isOpen; }

        // Scan the vault directory (recursive) and rebuild note list + file tree
        void Refresh();

        // Notes
        const std::vector<std::shared_ptr<Note>>& GetNotes() const { return m_notes; }

        // File tree for sidebar
        const std::vector<std::shared_ptr<VaultEntry>>& GetFileTree() const { return m_fileTree; }

        // CRUD
        std::shared_ptr<Note> CreateNote(const std::string& relativeDir = "");
        bool RenameNote(const std::shared_ptr<Note>& note, const std::string& newName);
        bool DeleteNote(const std::shared_ptr<Note>& note);
        void SaveNote(const std::shared_ptr<Note>& note);

        // Search
        std::vector<std::shared_ptr<Note>> Search(const std::string& query) const;

        // Wiki-link support
        std::shared_ptr<Note> FindNote(const std::string& name) const;
        std::shared_ptr<Note> FindNoteByPath(const std::string& relativePath) const;
        std::shared_ptr<Note> FindOrCreateNote(const std::string& name);

        // Config directory
        std::filesystem::path GetObsidianPath() const;

    private:
        std::string m_vaultName;
        std::filesystem::path m_vaultPath;
        bool m_isOpen = false;
        std::vector<std::shared_ptr<Note>> m_notes;
        std::vector<std::shared_ptr<VaultEntry>> m_fileTree;

        void EnsureDirectory(const std::filesystem::path& dir) const;
        void BuildFileTree();
        void SortTree(std::vector<std::shared_ptr<VaultEntry>>& entries);

        static bool AtomicReplaceFile(const std::filesystem::path& from,
                                      const std::filesystem::path& to);
        static std::string GenerateNoteFilename();
        static std::string SanitizeFileName(const std::string& name);
    };
}
