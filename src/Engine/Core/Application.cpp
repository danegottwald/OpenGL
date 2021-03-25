
#include "Application.h"

#include <iostream>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

Application* Application::s_Instance = nullptr;

Application::Application() : m_Window({ "OpenGL", 1280, 720 }) {
    std::cout << "Application Constructor" << std::endl;
    assert(!s_Instance, "Application already exists!");
    s_Instance = this;

    //Init();
	
}

Application::~Application() {
    std::cout << "Application Destructor" << std::endl;
    // Shutdown ImGui (each backend)
    /*ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();*/
}

void Application::Run() {
	
}

void Application::Init() {
    m_Window = Window({ "OpenGL", 1280, 720 });
}

void Application::SetOpenGLCoreProfile(unsigned int majorVersion, unsigned int minorVersion) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

void Application::InitGui(const std::string& glslVersion) {
    // ImGui Initialization for each backend
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    //ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init(glslVersion.c_str());
    ImGui::StyleColorsDark();
}

void Application::CreateGuiFrame() {
    // Initialize New Frames for ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::RenderGui() {
    // Render the ImGui Elements to OpenGL
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

