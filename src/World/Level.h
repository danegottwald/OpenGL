#pragma once

#include "pch.h"
#include <FastNoiseLite/FastNoiseLite.h>

using BlockId                        = uint16_t; // up to 65535 block types
static constexpr BlockId BLOCK_AIR   = 0;
static constexpr BlockId BLOCK_DIRT  = 1;
static constexpr BlockId BLOCK_STONE = 2;

struct Block
{
   BlockId id;
   uint8_t meta; // bit-packed flags or small subtype data
};

// Fixed chunk dimensions
static constexpr int CHUNK_SIZE_X = 16;
static constexpr int CHUNK_SIZE_Y = 256;
static constexpr int CHUNK_SIZE_Z = 16;
static constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

struct ChunkVertex
{
   glm::vec3 position;
   glm::vec3 normal;
   glm::vec2 uv;
   // add color/light/ao if needed

   uint16_t textureIndex;
};

struct ChunkMesh
{
   std::vector< ChunkVertex > vertices;
   std::vector< uint32_t >    indices;

   void Clear()
   {
      vertices.clear();
      indices.clear();
   }

   bool FEmpty() const { return vertices.empty(); }
};

class Chunk
{
public:
   Chunk( class Level& level, int cx, int cy, int cz );

   BlockId GetBlock( int x, int y, int z ) const;
   void    SetBlock( int x, int y, int z, BlockId id );

   // Position in chunk grid
   int ChunkX() const { return m_chunkX; }
   int ChunkY() const { return m_chunkY; }
   int ChunkZ() const { return m_chunkZ; }

   void             GenerateMesh();
   const ChunkMesh& GetMesh() const { return m_mesh; }

private:
   bool       InBounds( int x, int y, int z ) const;
   static int ToIndex( int x, int y, int z );

   Level&                 m_level;
   int                    m_chunkX, m_chunkY, m_chunkZ;
   std::vector< BlockId > m_blocks;
   bool                   m_dirty = false; // needs mesh rebuild, etc.

   ChunkMesh m_mesh {};
};

struct ChunkCoord
{
   int  x;
   int  z;
   bool operator==( const ChunkCoord& ) const = default;
};

struct ChunkCoordHash
{
   std::size_t operator()( const ChunkCoord& c ) const noexcept
   {
      // Simple 3D hash; can be improved later
      std::size_t h   = 1469598103934665603ull;
      auto        mix = [ &h ]( int v ) { h ^= static_cast< std::size_t >( v ) + 0x9e3779b97f4a7c15ull + ( h << 6 ) + ( h >> 2 ); };
      mix( c.x );
      mix( c.z );
      return h;
   }
};


struct ChunkRender
{
   GLuint  vao        = 0;
   GLuint  vbo        = 0;
   GLuint  ebo        = 0;
   GLsizei indexCount = 0;
   bool    uploaded   = false;

   void Upload( const ChunkMesh& mesh )
   {
      if( mesh.FEmpty() )
      {
         indexCount = 0;
         return;
      }

      if( !vao )
      {
         glGenVertexArrays( 1, &vao );
         glGenBuffers( 1, &vbo );
         glGenBuffers( 1, &ebo );
      }

      glBindVertexArray( vao );
      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof( ChunkVertex ), mesh.vertices.data(), GL_STATIC_DRAW );

      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof( uint32_t ), mesh.indices.data(), GL_STATIC_DRAW );

      // Setup vertex attributes
      {
         // layout(location = 0) vec3 position
         glEnableVertexAttribArray( 0 );
         glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( ChunkVertex ), ( void* )offsetof( ChunkVertex, position ) );
         // layout(location = 1) vec3 normal
         glEnableVertexAttribArray( 1 );
         glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( ChunkVertex ), ( void* )offsetof( ChunkVertex, normal ) );
         // layout(location = 2) vec2 uv
         glEnableVertexAttribArray( 2 );
         glVertexAttribPointer( 2, 2, GL_FLOAT, GL_FALSE, sizeof( ChunkVertex ), ( void* )offsetof( ChunkVertex, uv ) );
         // layout(location = 3) uint16_t textureIndex
         glEnableVertexAttribArray( 3 );
         glVertexAttribPointer( 3, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof( ChunkVertex ), ( void* )offsetof( ChunkVertex, textureIndex ) );
      }

      glBindVertexArray( 0 );

      indexCount = static_cast< GLsizei >( mesh.indices.size() );
      uploaded   = true;
   }

   void Render() const
   {
      if( !uploaded || indexCount == 0 )
         return;

      glBindVertexArray( vao );
      glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr );
      glBindVertexArray( 0 );
   }
};


// Owns chunk storage and world parameters (seed, world height, etc.)
// API for querying/modifying blocks by world coordinates
// Manages chunk lifetime and generation
class Level
{
public:
   Level();
   ~Level() = default;

   BlockId GetBlock( int wx, int wy, int wz ) const;
   void    SetBlock( int wx, int wy, int wz, BlockId id );

   void Explode( int wx, int wy, int wz, uint8_t radius );

   // Call during update to load/unload chunks around player
   void UpdateVisibleRegion( const glm::vec3& playerPos, int viewRadius );

   // Read-only access to all loaded chunks
   const std::unordered_map< ChunkCoord, std::unique_ptr< Chunk >, ChunkCoordHash >& GetChunks() const { return m_chunks; }

   // Chunk render management
   const ChunkRender* GetChunkRender( const ChunkCoord& coord ) const;
   ChunkRender&       EnsureChunkRender( const ChunkCoord& coord );
   void               SyncChunkRender( const ChunkCoord& coord );

private:
   void   WorldToChunk( int wx, int wy, int wz, ChunkCoord& outChunk, glm::ivec3& outLocal ) const;
   Chunk& EnsureChunk( const ChunkCoord& cc );
   void   GenerateChunkData( Chunk& chunk );

   std::unordered_map< ChunkCoord, std::unique_ptr< Chunk >, ChunkCoordHash > m_chunks;
   std::unordered_map< ChunkCoord, ChunkRender, ChunkCoordHash >              m_chunkRenders;

   FastNoiseLite m_noise;
};
