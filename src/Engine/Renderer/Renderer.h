
#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

class Renderer {
private:

public:
	Renderer();
	~Renderer();
	
    // Member Functions
    void Clear() const;
	void Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const;

	void UpdateViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
	
};

