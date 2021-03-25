
#include <iostream>

#include "Engine/Core/Application.h"

int main(int argc, char **argv) {
	Application app;
	std::cout << "vsync: " << app.GetWindowRef().GetVSync() << std::endl;
	app.GetWindowRef().SetVSync(true);
	std::cout << "vsync: " << app.GetWindowRef().GetVSync() << std::endl;

	GLFWwindow* w;
	while (!glfwWindowShouldClose(w = app.GetWindowRef().GetWindow())) {
		// Render here
		glClear(GL_COLOR_BUFFER_BIT);

		// Swap front and back buffers
		glfwSwapBuffers(w);

		// Poll for and process events
		glfwPollEvents();
	}

	return 0;
	
 //   Application app;
 //   app.SetOpenGLCoreProfile(3, 3);

 //   if (!app.CreateWindow(1280, 720, "OpenGL")) {
 //       return -1;
 //   }

 //   app.VerticalSync(true);
 //   if (!app.InitWindow()) {
 //       return -2;
 //   }

 //   std::array<float, 6> positions = {
 //       -0.5f, -0.5f,
 //       0.5f, -0.5f,
 //       0.0f, 0.5f
 //   };

 //   std::array<unsigned int, 3> indices = {
 //       0, 1, 2
 //   };

 //   VertexBuffer vb(positions);
 //   VertexArray va;
 //   VertexBufferLayout layout;
 //   layout.Push<float>(2);
 //   va.AddBuffer(vb, layout);
 //   IndexBuffer ib(indices);
 //   Shader shader("BasicTwo.glsl");
	//shader.Bind();
	//
 //   // Print Version and Set Debug Mode
 //   std::cout << app.GetOpenGLVersion() << std::endl;

 //   // Renderer
 //   Renderer renderer;
 //   renderer.EnableBlending(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

 //   // ImGui Initialization for each backend
 //   app.InitGui("#version 330");

 //   // Loop until the user closes the window
 //   while (app.WindowIsOpen()) {
 //       renderer.Clear();

 //       app.CreateGuiFrame();

 //       renderer.Draw(va, ib, shader);

 //       app.RenderGui();
 //       renderer.Present();
 //   }

 //   return 0;
}

