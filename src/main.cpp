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
#include <filesystem>
#include <string>
#include <fstream>
#include <iterator>

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
static std::filesystem::path GetVaultPath(const char* vaultName)
{
    char* docsPath = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
    if (!docsPath)
    {
        // Fallback to the current working directory if SDL cannot determine
        // the user's documents folder.
        return std::filesystem::current_path() / "kalimari" / vaultName;
    }

    std::filesystem::path vaultPath = std::filesystem::path(docsPath) / "kalimari" / vaultName;
    SDL_free(docsPath);
    return vaultPath;
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

    try
    {
        std::ifstream file(notesPath, std::ios::binary);
        if (!file) return;

        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());

        size_t copyLen = content.size();
        if (copyLen >= bufferSize) copyLen = bufferSize - 1;
        std::memcpy(buffer, content.data(), copyLen);
        buffer[copyLen] = '\0';
    }
    catch (...)
    {
        SDL_Log("Warning: Failed to load notes from %s", notesPath.string().c_str());
    }
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
    try
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
            size_t writeLen = std::strnlen(buffer, bufferSize);
            file.write(buffer, static_cast<std::streamsize>(writeLen));
        }

        if (!AtomicReplaceFile(tempPath, notesPath))
        {
            SDL_Log("Warning: Failed to replace notes file at %s", notesPath.string().c_str());
            std::error_code ec;
            std::filesystem::remove(tempPath, ec);
        }
    }
    catch (...)
    {
        SDL_Log("Warning: Failed to save notes to %s", notesPath.string().c_str());
    }
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
    static char notesBuffer[8192] =
        "Welcome to Kalamari!\n\n"
        "Start writing your notes here.\n"
        "Use the markdown guide below to format your text.\n";

    // ------------------------------------------------------------------
    // Ensure default vault directory exists and load existing notes
    // ------------------------------------------------------------------
    std::filesystem::path vaultPath = EnsureVaultDirectory(DEFAULT_VAULT_NAME);
    SDL_Log("Vault directory: %s", vaultPath.string().c_str());

    std::filesystem::path notesFilePath = GetNotesFilePath(vaultPath);
    LoadNotes(notesFilePath, notesBuffer, sizeof(notesBuffer));

    static bool showMarkdownTutorial = true;
    static float tutorialAlpha = 0.0f; // for fade-in animation
    static float tutorialTimer  = 0.0f;

    // Auto-save every 30 seconds
    constexpr Uint32 AUTO_SAVE_INTERVAL_MS = 30000;
    Uint64 lastAutoSaveTicks = SDL_GetTicks();

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

        // ---- Fade-in animation for the tutorial overlay ----
        if (showMarkdownTutorial && tutorialAlpha < 1.0f)
        {
            tutorialTimer += io.DeltaTime;
            tutorialAlpha = tutorialTimer / 0.6f;
            if (tutorialAlpha < 0.0f) tutorialAlpha = 0.0f;
            if (tutorialAlpha > 1.0f) tutorialAlpha = 1.0f;
        }

        // ==================================================
        // Fullscreen ImGui window (the app "desktop")
        // ==================================================
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("##KalamariApp", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoBringToFrontOnFocus
            | ImGuiWindowFlags_NoScrollWithMouse);

        // ==================================================
        // 1. HEADER - Centered title bar
        // ==================================================
        {
            float windowW = ImGui::GetContentRegionAvail().x;
            float fullW = ImGui::GetWindowWidth();
            float padX  = ImGui::GetStyle().WindowPadding.x;

            // --- "Kalamari" in Amatic SC, orange accent, centered ---
            ImGui::PushFont(amaticFont);
            ImVec2 titleSize = ImGui::CalcTextSize("Kalamari");
            float titleX = padX + (fullW - 2.0f * padX - titleSize.x) * 0.5f;
            ImGui::SetCursorPosX(titleX);
            ImGui::TextColored(ACCENT_COLOR, "Kalamari");
            ImGui::PopFont();

            // --- "Ink your ideas" in Kameron, centered ---
            const char* slogan = "Ink your ideas";
            ImGui::PushFont(kameronFont);
            ImVec2 sloganSize = ImGui::CalcTextSize(slogan);
            float sloganX = padX + (fullW - 2.0f * padX - sloganSize.x) * 0.5f;
            ImGui::SetCursorPosX(sloganX);
            ImGui::TextDisabled("%s", slogan);
            ImGui::PopFont();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
        }

        // ==================================================
        // Theme toggle button (top-right corner)
        // ==================================================
        {
            const char* label = darkMode ? "Light Mode" : "Dark Mode";
            ImVec2 btnSize = ImGui::CalcTextSize(label);
            btnSize.x += ImGui::GetStyle().FramePadding.x * 2.0f + 16.0f;
            btnSize.y += ImGui::GetStyle().FramePadding.y * 2.0f;

            float fullW = ImGui::GetWindowWidth();
            float padX  = ImGui::GetStyle().WindowPadding.x;
            ImGui::SetCursorPos(ImVec2(fullW - padX - btnSize.x,
                ImGui::GetCursorPosY() - btnSize.y - ImGui::GetStyle().ItemSpacing.y));

            if (ImGui::Button(label, btnSize))
            {
                darkMode = !darkMode;
                if (darkMode)
                    ImGui::StyleColorsDark();
                else
                    ImGui::StyleColorsLight();
                ApplyTheme(darkMode);
            }
        }

        ImGui::Spacing();

        // ==================================================
        // 2. MAIN CONTENT AREA
        // ==================================================
        float totalH = ImGui::GetContentRegionAvail().y;
        float tutorialH = showMarkdownTutorial ? totalH * 0.38f : 0.0f;
        float notesH = totalH - tutorialH - ImGui::GetStyle().ItemSpacing.y;

        // --- Notes editor (resizable input area) ---
        ImGui::PushFont(kameronFont);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));
        ImGuiChildFlags notesChildFlags = static_cast<ImGuiChildFlags>(
            ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY);
        ImGui::BeginChild("NotesArea", ImVec2(0, notesH), notesChildFlags,
            ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::PopStyleColor();

        {
            ImVec2 childSize = ImGui::GetContentRegionAvail();
            ImGui::InputTextMultiline("##Notes", notesBuffer, sizeof(notesBuffer),
                childSize,
                ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_EnterReturnsTrue);
        }

        ImGui::EndChild();
        ImGui::PopFont();

        // --- 3. MARKDOWN TUTORIAL (collapsible child window with fade-in) ---
        if (showMarkdownTutorial)
        {
            ImGui::Spacing();
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tutorialAlpha);

            if (ImGui::CollapsingHeader("Markdown Quick Reference", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginChild("MarkdownTutorial", ImVec2(0, tutorialH - 30.0f),
                    ImGuiChildFlags_Borders, ImGuiWindowFlags_None);

                ImGui::PushFont(kameronFont);
                ImGui::PushStyleColor(ImGuiCol_Text, DARK_TEXT);

                ImGui::TextWrapped("Headers");
                ImGui::Indent();
                ImGui::TextColored(ACCENT_COLOR, "# "); ImGui::SameLine();
                ImGui::Text("Header 1");
                ImGui::TextColored(ACCENT_COLOR, "## "); ImGui::SameLine();
                ImGui::Text("Header 2");
                ImGui::TextColored(ACCENT_COLOR, "### "); ImGui::SameLine();
                ImGui::Text("Header 3");
                ImGui::Unindent();

                ImGui::Spacing();

                ImGui::TextWrapped("Bold & Italic");
                ImGui::Indent();
                ImGui::TextColored(ACCENT_COLOR, "**bold** "); ImGui::SameLine();
                ImGui::Text("-> bold");
                ImGui::TextColored(ACCENT_COLOR, "*italic* "); ImGui::SameLine();
                ImGui::Text("-> italic");
                ImGui::Unindent();

                ImGui::Spacing();

                ImGui::TextWrapped("Lists");
                ImGui::Indent();
                ImGui::TextColored(ACCENT_COLOR, "- item one");
                ImGui::TextColored(ACCENT_COLOR, "- item two");
                ImGui::TextColored(ACCENT_COLOR, "1. numbered");
                ImGui::Unindent();

                ImGui::Spacing();

                ImGui::TextWrapped("Links & Code");
                ImGui::Indent();
                ImGui::TextColored(ACCENT_COLOR, "[text](url) "); ImGui::SameLine();
                ImGui::Text("-> hyperlink");
                ImGui::TextColored(ACCENT_COLOR, "`code` "); ImGui::SameLine();
                ImGui::Text("-> inline code");
                ImGui::Unindent();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Clickable link to the Markdown Guide
                ImGui::Text("Learn more at:");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ACCENT_COLOR);
                ImGui::Text("[Markdown Guide](https://www.markdownguide.org/)");
                ImGui::PopStyleColor();

                if (ImGui::IsItemClicked())
                {
                    SDL_OpenURL("https://www.markdownguide.org/");
                }

                // Hover underline effect
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                }

                ImGui::PopStyleColor();
                ImGui::PopFont();
                ImGui::EndChild();
            }

            ImGui::PopStyleVar(); // Alpha
        }

        // ==================================================
        // 4. FOOTER - Privacy Policy & Terms of Service
        // ==================================================
        {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::PushFont(kameronFont);
            float fullW = ImGui::GetWindowWidth();
            float padX  = ImGui::GetStyle().WindowPadding.x;

            // Layout: [Privacy Policy] centered [Terms of Service]
            const char* privacy = "Privacy Policy";
            const char* tos     = "Terms of Service";
            ImVec2 privSize = ImGui::CalcTextSize(privacy);
            ImVec2 tosSize  = ImGui::CalcTextSize(tos);
            float gap = 30.0f;
            float totalLinkW = privSize.x + gap + tosSize.x;
            float startX = padX + (fullW - 2.0f * padX - totalLinkW) * 0.5f;

            // Privacy Policy link
            ImGui::SetCursorPosX(startX);
            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT_COLOR);
            ImGui::Text("%s", privacy);
            if (ImGui::IsItemClicked())
                SDL_OpenURL("https://callen.page/projects/kalamari/privacy.md");
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::PopStyleColor();

            ImGui::SameLine(startX + privSize.x + gap);

            // Terms of Service link
            ImGui::PushStyleColor(ImGuiCol_Text, ACCENT_COLOR);
            ImGui::Text("%s", tos);
            if (ImGui::IsItemClicked())
                SDL_OpenURL("https://callen.page/projects/kalamari/tos.md");
            if (ImGui::IsItemHovered())
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::PopStyleColor();

            ImGui::PopFont();
        }

        ImGui::End(); // ##KalamariApp

        // ==================================================
        // Rendering
        // ==================================================
        // ---- Auto-save notes periodically ----
        Uint64 currentTicks = SDL_GetTicks();
        if (currentTicks - lastAutoSaveTicks >= AUTO_SAVE_INTERVAL_MS)
        {
            SaveNotes(notesFilePath, notesBuffer, sizeof(notesBuffer));
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
    SaveNotes(notesFilePath, notesBuffer, sizeof(notesBuffer));

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
