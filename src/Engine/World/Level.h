#pragma once

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

struct ChunkCoord
{
   int  x { INT32_MIN };
   int  z { INT32_MIN };
   bool operator==( const ChunkCoord& ) const = default;
};

struct ChunkCoordHash
{
   std::size_t operator()( const ChunkCoord& c ) const noexcept
   {
      std::size_t h   = 1469598103934665603ull;
      auto        mix = [ &h ]( int v ) { h ^= static_cast< std::size_t >( v ) + 0x9e3779b97f4a7c15ull + ( h << 6 ) + ( h >> 2 ); };
      mix( c.x );
      mix( c.z );
      return h;
   }
};

// ----------------------------------------------------------------
// Chunk - world data for a fixed-size region (no rendering ownership)
// ----------------------------------------------------------------
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

class ChunkSection
{
public:
   ChunkSection()  = default;
   ~ChunkSection() = default;

   BlockState GetBlock( int x, int y, int z ) const noexcept;
   void       SetBlock( int x, int y, int z, BlockState state );

   bool FDirty() const noexcept { return m_fDirty; }
   void ClearDirty() noexcept { m_fDirty = false; }

private:
   NO_COPY_MOVE( ChunkSection )

   static size_t ToIndex( int x, int y, int z ) noexcept;
   static bool   FInBounds( int x, int y, int z ) noexcept;

   std::array< BlockState, CHUNK_SECTION_VOLUME > m_blocks { BlockState( BlockId::Air ) };
   bool                                           m_fDirty { true };
};

class Chunk
{
public:
   Chunk( class Level& level, int cx, int cy, int cz );

   BlockState GetBlock( int x, int y, int z ) const noexcept;
   void       SetBlock( int x, int y, int z, BlockState state );

   int  ChunkX() const { return m_chunkX; }
   int  ChunkY() const { return m_chunkY; }
   int  ChunkZ() const { return m_chunkZ; }
   bool FInBounds( int x, int y, int z ) const noexcept;

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

   World::ChunkCoord3 GetCoord3() const noexcept { return World::ChunkCoord3 { m_chunkX, m_chunkY, m_chunkZ }; }

   class Level& m_level;
   int          m_chunkX, m_chunkY, m_chunkZ;

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

   BlockState GetBlock( int wx, int wy, int wz ) const noexcept;
   void       SetBlock( int wx, int wy, int wz, BlockState state );
   void       SetBlock( glm::ivec3 pos, BlockState state ) { SetBlock( pos.x, pos.y, pos.z, state ); }

   void Explode( int wx, int wy, int wz, uint8_t radius );
   void Explode( glm::ivec3 pos, uint8_t radius ) { Explode( pos.x, pos.y, pos.z, radius ); }

   int GetSurfaceY( int wx, int wz ) noexcept;

   // Streaming only: ensures chunk *data* exists around the player.
   // Rendering caches are owned elsewhere.
   void UpdateStreaming( const glm::vec3& playerPos, uint8_t viewRadius );

   const auto& GetChunks() const { return m_chunks; }

private:
   NO_COPY_MOVE( Level )

   std::tuple< ChunkCoord, glm::ivec3 > WorldToChunk( int wx, int wy, int wz ) const noexcept;
   Chunk&                               EnsureChunk( const ChunkCoord& cc );
   void                                 GenerateChunkData( Chunk& chunk );
   void                                 MarkChunkAndNeighborsMeshDirty( const ChunkCoord& cc );

   // World saving/loading
   static constexpr float AUTOSAVE_INTERVAL = 10.0f; // seconds
   Time::IntervalTimer    m_autosaveTimer;
   std::filesystem::path  m_worldDir;
   World::WorldMeta       m_meta;

   ChunkCoord m_lastPlayerChunk { INT32_MIN, INT32_MIN };

   std::unordered_map< ChunkCoord, Chunk, ChunkCoordHash > m_chunks;

   FastNoiseLite m_noise;

   friend class Chunk;
};
