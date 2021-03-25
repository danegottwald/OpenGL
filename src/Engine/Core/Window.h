
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string>

struct WindowData {
	std::string Name;
	unsigned int Width, Height;
	bool VSync;
};

class Window {
private:
	GLFWwindow *m_Window;
	WindowData m_Data;

	void Init(const WindowData &props);
	void SetCallbacks() const;

public:
	explicit Window(const WindowData &props);
	~Window();

	void Update() const;

	GLFWwindow* GetWindow() const { return m_Window; }

	// Attribute Functions
	std::string GetName() const { return m_Data.Name; }
	unsigned int GetWidth() const { return m_Data.Width; }
	unsigned int GetHeight() const { return m_Data.Height; }
	bool GetVSync() const { return m_Data.VSync; }
	
	void SetVSync(bool state);
	
};

