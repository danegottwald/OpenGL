
#pragma once

#include <GL/glew.h>

#include <memory>

#include "Window.h"
#include "GuiOverlay.h"
#include "../Renderer/Renderer.h"

class Application {
private:
	static Application *s_Instance;
	std::unique_ptr<Window> m_Window;
	std::unique_ptr<Renderer> m_Renderer;
	std::unique_ptr<GuiOverlay> m_Gui;

public:
	Application();
	~Application();

	void Run();
	static Application &Get() { return *s_Instance; }
	Window& GetWindowRef() const { return *m_Window; }
	Renderer& GetRendererRef() const { return *m_Renderer; }
	GuiOverlay& GetGuiRef() const { return *m_Gui; }
	
	// GLFW, GLEW, OpenGL Functions
	void SetOpenGLCoreProfile(unsigned int major, unsigned int minor);
	const GLubyte *GetOpenGLVersion() { return glGetString(GL_VERSION); }
	
};

