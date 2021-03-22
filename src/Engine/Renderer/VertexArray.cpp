
#include "VertexArray.h"
#include <iostream>

VertexArray::VertexArray() {
    // Generate a Vertex Array object and store its ID
    glGenVertexArrays(1, &m_RendererID);
}

VertexArray::~VertexArray() {
    // Delete the Vertex Array object specified by m_RendererID
    glDeleteVertexArrays(1, &m_RendererID);
}

void VertexArray::Bind() const {
    // Binds the Vertex Array object
    glBindVertexArray(m_RendererID);
}

void VertexArray::Unbind() const {
    // Unbind/break the currently bound Vertex Array
    glBindVertexArray(0);
}

void VertexArray::AddBuffer(const VertexBuffer &vb, const VertexBufferLayout &layout) {
    // Binds 'this' Vertex Array
    Bind();
    // Binds the Vertex Buffer object
    vb.Bind();
    unsigned int offset = 0;
    const auto &elements = layout.GetElements();
    for (unsigned int i = 0; i < elements.size(); i++) {
        const auto &element = elements[i];
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, element.count, element.type, element.normalized,
                              layout.GetStride(), reinterpret_cast<const void *>(offset));
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }
}
