// ==========================================================================
// Kalamari - "Ink your ideas"
// A cross-platform desktop notes application
// Built with C++17, SDL 3, and Dear ImGui
// ==========================================================================

#include "KalamariApp.hpp"
#include <SDL3/SDL_main.h>

int main(int, char**)
{
    Kalamari::KalamariApp app;
    if (!app.Init())
    {
        return 1;
    }

    app.Run();
    app.Shutdown();

    return 0;
}
