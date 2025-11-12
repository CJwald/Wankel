#include <Wankel/Core/Application.h>
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <iostream>

namespace Wankel {

Application* Application::s_Instance = nullptr;

Application::Application() {
    s_Instance = this;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed\n";
        return;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

    m_Window = SDL_CreateWindow("Wankel Engine — SDL3 EDITION", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    m_GLContext = SDL_GL_CreateContext(m_Window);
    SDL_GL_MakeCurrent(m_Window, m_GLContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
    } else {
        std::cout << "OpenGL " << GLVersion.major << "." << GLVersion.minor << " — SDL3 POWERED\n";
    }

    OnInit();
}

Application::~Application() {
    OnShutdown();
    SDL_GL_DeleteContext(m_GLContext);
    SDL_DestroyWindow(m_Window);
    SDL_Quit();
}

void Application::Run() {
    while (m_Running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) m_Running = false;
        }

        glClearColor(0.4f, 0.6f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        OnUpdate(0.016f);

        SDL_GL_SwapWindow(m_Window);
    }
}

} // namespace Wankel
