
#include "Window.h"

#include <iostream>
#include <stdexcept>

Window::Window(const WindowData &props) {
    std::cout << "Window Constructor" << std::endl;
    Init(props);
}

Window::~Window() {
    std::cout << "Window Destructor" << std::endl;
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
	if (state != m_Data.VSync) {
	    m_Data.VSync = state;
	    glfwSwapInterval(state);
	}
}

void Window::Init(const WindowData& props) {
    // Initialize Library
    if (!glfwInit()) {
        throw std::runtime_error("GLFW failed to initialize.");
    }
	
    m_Data.Name = props.Name;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.VSync = props.VSync;
    m_Window = glfwCreateWindow(props.Width, props.Height, props.Name.c_str(), nullptr, nullptr);
    if (!m_Window) {
        throw std::runtime_error("Window failed to initialize.");
        glfwTerminate();
    }
    glfwMakeContextCurrent(m_Window);

    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("GLEW failed to initialize.");
    }

    // Set Window Callbacks
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

    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
        WindowData& data = *(WindowData *)glfwGetWindowUserPointer(window);
    	data.Width = width;
        data.Height = height;
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, 
        int scancode, int action, int mods) {
    	if (action == GLFW_PRESS) {
			std::cout << "[LOG](GLFW) [KEY]: key code '" << key << "'" << std::endl;
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

