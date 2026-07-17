#pragma once

#include "imgui.h"
#include <SDL3/SDL.h>
#include <string>

namespace Kalamari
{
    class AppRenderer
    {
    public:
        AppRenderer() = default;
        ~AppRenderer() = default;

        bool Init();
        void Shutdown();

        SDL_Window* GetWindow() const { return m_window; }
        SDL_Renderer* GetRenderer() const { return m_renderer; }
        float GetScale() const { return m_scale; }

        void NewFrame();
        void Render(ImVec4 clearColor);

        std::string GetResourcePath(const char* relativePath) const;
        ImFont* LoadFont(const char* relativePath, float size);

    private:
        SDL_Window* m_window = nullptr;
        SDL_Renderer* m_renderer = nullptr;
        float m_scale = 1.0f;
    };
}
