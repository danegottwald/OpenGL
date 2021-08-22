//#include <GL/glew.h>
//#include <GLFW/glfw3.h>
//
//#include <iostream>
//
//#include "../Engine/Core/Application.h"
//#include "../Engine/Renderer/Renderer.h"
//
//#include "ClearColor.h"
//#include "RenderTexture.h"
//
//void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
//    if (key == GLFW_KEY_P && action == GLFW_PRESS) { // Flip flop wireframe and fillframe
//        static bool wireMode = true;
//        if (wireMode) {
//            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//            wireMode = false;
//        }
//        else {
//            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//            wireMode = true;
//        }
//    }
//}
//
//int main(int argc, char** argv) {
//    Application app;
//    app.SetOpenGLCoreProfile(3, 3);
//
//    if (!app.CreateWindow(1280, 720, "OpenGL")) {
//        return -1;
//    }
//
//    app.VerticalSync(true);
//    if (!app.InitWindow()) {
//        return -2;
//    }
//
//    // Print Version and Set Debug Mode
//    std::cout << app.GetOpenGLVersion() << std::endl;
//
//    // Renderer
//    Renderer renderer;
//    renderer.EnableBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    // ImGui Initialization for each backend
//    app.CreateGui("#version 330");
//
//    // Setup Test Environment
//    TestSpace::Test* currentTest = nullptr;
//    TestSpace::TestMenu* testMenu = new TestSpace::TestMenu(currentTest);
//    currentTest = testMenu;
//
//    // Add Tests
//    testMenu->RegisterTest<TestSpace::ClearColor>("Clear Color");
//    testMenu->RegisterTest<TestSpace::RenderTexture>("Render Texture");
//
//    // Loop until the user closes the window
//    while (app.WindowIsOpen()) {
//        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//        renderer.Clear();
//
//        app.CreateGuiFrame();
//        if (currentTest) {
//            currentTest->OnUpdate(0.0f);
//            currentTest->OnRender();
//            ImGui::Begin("Test");
//            if (currentTest != testMenu && ImGui::Button("<-")) {
//                delete currentTest;
//                currentTest = testMenu;
//            }
//            currentTest->OnImGuiRender();
//            ImGui::End();
//        }
//
//        app.RenderGui();
//        app.UpdateWindow();
//    }
//
//    delete currentTest;
//	if (currentTest != testMenu) {
//        delete testMenu;
//	}
//
//    return 0;
//}
