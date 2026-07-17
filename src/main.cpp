// ==========================================================================
// Kalamari - "Ink your ideas"
// A cross-platform desktop notes application
// Built with C++17, SDL 3, and Dear ImGui
// ==========================================================================

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <sentry.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

// ==========================================================================
// Theme Colors (hex -> float RGBA)
// ==========================================================================

// Orange accent shared by both themes: #ED5001 -> (0.929, 0.314, 0.004)
static constexpr ImVec4 ACCENT_COLOR      = ImVec4(0.929f, 0.314f, 0.004f, 1.0f);
static constexpr ImVec4 ACCENT_COLOR_HDR  = ImVec4(1.0f, 0.35f, 0.02f, 1.0f);

// Light theme
static constexpr ImVec4 LIGHT_BG          = ImVec4(1.000f, 0.937f, 0.878f, 1.0f); // #FFEFE0
static constexpr ImVec4 LIGHT_TEXT        = ImVec4(0.141f, 0.141f, 0.141f, 1.0f); // #242424
static constexpr ImVec4 LIGHT_BORDER      = ImVec4(0.800f, 0.780f, 0.750f, 1.0f);
static constexpr ImVec4 LIGHT_FRAME_BG    = ImVec4(0.960f, 0.910f, 0.850f, 1.0f);
static constexpr ImVec4 LIGHT_SCROLLBAR   = ImVec4(0.900f, 0.870f, 0.820f, 1.0f);

// Dark theme
static constexpr ImVec4 DARK_BG           = ImVec4(0.118f, 0.118f, 0.118f, 1.0f); // #1E1E1E
static constexpr ImVec4 DARK_TEXT         = ImVec4(1.000f, 0.992f, 0.969f, 1.0f); // #FFFDF7
static constexpr ImVec4 DARK_BORDER       = ImVec4(0.300f, 0.300f, 0.300f, 1.0f);
static constexpr ImVec4 DARK_FRAME_BG     = ImVec4(0.180f, 0.180f, 0.180f, 1.0f);
static constexpr ImVec4 DARK_SCROLLBAR    = ImVec4(0.250f, 0.250f, 0.250f, 1.0f);

// ==========================================================================
// Cross-platform vault path: <Documents>/kalimari/<vault>
// ==========================================================================
static std::filesystem::path Utf8ToPath(const char* utf8)
{
#ifdef _WIN32
    if (!utf8) return {};
    // len includes the null terminator; the wstring will hold len-1 chars.
    int len = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 1) return {};
    std::wstring wstr(len - 1, 0);
    int written = ::MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr.data(), len - 1);
    if (written <= 0) return {};
    return std::filesystem::path(wstr);
#else
    return std::filesystem::path(utf8);
#endif
}

static std::filesystem::path GetVaultPath(const char* vaultName)
{
    const char* docsPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (!docsPath)
    {
        // Fallback to the current working directory if SDL cannot determine
        // the user's documents folder.
        return std::filesystem::current_path() / "kalimari" / vaultName;
    }

    return Utf8ToPath(docsPath) / "kalimari" / vaultName;
}

static constexpr const char* DEFAULT_VAULT_NAME = "steven";
static constexpr const char* NOTES_FILE_NAME = "notes.md";

static std::filesystem::path EnsureVaultDirectory(const char* vaultName)
{
    std::filesystem::path path = GetVaultPath(vaultName);
    std::error_code ec;
    std::filesystem::create_directories(path, ec);
    if (ec)
    {
        SDL_Log("Warning: Could not create vault directory at %s", path.string().c_str());
    }
    return path;
}

static std::filesystem::path GetNotesFilePath(const std::filesystem::path& vaultPath)
{
    return vaultPath / NOTES_FILE_NAME;
}

static void LoadNotes(const std::filesystem::path& notesPath, char* buffer, size_t bufferSize)
{
    if (bufferSize == 0) return;
    buffer[0] = '\0';

    std::error_code ec;
    if (!std::filesystem::exists(notesPath, ec) || ec) return;

    std::ifstream file(notesPath, std::ios::binary);
    if (!file) return;

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    size_t copyLen = content.size();
    if (copyLen >= bufferSize) copyLen = bufferSize - 1;
    std::memcpy(buffer, content.data(), copyLen);
    buffer[copyLen] = '\0';
}

static bool AtomicReplaceFile(const std::filesystem::path& from, const std::filesystem::path& to)
{
#ifdef _WIN32
    // On Windows, MoveFileExW with MOVEFILE_REPLACE_EXISTING performs an
    // atomic replacement of the destination file.
    std::wstring fromW = from.wstring();
    std::wstring toW = to.wstring();
    return ::MoveFileExW(fromW.c_str(), toW.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    return !ec;
#endif
}

static void SaveNotes(const std::filesystem::path& notesPath, const char* buffer, size_t bufferSize)
{
    std::filesystem::path tempPath = notesPath;
    tempPath += ".tmp";

    {
        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            SDL_Log("Warning: Could not open notes file for writing: %s", tempPath.string().c_str());
            return;
        }
        size_t writeLen = static_cast<size_t>(std::find(buffer, buffer + bufferSize, '\0') - buffer);
        file.write(buffer, static_cast<std::streamsize>(writeLen));
    }

    if (!AtomicReplaceFile(tempPath, notesPath))
    {
        SDL_Log("Warning: Failed to replace notes file at %s", notesPath.string().c_str());
        std::error_code ec;
        std::filesystem::remove(tempPath, ec);
    }
}

static std::string GenerateNoteFilename()
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

static void RefreshVaultFiles(const std::filesystem::path& vaultPath,
                              std::vector<std::filesystem::path>& files)
{
    files.clear();
    std::error_code ec;
    if (!std::filesystem::exists(vaultPath, ec) || ec) return;

    for (const auto& entry : std::filesystem::directory_iterator(vaultPath, ec))
    {
        if (entry.is_regular_file(ec) && entry.path().extension() == ".md")
        {
            files.push_back(entry.path());
        }
    }

    std::sort(files.begin(), files.end(),
              [](const std::filesystem::path& a, const std::filesystem::path& b) {
                  return a.filename().string() < b.filename().string();
              });
}

static std::filesystem::path CreateNewNote(const std::filesystem::path& vaultPath)
{
    std::filesystem::path path = vaultPath / GenerateNoteFilename();
    int suffix = 1;
    while (std::filesystem::exists(path))
    {
        std::string name = GenerateNoteFilename();
        name.insert(name.size() - 3, "-" + std::to_string(suffix));
        path = vaultPath / name;
        ++suffix;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (file)
    {
        const char* welcome = "# New Note\n\nStart writing here...\n";
        file.write(welcome, std::strlen(welcome));
    }
    return path;
}

// ==========================================================================
// Apply theme to ImGui style
// ==========================================================================
static void ApplyTheme(bool darkMode)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    if (darkMode)
    {
        c[ImGuiCol_WindowBg]           = DARK_BG;
        c[ImGuiCol_ChildBg]            = DARK_BG;
        c[ImGuiCol_PopupBg]            = DARK_BG;
        c[ImGuiCol_Text]               = DARK_TEXT;
        c[ImGuiCol_Border]             = DARK_BORDER;
        c[ImGuiCol_FrameBg]            = DARK_FRAME_BG;
        c[ImGuiCol_FrameBgHovered]     = ImVec4(0.24f, 0.24f, 0.24f, 1.0f);
        c[ImGuiCol_FrameBgActive]      = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
        c[ImGuiCol_TitleBg]            = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
        c[ImGuiCol_TitleBgActive]      = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
        c[ImGuiCol_ScrollbarBg]        = DARK_SCROLLBAR;
        c[ImGuiCol_ScrollbarGrab]      = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.60f, 0.60f, 0.60f, 1.0f);
        c[ImGuiCol_Separator]          = DARK_BORDER;
        c[ImGuiCol_Header]             = ImVec4(0.28f, 0.16f, 0.10f, 1.0f);
        c[ImGuiCol_HeaderHovered]      = ImVec4(0.36f, 0.22f, 0.13f, 1.0f);
        c[ImGuiCol_HeaderActive]       = ImVec4(0.44f, 0.28f, 0.16f, 1.0f);
        c[ImGuiCol_Button]             = ACCENT_COLOR;
        c[ImGuiCol_ButtonHovered]      = ACCENT_COLOR_HDR;
        c[ImGuiCol_ButtonActive]       = ImVec4(0.80f, 0.26f, 0.00f, 1.0f);
        c[ImGuiCol_SliderGrab]         = ACCENT_COLOR;
        c[ImGuiCol_SliderGrabActive]   = ACCENT_COLOR_HDR;
        c[ImGuiCol_CheckMark]          = ACCENT_COLOR;
        c[ImGuiCol_TextSelectedBg]     = ImVec4(0.929f, 0.314f, 0.004f, 0.35f);
        c[ImGuiCol_Tab]                = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
        c[ImGuiCol_TabHovered]         = ACCENT_COLOR_HDR;
        c[ImGuiCol_TabSelected]        = ImVec4(0.22f, 0.22f, 0.22f, 1.0f);
        c[ImGuiCol_ResizeGrip]         = ImVec4(0.929f, 0.314f, 0.004f, 0.20f);
        c[ImGuiCol_ResizeGripHovered]  = ImVec4(0.929f, 0.314f, 0.004f, 0.50f);
        c[ImGuiCol_ResizeGripActive]   = ImVec4(0.929f, 0.314f, 0.004f, 0.80f);
    }
    else
    {
        c[ImGuiCol_WindowBg]           = LIGHT_BG;
        c[ImGuiCol_ChildBg]            = LIGHT_BG;
        c[ImGuiCol_PopupBg]            = ImVec4(1.0f, 0.96f, 0.91f, 1.0f);
        c[ImGuiCol_Text]               = LIGHT_TEXT;
        c[ImGuiCol_Border]             = LIGHT_BORDER;
        c[ImGuiCol_FrameBg]            = LIGHT_FRAME_BG;
        c[ImGuiCol_FrameBgHovered]     = ImVec4(0.94f, 0.88f, 0.82f, 1.0f);
        c[ImGuiCol_FrameBgActive]      = ImVec4(0.90f, 0.84f, 0.78f, 1.0f);
        c[ImGuiCol_TitleBg]            = ImVec4(0.93f, 0.86f, 0.80f, 1.0f);
        c[ImGuiCol_TitleBgActive]      = ImVec4(0.90f, 0.83f, 0.77f, 1.0f);
        c[ImGuiCol_ScrollbarBg]        = LIGHT_SCROLLBAR;
        c[ImGuiCol_ScrollbarGrab]      = ImVec4(0.80f, 0.76f, 0.70f, 1.0f);
        c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.72f, 0.68f, 0.62f, 1.0f);
        c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.65f, 0.61f, 0.55f, 1.0f);
        c[ImGuiCol_Separator]          = LIGHT_BORDER;
        c[ImGuiCol_Header]             = ImVec4(0.92f, 0.80f, 0.72f, 1.0f);
        c[ImGuiCol_HeaderHovered]      = ImVec4(0.90f, 0.75f, 0.65f, 1.0f);
        c[ImGuiCol_HeaderActive]       = ImVec4(0.88f, 0.72f, 0.60f, 1.0f);
        c[ImGuiCol_Button]             = ACCENT_COLOR;
        c[ImGuiCol_ButtonHovered]      = ACCENT_COLOR_HDR;
        c[ImGuiCol_ButtonActive]       = ImVec4(0.80f, 0.26f, 0.00f, 1.0f);
        c[ImGuiCol_SliderGrab]         = ACCENT_COLOR;
        c[ImGuiCol_SliderGrabActive]   = ACCENT_COLOR_HDR;
        c[ImGuiCol_CheckMark]          = ACCENT_COLOR;
        c[ImGuiCol_TextSelectedBg]     = ImVec4(0.929f, 0.314f, 0.004f, 0.30f);
        c[ImGuiCol_Tab]                = ImVec4(0.92f, 0.86f, 0.80f, 1.0f);
        c[ImGuiCol_TabHovered]         = ACCENT_COLOR_HDR;
        c[ImGuiCol_TabSelected]        = ImVec4(0.88f, 0.80f, 0.74f, 1.0f);
        c[ImGuiCol_ResizeGrip]         = ImVec4(0.929f, 0.314f, 0.004f, 0.15f);
        c[ImGuiCol_ResizeGripHovered]  = ImVec4(0.929f, 0.314f, 0.004f, 0.40f);
        c[ImGuiCol_ResizeGripActive]   = ImVec4(0.929f, 0.314f, 0.004f, 0.65f);
    }
}

// ==========================================================================
// Main
// ==========================================================================
int main(int, char**)
{
    // ------------------------------------------------------------------
    // Initialize SDL 3
    // ------------------------------------------------------------------
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
            "Failed to initialize SDL3", nullptr);
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    // ------------------------------------------------------------------
    // Initialize Sentry crash monitoring
    // ------------------------------------------------------------------
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

    // ------------------------------------------------------------------
    // Create window with High-DPI support
    // ------------------------------------------------------------------
    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    SDL_Window* window = SDL_CreateWindow(
        "Kalamari", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
            "Failed to create window", nullptr);
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // ------------------------------------------------------------------
    // Create SDL Renderer
    // ------------------------------------------------------------------
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    SDL_SetRenderVSync(renderer, 1);
    if (renderer == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
            "Failed to create renderer", nullptr);
        printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    // ------------------------------------------------------------------
    // Setup Dear ImGui context
    // ------------------------------------------------------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Start in dark mode
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;
    style.WindowRounding   = 8.0f;
    style.ChildRounding    = 6.0f;
    style.FrameRounding    = 4.0f;
    style.PopupRounding    = 4.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding     = 4.0f;
    style.TabRounding      = 4.0f;
    style.FramePadding     = ImVec2(8.0f, 4.0f);
    style.ItemSpacing      = ImVec2(8.0f, 6.0f);

    bool darkMode = true;
    ApplyTheme(darkMode);

    // ------------------------------------------------------------------
    // Initialize ImGui SDL3 + SDL_Renderer backends
    // ------------------------------------------------------------------
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // ------------------------------------------------------------------
    // Load fonts from assets/ folder
    // Font paths are relative to the executable.
    // Adjust these paths if your assets are elsewhere.
    // ------------------------------------------------------------------
    // assets/Kameron/static/Kameron-Regular.ttf  -> default UI font
    // assets/Amatic_SC/AmaticSC-Bold.ttf         -> accent / app name font
    ImFont* kameronFont = io.Fonts->AddFontFromFileTTF(
        "assets/Kameron/static/Kameron-Regular.ttf", 18.0f);
    if (!kameronFont)
    {
        printf("Warning: Could not load Kameron font, using ImGui default.\n");
        kameronFont = io.Fonts->AddFontDefault();
    }

    ImFont* amaticFont = io.Fonts->AddFontFromFileTTF(
        "assets/Amatic_SC/AmaticSC-Bold.ttf", 36.0f);
    if (!amaticFont)
    {
        printf("Warning: Could not load Amatic SC font, using ImGui default.\n");
        amaticFont = io.Fonts->AddFontDefault();
    }

    // TODO: Load additional assets here
    // assets/kalamari.png            -> logo (use SDL_LoadBMP + SDL_CreateTextureFromSurface)
    // assets/kalamari_square_name.png -> square logo variant

    // ------------------------------------------------------------------
    // Application state
    // ------------------------------------------------------------------
    static char notesBuffer[8192] = {};

    // Ensure default vault directory exists and load existing notes
    std::filesystem::path vaultPath = EnsureVaultDirectory(DEFAULT_VAULT_NAME);
    SDL_Log("Vault directory: %s", vaultPath.string().c_str());

    std::vector<std::filesystem::path> vaultFiles;
    RefreshVaultFiles(vaultPath, vaultFiles);

    std::filesystem::path currentFile;
    if (!vaultFiles.empty())
    {
        currentFile = vaultFiles.front();
        LoadNotes(currentFile, notesBuffer, sizeof(notesBuffer));
    }

    // Auto-save every 30 seconds when dirty
    constexpr Uint32 AUTO_SAVE_INTERVAL_MS = 30000;
    Uint64 lastAutoSaveTicks = SDL_GetTicks();
    bool notesDirty = false;
    static char searchBuffer[256] = {};

    auto MarkNotesDirty = [](ImGuiInputTextCallbackData* data) -> int {
        bool* dirty = static_cast<bool*>(data->UserData);
        if (dirty) *dirty = true;
        return 0;
    };

    // ------------------------------------------------------------------
    // Main loop
    // ------------------------------------------------------------------
    bool done = false;
    while (!done)
    {
        // Poll events
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED
                && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Skip rendering when minimized
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // ==================================================
        // Fullscreen ImGui window (the app "desktop")
        // ==================================================
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##KalamariApp", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::PopStyleVar();

        ImGuiStyle& style = ImGui::GetStyle();
        float sidebarWidth = 260.0f * main_scale;

        // ==================================================
        // LEFT SIDEBAR
        // ==================================================
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, darkMode ? DARK_FRAME_BG : LIGHT_FRAME_BG);
            ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), ImGuiChildFlags_Borders);

            ImGui::PushFont(kameronFont);
            ImGui::SetCursorPosX((sidebarWidth - ImGui::CalcTextSize("Kalamari").x) * 0.5f);
            ImGui::TextColored(ACCENT_COLOR, "Kalamari");
            ImGui::PopFont();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("+ New Note", ImVec2(-1, 0)))
            {
                if (notesDirty && !currentFile.empty())
                {
                    SaveNotes(currentFile, notesBuffer, sizeof(notesBuffer));
                    notesDirty = false;
                }

                currentFile = CreateNewNote(vaultPath);
                RefreshVaultFiles(vaultPath, vaultFiles);
                LoadNotes(currentFile, notesBuffer, sizeof(notesBuffer));
            }

            ImGui::Spacing();
            ImGui::TextDisabled("Notes");
            ImGui::Spacing();

            ImGui::PushStyleColor(ImGuiCol_FrameBg, darkMode ? DARK_BG : LIGHT_BG);
            ImGui::InputTextWithHint("##Search", "Search notes...", searchBuffer, sizeof(searchBuffer));
            ImGui::PopStyleColor();
            ImGui::Spacing();

            ImGui::BeginChild("FileList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() - 8.0f));
            for (const auto& filePath : vaultFiles)
            {
                std::string filename = filePath.filename().string();
                if (std::search(filename.begin(), filename.end(), searchBuffer, searchBuffer + std::strlen(searchBuffer),
                                [](char a, char b) { return std::tolower(a) == std::tolower(b); }) == filename.end())
                {
                    continue;
                }

                bool isSelected = (filePath == currentFile);
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Header, ACCENT_COLOR);
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ACCENT_COLOR_HDR);
                }

                if (ImGui::Selectable(filename.c_str(), isSelected))
                {
                    if (notesDirty && !currentFile.empty())
                    {
                        SaveNotes(currentFile, notesBuffer, sizeof(notesBuffer));
                        notesDirty = false;
                    }

                    currentFile = filePath;
                    LoadNotes(currentFile, notesBuffer, sizeof(notesBuffer));
                }

                if (isSelected)
                {
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndChild();

            // Theme toggle at the bottom of the sidebar
            if (ImGui::Button(darkMode ? "Light Mode" : "Dark Mode", ImVec2(-1, 0)))
            {
                darkMode = !darkMode;
                if (darkMode)
                    ImGui::StyleColorsDark();
                else
                    ImGui::StyleColorsLight();
                ApplyTheme(darkMode);
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        // ==================================================
        // RIGHT EDITOR
        // ==================================================
        {
            ImGui::BeginChild("Editor", ImVec2(0, 0), ImGuiChildFlags_None);

            if (currentFile.empty())
            {
                ImVec2 region = ImGui::GetContentRegionAvail();
                ImGui::SetCursorPos(ImVec2((region.x - ImGui::CalcTextSize("Select or create a note to begin.").x) * 0.5f,
                                            region.y * 0.35f));
                ImGui::TextDisabled("Select or create a note to begin.");

                ImGui::Spacing();
                ImGui::Spacing();

                ImGui::TextWrapped("Markdown basics:");
                ImGui::BulletText("# Heading");
                ImGui::BulletText("**bold**");
                ImGui::BulletText("*italic*");
                ImGui::BulletText("- list item");
                ImGui::BulletText("[link](url)");
            }
            else
            {
                std::string title = currentFile.stem().string();
                ImGui::PushFont(kameronFont);
                ImGui::SetWindowFontScale(1.35f);
                ImGui::Text("%s", title.c_str());
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::PushFont(kameronFont);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
                ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput
                                          | ImGuiInputTextFlags_CallbackEdit;
                ImVec2 avail = ImGui::GetContentRegionAvail();
                ImGui::InputTextMultiline("##Notes", notesBuffer, sizeof(notesBuffer),
                    avail, flags, MarkNotesDirty, &notesDirty);
                ImGui::PopStyleColor();
                ImGui::PopFont();
            }

            ImGui::EndChild();
        }

        ImGui::End(); // ##KalamariApp

        // ==================================================
        // Rendering
        // ==================================================
        // ---- Auto-save notes periodically ----
        Uint64 currentTicks = SDL_GetTicks();
        if (notesDirty && !currentFile.empty() && currentTicks - lastAutoSaveTicks >= AUTO_SAVE_INTERVAL_MS)
        {
            SaveNotes(currentFile, notesBuffer, sizeof(notesBuffer));
            notesDirty = false;
            lastAutoSaveTicks = currentTicks;
        }

        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        // Clear with current theme background color
        if (darkMode)
            SDL_SetRenderDrawColorFloat(renderer, DARK_BG.x, DARK_BG.y, DARK_BG.z, DARK_BG.w);
        else
            SDL_SetRenderDrawColorFloat(renderer, LIGHT_BG.x, LIGHT_BG.y, LIGHT_BG.z, LIGHT_BG.w);

        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // ------------------------------------------------------------------
    // Save notes before shutting down
    // ------------------------------------------------------------------
    if (notesDirty && !currentFile.empty())
    {
        SaveNotes(currentFile, notesBuffer, sizeof(notesBuffer));
        notesDirty = false;
    }

    // ------------------------------------------------------------------
    // Cleanup
    // ------------------------------------------------------------------
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    // Flush and shut down Sentry
    sentry_close();

    return 0;
}
