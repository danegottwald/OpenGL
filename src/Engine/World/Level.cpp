#include "Level.h"

#include <Engine/Core/Time.h>


// ----------------------------------------------------------------
// ChunkSection
// ----------------------------------------------------------------
BlockId ChunkSection::GetBlock( int x, int y, int z ) const noexcept
{
   return FInBounds( x, y, z ) ? m_blocks[ ToIndex( x, y, z ) ] : BlockId::Air;
}


void ChunkSection::SetBlock( int x, int y, int z, BlockId id )
{
   if( !FInBounds( x, y, z ) )
      return;

   const size_t idx = ToIndex( x, y, z );
   if( m_blocks[ idx ] == id )
      return;

   m_blocks[ idx ] = id;
   m_fDirty        = true;
}


/*static*/ size_t ChunkSection::ToIndex( int x, int y, int z ) noexcept
{
   return static_cast< size_t >( x + ( z * CHUNK_SIZE_X ) + ( y * CHUNK_SIZE_X * CHUNK_SIZE_Z ) );
}


/*static*/ bool ChunkSection::FInBounds( int x, int y, int z ) noexcept
{
   return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SECTION_SIZE && z >= 0 && z < CHUNK_SIZE_Z;
}


// ----------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------
Chunk::Chunk( Level& level, int cx, int cy, int cz ) :
   m_level( level ),
   m_chunkX( cx ),
   m_chunkY( cy ),
   m_chunkZ( cz )
{}


bool Chunk::FLoadFromDisk()
{
   std::vector< std::byte > bytes;
   if( !World::WorldSave::FLoadChunkBytes( m_level.m_worldDir, GetCoord3(), bytes ) )
      return false;

   const size_t needed = static_cast< size_t >( CHUNK_VOLUME ) * sizeof( BlockId );
   if( bytes.size() != needed )
      return false;

   size_t         p   = 0;
   const BlockId* src = reinterpret_cast< const BlockId* >( bytes.data() );
   for( int y = 0; y < CHUNK_SIZE_Y; ++y )
   {
      for( int z = 0; z < CHUNK_SIZE_Z; ++z )
      {
         for( int x = 0; x < CHUNK_SIZE_X; ++x )
            m_sections[ ToSectionIndex( y ) ].SetBlock( x, ToSectionLocalY( y ), z, src[ p++ ] );
      }
   }

   m_dirty        = ChunkDirty::Mesh;
   m_meshRevision = m_meshRevision + 1;
   return true;
}


void Chunk::SaveToDisk()
{
   if( !( Any( m_dirty & ChunkDirty::Save ) ) )
      return;

   // Persist in the original flat format for compatibility.
   std::vector< BlockId > flat;
   flat.resize( static_cast< size_t >( CHUNK_VOLUME ), BlockId::Air );

   size_t p = 0;
   for( int y = 0; y < CHUNK_SIZE_Y; ++y )
   {
      for( int z = 0; z < CHUNK_SIZE_Z; ++z )
      {
         for( int x = 0; x < CHUNK_SIZE_X; ++x )
            flat[ p++ ] = m_sections[ ToSectionIndex( y ) ].GetBlock( x, ToSectionLocalY( y ), z );
      }
   }

   const auto byteSpan = std::span< const std::byte >( reinterpret_cast< const std::byte* >( flat.data() ), flat.size() * sizeof( BlockId ) );
   World::WorldSave::FSaveChunkBytes( m_level.m_worldDir, GetCoord3(), byteSpan );

   ClearDirty( ChunkDirty::Save );
}


BlockId Chunk::GetBlock( int x, int y, int z ) const noexcept
{
   if( !FInBounds( x, y, z ) )
      return BlockId::Air;

   const int sIndex = ToSectionIndex( y );
   const int ly     = ToSectionLocalY( y );
   return m_sections[ sIndex ].GetBlock( x, ly, z );
}


void Chunk::SetBlock( int x, int y, int z, BlockId id )
{
   if( !FInBounds( x, y, z ) )
      return;

   const int sIndex = ToSectionIndex( y );
   const int ly     = ToSectionLocalY( y );
   if( m_sections[ sIndex ].GetBlock( x, ly, z ) == id )
      return;

   m_sections[ sIndex ].SetBlock( x, ly, z, id );

   MarkDirty( ChunkDirty::Save | ChunkDirty::Mesh );
   ++m_meshRevision;
}


bool Chunk::FInBounds( int x, int y, int z ) const noexcept
{
   return x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y && z >= 0 && z < CHUNK_SIZE_Z;
}


// ----------------------------------------------------------------
// Level
// ----------------------------------------------------------------
Level::Level( std::filesystem::path worldName ) :
   m_worldDir( World::WorldSave::RootDir( worldName ) ),
   m_autosaveTimer( AUTOSAVE_INTERVAL )
{
   // Load meta if present; otherwise defaults
   if( auto meta = World::WorldSave::LoadMeta( m_worldDir ) )
   {
      m_meta         = *meta;
      m_meta.version = 1;
      m_meta.seed    = 5;
      m_meta.tick    = 0;
      World::WorldSave::FSaveMeta( m_worldDir, m_meta );
   }

   m_noise.SetSeed( static_cast< int >( m_meta.seed ) );
   m_noise.SetNoiseType( FastNoiseLite::NoiseType_Perlin );
   m_noise.SetFrequency( 0.005f );
   m_noise.SetFractalType( FastNoiseLite::FractalType_FBm );
   m_noise.SetFractalOctaves( 5 );
}


void Level::Update( float dt )
{
   if( m_autosaveTimer.FTick( dt ) )
      Save();
}


void Level::SaveMeta() const
{
   World::WorldSave::FSaveMeta( m_worldDir, m_meta );
}


void Level::SavePlayer( const glm::vec3& playerPos ) const
{
   World::PlayerSave ps;
   ps.position = playerPos;

   World::WorldSave::FSavePlayer( m_worldDir, ps );
}


void Level::Save() const
{
   SaveMeta();

   for( auto& [ _, chunk ] : m_chunks )
      const_cast< Chunk& >( chunk ).SaveToDisk();
}


std::tuple< ChunkCoord, glm::ivec3 > Level::WorldToChunk( int wx, int wy, int wz ) const noexcept
{
   // floor division for negative values
   auto divFloor = []( int a, int b )
   {
      const int q = a / b, r = a % b;
      return ( r != 0 ) && ( ( r > 0 ) != ( b > 0 ) ) ? q - 1 : q;
   };

   const int cx = divFloor( wx, CHUNK_SIZE_X ), lx = wx - cx * CHUNK_SIZE_X;
   const int cz = divFloor( wz, CHUNK_SIZE_Z ), lz = wz - cz * CHUNK_SIZE_Z;
   return std::tuple< ChunkCoord, glm::ivec3 > {
      ChunkCoord { cx, cz },
       glm::ivec3 { lx, wy, lz }
   };
}


BlockId Level::GetBlock( int wx, int wy, int wz ) const noexcept
{
   auto [ cc, local ] = WorldToChunk( wx, wy, wz );
   auto it            = m_chunks.find( cc );
   return it != m_chunks.end() ? it->second.GetBlock( local.x, local.y, local.z ) : BlockId::Air;
}


void Level::MarkChunkAndNeighborsMeshDirty( const ChunkCoord& cc )
{
   auto mark = [ & ]( const ChunkCoord& c )
   {
      auto it = m_chunks.find( c );
      if( it == m_chunks.end() )
         return;
      it->second.MarkDirty( ChunkDirty::Mesh );
   };

   mark( cc );
   mark( { cc.x - 1, cc.z } );
   mark( { cc.x + 1, cc.z } );
   mark( { cc.x, cc.z - 1 } );
   mark( { cc.x, cc.z + 1 } );
}


void Level::SetBlock( int wx, int wy, int wz, BlockId id )
{
   auto [ cc, local ] = WorldToChunk( wx, wy, wz );
   Chunk& chunk       = EnsureChunk( cc );

   chunk.SetBlock( local.x, local.y, local.z, id );

   // Changing a boundary block can affect neighbor faces.
   if( local.x == 0 || local.x == CHUNK_SIZE_X - 1 || local.z == 0 || local.z == CHUNK_SIZE_Z - 1 )
      MarkChunkAndNeighborsMeshDirty( cc );
   else
      chunk.MarkDirty( ChunkDirty::Mesh );
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

   std::unordered_set< ChunkCoord, ChunkCoordHash > touched;

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
            Chunk& chunk       = EnsureChunk( cc );
            chunk.SetBlock( local.x, local.y, local.z, BlockId::Air );
            touched.insert( cc );
         }
      }
   }

   for( const ChunkCoord& cc : touched )
      MarkChunkAndNeighborsMeshDirty( cc );
}


int Level::GetSurfaceY( int wx, int wz ) noexcept
{
   const Chunk& chunk = EnsureChunk( ChunkCoord { .x = wx, .z = wz } );
   for( int wy = CHUNK_SIZE_Y; wy >= 0; --wy )
   {
      auto [ _, local ] = WorldToChunk( wx, wy, wz );
      if( chunk.GetBlock( local.x, local.y, local.z ) != BlockId::Air )
         return wy;
   }

   return 0;
}


Chunk& Level::EnsureChunk( const ChunkCoord& cc )
{
   if( auto it = m_chunks.find( cc ); it != m_chunks.end() )
      return it->second;

   Chunk& chunk = m_chunks.try_emplace( cc, *this, cc.x, 0, cc.z ).first->second;

   if( !chunk.FLoadFromDisk() )
      GenerateChunkData( chunk );

   MarkChunkAndNeighborsMeshDirty( cc );

   return chunk;
}


void Level::UpdateStreaming( const glm::vec3& playerPos, uint8_t viewRadius )
{
   auto [ playerChunk, _ ] = WorldToChunk( static_cast< int >( playerPos.x ), static_cast< int >( playerPos.y ), static_cast< int >( playerPos.z ) );

   for( int dx = -viewRadius; dx <= viewRadius; ++dx )
   {
      for( int dz = -viewRadius; dz <= viewRadius; ++dz )
      {
         const ChunkCoord cc { playerChunk.x + dx, playerChunk.z + dz };
         if( !m_chunks.contains( cc ) )
            EnsureChunk( cc );
      }
   }

   auto inView = [ & ]( const ChunkCoord& cc ) { return std::abs( cc.x - playerChunk.x ) <= viewRadius && std::abs( cc.z - playerChunk.z ) <= viewRadius; };
   for( auto it = m_chunks.begin(); it != m_chunks.end(); )
   {
      if( !inView( it->first ) )
      {
         it->second.SaveToDisk();
         it = m_chunks.erase( it );
      }
      else
         ++it;
   }

   m_lastPlayerChunk = playerChunk;
}


void Level::GenerateChunkData( Chunk& chunk )
{
   const int baseX = chunk.ChunkX() * CHUNK_SIZE_X;
   const int baseZ = chunk.ChunkZ() * CHUNK_SIZE_Z;
   for( int z = 0; z < CHUNK_SIZE_Z; ++z )
   {
      for( int x = 0; x < CHUNK_SIZE_X; ++x )
      {
         const float worldX = static_cast< float >( baseX + x );
         const float worldZ = static_cast< float >( baseZ + z );

         const int minHeight    = 32;
         const int maxHeight    = 128;
         const int heightRange  = maxHeight - minHeight;
         float     noise        = ( m_noise.GetNoise( worldX, worldZ ) * 0.5f ) + 0.5f;
         int       columnHeight = std::clamp( static_cast< int >( noise * heightRange ), minHeight, CHUNK_SIZE_Y - 1 );
         for( int y = 0; y <= columnHeight; ++y )
         {
            BlockId id { BlockId::Air };
            if( y == 0 )
               id = BlockId::Bedrock;
            else if( y < columnHeight - 4 )
               id = BlockId::Stone;
            else
               id = BlockId::Dirt;

            chunk.SetBlock( x, y, z, id );
         }
      }
   }

   // Mark chunk dirty and save
   chunk.MarkDirty( ChunkDirty::Save | ChunkDirty::Mesh );
   chunk.SaveToDisk();
}
