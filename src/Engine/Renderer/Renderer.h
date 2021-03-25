
#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "../Engine/Core/Application.h"
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

class Renderer : private Application {
private:

public:
    // Member Functions
    void Clear() const;
	void Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const;
	void Present();
	
	void EnableBlending(GLenum sFactor, GLenum dFactor);

};

