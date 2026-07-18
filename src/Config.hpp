#pragma once

#include <string>

namespace Kalamari
{
    // Persistent app configuration stored as simple key=value pairs.
    struct Config
    {
        std::string lastVaultPath;
        float       sidebarWidth = 260;
        int         windowW = 1280;
        int         windowH = 800;
        bool        darkMode = true;

        // Load from an exact file path
        static Config LoadFromPath(const std::string& filePath);
        // Save to an exact file path
        void SaveToPath(const std::string& filePath) const;

        // Convenience: load from a vault's .obsidian directory
        static Config Load(const std::string& vaultPath);
        // Convenience: save to a vault's .obsidian directory
        void Save(const std::string& vaultPath) const;
    };
}
