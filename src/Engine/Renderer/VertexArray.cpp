
#include "VertexArray.h"

VertexArray::VertexArray() {
    // Generate a Vertex Array object and store its ID
    glGenVertexArrays(1, &m_RendererID);
}

// Delete the Vertex Array object specified by m_RendererID
VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &m_RendererID);
}

// Binds the Vertex Array object
void VertexArray::Bind() const {
    glBindVertexArray(m_RendererID);
}

// Unbind/break the currently bound Vertex Array
void VertexArray::Unbind() const {
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
        // Enable the Vertex Attribute Array by the specified by the index value
        glEnableVertexAttribArray(i);
        // Set up the Vertex Attribute data for the enabled Vertex Attribute Array
        glVertexAttribPointer(i, element.count, element.type, element.normalized,
                              layout.GetStride(), reinterpret_cast<const void *>(offset));
        // Calculate the offset for the next attribute from the first value in the Vertex Buffer
        offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
    }
}

