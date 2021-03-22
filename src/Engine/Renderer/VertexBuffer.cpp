
#include "VertexBuffer.h"

#include "Renderer.h"

VertexBuffer::VertexBuffer(const void *data, unsigned int size) {
    // Generates 1 buffer object name (ID) into m_RendererID
    glGenBuffers(1, &m_RendererID);
    // Binds the Vertex Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    // Creates/initializes the bound Vertex Buffer with data of size 'size'
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::~VertexBuffer() {
    // Deletes the Vertex Buffer referenced by its ID
    glDeleteBuffers(1, &m_RendererID);
}

void VertexBuffer::Bind() const {
    // Manually bind the Vertex Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

void VertexBuffer::Unbind() const {
    // Unbinds the most recently bound Vertex Buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
