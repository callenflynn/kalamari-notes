#include "Vault.hpp"

#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Kalamari
{
    // =========================================================================
    // Internal helpers
    // =========================================================================
    namespace
    {
        std::string ToLower(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        std::string ReadFile(const std::filesystem::path& path)
        {
            std::ifstream file(path, std::ios::binary);
            if (!file) return "";
            return std::string((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
        }

        bool WriteFile(const std::filesystem::path& path, const std::string& content)
        {
            std::ofstream file(path, std::ios::binary | std::ios::trunc);
            if (!file) return false;
            file.write(content.data(), static_cast<std::streamsize>(content.size()));
            return file.good();
        }

        // Get the relative path from base to target
        std::string MakeRelative(const std::filesystem::path& base,
                                 const std::filesystem::path& target)
        {
            auto rel = std::filesystem::relative(target, base);
            // Normalize backslashes to forward slashes for display
            std::string s = rel.string();
            std::replace(s.begin(), s.end(), '\\', '/');
            return s;
        }
    }

    // =========================================================================
    // Open / Close
    // =========================================================================
    bool Vault::OpenVault(const std::filesystem::path& vaultPath)
    {
        std::error_code ec;
        std::filesystem::create_directories(vaultPath, ec);
        if (ec)
        {
            SDL_Log("Vault: Cannot create directory %s: %s",
                vaultPath.string().c_str(), ec.message().c_str());
            return false;
        }

        // Create .obsidian directory for config
        std::filesystem::create_directories(vaultPath / ".obsidian", ec);

        m_vaultPath = vaultPath;
        m_vaultName = vaultPath.filename().string();
        if (m_vaultName.empty()) m_vaultName = "Vault";
        m_isOpen = true;
        Refresh();
        return true;
    }

    void Vault::CloseVault()
    {
        m_isOpen = false;
        m_vaultName.clear();
        m_vaultPath.clear();
        m_notes.clear();
        m_fileTree.clear();
    }

    std::filesystem::path Vault::GetObsidianPath() const
    {
        return m_vaultPath / ".obsidian";
    }

    // =========================================================================
    // Refresh — recursive scan
    // =========================================================================
    void Vault::Refresh()
    {
        m_notes.clear();
        m_fileTree.clear();

        if (!m_isOpen || !std::filesystem::exists(m_vaultPath)) return;

        std::error_code ec;
        for (auto it = std::filesystem::recursive_directory_iterator(m_vaultPath, ec);
             it != std::filesystem::recursive_directory_iterator(); ++it)
        {
            if (ec) { ec.clear(); continue; }

            const auto& entry = *it;

            // Skip hidden directories (like .obsidian, .git)
            if (entry.is_directory())
            {
                std::string dirName = entry.path().filename().string();
                if (!dirName.empty() && dirName[0] == '.')
                {
                    it.disable_recursion_pending();
                    continue;
                }
                continue;
            }

            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".md") continue;

            auto note = std::make_shared<Note>();
            note->path = entry.path();
            note->fileName = entry.path().filename().string();
            note->relativePath = MakeRelative(m_vaultPath, entry.path());
            note->content = ReadFile(entry.path());
            m_notes.push_back(note);
        }

        // Sort notes by relative path (folders first, then alphabetical)
        std::sort(m_notes.begin(), m_notes.end(),
            [](const auto& a, const auto& b) {
                return a->relativePath < b->relativePath;
            });

        BuildFileTree();
    }

    // =========================================================================
    // File tree
    // =========================================================================
    void Vault::BuildFileTree()
    {
        m_fileTree.clear();

        for (const auto& note : m_notes)
        {
            // Split relative path into parts
            std::string rel = note->relativePath;
            std::replace(rel.begin(), rel.end(), '\\', '/');

            // Find or create path components
            std::vector<std::shared_ptr<VaultEntry>>* current = &m_fileTree;
            size_t pos = 0;
            while (pos < rel.size())
            {
                size_t slash = rel.find('/', pos);
                std::string part;
                if (slash == std::string::npos)
                {
                    part = rel.substr(pos);
                    pos = rel.size();
                }
                else
                {
                    part = rel.substr(pos, slash - pos);
                    pos = slash + 1;
                }

                if (part.empty()) continue;

                bool isLast = (pos >= rel.size());
                bool found = false;

                for (auto& child : *current)
                {
                    if (child->name == part)
                    {
                        found = true;
                        if (!isLast)
                            current = &child->children;
                        break;
                    }
                }

                if (!found)
                {
                    auto entry = std::make_shared<VaultEntry>();
                    entry->name = part;
                    entry->relativePath = note->relativePath;
                    entry->isDirectory = !isLast;
                    current->push_back(entry);
                    if (!isLast)
                        current = &entry->children;
                }
            }
        }

        SortTree(m_fileTree);
    }

    void Vault::SortTree(std::vector<std::shared_ptr<VaultEntry>>& entries)
    {
        std::sort(entries.begin(), entries.end(),
            [](const auto& a, const auto& b) {
                // Directories first, then files, alphabetical within each
                if (a->isDirectory != b->isDirectory)
                    return a->isDirectory > b->isDirectory;
                return ToLower(a->name) < ToLower(b->name);
            });

        for (auto& entry : entries)
            SortTree(entry->children);
    }

    // =========================================================================
    // CRUD operations
    // =========================================================================
    std::shared_ptr<Note> Vault::CreateNote(const std::string& relativeDir)
    {
        if (!m_isOpen) return nullptr;

        std::filesystem::path dir = m_vaultPath;
        if (!relativeDir.empty())
            dir /= relativeDir;

        EnsureDirectory(dir);

        std::filesystem::path newPath = dir / GenerateNoteFilename();
        int suffix = 1;
        while (std::filesystem::exists(newPath))
        {
            std::string base = GenerateNoteFilename();
            base.insert(base.size() - 3, "-" + std::to_string(suffix));
            newPath = dir / base;
            ++suffix;
        }

        std::string heading = "New Note";
        if (!relativeDir.empty())
            heading = relativeDir;
        // Clean heading: last path component
        size_t lastSlash = heading.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            heading = heading.substr(lastSlash + 1);

        WriteFile(newPath, "# " + heading + "\n\nStart writing...\n");
        Refresh();

        for (const auto& note : m_notes)
        {
            if (note->path == newPath) return note;
        }
        return nullptr;
    }

    bool Vault::RenameNote(const std::shared_ptr<Note>& note, const std::string& newName)
    {
        if (!note || !m_isOpen) return false;

        std::string safe = SanitizeFileName(newName);
        if (safe.empty()) return false;
        if (safe.size() < 3 || safe.substr(safe.size() - 3) != ".md")
            safe += ".md";

        std::filesystem::path newPath = note->path.parent_path() / safe;

        std::error_code ec;
        if (std::filesystem::exists(newPath, ec) && newPath != note->path)
        {
            SDL_Log("Vault: Rename target already exists: %s", newPath.string().c_str());
            return false;
        }

        std::filesystem::rename(note->path, newPath, ec);
        if (ec)
        {
            SDL_Log("Vault: Rename failed: %s", ec.message().c_str());
            return false;
        }

        note->path = newPath;
        note->fileName = newPath.filename().string();
        note->relativePath = MakeRelative(m_vaultPath, newPath);
        Refresh();
        return true;
    }

    bool Vault::DeleteNote(const std::shared_ptr<Note>& note)
    {
        if (!note || !m_isOpen) return false;

        std::error_code ec;
        std::filesystem::remove(note->path, ec);
        if (ec)
        {
            SDL_Log("Vault: Delete failed: %s", ec.message().c_str());
            return false;
        }

        Refresh();
        return true;
    }

    void Vault::SaveNote(const std::shared_ptr<Note>& note)
    {
        if (!note || !m_isOpen) return;

        // Atomic save: write to temp file, then rename
        std::filesystem::path tempPath = note->path;
        tempPath += ".tmp";

        if (!WriteFile(tempPath, note->content))
        {
            SDL_Log("Vault: Failed to write temp file: %s", tempPath.string().c_str());
            return;
        }

        if (!AtomicReplaceFile(tempPath, note->path))
        {
            SDL_Log("Vault: Atomic replace failed: %s", note->path.string().c_str());
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
            return;
        }

        note->dirty = false;
    }

    // =========================================================================
    // Search
    // =========================================================================
    std::vector<std::shared_ptr<Note>> Vault::Search(const std::string& query) const
    {
        std::vector<std::shared_ptr<Note>> results;
        if (query.empty()) return m_notes;

        std::string lq = ToLower(query);
        for (const auto& note : m_notes)
        {
            if (ToLower(note->fileName).find(lq) != std::string::npos ||
                ToLower(note->content).find(lq) != std::string::npos)
            {
                results.push_back(note);
            }
        }
        return results;
    }

    // =========================================================================
    // Wiki-link support
    // =========================================================================
    std::shared_ptr<Note> Vault::FindNote(const std::string& name) const
    {
        for (const auto& note : m_notes)
        {
            // Match by filename (with or without .md)
            std::string baseName = note->fileName;
            if (baseName.size() > 3 && baseName.substr(baseName.size() - 3) == ".md")
                baseName = baseName.substr(0, baseName.size() - 3);

            if (baseName == name || note->fileName == name ||
                note->fileName == name + ".md")
            {
                return note;
            }

            // Also match by relative path (e.g., "folder/note" matches "folder/note.md")
            std::string relBase = note->relativePath;
            if (relBase.size() > 3 && relBase.substr(relBase.size() - 3) == ".md")
                relBase = relBase.substr(0, relBase.size() - 3);
            if (relBase == name)
                return note;
        }
        return nullptr;
    }

    std::shared_ptr<Note> Vault::FindOrCreateNote(const std::string& name)
    {
        auto existing = FindNote(name);
        if (existing) return existing;

        // Sanitize and create
        std::string safe = SanitizeFileName(name);
        if (safe.empty()) safe = "untitled";

        // Remove .md if present (will be re-added)
        if (safe.size() > 3 && safe.substr(safe.size() - 3) == ".md")
            safe = safe.substr(0, safe.size() - 3);

        std::filesystem::path newPath = m_vaultPath / (safe + ".md");
        int suffix = 1;
        while (std::filesystem::exists(newPath))
        {
            newPath = m_vaultPath / (safe + "-" + std::to_string(suffix) + ".md");
            ++suffix;
        }

        WriteFile(newPath, "# " + safe + "\n\n");
        Refresh();

        for (const auto& note : m_notes)
        {
            if (note->path == newPath) return note;
        }
        return nullptr;
    }

    // =========================================================================
    // Helpers
    // =========================================================================
    void Vault::EnsureDirectory(const std::filesystem::path& dir) const
    {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }

    std::string Vault::GenerateNoteFilename()
    {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d-%02d%02d%02d.md",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
        return buf;
    }

    std::string Vault::SanitizeFileName(const std::string& name)
    {
        std::string result;
        for (char c : name)
        {
            if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
                c == '"' || c == '<' || c == '>' || c == '|' || c == '\0' ||
                c == '\r' || c == '\n' || c == '\t')
            {
                result += '-';
            }
            else
            {
                result += c;
            }
        }

        // Trim leading/trailing dots and spaces
        while (!result.empty() && (result.front() == '.' || result.front() == ' '))
            result.erase(0, 1);
        while (!result.empty() && (result.back() == '.' || result.back() == ' '))
            result.pop_back();

        return result.empty() ? "untitled" : result;
    }

    bool Vault::AtomicReplaceFile(const std::filesystem::path& from,
                                   const std::filesystem::path& to)
    {
#ifdef _WIN32
        // On Windows, MoveFileEx with MOVEFILE_REPLACE_EXISTING is atomic
        int fromLen = ::MultiByteToWideChar(CP_UTF8, 0, from.string().c_str(), -1, nullptr, 0);
        int toLen   = ::MultiByteToWideChar(CP_UTF8, 0, to.string().c_str(),   -1, nullptr, 0);
        if (fromLen <= 1 || toLen <= 1) return false;

        std::wstring wFrom(fromLen - 1, 0);
        std::wstring wTo(toLen - 1, 0);
        ::MultiByteToWideChar(CP_UTF8, 0, from.string().c_str(), -1, wFrom.data(), fromLen - 1);
        ::MultiByteToWideChar(CP_UTF8, 0, to.string().c_str(),   -1, wTo.data(),   toLen - 1);

        return ::MoveFileExW(wFrom.c_str(), wTo.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
        std::error_code ec;
        std::filesystem::rename(from, to, ec);
        return !ec;
#endif
    }
}
