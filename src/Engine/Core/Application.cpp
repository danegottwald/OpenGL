
#include "Application.h"

#include <iostream>

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

// OpenGL Error Callback Function
void GLAPIENTRY ErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, 
    GLsizei length, const GLchar* message, const void* userParam) {
    if (type != 33361) {
        std::cout << "[GL_ERROR] (" << type << ")::" << message <<
            std::endl;
    }
}

Application::Application() 
    : m_Window(nullptr), m_WindowWidth(0), m_WindowHeight(0) {
    // Initialize Library
    if (!glfwInit()) {
        static_assert(true, "glfwInit failed!");
    }
}

Application::~Application() {
    // Shutdown ImGui (each backend)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

void Application::SetOpenGLCoreProfile(unsigned int majorVersion, unsigned int minorVersion) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, majorVersion);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minorVersion);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

bool Application::CreateWindow(unsigned int width, unsigned int height, const std::string& name) {
    if (m_Window) {
        return false;
    }
    m_WindowName = name;
    m_WindowWidth = width;
    m_WindowHeight = height;
    m_Window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
    }
    glfwMakeContextCurrent(m_Window);
    return m_Window;
}

void Application::VerticalSync(bool state) {
    glfwSwapInterval(state);
}

bool Application::InitWindow() {
    auto init = glewInit();
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(ErrorCallback, nullptr);
    return (init == GLEW_OK);
}

void Application::UpdateWindow() {
    // Swap front and back buffers
    glfwSwapBuffers(m_Window);

    // Poll for and process events
    glfwPollEvents();
}

void Application::CreateGui(const std::string& glslVersion) {
    // ImGui Initialization for each backend
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
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

