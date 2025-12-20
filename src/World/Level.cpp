#include "Level.h"

namespace
{
struct Direction
{
   int       dx, dy, dz;
   glm::vec3 normal;
};

constexpr Direction kDirections[ 6 ] = {
   { 1,  0,  0,  { 1.f, 0.f, 0.f }  }, // +X
   { -1, 0,  0,  { -1.f, 0.f, 0.f } }, // -X
   { 0,  1,  0,  { 0.f, 1.f, 0.f }  }, // +Y
   { 0,  -1, 0,  { 0.f, -1.f, 0.f } }, // -Y
   { 0,  0,  1,  { 0.f, 0.f, 1.f }  }, // +Z
   { 0,  0,  -1, { 0.f, 0.f, -1.f } }, // -Z
};

// For each direction, define a unit quad in [0,1] block space.
// Each face is 4 vertices, min-corner at (0,0,0), max at (1,1,1).
const glm::vec3 kFaceVerts[ 6 ][ 4 ] = {
   // +X (x+1 face)
   {
    { 1.f, 0.f, 0.f },
    { 1.f, 1.f, 0.f },
    { 1.f, 1.f, 1.f },
    { 1.f, 0.f, 1.f },
    },
   // -X (x face)
   {
    { 0.f, 0.f, 1.f },
    { 0.f, 1.f, 1.f },
    { 0.f, 1.f, 0.f },
    { 0.f, 0.f, 0.f },
    },
   // +Y (y+1 face)
   {
    { 0.f, 1.f, 0.f },
    { 0.f, 1.f, 1.f },
    { 1.f, 1.f, 1.f },
    { 1.f, 1.f, 0.f },
    },
   // -Y (y face)
   {
    { 0.f, 0.f, 1.f },
    { 0.f, 0.f, 0.f },
    { 1.f, 0.f, 0.f },
    { 1.f, 0.f, 1.f },
    },
   // +Z (z+1 face)
   {
    { 0.f, 0.f, 1.f },
    { 1.f, 0.f, 1.f },
    { 1.f, 1.f, 1.f },
    { 0.f, 1.f, 1.f },
    },
   // -Z (z face)
   {
    { 1.f, 0.f, 0.f },
    { 0.f, 0.f, 0.f },
    { 0.f, 1.f, 0.f },
    { 1.f, 1.f, 0.f },
    },
};


const glm::vec2 kFaceUVs[ 4 ] = {
   { 0.0f, 0.0f },
   { 1.0f, 0.0f },
   { 1.0f, 1.0f },
   { 0.0f, 1.0f },
};
}

// ----------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------
Chunk::Chunk( int cx, int cy, int cz ) :
   m_chunkX( cx ),
   m_chunkY( cy ),
   m_chunkZ( cz ),
   m_blocks( CHUNK_VOLUME, BLOCK_AIR )
{}


BlockId Chunk::GetBlock( int x, int y, int z ) const
{
   return InBounds( x, y, z ) ? m_blocks[ ToIndex( x, y, z ) ] : BLOCK_AIR;
}


void Chunk::SetBlock( int x, int y, int z, BlockId id )
{
   if( !InBounds( x, y, z ) )
      return;
   m_blocks[ ToIndex( x, y, z ) ] = id;
   m_dirty                        = true; // mark for remesh, lighting update, etc.
}


bool Chunk::InBounds( int x, int y, int z ) const
{
   return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z;
}


int Chunk::ToIndex( int x, int y, int z )
{
   // X + Z*XSize + Y*XSize*ZSize keeps vertical columns contiguous by Y
   return x + z * CHUNK_SIZE_X + y * CHUNK_SIZE_X * CHUNK_SIZE_Z;
}


void Chunk::GenerateMesh()
{
   m_mesh.Clear();

   m_mesh.vertices.reserve( CHUNK_VOLUME * 4 );
   m_mesh.indices.reserve( CHUNK_VOLUME * 6 );

   for( int x = 0; x < CHUNK_SIZE_X; ++x )
   {
      for( int y = 0; y < CHUNK_SIZE_Y; ++y )
      {
         for( int z = 0; z < CHUNK_SIZE_Z; ++z )
         {
            const BlockId id = GetBlock( x, y, z );
            if( id == BLOCK_AIR )
               continue;

            const glm::vec3 basePos( static_cast< float >( x ), static_cast< float >( y ), static_cast< float >( z ) );

            for( int dir = 0; dir < 6; ++dir )
            {
               const auto& d  = kDirections[ dir ];
               const int   nx = x + d.dx;
               const int   ny = y + d.dy;
               const int   nz = z + d.dz;

               const bool neighborInBounds = nx >= 0 && nx < CHUNK_SIZE_X && ny >= 0 && ny < CHUNK_SIZE_Y && nz >= 0 && nz < CHUNK_SIZE_Z;

               BlockId neighborId = BLOCK_AIR;
               if( neighborInBounds )
                  neighborId = GetBlock( nx, ny, nz );

               if( neighborId != BLOCK_AIR )
                  continue; // face hidden

               const uint32_t indexOffset = static_cast< uint32_t >( m_mesh.vertices.size() );
               for( int i = 0; i < 4; ++i )
               {
                  ChunkVertex v {};
                  v.position = basePos + kFaceVerts[ dir ][ i ];
                  v.normal   = d.normal;
                  v.uv       = kFaceUVs[ i ];
                  m_mesh.vertices.push_back( v );
               }

               // Two CCW triangles: (0,1,2) and (0,2,3)
               m_mesh.indices.push_back( indexOffset + 0 );
               m_mesh.indices.push_back( indexOffset + 1 );
               m_mesh.indices.push_back( indexOffset + 2 );

               m_mesh.indices.push_back( indexOffset + 0 );
               m_mesh.indices.push_back( indexOffset + 2 );
               m_mesh.indices.push_back( indexOffset + 3 );
            }
         }
      }
   }

   m_dirty = false;
}


// ----------------------------------------------------------------
// Level
// ----------------------------------------------------------------
Level::Level()
{
   // Basic terrain noise configuration; tweak later as needed
   //m_noise.SetNoiseType( FastNoiseLite::NoiseType_OpenSimplex2 );
   //m_noise.SetFrequency( 0.01f );

   m_noise.SetSeed( 5 );                                     // Set the seed for reproducibility
   m_noise.SetNoiseType( FastNoiseLite::NoiseType_Perlin );  // Use Perlin noise for smooth terrain
   m_noise.SetFrequency( 0.005f );                           // Lower frequency = larger features (mountains, valleys)
   m_noise.SetFractalType( FastNoiseLite::FractalType_FBm ); // Use fractal Brownian motion for more realistic terrain
   m_noise.SetFractalOctaves( 5 );                           // Controls the detail level (higher = more detail)
}


BlockId Level::GetBlock( int wx, int wy, int wz ) const
{
   ChunkCoord cc;
   glm::ivec3 local;
   WorldToChunk( wx, wy, wz, cc, local );

   auto it = m_chunks.find( cc );
   return it != m_chunks.end() ? it->second->GetBlock( local.x, local.y, local.z ) : BLOCK_AIR;
}


void Level::SetBlock( int wx, int wy, int wz, BlockId id )
{
   ChunkCoord cc;
   glm::ivec3 local;
   WorldToChunk( wx, wy, wz, cc, local );

   Chunk& chunk = EnsureChunk( cc );
   chunk.SetBlock( local.x, local.y, local.z, id );

   // For now: rebuild whole chunk mesh (eventually rebuild section only?)
   chunk.GenerateMesh(); // eventually only rebuild section?

   SyncChunkRender( cc );
}


void Level::WorldToChunk( int wx, int wy, int wz, ChunkCoord& outChunk, glm::ivec3& outLocal ) const
{
   // floor division for negative values
   auto divFloor = []( int a, int b )
   {
      int q = a / b;
      int r = a % b;
      if( ( r != 0 ) && ( ( r > 0 ) != ( b > 0 ) ) )
         --q;
      return q;
   };

   int cx   = divFloor( wx, CHUNK_SIZE_X );
   int cy   = divFloor( wy, CHUNK_SIZE_Y );
   int cz   = divFloor( wz, CHUNK_SIZE_Z );
   int lx   = wx - cx * CHUNK_SIZE_X;
   int ly   = wy - cy * CHUNK_SIZE_Y;
   int lz   = wz - cz * CHUNK_SIZE_Z;
   outChunk = { cx, cz };
   outLocal = { lx, ly, lz };
}


Chunk& Level::EnsureChunk( const ChunkCoord& cc )
{
   if( auto it = m_chunks.find( cc ); it != m_chunks.end() )
      return *( it->second );

   std::unique_ptr< Chunk > pChunk = std::make_unique< Chunk >( cc.x, 0, cc.z );
   GenerateChunkData( *pChunk );
   pChunk->GenerateMesh();

   Chunk& ref = *pChunk;
   m_chunks.emplace( cc, std::move( pChunk ) );
   SyncChunkRender( cc );
   return ref;
}


void Level::UpdateVisibleRegion( const glm::vec3& playerPos, int viewRadius )
{
   // Calculate the chunk coordinates for the player's position
   ChunkCoord playerChunk;
   glm::ivec3 playerLocal;
   WorldToChunk( playerPos.x, playerPos.y, playerPos.z, playerChunk, playerLocal );

   // Update the visible chunks based on the player's position and view radius
   for( int x = -viewRadius; x <= viewRadius; ++x )
   {
      for( int y = -viewRadius; y <= viewRadius; ++y )
      {
         for( int z = -viewRadius; z <= viewRadius; ++z )
            EnsureChunk( ChunkCoord { playerChunk.x + x, playerChunk.z + z } );
      }
   }
}


const ChunkRender* Level::GetChunkRender( const ChunkCoord& coord ) const
{
   auto it = m_chunkRenders.find( coord );
   return it != m_chunkRenders.end() ? &it->second : nullptr;
}


ChunkRender& Level::EnsureChunkRender( const ChunkCoord& coord )
{
   return m_chunkRenders[ coord ];
}


void Level::SyncChunkRender( const ChunkCoord& coord )
{
   auto it = m_chunks.find( coord );
   if( it == m_chunks.end() )
      return;

   ChunkRender& render = EnsureChunkRender( coord );
   render.Upload( it->second->GetMesh() );
}


void Level::GenerateChunkData( Chunk& chunk )
{
   // Simple heightmap terrain using FastNoiseLite. Runs in chunk-local space
   // but samples noise in world space for seamless stitching.

   const int baseX = chunk.ChunkX() * CHUNK_SIZE_X;
   const int baseZ = chunk.ChunkZ() * CHUNK_SIZE_Z;
   for( int z = 0; z < CHUNK_SIZE_Z; ++z )
   {
      for( int x = 0; x < CHUNK_SIZE_X; ++x )
      {
         const float worldX = static_cast< float >( baseX + x );
         const float worldZ = static_cast< float >( baseZ + z );

         // Sample noise in 2D; remap [-1,1] to [0,1]
         float n = ( m_noise.GetNoise( worldX, worldZ ) * 0.5f ) + 0.5f;

         // Scale to a terrain height range within the chunk vertical span
         const int minHeight    = 32;
         const int maxHeight    = 128;
         const int heightRange  = maxHeight - minHeight;
         int       columnHeight = minHeight + static_cast< int >( n * heightRange );
         if( columnHeight >= CHUNK_SIZE_Y )
            columnHeight = CHUNK_SIZE_Y - 1;

         for( int y = 0; y <= columnHeight; ++y )
         {
            BlockId id = ( y < columnHeight - 4 ) ? BLOCK_STONE : BLOCK_DIRT;
            chunk.SetBlock( x, y, z, id );
         }
      }
   }
}
