// Sandbox/main.cpp
#include <Wankel/Core/Application.h>
#include <iostream>

class Sandbox : public Wankel::Application
{
public:
    Sandbox()
        : Wankel::Application("Wankel Engine — Sandbox — @CJ_wald", 1600, 900)
    {
    }

    void OnInit() override
    {
        std::cout << "\033[1;36m";  // Cyan bold
        std::cout << R"(
╔══════════════════════════════════════════════════════╗
║                                                      ║
║    W A N K E L   E N G I N E   v0.1                   ║
║                                                      ║
║    SANDBOX RUNNING — @CJ_wald                        ║
║    November 10, 2025 — 11:15 PM EST                   ║
║                                                      ║
║    BLUE WINDOW ACHIEVED                              ║
║    SDL3 + GLAD + C++20                               ║
║    YOU DID IT. YOU ARE A GOD.                        ║
║                                                      ║
╚══════════════════════════════════════════════════════╝
)" << "\033[0m" << std::endl;
    }

    void OnUpdate(float dt) override
    {
        // Your game logic here later
        // dt is in seconds
    }

    void OnRender() override
    {
        // Your rendering here later
        // Clear color is already blue (0.4, 0.6, 1.0, 1.0)
    }

    void OnShutdown() override
    {
        std::cout << "\033[1;35m";  // Magenta bold
        std::cout << R"(
WANKEL ENGINE SHUTTING DOWN.
YOU EARNED THIS.
@CJ_wald — NOVEMBER 10, 2025
THE BLUE WINDOW WILL RISE AGAIN.
WANKEL FOREVER.
)" << "\033[0m" << std::endl;
    }
};

// ────────────────────────────────
//  ENTRY POINT
// ────────────────────────────────
int main(int argc, char** argv)
{
    Sandbox app;
    app.Run();
    return 0;
}
