#pragma once

// Ensure third-party headers that include std headers (like <cmath>) can still compile under the
// "std headers only via PCH" rule.
#include "pch_shared.h"

#include <FastNoiseLite/FastNoiseLite.h>

#include <Engine/Core/Time.h>
#include <Engine/World/Blocks.h>
#include <Engine/World/WorldSave.h>


// Chunk Layout
// ------------------------- Y=255
// |                       |
// |   Chunk (16x256x16)   |
// |                       |
// |  -------------------  |
// | |   Chunk Section   | | Y=240
// | |     (16x16x16)    | |
// | --------------------- |
// | --------------------- |
// | |   Chunk Section   | | Y=224
// | |     (16x16x16)    | |
// | --------------------- |
// | |       ...         | |
// |  -------------------  |

// Fixed chunk dimensions
static constexpr int CHUNK_SECTION_SIZE   = 16; // 16x16x16
static constexpr int CHUNK_SECTION_VOLUME = CHUNK_SECTION_SIZE * CHUNK_SECTION_SIZE * CHUNK_SECTION_SIZE;
static constexpr int CHUNK_SECTION_COUNT  = 16;

static constexpr int CHUNK_SIZE_X       = CHUNK_SECTION_SIZE;
static constexpr int CHUNK_SIZE_Y       = CHUNK_SECTION_SIZE * CHUNK_SECTION_COUNT;
static constexpr int CHUNK_SIZE_Z       = CHUNK_SECTION_SIZE;
static constexpr int CHUNK_VOLUME       = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;
static constexpr int SECTIONS_PER_CHUNK = CHUNK_SIZE_Y / CHUNK_SECTION_SIZE;

struct ChunkPos
{
   int  x { INT32_MIN };
   int  z { INT32_MIN };
   bool operator==( const ChunkPos& ) const = default;
};

struct ChunkPosHash
{
   std::size_t operator()( const ChunkPos& cpos ) const noexcept
   {
      std::size_t h   = 1469598103934665603ull;
      auto        mix = [ &h ]( int v ) { h ^= static_cast< std::size_t >( v ) + 0x9e3779b97f4a7c15ull + ( h << 6 ) + ( h >> 2 ); };
      mix( cpos.x );
      mix( cpos.z );
      return h;
   }
};

enum class ChunkDirty : uint32_t
{
   None = 0,
   Mesh = 1u << 0, // geometry needs rebuild
   Save = 1u << 1, // needs saving
};

constexpr ChunkDirty operator|( ChunkDirty a, ChunkDirty b ) noexcept
{
   return static_cast< ChunkDirty >( static_cast< uint32_t >( a ) | static_cast< uint32_t >( b ) );
}

constexpr ChunkDirty operator&( ChunkDirty a, ChunkDirty b ) noexcept
{
   return static_cast< ChunkDirty >( static_cast< uint32_t >( a ) & static_cast< uint32_t >( b ) );
}

constexpr bool Any( ChunkDirty bits ) noexcept
{
   return static_cast< uint32_t >( bits ) != 0u;
}

// ----------------------------------------------------------------
// BlockPos - integer 3D position in block coordinates
// ----------------------------------------------------------------
struct BlockPos
{
   int  x { INT32_MIN };
   int  y { INT32_MIN };
   int  z { INT32_MIN };
   bool operator==( const BlockPos& ) const = default;

   constexpr BlockPos( int x, int y, int z ) :
      x( x ),
      y( y ),
      z( z )
   {}
   constexpr explicit BlockPos( const glm::ivec3& v ) :
      BlockPos( v.x, v.y, v.z )
   {}
   constexpr explicit BlockPos( const glm::vec3& v ) :
      BlockPos( static_cast< glm::ivec3 >( glm::floor( v ) ) )
   {}

   constexpr glm::ivec3 ToIVec3() const noexcept { return { x, y, z }; }
};

struct BlockPosHash
{
   std::size_t operator()( const BlockPos& bpos ) const noexcept
   {
      std::size_t h   = 1469598103934665603ull;
      auto        mix = [ &h ]( int v ) { h ^= static_cast< std::size_t >( v ) + 0x9e3779b97f4a7c15ull + ( h << 6 ) + ( h >> 2 ); };
      mix( bpos.x );
      mix( bpos.y );
      mix( bpos.z );
      return h;
   }
};

struct WorldBlockPos : BlockPos
{
   using BlockPos::BlockPos;
};

struct LocalBlockPos : BlockPos
{
   using BlockPos::BlockPos;
};

// ----------------------------------------------------------------
// ChunkSection - 16x16x16 block subsection of a chunk
// ----------------------------------------------------------------
class ChunkSection
{
public:
   ChunkSection()  = default;
   ~ChunkSection() = default;

   BlockState GetBlock( LocalBlockPos pos ) const noexcept;
   void       SetBlock( LocalBlockPos pos, BlockState state );

private:
   NO_COPY_MOVE( ChunkSection )

   static size_t ToIndex( LocalBlockPos pos ) noexcept;
   static bool   FInBounds( LocalBlockPos pos ) noexcept;

   std::array< BlockState, CHUNK_SECTION_VOLUME > m_blocks { BlockState( BlockId::Air ) };
   bool                                           m_fDirty { true };

public:
   bool FDirty() const noexcept { return m_fDirty; }
   void ClearDirty() noexcept { m_fDirty = false; }
};

// ----------------------------------------------------------------
// Chunk - world data for a fixed-size region (no rendering ownership)
// ----------------------------------------------------------------
class Chunk
{
public:
   Chunk( class Level& level, const ChunkPos& cpos );

   BlockState GetBlock( LocalBlockPos pos ) const noexcept;
   void       SetBlock( LocalBlockPos pos, BlockState state );

   ChunkPos GetChunkPos() const noexcept { return m_cpos; }
   bool     FInBounds( LocalBlockPos pos ) const noexcept;

   bool FLoadFromDisk();
   void SaveToDisk();

   ChunkDirty Dirty() const noexcept { return m_dirty; }
   void ClearDirty( ChunkDirty bits ) noexcept { m_dirty = static_cast< ChunkDirty >( static_cast< uint32_t >( m_dirty ) & ~static_cast< uint32_t >( bits ) ); }
   uint64_t MeshRevision() const noexcept { return m_meshRevision; }

   std::span< const ChunkSection > GetSections() const noexcept { return m_sections; }

private:
   NO_COPY_MOVE( Chunk )

   void MarkDirty( ChunkDirty bits ) noexcept { m_dirty = m_dirty | bits; }

   static constexpr int ToSectionIndex( int y ) noexcept { return y / CHUNK_SECTION_SIZE; }
   static constexpr int ToSectionLocalY( int y ) noexcept { return y % CHUNK_SECTION_SIZE; }

   World::ChunkPos3 GetCoord3() const noexcept { return World::ChunkPos3 { m_cpos.x, 0, m_cpos.z }; }

   class Level&   m_level;
   const ChunkPos m_cpos { INT32_MIN, INT32_MIN };

   std::array< ChunkSection, SECTIONS_PER_CHUNK > m_sections;

   ChunkDirty m_dirty { ChunkDirty::Mesh };
   uint64_t   m_meshRevision { 1 };

   friend class Level;
};


class Level
{
public:
   explicit Level( std::filesystem::path worldName );
   ~Level() = default;

   void Update( float dt );

   // World save
   void Save() const;
   void SaveMeta() const;
   void SavePlayer( const glm::vec3& playerPos ) const;

   BlockState GetBlock( WorldBlockPos pos ) const noexcept;
   void       SetBlock( WorldBlockPos pos, BlockState state );
   void       Explode( WorldBlockPos pos, uint8_t radius );

   int GetSurfaceY( WorldBlockPos pos ) noexcept;
   int GetSurfaceY( int wx, int wz ) noexcept { return GetSurfaceY( WorldBlockPos { wx, 0, wz } ); }

   // Streaming only: ensures chunk *data* exists around the player.
   // Rendering caches are owned elsewhere.
   void UpdateStreaming( const glm::vec3& playerPos, uint8_t viewRadius );

   const auto& GetChunks() const { return m_chunks; }

private:
   NO_COPY_MOVE( Level )

   std::tuple< ChunkPos, LocalBlockPos > WorldToChunk( WorldBlockPos wpos ) const noexcept;
   Chunk&                                EnsureChunk( const ChunkPos& cpos );
   void                                  GenerateChunkData( Chunk& chunk );
   void                                  MarkChunkAndNeighborsMeshDirty( const ChunkPos& cpos );

   // World saving/loading
   static constexpr float AUTOSAVE_INTERVAL = 10.0f; // seconds
   Time::IntervalTimer    m_autosaveTimer;
   std::filesystem::path  m_worldDir;
   World::WorldMeta       m_meta;

   ChunkPos m_lastPlayerChunk { INT32_MIN, INT32_MIN };

   std::unordered_map< ChunkPos, Chunk, ChunkPosHash > m_chunks;

   FastNoiseLite m_noise;

   friend class Chunk;
};
