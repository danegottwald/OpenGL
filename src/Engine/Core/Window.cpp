#include "Window.h"

#include <iostream>

Window* Window::s_Instance = nullptr;

Window::Window(const WindowData& props)
    : m_Minimized(false), m_Properties(props) {
    s_Instance = this;
    Init(props);
}

Window::~Window() {
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

void Window::Update() const {
    // Swap front and back buffers
    glfwSwapBuffers(m_Window);

    // Poll for and process events
    glfwPollEvents();
}

void Window::SetVSync(bool state) {
    if (state != m_Properties.VSync) {
        m_Properties.VSync = state;
        glfwSwapInterval(state);
    }
}

void Window::Init(const WindowData& props) {
    // Initialize Library
    if (!glfwInit()) {
        throw std::runtime_error("GLFW failed to initialize.");
    }

    // GLFW Error Callback
    glfwSetErrorCallback([](int code, const char* message) {
        std::cout << "[ERROR](GLFW) [" << code << "]: " << message << std::endl;
    });

    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    m_Window = glfwCreateWindow(m_Properties.Width, m_Properties.Height,
                                m_Properties.Name.c_str(), nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Window failed to initialize.");
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwMakeContextCurrent(m_Window);

    // Load OpenGL Functions, returns OpenGL Version
    int version = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!version) {
        throw std::runtime_error("OpenGL context failed to initialize.");
    }

    // MSAA
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    // Enable Face Culling and Winding Order
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    SetCallbacks();

    m_Running = true;
}

void Window::SetCallbacks() const {
    // OpenGL Error Callback
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(
        [](GLenum source, GLenum type, GLuint id, GLenum severity,
           GLsizei length, const char* message, const void* userParam) {
            if (type != 33361) {
                std::cout << "[ERROR](OpenGL) [" << id << "]: " << message
                          << std::endl;
            }
        },
        nullptr);

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto& thisWindow =
            *static_cast<Window*>(glfwGetWindowUserPointer(window));
        thisWindow.m_Running = false;
    });

    glfwSetWindowSizeCallback(m_Window,
                              [](GLFWwindow* window, int width, int height) {
                                  auto& thisWindow = Get();
                                  thisWindow.m_Properties.Width = width;
                                  thisWindow.m_Properties.Height = height;

                                  if (width == 0 && height == 0) {
                                      thisWindow.m_Minimized = true;
                                  } else {
                                      thisWindow.m_Minimized = false;
                                      glViewport(0, 0, width, height);
                                  }
                              });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode,
                                    int action, int mods) {
        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            static bool wireMode = true;
            if (wireMode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                wireMode = false;
            } else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                wireMode = true;
            }
        }
        if (key == GLFW_KEY_O && action == GLFW_PRESS) {
            auto& thisWindow = Get();
            static bool mouseGrab = true;
            if (mouseGrab) {
                glfwSetInputMode(thisWindow.m_Window, GLFW_CURSOR,
                                 GLFW_CURSOR_NORMAL);
                mouseGrab = false;
            } else {
                glfwSetInputMode(thisWindow.m_Window, GLFW_CURSOR,
                                 GLFW_CURSOR_DISABLED);
                mouseGrab = true;
            }
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            auto& thisWindow = Get();
            thisWindow.Shutdown();
        }
    });
}
