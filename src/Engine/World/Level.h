#pragma once

#include <FastNoiseLite/FastNoiseLite.h>

#include <Engine/Core/Time.h>
#include <Engine/World/Blocks.h>
#include <Engine/World/WorldSave.h>

// Forward Declarations
class TimeAccumulator;

// Fixed chunk dimensions
static constexpr int CHUNK_SIZE_X = 16;
static constexpr int CHUNK_SIZE_Y = 256;
static constexpr int CHUNK_SIZE_Z = 16;
static constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

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
   None  = 0,
   Mesh  = 1u << 0, // geometry needs rebuild
   Save  = 1u << 1, // needs saving
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

class Chunk
{
public:
   Chunk( class Level& level, int cx, int cy, int cz );

   // Block accessors/mutators
   BlockId GetBlock( int x, int y, int z ) const noexcept;
   void    SetBlock( int x, int y, int z, BlockId id );

   // Coordinate accessors
   int  ChunkX() const { return m_chunkX; }
   int  ChunkY() const { return m_chunkY; }
   int  ChunkZ() const { return m_chunkZ; }
   bool FInBounds( int x, int y, int z ) const noexcept;

   // Save/Load
   bool FLoadFromDisk();
   void SaveToDisk();

   // Owning level access
   const Level& GetLevel() const noexcept { return m_level; }
   Level&       GetLevel() noexcept { return m_level; }

   // Dirty tracking
   ChunkDirty Dirty() const noexcept { return m_dirty; }
   void       ClearDirty( ChunkDirty bits ) noexcept { m_dirty = static_cast< ChunkDirty >( static_cast< uint32_t >( m_dirty ) & ~static_cast< uint32_t >( bits ) ); }
   uint64_t   MeshRevision() const noexcept { return m_meshRevision; }

private:
   NO_COPY_MOVE( Chunk )

   void MarkDirty( ChunkDirty bits ) noexcept { m_dirty = m_dirty | bits; }

   static int         ToIndex( int x, int y, int z ) noexcept;
   World::ChunkCoord3 GetCoord3() const noexcept { return World::ChunkCoord3 { m_chunkX, m_chunkY, m_chunkZ }; }

   class Level&           m_level;
   int                    m_chunkX, m_chunkY, m_chunkZ;
   std::vector< BlockId > m_blocks;

   ChunkDirty m_dirty { ChunkDirty::Mesh }; // new chunks need a mesh
   uint64_t   m_meshRevision { 1 };         // increments when blocks affecting mesh change

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

   BlockId GetBlock( int wx, int wy, int wz ) const noexcept;
   void    SetBlock( int wx, int wy, int wz, BlockId id );
   void    SetBlock( glm::ivec3 pos, BlockId id ) { SetBlock( pos.x, pos.y, pos.z, id ); }

   void Explode( int wx, int wy, int wz, uint8_t radius );
   void Explode( glm::ivec3 pos, uint8_t radius ) { Explode( pos.x, pos.y, pos.z, radius ); }

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
   TimeAccumulator        m_autosaveTimer;
   std::filesystem::path  m_worldDir;
   World::WorldMeta       m_meta;

   ChunkCoord m_lastPlayerChunk { INT32_MIN, INT32_MIN };

   std::unordered_map< ChunkCoord, Chunk, ChunkCoordHash > m_chunks;

   FastNoiseLite m_noise;

   friend class Chunk;
};
