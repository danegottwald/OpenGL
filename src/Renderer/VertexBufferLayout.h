#pragma once

struct VertexBufferElement
{
   uint32_t m_type { 0 };
   uint32_t m_count { 0 };
   uint8_t  m_normalized { 0 };
   uint64_t m_offset { 0 };

   static unsigned int GetSizeOfType( unsigned int type )
   {
      switch( type )
      {
         case GL_FLOAT:         return sizeof( GLfloat );
         case GL_UNSIGNED_INT:  return sizeof( GLuint );
         case GL_UNSIGNED_BYTE: return sizeof( GLubyte );
         default:               assert( false && "Unknown type" ); return 0;
      }
   }
};

class VertexBufferLayout
{
public:
   VertexBufferLayout() = default;

   template< typename T >
   void Push( unsigned int count )
   {
      static_assert( true );
   }

   inline const std::vector< VertexBufferElement >& GetElements() const { return m_elements; }
   inline unsigned int                              GetStride() const { return m_stride; }
   std::string                                      GetVertexFormat();

private:
   std::vector< VertexBufferElement > m_elements;
   unsigned int                       m_stride { 0 };
};

template<>
inline void VertexBufferLayout::Push< float >( unsigned int count )
{
   // The stride can be used as the offset prior to the add
   m_elements.push_back( { GL_FLOAT, count, GL_FALSE, m_stride /*offset*/ } );
   m_stride += count * VertexBufferElement::GetSizeOfType( GL_FLOAT );
}

template<>
inline void VertexBufferLayout::Push< unsigned int >( unsigned int count )
{
   m_elements.push_back( { GL_UNSIGNED_INT, count, GL_FALSE, m_stride /*offset*/ } );
   m_stride += count * VertexBufferElement::GetSizeOfType( GL_UNSIGNED_INT );
}

template<>
inline void VertexBufferLayout::Push< unsigned char >( unsigned int count )
{
   m_elements.push_back( { GL_UNSIGNED_BYTE, count, GL_TRUE, m_stride /*offset*/ } );
   m_stride += count * VertexBufferElement::GetSizeOfType( GL_UNSIGNED_BYTE );
}
