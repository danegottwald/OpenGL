#pragma once

#include <glad/gl.h>

#include <memory>

#include "Window.h"
#include "Layer.h"
#include "GuiOverlay.h"
#include "PlayerController.h"
#include "../Renderer/Renderer.h"

class Application {
private:
    static Application* s_Instance;
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<GuiOverlay> m_Gui;
    std::unique_ptr<PlayerController> m_Controller;
    std::vector<Layer> m_Layers;

public:
    Application();

    void Run();

    static Application& Get() {
        return *s_Instance;
    }
    Renderer& GetRenderer() const {
        return *m_Renderer;
    }

    // GLFW, Glad, OpenGL Functions
    void SetOpenGLCoreProfile(unsigned int major, unsigned int minor);
    const GLubyte* GetOpenGLVersion() {
        return glGetString(GL_VERSION);
    }
};
