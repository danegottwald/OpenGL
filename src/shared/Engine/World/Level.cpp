#include "Level.h"

#include <Engine/Core/Time.h>


// ----------------------------------------------------------------
// ChunkSection
// ----------------------------------------------------------------
BlockState ChunkSection::GetBlock( LocalBlockPos pos ) const noexcept
{
   return FInBounds( pos ) ? m_blocks[ ToIndex( pos ) ] : BlockState( BlockId::Air );
}


void ChunkSection::SetBlock( LocalBlockPos pos, BlockState state )
{
   if( !FInBounds( pos ) )
      return;

   const size_t idx = ToIndex( pos );
   if( m_blocks[ idx ] == state )
      return;

   m_blocks[ idx ] = state;
   m_fDirty        = true;
}


/*static*/ size_t ChunkSection::ToIndex( LocalBlockPos pos ) noexcept
{
   return static_cast< size_t >( pos.x + ( pos.z * CHUNK_SIZE_X ) + ( pos.y * CHUNK_SIZE_X * CHUNK_SIZE_Z ) );
}


/*static*/ bool ChunkSection::FInBounds( LocalBlockPos pos ) noexcept
{
   return pos.x >= 0 && pos.x < CHUNK_SIZE_X && pos.y >= 0 && pos.y < CHUNK_SECTION_SIZE && pos.z >= 0 && pos.z < CHUNK_SIZE_Z;
}


// ----------------------------------------------------------------
// Chunk
// ----------------------------------------------------------------
Chunk::Chunk( Level& level, const ChunkPos& cpos ) :
   m_level( level ),
   m_cpos( cpos )
{}


bool Chunk::FLoadFromDisk()
{
   std::vector< std::byte > bytes;
   if( !World::WorldSave::FLoadChunkBytes( m_level.m_worldDir, GetCoord3(), bytes ) )
      return false;

   const size_t needed = static_cast< size_t >( CHUNK_VOLUME ) * sizeof( BlockState );
   if( bytes.size() != needed )
      return false;

   size_t            p   = 0;
   const BlockState* src = reinterpret_cast< const BlockState* >( bytes.data() );
   for( int y = 0; y < CHUNK_SIZE_Y; ++y )
   {
      for( int z = 0; z < CHUNK_SIZE_Z; ++z )
      {
         for( int x = 0; x < CHUNK_SIZE_X; ++x )
            m_sections[ ToSectionIndex( y ) ].SetBlock( LocalBlockPos { x, ToSectionLocalY( y ), z }, src[ p++ ] );
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
   std::vector< BlockState > flat;
   flat.resize( static_cast< size_t >( CHUNK_VOLUME ), BlockState( BlockId::Air ) );

   size_t p = 0;
   for( int y = 0; y < CHUNK_SIZE_Y; ++y )
   {
      for( int z = 0; z < CHUNK_SIZE_Z; ++z )
      {
         for( int x = 0; x < CHUNK_SIZE_X; ++x )
            flat[ p++ ] = m_sections[ ToSectionIndex( y ) ].GetBlock( LocalBlockPos { x, ToSectionLocalY( y ), z } );
      }
   }

   const auto byteSpan = std::span< const std::byte >( reinterpret_cast< const std::byte* >( flat.data() ), flat.size() * sizeof( BlockState ) );
   World::WorldSave::FSaveChunkBytes( m_level.m_worldDir, GetCoord3(), byteSpan );

   ClearDirty( ChunkDirty::Save );
}


BlockState Chunk::GetBlock( LocalBlockPos pos ) const noexcept
{
   if( !FInBounds( pos ) )
      return BlockState( BlockId::Air );

   const int sIndex = ToSectionIndex( pos.y );
   const int ly     = ToSectionLocalY( pos.y );
   return m_sections[ sIndex ].GetBlock( LocalBlockPos { pos.x, ly, pos.z } );
}


void Chunk::SetBlock( LocalBlockPos pos, BlockState state )
{
   if( !FInBounds( pos ) )
      return;

   const int sIndex = ToSectionIndex( pos.y );
   const int ly     = ToSectionLocalY( pos.y );
   if( m_sections[ sIndex ].GetBlock( LocalBlockPos { pos.x, ly, pos.z } ) == state )
      return;

   m_sections[ sIndex ].SetBlock( LocalBlockPos { pos.x, ly, pos.z }, state );

   MarkDirty( ChunkDirty::Save | ChunkDirty::Mesh );
   ++m_meshRevision;
}


bool Chunk::FInBounds( LocalBlockPos pos ) const noexcept
{
   return pos.x >= 0 && pos.x < CHUNK_SIZE_X && pos.y >= 0 && pos.y < CHUNK_SIZE_Y && pos.z >= 0 && pos.z < CHUNK_SIZE_Z;
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


std::tuple< ChunkPos, LocalBlockPos > Level::WorldToChunk( WorldBlockPos wpos ) const noexcept
{
   // floor division for negative values
   auto divFloor = []( int a, int b )
   {
      const int q = a / b, r = a % b;
      return ( r != 0 ) && ( ( r > 0 ) != ( b > 0 ) ) ? q - 1 : q;
   };

   const int cx = divFloor( wpos.x, CHUNK_SIZE_X ), lx = wpos.x - cx * CHUNK_SIZE_X;
   const int cz = divFloor( wpos.z, CHUNK_SIZE_Z ), lz = wpos.z - cz * CHUNK_SIZE_Z;
   return {
      ChunkPos { cx, cz },
       LocalBlockPos { lx, wpos.y, lz }
   };
}


BlockState Level::GetBlock( WorldBlockPos pos ) const noexcept
{
   auto [ cpos, local ] = WorldToChunk( pos );
   auto it              = m_chunks.find( cpos );
   return it != m_chunks.end() ? it->second.GetBlock( local ) : BlockState( BlockId::Air );
}


void Level::SetBlock( WorldBlockPos pos, BlockState state )
{
   auto [ cpos, local ] = WorldToChunk( pos );

   Chunk& chunk = EnsureChunk( cpos );
   chunk.SetBlock( local, state );

   // Changing a boundary block can affect neighbor faces.
   if( local.x == 0 || local.x == CHUNK_SIZE_X - 1 || local.z == 0 || local.z == CHUNK_SIZE_Z - 1 )
      MarkChunkAndNeighborsMeshDirty( cpos );
   else
      chunk.MarkDirty( ChunkDirty::Mesh );
}


void Level::Explode( WorldBlockPos pos, uint8_t radius )
{
   float radiusSq = radius * radius;
   int   minX     = static_cast< int >( std::floor( pos.x - radius ) );
   int   maxX     = static_cast< int >( std::ceil( pos.x + radius ) );
   int   minY     = static_cast< int >( std::floor( pos.y - radius ) );
   int   maxY     = static_cast< int >( std::ceil( pos.y + radius ) );
   int   minZ     = static_cast< int >( std::floor( pos.z - radius ) );
   int   maxZ     = static_cast< int >( std::ceil( pos.z + radius ) );

   std::unordered_set< ChunkPos, ChunkPosHash > touched;
   for( int x = minX; x <= maxX; ++x )
   {
      for( int y = minY; y <= maxY; ++y )
      {
         for( int z = minZ; z <= maxZ; ++z )
         {
            float dx = x + 0.5f - pos.x;
            float dy = y + 0.5f - pos.y;
            float dz = z + 0.5f - pos.z;
            if( dx * dx + dy * dy + dz * dz > radiusSq )
               continue;

            auto [ cpos, local ] = WorldToChunk( WorldBlockPos { x, y, z } );
            Chunk& chunk         = EnsureChunk( cpos );
            chunk.SetBlock( local, BlockState( BlockId::Air ) );
            touched.insert( cpos );
         }
      }
   }

   for( const ChunkPos& cpos : touched )
      MarkChunkAndNeighborsMeshDirty( cpos );
}


int Level::GetSurfaceY( WorldBlockPos pos ) noexcept
{
   const Chunk& chunk = EnsureChunk( ChunkPos { .x = pos.x, .z = pos.z } );
   for( int wy = CHUNK_SIZE_Y; wy >= 0; --wy )
   {
      auto [ _, local ] = WorldToChunk( WorldBlockPos { pos.x, wy, pos.z } );
      if( chunk.GetBlock( local ).GetId() != BlockId::Air )
         return wy;
   }

   return 0;
}


void Level::UpdateStreaming( const glm::vec3& playerPos, uint8_t viewRadius )
{
   auto [ playerChunk, _ ] = WorldToChunk( WorldBlockPos { playerPos } );
   for( int dx = -viewRadius; dx <= viewRadius; ++dx )
   {
      for( int dz = -viewRadius; dz <= viewRadius; ++dz )
      {
         const ChunkPos cpos { playerChunk.x + dx, playerChunk.z + dz };
         if( !m_chunks.contains( cpos ) )
            EnsureChunk( cpos );
      }
   }

   auto inView = [ & ]( const ChunkPos& cpos ) { return std::abs( cpos.x - playerChunk.x ) <= viewRadius && std::abs( cpos.z - playerChunk.z ) <= viewRadius; };
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
   const int baseX = chunk.GetChunkPos().x * CHUNK_SIZE_X;
   const int baseZ = chunk.GetChunkPos().z * CHUNK_SIZE_Z;
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

            chunk.SetBlock( LocalBlockPos { x, y, z }, BlockState( id ) );
         }
      }
   }

   // Mark chunk dirty and save
   chunk.MarkDirty( ChunkDirty::Save | ChunkDirty::Mesh );
   chunk.SaveToDisk();
}


void Level::MarkChunkAndNeighborsMeshDirty( const ChunkPos& cpos )
{
   auto mark = [ & ]( const ChunkPos& c )
   {
      if( auto it = m_chunks.find( c ); it != m_chunks.end() )
         it->second.MarkDirty( ChunkDirty::Mesh );
   };

   mark( cpos );
   mark( { cpos.x - 1, cpos.z } );
   mark( { cpos.x + 1, cpos.z } );
   mark( { cpos.x, cpos.z - 1 } );
   mark( { cpos.x, cpos.z + 1 } );
}


Chunk& Level::EnsureChunk( const ChunkPos& cpos )
{
   if( auto it = m_chunks.find( cpos ); it != m_chunks.end() )
      return it->second;

   Chunk& chunk = m_chunks.try_emplace( cpos, *this, cpos ).first->second;
   if( !chunk.FLoadFromDisk() )
      GenerateChunkData( chunk );

   MarkChunkAndNeighborsMeshDirty( cpos );

   return chunk;
}
