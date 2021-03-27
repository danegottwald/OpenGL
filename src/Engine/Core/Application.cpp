
#include "Application.h"

#include <iostream>

#include "imgui/imgui.h"

Application* Application::s_Instance = nullptr;

Application::Application() : m_Renderer(nullptr) {
    assert(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window.reset(new Window({ "OpenGL", 1280, 720, false }));
    
}

Application::~Application() {
}

void Application::Run() {
    auto &window = GetWindow();
    m_Renderer.reset(new Renderer());
    m_Gui.reset(new GuiOverlay);
    m_Controller.reset(new PlayerController());
    Layer a;
    a.Enable();
    m_Layers.emplace_back(a);
    window.SetVSync(true);

    float frametime = 0.0f;

    m_Gui->Attach();
    while (window.IsRunning()) {
        if (!window.IsMinimized()) {
            float now = static_cast<float>(glfwGetTime());
            frametime = now - frametime;
            for (auto &layer : m_Layers) {
                layer.Draw(frametime);
            }
            frametime = now;

            // for each gui to render in queue
            // m_Renderer.DrawGui();

            m_Gui->Begin();

            //ImGui::BeginMainMenuBar();
            //ImGui::Text("%d FPS (%.1f ms)", (int)ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
            //ImGui::EndMainMenuBar();

            m_Gui->End();
        }
        m_Window->Update();
    }
    m_Gui->Detach();
}

void Application::SetOpenGLCoreProfile(unsigned int major, unsigned int minor) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

