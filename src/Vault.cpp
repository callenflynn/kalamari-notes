#include "Vault.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Kalamari
{
    namespace
    {
        constexpr const char* DEFAULT_VAULT_NAME = "steven";

        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }
    }

    Vault::Vault()
        : m_vaultName(DEFAULT_VAULT_NAME)
    {
    }

    void Vault::SetVault(const std::string& vaultName)
    {
        m_vaultName = vaultName.empty() ? DEFAULT_VAULT_NAME : vaultName;
        Refresh();
    }

    std::filesystem::path Vault::GetVaultPath() const
    {
        return GetVaultPathFor(m_vaultName);
    }

    std::filesystem::path Vault::GetVaultPathFor(const std::string& vaultName)
    {
        const char* docsPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
        if (!docsPath)
        {
            return std::filesystem::current_path() / "kalimari" / vaultName;
        }

#ifdef _WIN32
        int len = ::MultiByteToWideChar(CP_UTF8, 0, docsPath, -1, nullptr, 0);
        if (len > 1)
        {
            std::wstring wstr(len - 1, 0);
            ::MultiByteToWideChar(CP_UTF8, 0, docsPath, -1, wstr.data(), len - 1);
            return std::filesystem::path(wstr) / "kalimari" / vaultName;
        }
        return std::filesystem::current_path() / "kalimari" / vaultName;
#else
        return std::filesystem::path(docsPath) / "kalimari" / vaultName;
#endif
    }

    void Vault::EnsureDirectory()
    {
        std::error_code ec;
        std::filesystem::create_directories(GetVaultPath(), ec);
        if (ec)
        {
            SDL_Log("Warning: Could not create vault directory at %s", GetVaultPath().string().c_str());
        }
    }

    void Vault::Refresh()
    {
        EnsureDirectory();
        m_notes.clear();

        std::error_code ec;
        std::filesystem::path path = GetVaultPath();
        if (!std::filesystem::exists(path, ec) || ec) return;

        std::vector<std::filesystem::path> paths;
        for (const auto& entry : std::filesystem::directory_iterator(path, ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".md")
            {
                paths.push_back(entry.path());
            }
        }

        std::sort(paths.begin(), paths.end(),
                  [](const std::filesystem::path& a, const std::filesystem::path& b) {
                      return a.filename().string() < b.filename().string();
                  });

        for (const auto& p : paths)
        {
            auto note = std::make_shared<Note>();
            note->path = p;
            note->fileName = p.filename().string();

            std::ifstream file(p, std::ios::binary);
            if (file)
            {
                note->content = std::string((std::istreambuf_iterator<char>(file)),
                                            std::istreambuf_iterator<char>());
            }

            m_notes.push_back(note);
        }
    }

    std::shared_ptr<Note> Vault::CreateNote()
    {
        EnsureDirectory();
        std::filesystem::path path = GetVaultPath() / GenerateNoteFilename();
        int suffix = 1;
        while (std::filesystem::exists(path))
        {
            std::string name = GenerateNoteFilename();
            name.insert(name.size() - 3, "-" + std::to_string(suffix));
            path = GetVaultPath() / name;
            ++suffix;
        }

        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (file)
        {
            const char* welcome = "# New Note\n\nStart writing here...\n";
            file.write(welcome, std::strlen(welcome));
        }

        Refresh();
        for (const auto& note : m_notes)
        {
            if (note->path == path) return note;
        }
        return nullptr;
    }

    bool Vault::RenameNote(const std::shared_ptr<Note>& note, const std::string& newName)
    {
        if (!note) return false;

        std::error_code ec;
        std::filesystem::path newPath = note->path.parent_path() / newName;
        if (newPath.extension() != ".md")
        {
            newPath += ".md";
        }

        if (std::filesystem::exists(newPath, ec) && newPath != note->path)
        {
            SDL_Log("Warning: Cannot rename to existing file %s", newPath.string().c_str());
            return false;
        }

        std::filesystem::rename(note->path, newPath, ec);
        if (ec)
        {
            SDL_Log("Warning: Failed to rename note: %s", ec.message().c_str());
            return false;
        }

        note->path = newPath;
        note->fileName = newPath.filename().string();
        Refresh();
        return true;
    }

    bool Vault::DeleteNote(const std::shared_ptr<Note>& note)
    {
        if (!note) return false;

        std::error_code ec;
        std::filesystem::remove(note->path, ec);
        if (ec)
        {
            SDL_Log("Warning: Failed to delete note: %s", ec.message().c_str());
            return false;
        }

        Refresh();
        return true;
    }

    void Vault::SaveNote(const std::shared_ptr<Note>& note)
    {
        if (!note) return;

        std::filesystem::path tempPath = note->path;
        tempPath += ".tmp";

        {
            std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                SDL_Log("Warning: Could not open notes file for writing: %s", tempPath.string().c_str());
                return;
            }
            file.write(note->content.data(), static_cast<std::streamsize>(note->content.size()));
        }

        if (!AtomicReplaceFile(tempPath, note->path))
        {
            SDL_Log("Warning: Failed to replace notes file at %s", note->path.string().c_str());
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
        }
        else
        {
            note->dirty = false;
        }
    }

    std::vector<std::shared_ptr<Note>> Vault::Search(const std::string& query) const
    {
        std::vector<std::shared_ptr<Note>> results;
        if (query.empty()) return m_notes;

        std::string lowerQuery = ToLower(query);
        for (const auto& note : m_notes)
        {
            if (ToLower(note->fileName).find(lowerQuery) != std::string::npos ||
                ToLower(note->content).find(lowerQuery) != std::string::npos)
            {
                results.push_back(note);
            }
        }
        return results;
    }

    bool Vault::AtomicReplaceFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
#ifdef _WIN32
        std::wstring fromW = from.wstring();
        std::wstring toW = to.wstring();
        return ::MoveFileExW(fromW.c_str(), toW.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        return !ec;
#endif
    }

    std::string Vault::GenerateNoteFilename()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d-note.md",
                      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
        return std::string(buf);
    }
}
