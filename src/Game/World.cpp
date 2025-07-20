#include "World.h"
#include "../Events/NetworkEvent.h"
#include "../Input/Input.h"

#include "Camera.h"
#include "Mesh.h"
#include "Renderer/Shader.h"
#include "Entity/Player.h"


std::unique_ptr< Shader > pShader;
Events::EventSubscriber   m_eventSubscriber;

World::World()
{
   pShader = std::make_unique< Shader >();

   m_eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ & ]( const Events::NetworkClientConnectEvent& e ) noexcept
   {
      const uint64_t clientID    = e.GetClientID();
      m_otherPlayers[ clientID ] = std::make_shared< CubeMesh >();
      m_otherPlayers[ clientID ]->SetColor( glm::vec3( 0.0f, 0.0f, 1.0f ) );
   } );
   m_eventSubscriber.Subscribe< Events::NetworkClientDisconnectEvent >( [ & ]( const Events::NetworkClientDisconnectEvent& e ) noexcept
   {
      if( auto it = m_otherPlayers.find( e.GetClientID() ); it != m_otherPlayers.end() )
         m_otherPlayers.erase( it );
   } );
   m_eventSubscriber.Subscribe< Events::NetworkPositionUpdateEvent >( [ this ]( const Events::NetworkPositionUpdateEvent& e ) noexcept
   { this->ReceivePlayerPosition( e.GetClientID(), e.GetPosition() ); } );
}

World::~World()
{
   m_fChunkThreadTerminate.store( true );
   if( m_chunkThread.joinable() )
      m_chunkThread.join();
}

class Chunk
{
public:
   Chunk( const glm::vec3& position ) noexcept :
      m_position( position )
   {}

   static constexpr uint8_t GetSize() noexcept { return 16; }

private:
   // Chunks only are positioned on the XZ plane (Y is always 0)
   glm::vec3 m_position { 0.0f };
};

enum class BlockFace
{
   North,
   South,
   East,
   West,
   Up,
   Down
};

class ChunkMesh : public IMesh
{
public:
   ChunkMesh( const glm::vec3& position = glm::vec3( 0.0f ) )
   {
      m_position = position;
      SetPosition( position );
   }

   void SetPosition( const glm::vec3& /*position*/ ) override {}

   void AddCube( const glm::vec3& cubePos )
   {
      // clang-format off
      std::array<Vertex, 24> cubeVertices = {{
         // Front Face
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {0.0f, 0.0f, 1.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {0.0f, 0.0f, 1.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {0.0f, 0.0f, 1.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {0.0f, 0.0f, 1.0f}),

         // Back Face
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {0.0f, 0.0f, -1.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {0.0f, 0.0f, -1.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {0.0f, 0.0f, -1.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {0.0f, 0.0f, -1.0f}),

         // Left Face
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {-1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {-1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {-1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {-1.0f, 0.0f, 0.0f}),

         // Right Face
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {1.0f, 0.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {1.0f, 0.0f, 0.0f}),

         // Top Face
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {0.0f, 1.0f, 0.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {0.0f, 1.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z + 0.5f}, {0.0f, 1.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y + 0.5f, cubePos.z - 0.5f}, {0.0f, 1.0f, 0.0f}),

         // Bottom Face
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {0.0f, -1.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z - 0.5f}, {0.0f, -1.0f, 0.0f}),
         Vertex({cubePos.x + 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {0.0f, -1.0f, 0.0f}),
         Vertex({cubePos.x - 0.5f, cubePos.y - 0.5f, cubePos.z + 0.5f}, {0.0f, -1.0f, 0.0f})
      }};

      // Adjust vertex count based on the existing mesh data
      unsigned int offset = vertices.size() / 6; // 6 floats per vertex (pos + normal)
      unsigned int cubeIndices[] = {
          // Front
          offset + 0, offset + 1, offset + 2, offset + 2, offset + 3, offset + 0,
          // Back
          offset + 4, offset + 5, offset + 6, offset + 6, offset + 7, offset + 4,
          // Left
          offset + 8, offset + 9, offset + 10, offset + 10, offset + 11, offset + 8,
          // Right
          offset + 12, offset + 13, offset + 14, offset + 14, offset + 15, offset + 12,
          // Top
          offset + 16, offset + 17, offset + 18, offset + 18, offset + 19, offset + 16,
          // Bottom
          offset + 20, offset + 21, offset + 22, offset + 22, offset + 23, offset + 20,
      };
      // clang-format on

      // Add vertex data
      for( const Vertex& vertex : cubeVertices )
      {
         vertices.push_back( vertex.position.x );
         vertices.push_back( vertex.position.y );
         vertices.push_back( vertex.position.z );
         vertices.push_back( vertex.normal.x );
         vertices.push_back( vertex.normal.y );
         vertices.push_back( vertex.normal.z );
      }

      indices.insert( indices.end(), std::begin( cubeIndices ), std::end( cubeIndices ) );
   }

   void AddFace( const glm::vec3& cubePos, BlockFace face )
   {
      const std::array< Vertex, 4 >* faceVertices = nullptr;
      switch( face )
      {
         case BlockFace::North: faceVertices = &m_faceNorth; break;
         case BlockFace::South: faceVertices = &m_faceSouth; break;
         case BlockFace::East:  faceVertices = &m_faceWest; break;
         case BlockFace::West:  faceVertices = &m_faceEast; break;
         case BlockFace::Up:    faceVertices = &m_faceUp; break;
         case BlockFace::Down:  faceVertices = &m_faceDown; break;
         default:               throw std::runtime_error( "Unexpected block face" );
      }

      for( const Vertex& vertex : *faceVertices )
      {
         vertices.push_back( vertex.position.x + cubePos.x );
         vertices.push_back( vertex.position.y + cubePos.y );
         vertices.push_back( vertex.position.z + cubePos.z );
         vertices.push_back( vertex.normal.x );
         vertices.push_back( vertex.normal.y );
         vertices.push_back( vertex.normal.z );
      }

      unsigned int offset = vertices.size() / 6 - 4; // 6 floats per vertex (pos + normal)
      indices.insert( indices.end(), { offset, offset + 1, offset + 2, offset + 2, offset + 3, offset } );
   }

   bool FIsFinalized() const noexcept { return m_fFinalized; }
   void Finalize()
   {
      if( m_fFinalized )
         return;

      m_fFinalized = true;
      m_meshBuffer.Initialize();
      m_meshBuffer.Bind();
      VertexBufferLayout layout;
      layout.Push< float >( 3 ); // Position
      layout.Push< float >( 3 ); // Normal
      m_meshBuffer.SetVertexData( vertices, std::move( layout ) );
      m_meshBuffer.SetIndexData( indices );
      m_meshBuffer.Unbind();
   }

private:
   std::vector< float >        vertices; // Store vertex data for all cubes in the chunk
   std::vector< unsigned int > indices;

   bool m_fFinalized = false;

   struct Vertex
   {
      Vertex( const glm::vec3& pos, const glm::vec3& norm ) :
         position( pos ),
         normal( norm )
      {}
      glm::vec3 position;
      glm::vec3 normal;
   };
   static inline const std::array< Vertex, 4 > m_faceNorth = {
      { Vertex( { -0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } ),
       Vertex( { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } ),
       Vertex( { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } ),
       Vertex( { -0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f } ) }
   };
   static inline const std::array< Vertex, 4 > m_faceSouth = {
      { Vertex( { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f } ),
       Vertex( { -0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f } ),
       Vertex( { 0.5f, 0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f } ),
       Vertex( { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, -1.0f } ) }
   };
   static inline const std::array< Vertex, 4 > m_faceEast = {
      { Vertex( { -0.5f, -0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f } ),
       Vertex( { -0.5f, -0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f } ),
       Vertex( { -0.5f, 0.5f, 0.5f }, { -1.0f, 0.0f, 0.0f } ),
       Vertex( { -0.5f, 0.5f, -0.5f }, { -1.0f, 0.0f, 0.0f } ) }
   };
   static inline const std::array< Vertex, 4 > m_faceWest = {
      { Vertex( { 0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } ),
       Vertex( { 0.5f, 0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } ),
       Vertex( { 0.5f, 0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } ),
       Vertex( { 0.5f, -0.5f, 0.5f }, { 1.0f, 0.0f, 0.0f } ) }
   };
   static inline const std::array< Vertex, 4 > m_faceUp = {
      { Vertex( { -0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } ),
       Vertex( { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } ),
       Vertex( { 0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f } ),
       Vertex( { 0.5f, 0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } ) }
   };
   static inline const std::array< Vertex, 4 > m_faceDown = {
      { Vertex( { -0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f } ),
       Vertex( { 0.5f, -0.5f, -0.5f }, { 0.0f, -1.0f, 0.0f } ),
       Vertex( { 0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } ),
       Vertex( { -0.5f, -0.5f, 0.5f }, { 0.0f, -1.0f, 0.0f } ) }
   };
};

void World::Setup()
{
   std::random_device                       rd;          // Non-deterministic seed generator
   std::mt19937                             gen( rd() ); // Mersenne Twister PRNG
   std::uniform_int_distribution< int32_t > dist;
   //m_mapNoise.SetSeed( dist( gen ) );                           // Set the seed for reproducibility
   m_mapNoise.SetSeed( 5 );                                     // Set the seed for reproducibility
   m_mapNoise.SetNoiseType( FastNoiseLite::NoiseType_Perlin );  // Use Perlin noise for smooth terrain
   m_mapNoise.SetFrequency( 0.005f );                           // Lower frequency = larger features (mountains, valleys)
   m_mapNoise.SetFractalType( FastNoiseLite::FractalType_FBm ); // Use fractal Brownian motion for more realistic terrain
   m_mapNoise.SetFractalOctaves( 5 );                           // Controls the detail level (higher = more detail)

   if( m_chunkThread.joinable() )
   {
      if( m_fChunkThreadActive.load() )
         m_fChunkThreadTerminate.store( true );

      m_chunkThread.join();
   }

   //std::future< void > chunkFuture = std::async( std::launch::async, [ this, noise ]() {} );
   m_chunkThread = std::thread( [ noise = m_mapNoise, this ]()
   {
      m_fChunkThreadActive.store( true );

      { // Lock the mutex and clear existing chunks
         std::lock_guard< std::mutex > lock( m_chunkMutex );
         m_chunkMeshes.clear();
      }

      constexpr int chunkSize   = Chunk::GetSize(); // 16x16 chunks
      constexpr int mapSize     = 64 * chunkSize;
      constexpr int halfMapSize = mapSize / 2;
      for( int chunkX = -halfMapSize; chunkX < halfMapSize; chunkX += chunkSize )
      {
         for( int chunkZ = -halfMapSize; chunkZ < halfMapSize; chunkZ += chunkSize )
         {
            if( m_fChunkThreadTerminate.load() )
            {
               m_fChunkThreadTerminate.store( false );
               return;
            }
            std::shared_ptr< ChunkMesh > chunk = std::make_shared< ChunkMesh >( glm::vec3( chunkX, 0, chunkZ ) );
            for( int x = chunkX; x < chunkX + chunkSize; x++ )
            {
               for( int z = chunkZ; z < chunkZ + chunkSize; z++ )
               {
                  if( m_fChunkThreadTerminate.load() )
                  {
                     m_fChunkThreadTerminate.store( false );
                     return;
                  }
                  //auto getNoise = [ &noise ]( int x, int z ) -> float { return 1; };
                  auto getNoise    = [ &noise ]( int x, int z ) -> float { return noise.GetNoise( static_cast< float >( x ), static_cast< float >( z ) ) + 1; };
                  const int height = static_cast< int >( getNoise( x, z ) * 127.5f ) - 64;
                  //chunk->AddCube( glm::vec3( x, height, z ) );
                  //continue;

                  const int adjHeightNorth = static_cast< int >( getNoise( x, z + 1 ) * 127.5f ) - 64;
                  const int adjHeightSouth = static_cast< int >( getNoise( x, z - 1 ) * 127.5f ) - 64;
                  const int adjHeightEast  = static_cast< int >( getNoise( x + 1, z ) * 127.5f ) - 64;
                  const int adjHeightWest  = static_cast< int >( getNoise( x - 1, z ) * 127.5f ) - 64;

                  const glm::vec3 cubePos( x, height, z );
                  chunk->AddFace( cubePos, BlockFace::Up );
                  //chunk->AddFace( cubePos, BlockFace::Down );

                  // Replace the repeated face-filling blocks with a lambda to reduce duplication
                  auto fillFace = [ & ]( int adjHeight, int height, BlockFace face )
                  {
                     if( height > adjHeight ) // Add faces if not adjacent to another block
                     {
                        chunk->AddFace( cubePos, face );
                        for( int h = adjHeight + 1; h < height; h++ )
                           chunk->AddFace( glm::vec3( x, h, z ), face );
                     }
                  };
                  fillFace( adjHeightNorth, height, BlockFace::North );
                  fillFace( adjHeightSouth, height, BlockFace::South );
                  fillFace( adjHeightEast, height, BlockFace::East );
                  fillFace( adjHeightWest, height, BlockFace::West );
               }
            }

            // set color of chunk randomly
            chunk->SetColor( glm::vec3( std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f ) );

            {
               std::lock_guard< std::mutex > lock( m_chunkMutex );
               m_chunkMeshes.push_back( chunk );
            }
         }
      }
      m_fChunkThreadActive.store( false );
   } );

   m_cubeMeshes.clear();
   m_pSun = std::make_shared< CubeMesh >( glm::vec3( 0.0f ) );
   m_cubeMeshes.push_back( m_pSun );
}

void World::Tick( float delta, Camera& camera )
{
   m_pSun->SetPosition( camera.GetPosition() );
}

// Render the world from provided perspective
void World::Render( const Camera& camera )
{
   // 1. Clear the color and depth buffers
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // 2. Update vertex buffer data (if changed)
   //glBindBuffer( GL_ARRAY_BUFFER, m_VBID ); // Bind the Vertex Buffer Object (VBO)
   //glBufferSubData( GL_ARRAY_BUFFER, 0, vt.size() * sizeof( float ), vt.data() ); // Update vertex data if changed

   // 3. Bind shader and set uniform values
   pShader->Bind();                                            // Bind the shader program
   pShader->SetUniform( "u_MVP", camera.GetProjectionView() ); // Set Model-View-Projection matrix
   pShader->SetUniform( "u_viewPos", camera.GetPosition() );   // Set camera position
   //pShader->SetUniform( "u_color", 0.5f, 0.0f, 0.0f );         // Set color (example: red)
   pShader->SetUniform( "u_lightPos", camera.GetPosition() ); // Set light position

   // 4. Optional: Bind texture (if any)
   // m_Texture->Bind(0); // Bind texture (if using one)
   // pShader->SetUniform1i("u_Texture", 0); // Set texture unit in shader
   ViewFrustum frustum( camera );
   if( !m_chunkMeshes.empty() )
   {
      std::unique_lock< std::mutex > lock( m_chunkMutex );
      for( const std::shared_ptr< IMesh >& mesh : m_chunkMeshes )
      {
         //if( !frustum.FInFrustum( mesh->GetPosition(), Chunk::GetSize() ) )
         //   continue; // Skip rendering this chunk

         std::shared_ptr< ChunkMesh > chunk = std::static_pointer_cast< ChunkMesh >( mesh );
         if( !chunk->FIsFinalized() )
            chunk->Finalize();

         // TODO implemente frustum culling for chunks
         pShader->SetUniform( "u_color", mesh->GetColor() );
         mesh->Render();
      }
   }

   for( const std::shared_ptr< IMesh >& mesh : m_cubeMeshes )
   {
      if( !frustum.FInFrustum( mesh->GetPosition(), 0.5f /*padding (half cube size)*/ ) )
         continue; // Skip rendering this mesh

      pShader->SetUniform( "u_color", mesh->GetColor() );
      mesh->Render();
   }

   for( const auto& [ _, pPlayerMesh ] : m_otherPlayers )
   {
      if( !frustum.FInFrustum( pPlayerMesh->GetPosition(), 0.5f /*padding (half cube size)*/ ) )
         continue; // Skip rendering this mesh

      pShader->SetUniform( "u_color", pPlayerMesh->GetColor() );
      pPlayerMesh->Render();
   }

   pShader->Unbind(); // Unbind shader program

   // 7. Optional: Unbind texture (if any)
   // glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture (if using one)

   // 8. Render additional objects (optional, e.g., chunks or entities)
   // renderChunks(); // Example: render chunks
   // renderEntities(); // Example: render entities
}

void World::ReceivePlayerPosition( uint64_t clientID, const glm::vec3& position )
{
   if( m_otherPlayers.find( clientID ) == m_otherPlayers.end() )
      return;

   m_otherPlayers[ clientID ]->SetPosition( position - glm::vec3( 0.0f, 1.0f, 0.0f ) ); // set position, adjust for head level
}
