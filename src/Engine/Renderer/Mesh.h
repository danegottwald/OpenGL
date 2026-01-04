#pragma once

// Project dependencies
#include <Engine/Renderer/VertexBufferLayout.h>
#include <Engine/Renderer/Texture.h>

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

   inline unsigned int GetVertexArrayID() const { return m_vertexArrayID; }
   inline unsigned int GetVertexBufferID() const { return m_vertexBufferID; }
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

   const MeshBuffer& GetMeshBuffer() { return m_meshBuffer; }

protected:
   // Compiles the mesh data into the MeshBuffer
   virtual void Compile() = 0;

   MeshBuffer m_meshBuffer;
   glm::vec3  m_color { 0.5f, 0.0f, 0.0f };
};

class CapsuleMesh : public IMesh
{
public:
   CapsuleMesh( float radius = 0.5f, float height = 2.0f, int segments = 24, int rings = 12 )
   {
      m_meshBuffer.Initialize();
      GenerateCapsule( radius, height, segments, rings );
      Compile();
   }

   void Compile() override
   {
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      m_meshBuffer.SetVertexData( m_vertices, std::move( layout ) );
      m_meshBuffer.SetIndexData( m_indices );
      m_meshBuffer.Unbind();
   }

private:
   std::vector< float >        m_vertices;
   std::vector< unsigned int > m_indices;

   void GenerateCapsule( float radius, float height, int segments, int rings )
   {
      m_vertices.clear();
      m_indices.clear();

      // `height` is the full capsule height (top pole to bottom pole).
      // The cylinder portion can become negative if height < 2*radius; clamp to 0.
      float halfHeight   = ( std::max )( 0.0f, height - 2.0f * radius ) * 0.5f;
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
         int topEnd   = hemiRings;                             // inclusive
         int cylEnd   = topEnd + ( std::max )( 0, rings - 1 ); // inclusive
         int botStart = cylEnd + 1;

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
   SphereMesh( float radius = 0.5f, int segments = 24, int rings = 12 )
   {
      m_meshBuffer.Initialize();
      GenerateSphere( radius, segments, rings );
      Compile();
   }

   void Compile() override
   {
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      m_meshBuffer.SetVertexData( m_vertices, std::move( layout ) );
      m_meshBuffer.SetIndexData( m_indices );
      m_meshBuffer.Unbind();
   }

private:
   std::vector< float >        m_vertices;
   std::vector< unsigned int > m_indices;

   void GenerateSphere( float radius, int segments, int rings )
   {
      m_vertices.clear();
      m_indices.clear();

      // Vertices (sphere is centered at origin)
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
   CubeMesh( float size = 1.0f ) :
      m_size( size )
   {
      m_meshBuffer.Initialize();
      Compile();
   }

   void Compile() override
   {
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal

      // Scale unit cube positions by `m_size` but keep normals unchanged.
      std::vector< float > scaled;
      scaled.reserve( m_vertices.size() );

      for( size_t i = 0; i < m_vertices.size(); i += m_vertexSize )
      {
         scaled.push_back( m_vertices[ i + 0 ] * m_size );
         scaled.push_back( m_vertices[ i + 1 ] * m_size );
         scaled.push_back( m_vertices[ i + 2 ] * m_size );

         scaled.push_back( m_vertices[ i + 3 ] );
         scaled.push_back( m_vertices[ i + 4 ] );
         scaled.push_back( m_vertices[ i + 5 ] );
      }

      m_meshBuffer.SetVertexData( scaled, std::move( layout ) );
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
   float m_size;
};

class BlockItemMesh : public IMesh
{
public:
   BlockItemMesh( BlockId blockId, float size = 0.25f ) :
      m_blockId( blockId ),
      m_size( size )
   {
      m_meshBuffer.Initialize();
      Compile();
   }

   void Compile() override
   {
      m_meshBuffer.Bind();

      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal
      layout.Push< float >( 2 ); // UV
      layout.Push< float >( 3 ); // Tint

      std::vector< float > vertices;
      vertices.reserve( 24 * ( 3 + 3 + 2 + 3 ) ); // 24 verts * 11 floats

      // Helper to add a face
      auto addFace =
         [ & ]( TextureAtlas::BlockFace face, const glm::vec3& normal, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3 )
      {
         const TextureAtlas::Region& region = TextureAtlasManager::Get().GetRegion( m_blockId, face );

         auto addVert = [ & ]( const glm::vec3& p, const glm::vec2& uv )
         {
            vertices.push_back( p.x * m_size );
            vertices.push_back( p.y * m_size );
            vertices.push_back( p.z * m_size );
            vertices.push_back( normal.x );
            vertices.push_back( normal.y );
            vertices.push_back( normal.z );
            vertices.push_back( uv.x );
            vertices.push_back( uv.y );
            vertices.push_back( 1.0f ); // Tint R
            vertices.push_back( 1.0f ); // Tint G
            vertices.push_back( 1.0f ); // Tint B
         };

         // Vertices order for quad: 0, 1, 2, 3
         addVert( p0, region.uvs[ 0 ] );
         addVert( p1, region.uvs[ 1 ] );
         addVert( p2, region.uvs[ 2 ] );
         addVert( p3, region.uvs[ 3 ] );
      };

      // Cube corners
      glm::vec3 p0( -0.5f, -0.5f, 0.5f );  // Front BL
      glm::vec3 p1( 0.5f, -0.5f, 0.5f );   // Front BR
      glm::vec3 p2( 0.5f, 0.5f, 0.5f );    // Front TR
      glm::vec3 p3( -0.5f, 0.5f, 0.5f );   // Front TL
      glm::vec3 p4( -0.5f, -0.5f, -0.5f ); // Back BL
      glm::vec3 p5( 0.5f, -0.5f, -0.5f );  // Back BR
      glm::vec3 p6( 0.5f, 0.5f, -0.5f );   // Back TR
      glm::vec3 p7( -0.5f, 0.5f, -0.5f );  // Back TL

      addFace( TextureAtlas::BlockFace::South, { 0, 0, 1 }, p0, p1, p2, p3 );   // Front face (Z+)
      addFace( TextureAtlas::BlockFace::North, { 0, 0, -1 }, p5, p4, p7, p6 );  // Back face (Z-)
      addFace( TextureAtlas::BlockFace::East, { 1, 0, 0 }, p1, p5, p6, p2 );    // Right face (X+)
      addFace( TextureAtlas::BlockFace::West, { -1, 0, 0 }, p4, p0, p3, p7 );   // Left face (X-)
      addFace( TextureAtlas::BlockFace::Top, { 0, 1, 0 }, p3, p2, p6, p7 );     // Top face (Y+)
      addFace( TextureAtlas::BlockFace::Bottom, { 0, -1, 0 }, p4, p5, p1, p0 ); // Bottom face (Y-)

      m_meshBuffer.SetVertexData( vertices, std::move( layout ) );

      // Indices (standard quad indices for 6 faces)
      std::vector< unsigned int > indices;
      for( int i = 0; i < 6; ++i )
      {
         unsigned int base = i * 4;
         indices.push_back( base + 0 );
         indices.push_back( base + 1 );
         indices.push_back( base + 2 );
         indices.push_back( base + 0 );
         indices.push_back( base + 2 );
         indices.push_back( base + 3 );
      }
      m_meshBuffer.SetIndexData( indices );

      m_meshBuffer.Unbind();
   }

private:
   BlockId m_blockId;
   float   m_size;
};
