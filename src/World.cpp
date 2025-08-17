#include "World.h"

// Project dependencies
#include <Events/NetworkEvent.h>
#include <Input/Input.h>
#include <Renderer/Mesh.h>

#include <Entity/Registry.h>
#include <Entity/Components/Transform.h>
#include <Entity/Components/Velocity.h>
#include <Entity/Components/Input.h>
#include <Entity/Components/PlayerTag.h>
#include <Entity/Components/Mesh.h>

Events::EventSubscriber m_eventSubscriber;

World::World( Entity::Registry& registry )
{
   m_eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ & ]( const Events::NetworkClientConnectEvent& e ) noexcept
   {
      std::shared_ptr< Entity::EntityHandle > psNewClient = registry.CreateWithHandle();
      registry.AddComponent< CTransform >( psNewClient->Get(), 0.0f, 128.0f, 0.0f );

      std::shared_ptr< CapsuleMesh > psClientMesh = std::make_shared< CapsuleMesh >();
      psClientMesh->SetColor( glm::vec3( 100.0f / 255.0f, 147.0f / 255.0f, 237.0f / 255.0f ) ); // cornflower blue
      registry.AddComponent< CMesh >( psNewClient->Get(), psClientMesh );

      m_playerHandles.insert( { e.GetClientID(), std::move( psNewClient ) } );
   } );

   m_eventSubscriber.Subscribe< Events::NetworkClientDisconnectEvent >( [ & ]( const Events::NetworkClientDisconnectEvent& e ) noexcept
   {
      if( auto it = m_playerHandles.find( e.GetClientID() ); it != m_playerHandles.end() )
         m_playerHandles.erase( it );
   } );

   m_eventSubscriber.Subscribe< Events::NetworkPositionUpdateEvent >( [ & ]( const Events::NetworkPositionUpdateEvent& e ) noexcept
   {
      auto it = m_playerHandles.find( e.GetClientID() );
      if( it == m_playerHandles.end() )
      {
         std::println( std::cerr, "Client with ID {} not found", e.GetClientID() );
         return; // Client not found
      }

      CTransform* pClientTransform = registry.GetComponent< CTransform >( it->second->Get() );
      if( !pClientTransform )
      {
         std::println( std::cerr, "Transform component not found for client ID {}", e.GetClientID() );
         return; // Transform component not found
      }

      glm::vec3 meshOffset( 0.0f, 1.0f, 0.0f ); // Offset for the mesh position
      if( CMesh* pClientMesh = registry.GetComponent< CMesh >( it->second->Get() ) )
         meshOffset.y = pClientMesh->mesh->GetCenterToBottomDistance();

      pClientTransform->position = e.GetPosition() + meshOffset; // position is at feet level, so adjust for height
   } );
}

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
   ChunkMesh() = default;

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

constexpr uint8_t  CHUNK_SIZE       = 16;
constexpr uint16_t WORLD_CHUNK_SIZE = 32; //64;
constexpr uint16_t WORLD_SIZE       = CHUNK_SIZE * WORLD_CHUNK_SIZE / 2;

void World::Setup( Entity::Registry& registry )
{
   PROFILE_SCOPE( "World::Setup" );
   m_entityHandles.clear();

   std::random_device                       rd;          // Non-deterministic seed generator
   std::mt19937                             gen( rd() ); // Mersenne Twister PRNG
   std::uniform_int_distribution< int32_t > dist;
   m_mapNoise.SetSeed( 5 );                                     // Set the seed for reproducibility
   m_mapNoise.SetNoiseType( FastNoiseLite::NoiseType_Perlin );  // Use Perlin noise for smooth terrain
   m_mapNoise.SetFrequency( 0.005f );                           // Lower frequency = larger features (mountains, valleys)
   m_mapNoise.SetFractalType( FastNoiseLite::FractalType_FBm ); // Use fractal Brownian motion for more realistic terrain
   m_mapNoise.SetFractalOctaves( 5 );                           // Controls the detail level (higher = more detail)

   for( int chunkX = 0; chunkX < WORLD_CHUNK_SIZE; ++chunkX )
   {
      for( int chunkZ = 0; chunkZ < WORLD_CHUNK_SIZE; ++chunkZ )
      {
         const glm::vec3 chunkStartPos( -WORLD_SIZE + chunkX * CHUNK_SIZE, 0.0f, -WORLD_SIZE + chunkZ * CHUNK_SIZE );

         std::shared_ptr< ChunkMesh > psChunkMesh = std::make_shared< ChunkMesh >();
         for( float x = chunkStartPos.x; x < chunkStartPos.x + CHUNK_SIZE; x++ )
         {
            for( float z = chunkStartPos.z; z < chunkStartPos.z + CHUNK_SIZE; z++ )
            {
               auto getNoise = [ &noise = m_mapNoise ]( float x, float z ) -> float { return noise.GetNoise( x, z ) + 1; };

               const int       height = static_cast< int >( getNoise( x, z ) * 127.5f ) - 64;
               const glm::vec3 cubePos( x, height, z );
               psChunkMesh->AddFace( cubePos, BlockFace::Up );
               //psChunkMesh->AddFace( cubePos, BlockFace::Down );

               // Replace the repeated face-filling blocks with a lambda to reduce duplication
               auto fillFace = [ & ]( int adjHeight, int height, BlockFace face )
               {
                  if( height > adjHeight ) // Add faces if not adjacent to another block
                  {
                     psChunkMesh->AddFace( cubePos, face );
                     for( int h = adjHeight + 1; h < height; h++ )
                        psChunkMesh->AddFace( glm::vec3( x, h, z ), face );
                  }
               };

               float adjHeightNorth = ( getNoise( x, z + 1 ) * 127.5f ) - 64;
               float adjHeightSouth = ( getNoise( x, z - 1 ) * 127.5f ) - 64;
               float adjHeightEast  = ( getNoise( x + 1, z ) * 127.5f ) - 64;
               float adjHeightWest  = ( getNoise( x - 1, z ) * 127.5f ) - 64;
               fillFace( adjHeightNorth, height, BlockFace::North );
               fillFace( adjHeightSouth, height, BlockFace::South );
               fillFace( adjHeightEast, height, BlockFace::East );
               fillFace( adjHeightWest, height, BlockFace::West );
            }
         }

         psChunkMesh->SetColor( glm::vec3( std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f ) );
         psChunkMesh->Finalize();

         std::shared_ptr< Entity::EntityHandle > psChunk = registry.CreateWithHandle();
         registry.AddComponent< CTransform >( psChunk->Get(), chunkStartPos.x, 0.0f, chunkStartPos.z );
         registry.AddComponent< CMesh >( psChunk->Get(), psChunkMesh );
         m_entityHandles.push_back( psChunk );
      }
   }

   std::shared_ptr< Entity::EntityHandle > psSun = registry.CreateWithHandle();
   registry.AddComponent< CTransform >( psSun->Get(), 0.0f, 76.0f, 0.0f );
   registry.AddComponent< CMesh >( psSun->Get(), std::make_shared< SphereMesh >() );

   m_entityHandles.push_back( psSun );
}

class ViewFrustum
{
public:
   ViewFrustum( const glm::mat4& pv )
   {
      m_planes[ 0 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 0 ],
                                 pv[ 3 ][ 3 ] + pv[ 3 ][ 0 ] ); // Left
      m_planes[ 1 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 0 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 0 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 0 ],
                                 pv[ 3 ][ 3 ] - pv[ 3 ][ 0 ] ); // Right
      m_planes[ 2 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 1 ],
                                 pv[ 3 ][ 3 ] - pv[ 3 ][ 1 ] ); // Top
      m_planes[ 3 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 1 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 1 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 1 ],
                                 pv[ 3 ][ 3 ] + pv[ 3 ][ 1 ] ); // Bottom
      m_planes[ 4 ] = glm::vec4( pv[ 0 ][ 3 ] + pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] + pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] + pv[ 2 ][ 2 ],
                                 pv[ 3 ][ 3 ] + pv[ 3 ][ 2 ] ); // Near
      m_planes[ 5 ] = glm::vec4( pv[ 0 ][ 3 ] - pv[ 0 ][ 2 ], pv[ 1 ][ 3 ] - pv[ 1 ][ 2 ], pv[ 2 ][ 3 ] - pv[ 2 ][ 2 ],
                                 pv[ 3 ][ 3 ] - pv[ 3 ][ 2 ] ); // Far

      for( glm::vec4& plane : m_planes )
         plane = glm::normalize( plane );
   }

   bool FInFrustum( const glm::vec3& point, float padding = 0.0f ) const
   {
      return std::none_of( m_planes.begin(),
                           m_planes.end(),
                           [ & ]( const glm::vec4& plane ) { return glm::dot( glm::vec3( plane ), point ) + plane.w + padding < 0; } );
   }

private:
   std::array< glm::vec4, 6 > m_planes;
};
