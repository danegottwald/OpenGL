
#pragma once

#include <GL/glew.h>

#include <array>

class IndexBuffer {
private:
    // Member Variables
    unsigned int m_RendererID;
    unsigned int m_Count;

public:
    // Constructors
    IndexBuffer(const unsigned int *data, unsigned int count);
    // Overload for std::array use
    template<size_t N>
    explicit IndexBuffer(const std::array<unsigned int, N> &ib);

    // Destructor
    ~IndexBuffer();

    // Member Functions
    void Bind() const;
    void Unbind() const;

    inline unsigned int GetCount() const { return m_Count; }

};

// Constructor template for std::array
template<size_t N>
IndexBuffer::IndexBuffer(const std::array<unsigned int, N> &ib) : m_Count(ib.size()) {
    // Generates 1 buffer object name (ID) into m_RendererID
    glGenBuffers(1, &m_RendererID);
    // Binds the Index Buffer (ID/m_RendererID) to the OpenGL state
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    // Creates/initializes the bound Index Buffer with 'count' indices provided in 'data'
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ib.size() * sizeof(unsigned int), ib.data(), GL_STATIC_DRAW);
}

