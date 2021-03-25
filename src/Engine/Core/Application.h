
#pragma once

#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "imgui/imgui.h"

class Application {
private:
	GLFWwindow *m_Window;
	std::string m_WindowName;
	unsigned int m_WindowWidth;
	unsigned int m_WindowHeight;

public:
	Application();
	~Application();

	// GLFW, GLEW, OpenGL Functions
	void SetOpenGLCoreProfile(unsigned int majorVersion, unsigned int minorVersion);
	bool CreateWindow(unsigned int width, unsigned int height, const std::string& name);
	void VerticalSync(bool state);
	bool InitWindow(bool debug = false);
	void UpdateWindow();
	inline bool WindowIsOpen() { return !glfwWindowShouldClose(m_Window); }

	// ImGui Functions
	void CreateGui(const std::string &glslVersion);
	void CreateGuiFrame();
	void RenderGui();

	// Getters/Setters
	inline const GLubyte *GetOpenGLVersion() { return glGetString(GL_VERSION); }

};
