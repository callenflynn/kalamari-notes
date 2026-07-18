#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>

namespace Kalamari
{
    struct Note
    {
        std::filesystem::path path;
        std::string fileName;
        std::string content;
        bool dirty = false;
    };

    class Vault
    {
    public:
        Vault();
        ~Vault() = default;

        void SetVault(const std::string& vaultName);
        const std::string& GetVaultName() const { return m_vaultName; }
        std::filesystem::path GetVaultPath() const;

        void Refresh();
        const std::vector<std::shared_ptr<Note>>& GetNotes() const { return m_notes; }

        static std::vector<std::string> GetAvailableVaults();
        static std::filesystem::path GetBaseVaultsPath();

        std::shared_ptr<Note> CreateNote();
        bool RenameNote(const std::shared_ptr<Note>& note, const std::string& newName);
        bool DeleteNote(const std::shared_ptr<Note>& note);
        void SaveNote(const std::shared_ptr<Note>& note);

        std::vector<std::shared_ptr<Note>> Search(const std::string& query) const;

        // Find a note by filename (with or without .md extension), returns nullptr if not found
        std::shared_ptr<Note> FindNote(const std::string& name) const;
        // Find existing note or create a new one with the given name
        std::shared_ptr<Note> FindOrCreateNote(const std::string& name);

        static std::filesystem::path GetVaultPathFor(const std::string& vaultName);

    private:
        std::string m_vaultName;
        std::vector<std::shared_ptr<Note>> m_notes;

        void EnsureDirectory();
        static bool AtomicReplaceFile(const std::filesystem::path& from, const std::filesystem::path& to);
        static std::string GenerateNoteFilename();
    };
}
