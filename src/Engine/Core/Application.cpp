
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

void FrameTime(double& lastTime, int& nbFrames) {
    double currentTime = glfwGetTime();
    nbFrames++;
    if (currentTime - lastTime >= 1.0) {
        float fps = double(nbFrames);
        float frametime = 1000.0 / double(nbFrames);
        std::cout << "FPS: " << double(nbFrames) << " | " << frametime << " ms" << std::endl;
        nbFrames = 0;
        lastTime += 1.0;
    }
}

void Application::Run() {
    auto &window = GetWindowRef();
    m_Renderer.reset(new Renderer());
    m_Gui.reset(new GuiOverlay);

    m_Gui->Attach();
    while (window.IsRunning()) {
        m_Renderer->Clear();
        // Render here

    	if (!window.IsMinimized()) {
            // Render stuff
            m_Gui->Begin();

    		// ImGui draw stuff here
            ImGui::Begin("Debug");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    		ImGui::End();

    		//ImGui Render stuff
            m_Gui->End();
    	}
        //FrameTime(lastTime, nbFrames);
    	
        m_Window->Update();
    }
    m_Gui->Detach();
}

void Application::SetOpenGLCoreProfile(unsigned int major, unsigned int minor) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

