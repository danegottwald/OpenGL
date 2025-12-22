#include "Level.h"

#include <Engine/Renderer/Texture.h>

namespace
{
struct Direction
{
   TextureAtlas::BlockFace face;
   int                     dx, dy, dz;
   glm::vec3               normal;
};

constexpr std::array< Direction, 6 > directions = {
   Direction { TextureAtlas::BlockFace::East,   1,  0,  0,  glm::vec3( 1.f,  0.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::West,   -1, 0,  0,  glm::vec3( -1.f, 0.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::Top,    0,  1,  0,  glm::vec3( 0.f,  1.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::Bottom, 0,  -1, 0,  glm::vec3( 0.f,  -1.f, 0.f )  },
   Direction { TextureAtlas::BlockFace::South,  0,  0,  1,  glm::vec3( 0.f,  0.f,  1.f )  },
   Direction { TextureAtlas::BlockFace::North,  0,  0,  -1, glm::vec3( 0.f,  0.f,  -1.f ) },
};

// For each direction, define a unit quad in [0,1] block space.
// Each face is 4 vertices, min-corner at (0,0,0), max at (1,1,1).
constexpr glm::vec3 kFaceVerts[ 6 ][ 4 ] = {
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

constexpr std::array< int, 4 > identity { 0, 1, 2, 3 };  // 0 rotation
constexpr std::array< int, 4 > rotate90 { 1, 2, 3, 0 };  // 90 clockwise
constexpr std::array< int, 4 > rotate180 { 2, 3, 0, 1 }; // 180 rotation
constexpr std::array< int, 4 > rotate270 { 3, 0, 1, 2 }; // 270 clockwise

// Face UVs - define which UV corner to use for each vertex of each face
constexpr std::array< std::array< int, 4 >, 6 > kFaceUVs = {
   // maps vertex i -> which uv corner to use
   // uv corners are: 0:(0,0) 1:(1,0) 2:(1,1) 3:(0,1)
   rotate90, // +X
   rotate90, // -X
   identity, // +Y
   identity, // -Y
   identity, // +Z
   identity, // -Z (flip U)
};
}

// ----------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------
Chunk::Chunk( Level& level, int cx, int cy, int cz ) :
   m_level( level ),
   m_chunkX( cx ),
   m_chunkY( cy ),
   m_chunkZ( cz ),
   m_blocks( CHUNK_VOLUME, BlockId::Air )
{}


BlockId Chunk::GetBlock( int x, int y, int z ) const noexcept
{
   return FInBounds( x, y, z ) ? m_blocks[ ToIndex( x, y, z ) ] : BlockId::Air;
}


void Chunk::SetBlock( int x, int y, int z, BlockId id )
{
   if( !FInBounds( x, y, z ) )
      return;

   m_blocks[ ToIndex( x, y, z ) ] = id;
   m_fDirty                       = true; // mark for remesh, lighting update, etc.
}


bool Chunk::FInBounds( int x, int y, int z ) const noexcept
{
   return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z;
}


int Chunk::ToIndex( int x, int y, int z ) noexcept
{
   // X + Z*XSize + Y*XSize*ZSize keeps vertical columns contiguous by Y
   return x + ( z * CHUNK_SIZE_X ) + ( y * CHUNK_SIZE_X * CHUNK_SIZE_Z );
}


void Chunk::GenerateMesh()
{
   m_mesh.Clear();

   m_mesh.vertices.reserve( CHUNK_VOLUME * 4 );
   m_mesh.indices.reserve( CHUNK_VOLUME * 6 );

   const int baseWX = m_chunkX * CHUNK_SIZE_X;
   const int baseWY = m_chunkY * CHUNK_SIZE_Y;
   const int baseWZ = m_chunkZ * CHUNK_SIZE_Z;
   for( int x = 0; x < CHUNK_SIZE_X; ++x )
   {
      for( int y = 0; y < CHUNK_SIZE_Y; ++y )
      {
         for( int z = 0; z < CHUNK_SIZE_Z; ++z )
         {
            const BlockId id = GetBlock( x, y, z );
            if( id == BlockId::Air )
               continue;

            const BlockInfo& info = GetBlockInfo( id );

            const int       wx = baseWX + x;
            const int       wy = baseWY + y;
            const int       wz = baseWZ + z;
            const glm::vec3 basePos( static_cast< float >( x ), static_cast< float >( y ), static_cast< float >( z ) );
            for( const Direction& dir : directions )
            {
               const int nx = x + dir.dx;
               const int ny = y + dir.dy;
               const int nz = z + dir.dz;

               BlockId neighborId = FInBounds( nx, ny, nz ) ? GetBlock( nx, ny, nz ) : m_level.GetBlock( wx + dir.dx, wy + dir.dy, wz + dir.dz );
               if( neighborId != BlockId::Air )
                  continue;

               // Pick face texture
               //if( dir == 2 ) // +Y (top)
               //   facePath = info.textureTop.empty() ? info.textureSide : info.textureTop;
               //else if( dir == 3 ) // -Y (bottom)
               //   facePath = info.textureBottom.empty() ? info.textureSide : info.textureBottom;

               glm::vec3 tint = glm::vec3( 1.0f );
               //tint     = glm::vec3( 0.79f, 0.75f, 0.35f ); // example tint for grass

               // id -> face uvs

               const TextureAtlas::Region& region      = TextureAtlasManager::Get().GetRegion( id, dir.face );
               const uint32_t              indexOffset = static_cast< uint32_t >( m_mesh.vertices.size() );
               for( int i = 0; i < 4; ++i )
               {
                  ChunkVertex v {};
                  v.position = basePos + kFaceVerts[ static_cast< size_t >( dir.face ) ][ i ];
                  v.normal   = dir.normal;
                  v.uv       = region.uvs[ kFaceUVs[ static_cast< size_t >( dir.face ) ][ i ] ];
                  v.tint     = tint;
                  m_mesh.vertices.push_back( v );
               }

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

   m_fDirty = false;
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


auto Level::WorldToChunk( int wx, int wy, int wz ) const noexcept
{
   // floor division for negative values
   auto divFloor = []( int a, int b )
   {
      const int q = a / b, r = a % b;
      return ( r != 0 ) && ( ( r > 0 ) != ( b > 0 ) ) ? q - 1 : q;
   };

   int cx = divFloor( wx, CHUNK_SIZE_X );
   int cy = divFloor( wy, CHUNK_SIZE_Y );
   int cz = divFloor( wz, CHUNK_SIZE_Z );
   int lx = wx - cx * CHUNK_SIZE_X;
   int ly = wy - cy * CHUNK_SIZE_Y;
   int lz = wz - cz * CHUNK_SIZE_Z;
   return std::tuple< ChunkCoord, glm::ivec3 > {
      ChunkCoord { cx, cz },
       glm::ivec3 { lx, ly, lz }
   };
}


BlockId Level::GetBlock( int wx, int wy, int wz ) const noexcept
{
   auto [ cc, local ] = WorldToChunk( wx, wy, wz );

   auto it = m_chunks.find( cc );
   return it != m_chunks.end() ? it->second.GetBlock( local.x, local.y, local.z ) : BlockId::Air;
}


void Level::SetBlock( int wx, int wy, int wz, BlockId id )
{
   auto [ cc, local ] = WorldToChunk( wx, wy, wz );

   Chunk& chunk = EnsureChunk( cc );
   chunk.SetBlock( local.x, local.y, local.z, id );

   // For now: rebuild whole chunk mesh (eventually rebuild section only?)
   chunk.GenerateMesh(); // eventually only rebuild section?

   SyncChunkRender( cc );
}


void Level::Explode( int wx, int wy, int wz, uint8_t radius )
{
   float radiusSq = radius * radius;
   int   minX     = static_cast< int >( std::floor( wx - radius ) );
   int   maxX     = static_cast< int >( std::ceil( wx + radius ) );
   int   minY     = static_cast< int >( std::floor( wy - radius ) );
   int   maxY     = static_cast< int >( std::ceil( wy + radius ) );
   int   minZ     = static_cast< int >( std::floor( wz - radius ) );
   int   maxZ     = static_cast< int >( std::ceil( wz + radius ) );

   std::unordered_set< Chunk* > affectedChunks;
   for( int x = minX; x <= maxX; ++x )
   {
      for( int y = minY; y <= maxY; ++y )
      {
         for( int z = minZ; z <= maxZ; ++z )
         {
            float dx = x + 0.5f - wx;
            float dy = y + 0.5f - wy;
            float dz = z + 0.5f - wz;
            if( dx * dx + dy * dy + dz * dz > radiusSq )
               continue;

            auto [ cc, local ] = WorldToChunk( x, y, z );

            Chunk& chunk = EnsureChunk( cc );
            chunk.SetBlock( local.x, local.y, local.z, BlockId::Air );
            affectedChunks.insert( &chunk );
         }
      }
   }

   for( Chunk* pChunk : affectedChunks )
   {
      pChunk->GenerateMesh();
      SyncChunkRender( { pChunk->ChunkX(), pChunk->ChunkZ() } );
   }
}


Chunk& Level::EnsureChunk( const ChunkCoord& cc )
{
   if( auto it = m_chunks.find( cc ); it != m_chunks.end() )
      return it->second;

   Chunk& newChunk = m_chunks.emplace( cc, Chunk( *this, cc.x, 0, cc.z ) ).first->second;
   GenerateChunkData( newChunk );

   // mark adjacent chunks dirty for remeshing since this new chunk may expose faces
   ChunkCoord neighborCoords[] = {
      { cc.x - 1, cc.z     },
      { cc.x + 1, cc.z     },
      { cc.x,     cc.z - 1 },
      { cc.x,     cc.z + 1 }
   };
   for( const ChunkCoord& neighborCoord : neighborCoords )
   {
      auto it = m_chunks.find( neighborCoord );
      if( it == m_chunks.end() )
         continue;

      Chunk& neighborChunk   = it->second;
      neighborChunk.m_fDirty = true;
   }

   //newChunk.GenerateMesh();
   //SyncChunkRender( cc );

   return newChunk;
}


void Level::UpdateVisibleRegion( const glm::vec3& playerPos, int viewRadius )
{
   // Calculate the chunk coordinates for the player's position
   auto [ playerChunk, playerLocal ] = WorldToChunk( static_cast< int >( playerPos.x ), static_cast< int >( playerPos.y ), static_cast< int >( playerPos.z ) );

   // Build set of desired chunk coordinates in view
   std::unordered_set< ChunkCoord, ChunkCoordHash > desiredChunks;
   desiredChunks.reserve( static_cast< std::size_t >( ( viewRadius * 2 + 1 ) * ( viewRadius * 2 + 1 ) ) );

   for( int dx = -viewRadius; dx <= viewRadius; ++dx )
   {
      for( int dz = -viewRadius; dz <= viewRadius; ++dz )
      {
         const ChunkCoord cc { playerChunk.x + dx, playerChunk.z + dz };
         desiredChunks.insert( cc );
         EnsureChunk( cc );
      }
   }

   for( auto it = m_chunks.begin(); it != m_chunks.end(); ++it )
   {
      Chunk& chunk = it->second;
      if( chunk.m_fDirty )
      {
         chunk.GenerateMesh();
         SyncChunkRender( it->first );
      }
   }

   // Remove chunks that are now outside of the view radius
   for( auto it = m_chunks.begin(); it != m_chunks.end(); )
   {
      if( !desiredChunks.contains( it->first ) )
      {
         // Erase render data first (if any)
         m_chunkRenders.erase( it->first );
         it = m_chunks.erase( it );
      }
      else
         ++it;
   }

   // Also prune any stray render entries that no longer have a backing chunk
   for( auto it = m_chunkRenders.begin(); it != m_chunkRenders.end(); )
   {
      if( !m_chunks.contains( it->first ) )
         it = m_chunkRenders.erase( it );
      else
         ++it;
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
   render.Upload( it->second.GetMesh() );
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
            BlockId id = ( y < columnHeight - 4 ) ? BlockId::Stone : BlockId::Dirt;
            chunk.SetBlock( x, y, z, id );
         }
      }
   }
}
