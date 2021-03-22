
#include "IndexBuffer.h"

#include "Renderer.h"

IndexBuffer::IndexBuffer(const unsigned int *data, unsigned int count) : m_Count(count) {
    // Generates 1 buffer object name (ID) into m_RendererID
    glGenBuffers(1, &m_RendererID);
    // Binds the Index Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    // Creates/initializes the bound Index Buffer with 'count' indices provided in 'data'
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() {
    // Deletes the Index Buffer referenced by its ID
    glDeleteBuffers(1, &m_RendererID);
}

void IndexBuffer::Bind() const {
    // Manually bind the Index Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
}

void IndexBuffer::Unbind() const {
    // Unbinds the most recently bound Index Buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
