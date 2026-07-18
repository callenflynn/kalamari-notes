#include "Config.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace Kalamari
{
    namespace
    {
        std::filesystem::path GetConfigPath(const std::string& vaultPath)
        {
            return std::filesystem::path(vaultPath) / ".obsidian" / "config.ini";
        }

        std::string Trim(const std::string& s)
        {
            size_t a = s.find_first_not_of(" \t\r\n");
            size_t b = s.find_last_not_of(" \t\r\n");
            if (a == std::string::npos) return "";
            return s.substr(a, b - a + 1);
        }

        std::unordered_map<std::string, std::string> ParseIni(const std::string& path)
        {
            std::unordered_map<std::string, std::string> map;
            std::ifstream file(path);
            if (!file) return map;
            std::string line;
            while (std::getline(file, line))
            {
                line = Trim(line);
                if (line.empty() || line[0] == '#' || line[0] == ';') continue;
                size_t eq = line.find('=');
                if (eq == std::string::npos) continue;
                std::string key = Trim(line.substr(0, eq));
                std::string val = Trim(line.substr(eq + 1));
                map[key] = val;
            }
            return map;
        }

        float GetFloat(const std::unordered_map<std::string, std::string>& m,
                       const std::string& key, float def)
        {
            auto it = m.find(key);
            if (it == m.end()) return def;
            try { return std::stof(it->second); } catch (...) { return def; }
        }

        int GetInt(const std::unordered_map<std::string, std::string>& m,
                   const std::string& key, int def)
        {
            auto it = m.find(key);
            if (it == m.end()) return def;
            try { return std::stoi(it->second); } catch (...) { return def; }
        }

        bool GetBool(const std::unordered_map<std::string, std::string>& m,
                     const std::string& key, bool def)
        {
            auto it = m.find(key);
            if (it == m.end()) return def;
            std::string v = it->second;
            return v == "1" || v == "true" || v == "yes";
        }

        std::string GetStr(const std::unordered_map<std::string, std::string>& m,
                           const std::string& key, const std::string& def)
        {
            auto it = m.find(key);
            return (it != m.end()) ? it->second : def;
        }
    }

    Config Config::LoadFromPath(const std::string& filePath)
    {
        Config cfg;
        auto map = ParseIni(filePath);
        cfg.lastVaultPath = GetStr(map, "last_vault_path", "");
        cfg.sidebarWidth  = GetFloat(map, "sidebar_width", 260.0f);
        cfg.windowW       = GetInt(map, "window_w", 1280);
        cfg.windowH       = GetInt(map, "window_h", 800);
        cfg.darkMode      = GetBool(map, "dark_mode", true);
        return cfg;
    }

    void Config::SaveToPath(const std::string& filePath) const
    {
        std::filesystem::path parent = std::filesystem::path(filePath).parent_path();
        std::error_code ec;
        std::filesystem::create_directories(parent, ec);
        if (ec) return;

        std::ofstream file(filePath);
        if (!file) return;
        file << "# Kalamari configuration\n";
        file << "last_vault_path = " << lastVaultPath << "\n";
        file << "sidebar_width = " << sidebarWidth << "\n";
        file << "window_w = " << windowW << "\n";
        file << "window_h = " << windowH << "\n";
        file << "dark_mode = " << (darkMode ? "true" : "false") << "\n";
    }

    Config Config::Load(const std::string& vaultPath)
    {
        Config cfg;
        auto map = ParseIni(GetConfigPath(vaultPath).string());
        cfg.lastVaultPath = GetStr(map, "last_vault_path", "");
        cfg.sidebarWidth  = GetFloat(map, "sidebar_width", 260.0f);
        cfg.windowW       = GetInt(map, "window_w", 1280);
        cfg.windowH       = GetInt(map, "window_h", 800);
        cfg.darkMode      = GetBool(map, "dark_mode", true);
        return cfg;
    }

    void Config::Save(const std::string& vaultPath) const
    {
        SaveToPath((std::filesystem::path(vaultPath) / ".obsidian" / "config.ini").string());
    }
}
