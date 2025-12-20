#include "World.h"

// Project dependencies
#include <Events/NetworkEvent.h>
#include <Input/Input.h>
#include <Renderer/Mesh.h>

#include <Entity/Components.h>
#include <Entity/Registry.h>

Events::EventSubscriber m_eventSubscriber;

World::World( Entity::Registry& registry )
{
   m_eventSubscriber.Subscribe< Events::NetworkClientConnectEvent >( [ & ]( const Events::NetworkClientConnectEvent& e ) noexcept
   {
      std::shared_ptr< Entity::EntityHandle > psNewClient = registry.CreateWithHandle();
      registry.Add< CTransform >( psNewClient->Get(), 0.0f, 128.0f, 0.0f );

      std::shared_ptr< CapsuleMesh > psClientMesh = std::make_shared< CapsuleMesh >();
      psClientMesh->SetColor( glm::vec3( 100.0f / 255.0f, 147.0f / 255.0f, 237.0f / 255.0f ) ); // cornflower blue
      registry.Add< CMesh >( psNewClient->Get(), psClientMesh );
      //registry.Add< CVelocity >( psNewClient->Get(), 0.0f, 0.0f, 0.0f );
      //registry.Add< CAABB >( psNewClient->Get(), glm::vec3( 0.3f, 1.9f * 0.5f, 0.3f ) ); // half extents

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

      CTransform* pClientTransform = registry.TryGet< CTransform >( it->second->Get() );
      if( !pClientTransform )
      {
         std::println( std::cerr, "Transform component not found for client ID {}", e.GetClientID() );
         return; // Transform component not found
      }

      glm::vec3 meshOffset( 0.0f, 1.0f, 0.0f ); // Offset for the mesh position
      if( CMesh* pClientMesh = registry.TryGet< CMesh >( it->second->Get() ) )
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

constexpr float    BLOCK_POS_Y = 64.0f; // constant height for all blocks
constexpr uint16_t WORLD_SIZE = 6; // number of chunks along one axis (world is WORLD_SIZE x WORLD_SIZE chunks)

void World::Setup( Entity::Registry& registry )
{
   PROFILE_SCOPE( "World::Setup" );
   m_entityHandles.clear();

   for( int chunkX = 0; chunkX < WORLD_SIZE; chunkX++ )
   {
      for( int chunkY = 0; chunkY < WORLD_SIZE; chunkY++ )
      {
         std::shared_ptr< ChunkMesh > psChunkMesh = std::make_shared< ChunkMesh >();

         int chunkPosX = chunkX * CHUNK_SIZE, chunkPosZ = chunkY * CHUNK_SIZE;
         for( int x = 0; x < CHUNK_SIZE; x++ )
         {
            for( int z = 0; z < CHUNK_SIZE; z++ )
               psChunkMesh->AddCube( { static_cast< float >( chunkPosX + x ), BLOCK_POS_Y, static_cast< float >( chunkPosZ + z ) } );
         }

         //psChunkMesh->SetColor( glm::vec3( 34.0f / 255.0f, 139.0f / 255.0f, 34.0f / 255.0f ) ); // forest green
         psChunkMesh->SetColor( glm::vec3( std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f, std::rand() % 256 / 255.0f ) );
         psChunkMesh->Finalize();


         float halfX = CHUNK_SIZE * 0.5f;
         float halfZ = CHUNK_SIZE * 0.5f;
         float halfY = CHUNK_HEIGHT * 0.5f;

         float chunkWorldX = chunkPosX + halfX - 0.5f;
         float chunkWorldZ = chunkPosZ + halfZ - 0.5f;
         float chunkWorldY = BLOCK_POS_Y;

         std::shared_ptr< Entity::EntityHandle > psChunk = registry.CreateWithHandle();
         registry.Add< CChunkTag >( psChunk->Get() );
         registry.Add< CTransform >( psChunk->Get(), chunkWorldX, chunkWorldY, chunkWorldZ );
         registry.Add< CAABB >( psChunk->Get(), CAABB { glm::vec3( halfX, halfY, halfZ ) } );
         registry.Add< CMesh >( psChunk->Get(), psChunkMesh );
         m_entityHandles.push_back( psChunk );
      }
   }

   std::shared_ptr< Entity::EntityHandle > psSun = registry.CreateWithHandle();
   registry.Add< CTransform >( psSun->Get(), 5.0f, 76.0f, 5.0f );
   registry.Add< CMesh >( psSun->Get(), std::make_shared< SphereMesh >() );
   registry.Add< CVelocity >( psSun->Get(), glm::vec3( 1.0f, 0.0f, 1.0f ) );
   registry.Add< CAABB >( psSun->Get(), CAABB { glm::vec3( 0.5f ) } );

   m_entityHandles.push_back( psSun );
}

void World::CreateChunkMesh( Entity::Registry& registry, _Inout_ std::shared_ptr< ChunkMesh >& mesh, const glm::ivec2& chunkPos, FastNoiseLite& noise )
{
   std::random_device                       rd;          // Non-deterministic seed generator
   std::mt19937                             gen( rd() ); // Mersenne Twister PRNG
   std::uniform_int_distribution< int32_t > dist;
   noise.SetSeed( 5 );                                     // Set the seed for reproducibility
   noise.SetNoiseType( FastNoiseLite::NoiseType_Perlin );  // Use Perlin noise for smooth terrain
   noise.SetFrequency( 0.005f );                           // Lower frequency = larger features (mountains, valleys)
   noise.SetFractalType( FastNoiseLite::FractalType_FBm ); // Use fractal Brownian motion for more realistic terrain
   noise.SetFractalOctaves( 5 );                           // Controls the detail level (higher = more detail)

   for( int x = 0; x < CHUNK_SIZE; ++x )
   {
      for( int z = 0; z < CHUNK_SIZE; ++z )
      {
         auto getNoise = [ &noise ]( float x, float z ) -> float { return noise.GetNoise( x, z ) + 1.0f; };

         float posX = static_cast< float >( chunkPos.x * CHUNK_SIZE + x );
         float posZ = static_cast< float >( chunkPos.y * CHUNK_SIZE + z );

         // Quantize height to whole-block steps so terrain aligns to a
         // discrete grid in Y.
         const float     topY = std::floor( getNoise( posX, posZ ) * 127.5f - 64.0f );
         const glm::vec3 cubePos( posX, topY, posZ );
         mesh->AddFace( cubePos, BlockFace::Up );

         auto fillFace = [ & ]( float adjTopY, float topYLocal, BlockFace face )
         {
            if( topYLocal > adjTopY ) // Add faces if not adjacent to another block
            {
               mesh->AddFace( cubePos, face );
               for( float y = adjTopY + 1.0f; y < topYLocal; y += 1.0f )
                  mesh->AddFace( glm::vec3( posX, y, posZ ), face );
            }
         };

         float adjTopYNorth = std::floor( getNoise( posX,     posZ + 1 ) * 127.5f - 64.0f );
         float adjTopYSouth = std::floor( getNoise( posX,     posZ - 1 ) * 127.5f - 64.0f );
         float adjTopYEast  = std::floor( getNoise( posX + 1, posZ     ) * 127.5f - 64.0f );
         float adjTopYWest  = std::floor( getNoise( posX - 1, posZ     ) * 127.5f - 64.0f );
         fillFace( adjTopYNorth, topY, BlockFace::North );
         fillFace( adjTopYSouth, topY, BlockFace::South );
         fillFace( adjTopYEast, topY, BlockFace::East );
         fillFace( adjTopYWest, topY, BlockFace::West );

         // Block entities with transforms/AABBs are managed separately based on
         // player proximity; mesh generation is independent of entities.
      }
   }
}

void World::UpdateActiveChunks( Entity::Registry& registry, const glm::vec3& playerPos )
{
   auto chunkCoord = []( float v ) { return static_cast< int >( std::floor( v / CHUNK_SIZE ) ); };
   glm::ivec2 newChunk { chunkCoord( playerPos.x ), chunkCoord( playerPos.z ) };
   if( newChunk == m_playerChunk )
      return; // still in the same chunk

   PROFILE_SCOPE( "World::UpdateActiveChunks" );
   m_playerChunk = newChunk;

   constexpr int meshRenderDistance   = 6; // chunks with meshes
   constexpr int meshRenderDistanceSq = meshRenderDistance * meshRenderDistance;

   // Determine chunks that should have meshes (larger radius)
   std::vector< glm::ivec2 > meshChunks;
   meshChunks.reserve( ( meshRenderDistance * 2 + 1 ) * ( meshRenderDistance * 2 + 1 ) );
   for( int dx = -meshRenderDistance; dx <= meshRenderDistance; ++dx )
   {
      for( int dz = -meshRenderDistance; dz <= meshRenderDistance; ++dz )
      {
         if( dx * dx + dz * dz > meshRenderDistanceSq )
            continue;
         meshChunks.emplace_back( newChunk.x + dx, newChunk.y + dz );
      }
   }

   // Determine chunks that should have per-block entities (tight 3x3 around player)
   std::vector< glm::ivec2 > blockChunks;
   blockChunks.reserve( 9 );

   constexpr int blockRenderRadius   = 1; // 3x3 chunks: current + 8 neighbors
   for( int dx = -blockRenderRadius; dx <= blockRenderRadius; ++dx )
   {
      for( int dz = -blockRenderRadius; dz <= blockRenderRadius; ++dz )
         blockChunks.emplace_back( newChunk.x + dx, newChunk.y + dz );
   }

   // Remove meshes for chunks that are no longer needed
   for( auto it = m_activeChunks.begin(); it != m_activeChunks.end(); )
   {
      bool        keep = false;
      const glm::ivec2& pos  = it->first;
      for( const glm::ivec2& c : meshChunks )
      {
         if( c == pos )
         {
            keep = true;
            break;
         }
      }

      if( !keep )
      {
         registry.Destroy( it->second.m_entity );
         it = m_activeChunks.erase( it );
      }
      else
         ++it;
   }

   // Add new mesh chunks
   int newChunksLoaded = 0;
   for( const glm::ivec2& cPos : meshChunks )
   {
      if( m_activeChunks.find( cPos ) != m_activeChunks.end() )
         continue;

      ++newChunksLoaded;

      Chunk chunk;
      chunk.chunkPos = cPos;

      auto mesh = std::make_shared< ChunkMesh >();
      CreateChunkMesh( registry, mesh, cPos, m_mapNoise );

      mesh->SetColor( glm::vec3( std::rand() % 256 / 255.0f,
                                 std::rand() % 256 / 255.0f,
                                 std::rand() % 256 / 255.0f ) );
      mesh->Finalize();
      chunk.mesh = mesh;

      float chunkCenterX = float( cPos.x * CHUNK_SIZE + CHUNK_SIZE / 2 - 0.5f );
      float chunkCenterZ = float( cPos.y * CHUNK_SIZE + CHUNK_SIZE / 2 - 0.5f );

      std::shared_ptr< Entity::EntityHandle > pChunkEntity  = registry.CreateWithHandle();
      registry.Add< CTransform >( *pChunkEntity, chunkCenterX, BLOCK_POS_Y, chunkCenterZ );
      registry.Add< CMesh >( *pChunkEntity, mesh );
      registry.Add< CChunkTag >( *pChunkEntity );

      chunk.m_entity = *pChunkEntity;

      m_entityHandles.push_back( pChunkEntity );
      m_activeChunks.emplace( cPos, std::move( chunk ) );
   }

   // Destroy any existing block entities, then recreate them for the 3x3 region
   for( Entity::Entity e : m_activeBlocks )
      registry.Destroy( e );
   for( Block& block : m_activeBlocks2 )
      registry.Destroy( block.entity );
   m_activeBlocks.clear();
   m_activeBlocks2.clear();

   for( const glm::ivec2& cPos : blockChunks )
   {
      for( int x = 0; x < CHUNK_SIZE; ++x )
      {
         for( int z = 0; z < CHUNK_SIZE; ++z )
         {
            float posX = static_cast< float >( cPos.x * CHUNK_SIZE + x );
            float posZ = static_cast< float >( cPos.y * CHUNK_SIZE + z );

            // Use the same quantized height computation as CreateChunkMesh so
            // that block entities align exactly with the mesh cubes.
            float topY = std::floor( ( m_mapNoise.GetNoise( posX, posZ ) + 1.0f ) * 127.5f - 64.0f );

            Entity::Entity blockEntity = registry.Create();
            registry.Add< CTransform >( blockEntity, posX, topY, posZ );
            registry.Add< CAABB >( blockEntity, CAABB { glm::vec3( 0.5f ) } );
            m_activeBlocks.push_back( blockEntity );
            m_activeBlocks2.push_back( { blockEntity, glm::ivec3( static_cast<int>(posX), static_cast<int>(topY), static_cast<int>(posZ) ) } );
         }
      }
   }

   std::cout << "Loaded " << newChunksLoaded << " new chunks" << std::endl;
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
