#include "Application.h"

#include <iostream>
#include <filesystem>

#include "imgui/imgui.h"

Application* Application::s_Instance = nullptr;

Application::Application() {
    //assert(!s_Instance, "Application already exists!");
    s_Instance = this;
}

void Application::Run() {
    m_Window.reset(new Window({ "OpenGL", 1280, 720, false }));
    auto& window = Window::Get();
    //m_Renderer.reset(new Renderer());
    m_Gui.reset(new GuiOverlay);
    m_Controller.reset(new PlayerController());
    Layer a;
    a.Enable();
    m_Layers.emplace_back(a);
    window.SetVSync(true);

    int selectedItem = 0;
    std::vector<char *> items;
    for (const auto &item : std::filesystem::directory_iterator("res/models/")) {
        items.push_back(strdup(item.path().string().c_str()));
    }

    float deltaTime = 0.0f;

    m_Gui->Attach();
    while (window.IsRunning()) {
        if (!window.IsMinimized()) {
            float now = static_cast<float>(glfwGetTime());
            deltaTime = now - deltaTime;
            for (auto& layer : m_Layers) {
                layer.Draw(deltaTime);

                m_Gui->Begin();
                auto& pc = PlayerController::GetController();
                ImGui::Begin("Debug");
                ImGui::Text("%d FPS (%.1f ms)", static_cast<int>(ImGui::GetIO().Framerate),
                    1000.0f / ImGui::GetIO().Framerate);
                ImGui::Text("%f", deltaTime);
                ImGui::Text("Position: %.1f, %.1f, %.1f", pc.GetPosition().x,
                    pc.GetPosition().y, pc.GetPosition().z);

                int lastSelected = selectedItem;
                ImGui::Combo("Model", &selectedItem, items.data(), items.size());

                if (lastSelected != selectedItem) {
                    layer.LoadModel(items[selectedItem]);
                }

                ImGui::End();

                m_Gui->End();
            }
            deltaTime = now;

            // for each gui to render in queue
            // m_Renderer.DrawGui();
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
