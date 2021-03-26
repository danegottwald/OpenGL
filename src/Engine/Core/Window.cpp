
#include "Window.h"

#include <iostream>

Window::Window(const WindowData &props) : m_Minimized(false) {
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
	
    m_Properties.Name = props.Name;
    m_Properties.Width = props.Width;
    m_Properties.Height = props.Height;
    m_Properties.VSync = props.VSync;
    m_Window = glfwCreateWindow(props.Width, props.Height, props.Name.c_str(), nullptr, nullptr);
    if (!m_Window) {
        glfwTerminate();
        throw std::runtime_error("Window failed to initialize.");
    }
    glfwSetWindowUserPointer(m_Window, this);
    glfwMakeContextCurrent(m_Window);

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW failed to initialize.");
    }

    SetCallbacks();
	
}

void Window::SetCallbacks() const {

	// OpenGL Error Callback
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity,
        GLsizei length, const char* message, const void* userParam) {
        if (type != 33361) {
            std::cout << "[ERROR](OpenGL) [" << id << "]: " << message << std::endl;
        }
    }, nullptr);

	// GLFW Error Callback
    glfwSetErrorCallback([](int code, const char *message) {
        std::cout << "[ERROR](GLFW) [" << code << "]: " << message << std::endl;
    });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto& thisWindow = *static_cast<Window*>(glfwGetWindowUserPointer(window));
        thisWindow.m_Running = false;
    });

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow *window, int width, int height) {
        auto& thisWindow = *static_cast<Window *>(glfwGetWindowUserPointer(window));
        std::cout << width << " " << height << std::endl;
    	thisWindow.m_Properties.Width = width;
        thisWindow.m_Properties.Height = height;

    	if (width == 0 && height == 0) {
	        thisWindow.m_Minimized = true;
        }
        else {
	        thisWindow.m_Minimized = false;
	        glViewport(0, 0, width, height);
        }
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, 
        int scancode, int action, int mods) {
    	if (action == GLFW_PRESS) {
			std::cout << "[LOG](GLFW) [KEY]: key code '" << (char)(key) << "'" << std::endl;
    	}
        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            static bool wireMode = true;
            if (wireMode) {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                wireMode = false;
            }
            else {
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                wireMode = true;
            }
        }
    });
	
}

