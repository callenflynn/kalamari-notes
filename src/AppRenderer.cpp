#include "AppRenderer.hpp"

#include "Theme.hpp"
#include "UI.hpp"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <cstdio>

namespace Kalamari
{
    bool AppRenderer::Init(int windowW, int windowH)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
                "Failed to initialize SDL3", nullptr);
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            return false;
        }

        m_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

        // Clamp window size to reasonable bounds
        int w = (std::max)(800, (std::min)(windowW, 3840));
        int h = (std::max)(600, (std::min)(windowH, 2160));

        SDL_WindowFlags wflags = static_cast<SDL_WindowFlags>(
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        m_window = SDL_CreateWindow("Kalamari", w, h, wflags);
        if (!m_window)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
                "Failed to create window", nullptr);
            printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
            return false;
        }

        m_renderer = SDL_CreateRenderer(m_window, nullptr);
        SDL_SetRenderVSync(m_renderer, 1);
        if (!m_renderer)
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
                "Failed to create renderer", nullptr);
            printf("Error: SDL_CreateRenderer(): %s\n", SDL_GetError());
            return false;
        }

        SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(m_window);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.IniFilename = nullptr; // We handle config ourselves via Config

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(m_scale);
        style.FontScaleDpi = m_scale;

        // Modern base styling is applied through Theme::Apply, but make sure
        // the default ImGui look is already scaled for high-DPI displays.
        style.WindowRounding    = 10.0f;
        style.ChildRounding     = 8.0f;
        style.FrameRounding     = 8.0f;
        style.PopupRounding     = 10.0f;
        style.ScrollbarRounding = 8.0f;
        style.GrabRounding      = 6.0f;
        style.TabRounding       = 6.0f;
        style.WindowBorderSize  = 0.0f;
        style.ChildBorderSize   = 0.0f;
        style.FrameBorderSize   = 0.0f;
        style.PopupBorderSize   = 0.0f;
        style.TabBorderSize     = 0.0f;
        style.WindowPadding     = ImVec2(10, 10);
        style.FramePadding      = ImVec2(10, 6);
        style.ItemSpacing       = ImVec2(8, 8);
        style.ItemInnerSpacing  = ImVec2(6, 6);
        style.ScrollbarSize     = 8.0f;

        ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer3_Init(m_renderer);

        // Load body + heading fonts and register them with the UI helpers.
        // Use fixed point sizes; ImGui's DPI scaling (FontScaleDpi/ScaleAllSizes) handles HiDPI.
        ImFont* bodyFont = LoadFont("assets/Kameron/static/Kameron-Regular.ttf", 17.0f);
        ImFont* headingFont = LoadFont("assets/Kameron/static/Kameron-Bold.ttf", 22.0f);
        UI::SetFonts(bodyFont, headingFont);

        return true;
    }

    void AppRenderer::Shutdown()
    {
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(m_renderer);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }

    void AppRenderer::NewFrame()
    {
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void AppRenderer::Render(ImVec4 clearColor)
    {
        ImGui::Render();
        ImGuiIO& io = ImGui::GetIO();
        SDL_SetRenderScale(m_renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);

        SDL_SetRenderDrawColorFloat(m_renderer, clearColor.x, clearColor.y, clearColor.z, clearColor.w);
        SDL_RenderClear(m_renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), m_renderer);
        SDL_RenderPresent(m_renderer);
    }

    std::string AppRenderer::GetResourcePath(const char* relativePath) const
    {
        const char* base = SDL_GetBasePath();
        if (base)
            return std::string(base) + relativePath;
        return std::string(relativePath);
    }

    ImFont* AppRenderer::LoadFont(const char* relativePath, float size)
    {
        std::string path = GetResourcePath(relativePath);
        ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size);
        if (!font)
        {
            printf("Warning: Could not load font %s, using default.\n", path.c_str());
            font = ImGui::GetIO().Fonts->AddFontDefault();
        }
        return font;
    }
}
