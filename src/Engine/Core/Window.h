#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <string>

struct WindowData {
    std::string Name;
    unsigned int Width, Height;
    bool VSync;
};

class Window {
private:
    static Window* s_Instance;
    GLFWwindow* m_Window;
    WindowData m_Properties;
    bool m_Running, m_Minimized;

    void Init(const WindowData& props);
    void SetCallbacks() const;

public:
    explicit Window(const WindowData& props);
    ~Window();

    void Update() const;
    void Shutdown() {
        m_Running = false;
    }

    static Window& Get() {
        return *s_Instance;
    }
    GLFWwindow* GetWindow() const {
        return m_Window;
    }
    WindowData& GetWindowData() {
        return m_Properties;
    }

    bool IsRunning() const {
        return m_Running;
    }
    bool IsMinimized() const {
        return m_Minimized;
    }

    // Getters for WindowData
    std::string GetName() const {
        return m_Properties.Name;
    }
    unsigned int GetWidth() const {
        return m_Properties.Width;
    }
    unsigned int GetHeight() const {
        return m_Properties.Height;
    }
    bool GetVSync() const {
        return m_Properties.VSync;
    }

    void SetVSync(bool state);
};
