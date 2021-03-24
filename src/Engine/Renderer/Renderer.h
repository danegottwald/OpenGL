
#pragma once

#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

class Renderer {
private:

public:
    // Member Functions
    void Draw(const VertexArray &va, const IndexBuffer &ib, const Shader &shader) const;
    void Clear() const;
    void EnableBlending();
    void BlendFunction(GLenum sFactor, GLenum dFactor);

};


