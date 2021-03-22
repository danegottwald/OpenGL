
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/VertexBuffer.h"
#include "Engine/Renderer/VertexArray.h"
#include "Engine/Renderer/IndexBuffer.h"
#include "Engine/Renderer/Shader.h"

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

void FrameTime(double &lastTime, int &nbFrames) {
    double currentTime = glfwGetTime();
    nbFrames++;
    if (currentTime - lastTime >= 1.0) {
        std::cout << "FPS: " << double(nbFrames) << std::endl;
        std::cout << "Frametime: " << 1000.0 / double(nbFrames) << "ms" << std::endl;
        nbFrames = 0;
        lastTime += 1.0;
    }
}

int main(int argc, char **argv) {
    // Initialize the library
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow *window = glfwCreateWindow(640, 480, "OpenGL", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Limit FPS to monitor refresh rate (ie Vertical Sync)
    glfwSwapInterval(1);

    // Set the key callback function to accept inputs
    glfwSetKeyCallback(window, KeyCallback);

    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW error" << std::endl;
    }

    // Print Version and Set Debug Mode
    std::cout << glGetString(GL_VERSION) << std::endl;
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(ErrorCallback, nullptr);

    // Vertex Positions
    float positions[] = {
            -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.f, 0.5f, 0.0f, 0.0f, 1.0f,
    };

    unsigned int indices[] = {
            0, 1, 2,
    };

    // Vertex Buffer
    VertexBuffer vb(positions, sizeof(positions));

    // Vertex Array
    VertexArray va;
    VertexBufferLayout layout;
    layout.Push<float>(2);
    layout.Push<float>(3);
    va.AddBuffer(vb, layout);

    // Index Buffer
    IndexBuffer ib(indices, sizeof(indices) / sizeof(unsigned int));

    // Shader
    Shader shader("res/shaders/Basic.glsl");

    // Set time and Frames for Getting Frametime
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        // Render here
        glClear(GL_COLOR_BUFFER_BIT);

        // Binds the Shader
        shader.Bind();
        // Set uniform4f value in Shader
        //shader.SetUniform4f("u_Color", red, blue, green, 1.0f);

        // Bind the Vertex Array (this binds the vertex buffer too)
        va.Bind();
        // Bind the Index Buffer
        ib.Bind();

        // For Index Buffer
        glDrawElements(GL_TRIANGLES, ib.GetCount(), GL_UNSIGNED_INT, nullptr);

        // Print Frametime
        //FrameTime(lastTime, nbFrames);

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

