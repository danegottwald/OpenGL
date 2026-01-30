#pragma once

#include <glad/glad.h>

struct VertexBufferElement
{
   unsigned int  m_type {};
   unsigned int  m_count {};
   unsigned char m_normalized {};
   size_t        m_offset {};

   static constexpr unsigned int GetSizeOfType( unsigned int type )
   {
      switch( type )
      {
         case GL_FLOAT:         return sizeof( GLfloat );
         case GL_UNSIGNED_INT:  return sizeof( GLuint );
         case GL_UNSIGNED_BYTE: return sizeof( GLubyte );
         default:               return 0;
      }
   }
};

class VertexBufferLayout
{
public:
   VertexBufferLayout() = default;

   template< typename T >
   void Push( unsigned int /*count*/ )
   {
      static_assert( sizeof( T ) == 0, "Unsupported type" );
   }

   template<>
   void Push< float >( unsigned int count )
   {
      m_elements.push_back( { GL_FLOAT, count, GL_FALSE, m_stride } );
      m_stride += count * VertexBufferElement::GetSizeOfType( GL_FLOAT );
   }

   template<>
   void Push< unsigned int >( unsigned int count )
   {
      m_elements.push_back( { GL_UNSIGNED_INT, count, GL_FALSE, m_stride } );
      m_stride += count * VertexBufferElement::GetSizeOfType( GL_UNSIGNED_INT );
   }

   template<>
   void Push< unsigned char >( unsigned int count )
   {
      m_elements.push_back( { GL_UNSIGNED_BYTE, count, GL_TRUE, m_stride } );
      m_stride += count * VertexBufferElement::GetSizeOfType( GL_UNSIGNED_BYTE );
   }

   const std::vector< VertexBufferElement >& GetElements() const { return m_elements; }
   unsigned int                              GetStride() const { return m_stride; }

   std::string GetVertexFormat();

private:
   std::vector< VertexBufferElement > m_elements;
   unsigned int                       m_stride { 0 };
};
