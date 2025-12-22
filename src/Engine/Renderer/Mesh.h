#pragma once

// Project dependencies
#include <Engine/Renderer/VertexBufferLayout.h>

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

   virtual float GetCenterToBottomDistance() const { return 0.0f; }

protected:
   MeshBuffer m_meshBuffer;
   glm::vec3  m_position { 0.0f, 0.0f, 0.0f };
   glm::vec3  m_color { 0.5f, 0.0f, 0.0f };
};

class CapsuleMesh : public IMesh
{
public:
   CapsuleMesh( const glm::vec3& position = glm::vec3( 0.0f ), float radius = 0.5f, float height = 2.0f, int segments = 24, int rings = 12 ) :
      m_radius( radius ),
      m_height( height )
   {
      m_position = position;
      m_meshBuffer.Initialize();
      GenerateCapsule( radius, height, segments, rings );
      SetPosition( position );
   }

   void SetPosition( const glm::vec3& position ) override
   {
      m_position = position;
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      // Offset vertices by position
      std::vector< float > offsetVertices = m_vertices;
      for( size_t i = 0; i < offsetVertices.size(); i += 6 )
      {
         offsetVertices[ i ] += position.x;
         offsetVertices[ i + 1 ] += position.y;
         offsetVertices[ i + 2 ] += position.z;
      }
      m_meshBuffer.SetVertexData( offsetVertices, std::move( layout ) );
      m_meshBuffer.SetIndexData( m_indices );
      m_meshBuffer.Unbind();
   }

   float GetCenterToBottomDistance() const override { return ( m_height * 0.5f ) + m_radius; }

private:
   std::vector< float >        m_vertices;
   std::vector< unsigned int > m_indices;
   float                       m_radius;
   float                       m_height;

   void GenerateCapsule( float radius, float height, int segments, int rings )
   {
      m_vertices.clear();
      m_indices.clear();

      float halfHeight   = ( height - 2.0f * radius ) * 0.5f;
      int   hemiRings    = rings / 2; // rings for each hemisphere
      int   vertsPerRing = segments + 1;

      // Total rings laid out top->bottom:
      //  top hemisphere:        (hemiRings + 1) rings, includes top seam at end
      //  cylinder (interior):   (rings - 1) rings, excludes both seams
      //  bottom hemisphere:     (hemiRings + 1) rings, starts at seam
      int totalRings = ( hemiRings + 1 ) + ( std::max )( 0, rings - 1 ) + ( hemiRings + 1 );

      auto addVertex = [ & ]( float px, float py, float pz, float nx, float ny, float nz )
      {
         m_vertices.push_back( px );
         m_vertices.push_back( py );
         m_vertices.push_back( pz );
         m_vertices.push_back( nx );
         m_vertices.push_back( ny );
         m_vertices.push_back( nz );
      };

      // ---- Generate all rings (top -> bottom) ----
      for( int y = 0; y < totalRings; ++y )
      {
         // Decide which section we're in
         int topEnd   = hemiRings; // inclusive
         int cylStart = topEnd + 1;
         int cylEnd   = topEnd + ( std::max )( 0, rings - 1 ); // inclusive
         int botStart = cylEnd + 1;
         // int botEnd = botStart + hemiRings; // (unused) inclusive

         for( int x = 0; x <= segments; ++x )
         {
            float u     = float( x ) / segments;
            float theta = glm::two_pi< float >() * u;
            float sinT  = sinf( theta );
            float cosT  = cosf( theta );

            float px, py, pz;
            float nx, ny, nz;

            if( y <= topEnd )
            {
               // Top hemisphere: phi 0..pi/2 (0 at pole, pi/2 at seam)
               float v      = ( hemiRings == 0 ) ? 1.0f : float( y ) / float( hemiRings );
               float phi    = glm::half_pi< float >() * v;
               float sinPhi = sinf( phi );
               float cosPhi = cosf( phi );

               nx = cosT * sinPhi;
               ny = cosPhi;
               nz = sinT * sinPhi;

               px = radius * nx;
               py = halfHeight + radius * ny; // seam at +halfHeight when ny=0
               pz = radius * nz;
            }
            else if( y <= cylEnd )
            {
               // Cylinder interior: skip both seam rings
               // Map to v in (0,1): use (cy+1)/rings where cy in [0..rings-2]
               int   cy = y - ( topEnd + 1 );
               float v  = float( cy + 1 ) / float( rings ); // (0,1)
               py       = halfHeight - v * ( 2.0f * halfHeight );

               nx = cosT;
               ny = 0.0f;
               nz = sinT;

               px = radius * nx;
               pz = radius * nz;
            }
            else
            {
               // Bottom hemisphere: start at seam and go to bottom pole
               // Use phi in [pi/2 .. 0]: phi = (1 - v) * pi/2, v in [0..1]
               int   by    = y - botStart;
               float v     = ( hemiRings == 0 ) ? 1.0f : float( by ) / float( hemiRings );
               float phi   = glm::half_pi< float >() * ( 1.0f - v );
               float sinPh = sinf( phi );
               float cosPh = cosf( phi );

               nx = cosT * sinPh;
               ny = -cosPh; // points downward
               nz = sinT * sinPh;

               px = radius * nx;
               py = -halfHeight + radius * ny; // seam at -halfHeight when ny=0
               pz = radius * nz;
            }

            addVertex( px, py, pz, nx, ny, nz );
         }
      }

      // ---- Indices (shared for all sections) ----
      for( int y = 0; y < totalRings - 1; ++y )
      {
         int row0 = y * vertsPerRing;
         int row1 = ( y + 1 ) * vertsPerRing;

         for( int x = 0; x < segments; ++x )
         {
            int i0 = row0 + x;
            int i1 = row1 + x;
            int i2 = row1 + x + 1;
            int i3 = row0 + x + 1;

            // CCW winding facing outward
            m_indices.push_back( i0 );
            m_indices.push_back( i2 );
            m_indices.push_back( i1 );

            m_indices.push_back( i0 );
            m_indices.push_back( i3 );
            m_indices.push_back( i2 );
         }
      }
   }
};


class SphereMesh : public IMesh
{
public:
   SphereMesh( const glm::vec3& position = glm::vec3( 0.0f ), float radius = 0.5f, int segments = 24, int rings = 12 ) :
      m_radius( radius )
   {
      m_position = position;
      m_meshBuffer.Initialize();
      GenerateSphere( radius, segments, rings );
      SetPosition( position );
   }

   void SetPosition( const glm::vec3& position ) override
   {
      m_position = position;
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      // Offset vertices by position
      std::vector< float > offsetVertices = m_vertices;
      for( size_t i = 0; i < offsetVertices.size(); i += 6 )
      {
         offsetVertices[ i ] += position.x;
         offsetVertices[ i + 1 ] += position.y;
         offsetVertices[ i + 2 ] += position.z;
      }
      m_meshBuffer.SetVertexData( offsetVertices, std::move( layout ) );
      m_meshBuffer.SetIndexData( m_indices );
      m_meshBuffer.Unbind();
   }

   float GetCenterToBottomDistance() const override { return m_radius; }

private:
   std::vector< float >        m_vertices;
   std::vector< unsigned int > m_indices;
   float                       m_radius;

   void GenerateSphere( float radius, int segments, int rings )
   {
      m_vertices.clear();
      m_indices.clear();

      // Vertices
      for( int y = 0; y <= rings; ++y )
      {
         float v      = float( y ) / float( rings );
         float phi    = glm::pi< float >() * v;
         float sinPhi = sinf( phi );
         float cosPhi = cosf( phi );

         for( int x = 0; x <= segments; ++x )
         {
            float u        = float( x ) / float( segments );
            float theta    = glm::two_pi< float >() * u;
            float sinTheta = sinf( theta );
            float cosTheta = cosf( theta );

            float nx = cosTheta * sinPhi;
            float ny = cosPhi;
            float nz = sinTheta * sinPhi;

            float px = radius * nx;
            float py = radius * ny;
            float pz = radius * nz;

            m_vertices.push_back( px );
            m_vertices.push_back( py );
            m_vertices.push_back( pz );
            m_vertices.push_back( nx );
            m_vertices.push_back( ny );
            m_vertices.push_back( nz );
         }
      }

      // Indices
      int vertsPerRow = segments + 1;
      for( int y = 0; y < rings; ++y )
      {
         for( int x = 0; x < segments; ++x )
         {
            int i0 = y * vertsPerRow + x;
            int i1 = ( y + 1 ) * vertsPerRow + x;
            int i2 = ( y + 1 ) * vertsPerRow + ( x + 1 );
            int i3 = y * vertsPerRow + ( x + 1 );

            m_indices.push_back( i0 );
            m_indices.push_back( i2 );
            m_indices.push_back( i1 );

            m_indices.push_back( i0 );
            m_indices.push_back( i3 );
            m_indices.push_back( i2 );
         }
      }
   }
};

class CubeMesh : public IMesh
{
public:
   CubeMesh( const glm::vec3& position = glm::vec3( 0.0f ), float size = 1.0f ) :
      m_size( size )
   {
      m_position = position;
      m_meshBuffer.Initialize();
      SetPosition( position );
   }

   void SetPosition( const glm::vec3& position ) override
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

   float GetCenterToBottomDistance() const override { return m_size * 0.5f; }

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
   float m_size;
};
