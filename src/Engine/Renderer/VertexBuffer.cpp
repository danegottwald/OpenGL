
#include "VertexBuffer.h"

// Generate a Vertex Buffer, bind it, and fill it with the specified data
VertexBuffer::VertexBuffer(const void *data, unsigned int size) {
    // Generates 1 buffer object name (ID) into m_RendererID
    glGenBuffers(1, &m_RendererID);
    // Binds the Vertex Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    // Creates/initializes the bound Vertex Buffer with data of size 'size'
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

// Deletes the Vertex Buffer referenced by its ID
VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &m_RendererID);
}

// Manually bind the Vertex Buffer (ID/m_RendererID) to the OpenGL state
void VertexBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
}

// Unbinds the most recently bound Vertex Buffer
void VertexBuffer::Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
