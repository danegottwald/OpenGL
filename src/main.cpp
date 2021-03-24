
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Texture.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


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
    // Initialize the library
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    unsigned int width = 1280, height = 720;
    GLFWwindow *window = glfwCreateWindow(width, height, "OpenGL", nullptr, nullptr);
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
    std::array<float, 16> positions_0 = {
            -50.0f, -50.0f, 0.0f, 0.0f, // 0
             50.0f, -50.0f, 1.0f, 0.0f, // 1
             50.0f,  50.0f, 1.0f, 1.0f, // 2
            -50.0f,  50.0f, 0.0f, 1.0f, // 3
    };

    std::array<unsigned int, 6> indices = {
            0, 1, 2,
            2, 3, 0
    };

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Vertex Buffer
    VertexBuffer vb(positions_0);

    // Vertex Array
    VertexArray va;
    VertexBufferLayout layout;
    layout.Push<float>(2); // location = 0
    layout.Push<float>(2); // location = 1
    va.AddBuffer(vb, layout);

    // Index Buffer
    IndexBuffer ib(indices);

    // GLM
    glm::mat4 proj = glm::ortho(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f);
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
    glm::vec3 translationA(200, 200, 0);
    glm::vec3 translationB(400, 200, 0);

    // Shader
    Shader shader("res/shaders/Basic.glsl");
    shader.Bind();

    // Texture
    Texture texture("res/textures/kitty.jpg");
    texture.Bind();
    shader.SetUniform1i("u_Texture", 0);

    // Renderer
    Renderer renderer;

    // ImGui Initialization
    ImGui::CreateContext();
    // ImGui_ImplXXXX_Init() for each backend
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark();

    // Set time and Frames for Getting Frametime
    double lastTime = glfwGetTime();
    int nbFrames = 0;

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(window)) {
        renderer.Clear();

        // Initialize New Frames for ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {   // Draw Object A
            glm::mat4 model = glm::translate(glm::mat4(1.0f), translationA);
            glm::mat4 mvp = proj * view * model;
            shader.SetUniformMat4f("u_MVP", mvp);
            // Draw
            renderer.Draw(va, ib, shader);
        }

        {   // Draw Object B
            glm::mat4 model = glm::translate(glm::mat4(1.0f), translationB);
            glm::mat4 mvp = proj * view * model;
            shader.SetUniformMat4f("u_MVP", mvp);
            // Draw
            renderer.Draw(va, ib, shader);
        }

        // ImGui Debug Window
        // Begin a window session
        ImGui::Begin("Hello, world!");
        // Create a float slider on the window
        ImGui::SliderFloat2("Translation A", &translationA.x, 0.0f, (float)width); 
        ImGui::SliderFloat2("Translation B", &translationB.x, 0.0f, (float)width); 
        // Add text to the window
        ImGui::Text("FPS: %.0f FPS", ImGui::GetIO().Framerate);
        ImGui::Text("Frametime: %.1f ms", 1000.0f / ImGui::GetIO().Framerate);
        // End the window session
        ImGui::End();

        // Render the ImGui Elements to OpenGL
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap front and back buffers
        glfwSwapBuffers(window);

        // Poll for and process events
        glfwPollEvents();
    }

    // Shutdown ImGui (each backend)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

