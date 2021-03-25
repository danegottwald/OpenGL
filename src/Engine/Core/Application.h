
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

#include "Window.h"

#include "imgui/imgui.h"

class Application {
private:
	static Application *s_Instance;
	Window m_Window;

	void Init();

public:
	Application();
	~Application();

	void Run();

	// Get Reference to Window Object
	Window &GetWindowRef() { return *&m_Window; }

	// GLFW, GLEW, OpenGL Functions
	void SetOpenGLCoreProfile(unsigned int majorVersion, unsigned int minorVersion);
	//bool WindowIsOpen() { return !glfwWindowShouldClose(m_Window); }

	// ImGui Functions
	void InitGui(const std::string &glslVersion);
	void CreateGuiFrame();
	void RenderGui();

	// Getters/Setters
	const GLubyte *GetOpenGLVersion() { return glGetString(GL_VERSION); }
	
};

