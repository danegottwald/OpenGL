
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Engine/Core/Application.h"
#include "Engine/Renderer/Renderer.h"

#include "tests/ClearColor.h"


void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_P && action == GLFW_PRESS) { // Flip flop wireframe and fillframe
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
}

int main(int argc, char **argv) {
    
    Application app;
    app.SetOpenGLCoreProfile(3, 3);

    if (!app.CreateWindow(1280, 720, "OpenGL")) {
        return -1;
    }

    app.VerticalSync(true);
    if (!app.InitWindow()) {
        return -2;
    }

    // Print Version and Set Debug Mode
    std::cout << app.GetOpenGLVersion() << std::endl;

    // Renderer
    Renderer renderer;
    renderer.EnableBlending();
    renderer.BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ImGui Initialization for each backend
    app.CreateGui("#version 330");

    TestSpace::ClearColor test;

    // Loop until the user closes the window
    while (app.WindowIsOpen()) {
        renderer.Clear();

        test.OnUpdate(0.0f);
        test.OnRender();

        app.CreateGuiFrame();

        test.OnImGuiRender();

        app.RenderGui();
        app.UpdateWindow();
    }

    return 0;
}

