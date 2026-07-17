#include "AppRenderer.hpp"

#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <cstdio>

namespace Kalamari
{
    bool AppRenderer::Init()
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Kalamari",
                "Failed to initialize SDL3", nullptr);
            printf("Error: SDL_Init(): %s\n", SDL_GetError());
            return false;
        }

        m_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
        SDL_WindowFlags window_flags = static_cast<SDL_WindowFlags>(
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
        m_window = SDL_CreateWindow(
            "Kalamari", (int)(1280 * m_scale), (int)(800 * m_scale), window_flags);
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

        ImGui::StyleColorsDark();
        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(m_scale);
        style.FontScaleDpi = m_scale;
        style.WindowRounding   = 8.0f;
        style.ChildRounding    = 6.0f;
        style.FrameRounding    = 4.0f;
        style.PopupRounding    = 4.0f;
        style.ScrollbarRounding = 6.0f;
        style.GrabRounding     = 4.0f;
        style.TabRounding      = 4.0f;
        style.FramePadding     = ImVec2(8.0f, 4.0f);
        style.ItemSpacing      = ImVec2(8.0f, 6.0f);

        ImGui_ImplSDL3_InitForSDLRenderer(m_window, m_renderer);
        ImGui_ImplSDLRenderer3_Init(m_renderer);

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
        {
            std::string result = std::string(base) + relativePath;
            return result;
        }
        return std::string(relativePath);
    }

    ImFont* AppRenderer::LoadFont(const char* relativePath, float size)
    {
        std::string path = GetResourcePath(relativePath);
        ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF(path.c_str(), size);
        if (!font)
        {
            printf("Warning: Could not load font %s, using ImGui default.\n", path.c_str());
            font = ImGui::GetIO().Fonts->AddFontDefault();
            if (!font)
            {
                printf("Error: Could not load default ImGui font.\n");
            }
        }
        return font;
    }
}
