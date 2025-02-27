#pragma once

#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray
{
private:
   unsigned int m_VertexArrayID;
   unsigned int m_VertexBufferID;

public:
   VertexArray();
   ~VertexArray();

   void Bind() const;
   void Unbind() const;

   void AddBuffer();
};
