#pragma once

// Project dependencies
#include <Renderer/VertexBufferLayout.h>

class MeshBuffer
{
public:
   MeshBuffer() = default;
   ~MeshBuffer() { Cleanup(); }

   void Initialize()
   {
      Cleanup(); // Ensure previous buffers are deleted before reinitialization

      glGenVertexArrays( 1, &m_vertexArrayID );
      glGenBuffers( 1, &m_vertexBufferID );
      glGenBuffers( 1, &m_indexBufferID );
   }

   void Bind() const { glBindVertexArray( m_vertexArrayID ); }
   void Unbind() const { glBindVertexArray( 0 ); }

   // VertexBuffer Functions
   template< typename T >
   void SetVertexData( const T* verticesData, size_t verticesCount, VertexBufferLayout&& layout )
   {
      m_vertexBufferLayout = std::move( layout );
      glBindVertexArray( m_vertexArrayID );
      glBindBuffer( GL_ARRAY_BUFFER, m_vertexBufferID );
      glBufferData( GL_ARRAY_BUFFER, verticesCount * sizeof( T ), verticesData, GL_STATIC_DRAW );

      // Apply layout
      const std::vector< VertexBufferElement >& elements = m_vertexBufferLayout.GetElements();
      for( unsigned int i = 0; i < elements.size(); i++ )
      {
         const VertexBufferElement& element = elements[ i ];
         glEnableVertexAttribArray( i );
         glVertexAttribPointer( i, element.m_count, element.m_type, element.m_normalized, m_vertexBufferLayout.GetStride(), ( const void* )element.m_offset );
      }

      glBindBuffer( GL_ARRAY_BUFFER, 0 );
      glBindVertexArray( 0 );
   }

   template< typename T, size_t N >
   void SetVertexData( const std::array< T, N >& vertices, VertexBufferLayout&& layout )
   {
      SetVertexData( vertices.data(), vertices.size(), std::move( layout ) );
   }

   template< typename T >
   void SetVertexData( const std::vector< T >& vertices, VertexBufferLayout&& layout )
   {
      SetVertexData( vertices.data(), vertices.size(), std::move( layout ) );
   }

   // IndexBuffer Functions
   void SetIndexData( const unsigned int* indicesData, size_t indicesCount )
   {
      m_indexCount = static_cast< unsigned int >( indicesCount );
      glBindVertexArray( m_vertexArrayID );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_indexBufferID );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, indicesCount * sizeof( unsigned int ), indicesData, GL_STATIC_DRAW );
      glBindVertexArray( 0 );
   }

   template< size_t N >
   void SetIndexData( const std::array< unsigned int, N >& indices )
   {
      SetIndexData( indices.data(), indices.size() );
   }

   void SetIndexData( const std::vector< unsigned int >& indices ) { SetIndexData( indices.data(), indices.size() ); }

   inline unsigned int GetIndexCount() const { return m_indexCount; }

private:
   void Cleanup()
   {
      if( m_vertexArrayID )
         glDeleteVertexArrays( 1, &m_vertexArrayID );
      if( m_vertexBufferID )
         glDeleteBuffers( 1, &m_vertexBufferID );
      if( m_indexBufferID )
         glDeleteBuffers( 1, &m_indexBufferID );

      m_vertexArrayID  = 0;
      m_vertexBufferID = 0;
      m_indexBufferID  = 0;
      m_indexCount     = 0;
   }

   unsigned int       m_vertexArrayID { 0 };
   unsigned int       m_vertexBufferID { 0 };
   unsigned int       m_indexBufferID { 0 };
   unsigned int       m_indexCount { 0 };
   VertexBufferLayout m_vertexBufferLayout;
};

class IMesh
{
public:
   IMesh()          = default;
   virtual ~IMesh() = default;

   void Render() const
   {
      m_meshBuffer.Bind();
      glDrawElements( GL_TRIANGLES, m_meshBuffer.GetIndexCount(), GL_UNSIGNED_INT, nullptr );
      m_meshBuffer.Unbind();
   }

   void SetColor( int hexColor )
   {
      m_color = glm::vec3( ( ( hexColor >> 16 ) & 0xFF ) / 255.0f, ( ( hexColor >> 8 ) & 0xFF ) / 255.0f, ( hexColor & 0xFF ) / 255.0f );
   }
   void      SetColor( const glm::vec3& color ) { m_color = color; }
   glm::vec3 GetColor() const { return m_color; }

   virtual void SetPosition( const glm::vec3& position ) = 0;
   glm::vec3    GetPosition() { return m_position; }

protected:
   MeshBuffer m_meshBuffer;
   glm::vec3  m_position { 0.0f, 0.0f, 0.0f };
   glm::vec3  m_color { 0.5f, 0.0f, 0.0f };
};

class CubeMesh : public IMesh
{
public:
   CubeMesh( const glm::vec3& position = glm::vec3( 0.0f ) )
   {
      m_position = position;
      m_meshBuffer.Initialize();
      SetPosition( position );
   }

   void SetPosition( const glm::vec3& position )
   {
      m_position = position;
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      if( position != glm::vec3( 0.0f ) )
      {
         std::array< float, m_verticesSize > newVertices = m_vertices;
         for( size_t i = 0; i < m_vertices.size(); i += m_vertexSize )
         {
            newVertices[ i ] += position.x;
            newVertices[ i + 1 ] += position.y;
            newVertices[ i + 2 ] += position.z;
         }
         m_meshBuffer.SetVertexData( newVertices, std::move( layout ) );
      }
      else
         m_meshBuffer.SetVertexData( m_vertices, std::move( layout ) );

      m_meshBuffer.SetIndexData( m_indices );
      m_meshBuffer.Unbind();
   }

private:
   // clang-format off
   static constexpr size_t m_vertexSize   = 6; // (3 position, 3 normal)
   static constexpr size_t m_verticesSize = 144;
   static constexpr std::array< float, m_verticesSize > m_vertices = {
         // Positions          // Normals
         // Front face
         -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v1
          0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v2
          0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v3
         -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  // v4

         // Back face
         -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v5
         -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v6
          0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v7
          0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // v8

         // Left face
         -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  // v9
         -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  // v10
         -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  // v11
         -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  // v12

         // Right face
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  // v13
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  // v14
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  // v15
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  // v16

         // Top face
         -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  // v17
         -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  // v18
          0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  // v19
          0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  // v20

         // Bottom face
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  // v21
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  // v22
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  // v23
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f   // v24
   };

   static constexpr std::array< unsigned int, 36 > m_indices = {
      // Front face
      0, 1, 2, 2, 3, 0,
      // Back face
      4, 5, 6, 6, 7, 4,
      // Left face
      8, 9, 10, 10, 11, 8,
      // Right face
      12, 13, 14, 14, 15, 12,
      // Top face
      16, 17, 18, 18, 19, 16,
      // Bottom face
      20, 21, 22, 22, 23, 20
   };
   // clang-format on
};
