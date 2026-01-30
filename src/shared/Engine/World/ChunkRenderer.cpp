#include "ChunkRenderer.h"

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
   Direction { TextureAtlas::BlockFace::North,  0,  0,  -1, glm::vec3( 0.f,  0.f,  -1.f ) },
   Direction { TextureAtlas::BlockFace::East,   1,  0,  0,  glm::vec3( 1.f,  0.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::South,  0,  0,  1,  glm::vec3( 0.f,  0.f,  1.f )  },
   Direction { TextureAtlas::BlockFace::West,   -1, 0,  0,  glm::vec3( -1.f, 0.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::Top,    0,  1,  0,  glm::vec3( 0.f,  1.f,  0.f )  },
   Direction { TextureAtlas::BlockFace::Bottom, 0,  -1, 0,  glm::vec3( 0.f,  -1.f, 0.f )  },
};

// Unit quads in [0,1] block space.
constexpr glm::vec3 kFaceVerts[ 6 ][ 4 ] = {
   { { 1.f, 0.f, 0.f }, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 1.f, 1.f, 0.f } }, // -Z (North)
   { { 1.f, 0.f, 0.f }, { 1.f, 1.f, 0.f }, { 1.f, 1.f, 1.f }, { 1.f, 0.f, 1.f } }, // +X (East)
   { { 0.f, 0.f, 1.f }, { 1.f, 0.f, 1.f }, { 1.f, 1.f, 1.f }, { 0.f, 1.f, 1.f } }, // +Z (South)
   { { 0.f, 0.f, 1.f }, { 0.f, 1.f, 1.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 0.f } }, // -X (West)
   { { 0.f, 1.f, 0.f }, { 0.f, 1.f, 1.f }, { 1.f, 1.f, 1.f }, { 1.f, 1.f, 0.f } }, // +Y (Top)
   { { 0.f, 0.f, 1.f }, { 0.f, 0.f, 0.f }, { 1.f, 0.f, 0.f }, { 1.f, 0.f, 1.f } }, // -Y (Bottom)
};

constexpr std::array< int, 4 >                  identity { 0, 1, 2, 3 };
constexpr std::array< int, 4 >                  rotate90 { 1, 2, 3, 0 };
constexpr std::array< std::array< int, 4 >, 6 > kFaceUVs = {
   identity, // North
   rotate90, // East
   identity, // South
   rotate90, // West
   identity, // Top
   identity, // Bottom
};

// Standard quad UVs (0,0) -> (1,0) -> (1,1) -> (0,1)
constexpr glm::vec2 kQuadUVs[ 4 ] = {
   { 0.0f, 0.0f },
   { 1.0f, 0.0f },
   { 1.0f, 1.0f },
   { 0.0f, 1.0f }
};

}

void ChunkRenderer::DestroySectionGL( SectionEntry& e )
{
   if( e.vao )
      glDeleteVertexArrays( 1, &e.vao );
   if( e.vbo )
      glDeleteBuffers( 1, &e.vbo );
   if( e.ebo )
      glDeleteBuffers( 1, &e.ebo );

   e.vao           = 0;
   e.vbo           = 0;
   e.ebo           = 0;
   e.indexCount    = 0;
   e.builtRevision = 0;
   e.fEmpty        = true;
}

void ChunkRenderer::Clear()
{
   for( auto& [ _, ce ] : m_entries )
      for( auto& sec : ce.sections )
         DestroySectionGL( sec );

   m_entries.clear();
}

std::tuple< ChunkPos, LocalBlockPos > ChunkRenderer::WorldToChunkPos( WorldBlockPos wpos )
{
   auto divFloor = []( int a, int b )
   {
      const int q = a / b, r = a % b;
      return ( r != 0 ) && ( ( r > 0 ) != ( b > 0 ) ) ? q - 1 : q;
   };

   const int cx = divFloor( wpos.x, CHUNK_SIZE_X ), lx = wpos.x - cx * CHUNK_SIZE_X;
   const int cy = divFloor( wpos.y, CHUNK_SIZE_Y ), ly = wpos.y - cy * CHUNK_SIZE_Y;
   const int cz = divFloor( wpos.z, CHUNK_SIZE_Z ), lz = wpos.z - cz * CHUNK_SIZE_Z;
   return {
      ChunkPos { cx, cz },
       LocalBlockPos { lx, ly, lz }
   };
}

bool ChunkRenderer::InView( const ChunkPos& cc, const ChunkPos& center, uint8_t viewRadius )
{
   return std::abs( cc.x - center.x ) <= viewRadius && std::abs( cc.z - center.z ) <= viewRadius;
}

void ChunkRenderer::BuildSectionMesh( const Level& level, const Chunk& chunk, int sectionIndex, MeshData& out )
{
   out.Clear();
   out.vertices.reserve( CHUNK_SECTION_VOLUME * 4 );
   out.indices.reserve( CHUNK_SECTION_VOLUME * 6 );

   const int baseWX = chunk.GetChunkPos().x * CHUNK_SIZE_X;
   const int baseWZ = chunk.GetChunkPos().z * CHUNK_SIZE_Z;
   const int baseY  = sectionIndex * CHUNK_SECTION_SIZE;

   for( int x = 0; x < CHUNK_SIZE_X; ++x )
   {
      for( int ly = 0; ly < CHUNK_SECTION_SIZE; ++ly )
      {
         const int y = baseY + ly;
         for( int z = 0; z < CHUNK_SIZE_Z; ++z )
         {
            const BlockState state = chunk.GetBlock( LocalBlockPos { x, y, z } );
            const BlockId    id    = state.GetId();
            if( id == BlockId::Air )
               continue;

            const WorldBlockPos wpos { baseWX + x, y, baseWZ + z };
            const glm::vec3     basePos( static_cast< float >( x ), static_cast< float >( y ), static_cast< float >( z ) );
            for( const Direction& dir : directions )
            {
               const LocalBlockPos nlocal { x + dir.dx, y + dir.dy, z + dir.dz };
               const WorldBlockPos nworld { wpos.x + dir.dx, wpos.y + dir.dy, wpos.z + dir.dz };
               BlockState          neighborState = chunk.FInBounds( nlocal ) ? chunk.GetBlock( nlocal ) : level.GetBlock( nworld );
               if( neighborState.GetId() != BlockId::Air )
                  continue;

               const TextureAtlas::Region& region      = TextureAtlasManager::Get().GetRegion( state, dir.face );
               const float                 layerF      = static_cast< float >( region.layer );
               const uint32_t              indexOffset = static_cast< uint32_t >( out.vertices.size() );
               for( int i = 0; i < 4; ++i )
               {
                  Vertex            v {};
                  const int         uvIdx = kFaceUVs[ static_cast< size_t >( dir.face ) ][ i ];
                  const glm::vec2&  quadUV = kQuadUVs[ uvIdx ];
                  v.position = basePos + kFaceVerts[ static_cast< size_t >( dir.face ) ][ i ];
                  v.normal   = dir.normal;
                  v.uv       = glm::vec3( quadUV.x, quadUV.y, layerF );
                  v.tint     = glm::vec3( 1.0f );
                  out.vertices.push_back( v );
               }

               out.indices.push_back( indexOffset + 0 );
               out.indices.push_back( indexOffset + 1 );
               out.indices.push_back( indexOffset + 2 );

               out.indices.push_back( indexOffset + 0 );
               out.indices.push_back( indexOffset + 2 );
               out.indices.push_back( indexOffset + 3 );
            }
         }
      }
   }
}

void ChunkRenderer::Update( Level& level, const glm::vec3& playerPos, uint8_t viewRadius )
{
   level.UpdateStreaming( playerPos, viewRadius );

   auto [ playerChunk, _ ] = WorldToChunkPos( WorldBlockPos { playerPos } );
   for( auto it = m_entries.begin(); it != m_entries.end(); )
   {
      if( !InView( it->first, playerChunk, viewRadius ) )
      {
         for( auto& sec : it->second.sections )
            DestroySectionGL( sec );

         it = m_entries.erase( it );
      }
      else
         ++it;
   }

   MeshData mesh;
   for( const auto& [ cc, chunk ] : level.GetChunks() )
   {
      if( !InView( cc, playerChunk, viewRadius ) )
         continue;

      Entry&         ce  = m_entries[ cc ];
      const uint64_t rev = chunk.MeshRevision();
      if( ce.lastSeenRevision == rev && !Any( chunk.Dirty() & ChunkDirty::Mesh ) )
         continue;

      for( const auto& [ i, sec ] : ce.sections | std::views::enumerate )
      {
         if( sec.builtRevision == rev && !Any( chunk.Dirty() & ChunkDirty::Mesh ) )
            continue;

         BuildSectionMesh( level, chunk, i, mesh );
         Upload( sec, mesh );
         sec.builtRevision = rev;
      }

      ce.lastSeenRevision = rev;
      const_cast< Chunk& >( chunk ).ClearDirty( ChunkDirty::Mesh );
   }
}

void ChunkRenderer::Upload( SectionEntry& e, const MeshData& mesh )
{
   if( mesh.FEmpty() )
   {
      e.indexCount = 0;
      e.fEmpty     = true;
      return;
   }

   if( !e.vao )
   {
      glGenVertexArrays( 1, &e.vao );
      glGenBuffers( 1, &e.vbo );
      glGenBuffers( 1, &e.ebo );
   }

   glBindVertexArray( e.vao );

   glBindBuffer( GL_ARRAY_BUFFER, e.vbo );
   glBufferData( GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof( Vertex ), mesh.vertices.data(), GL_STATIC_DRAW );

   glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, e.ebo );
   glBufferData( GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof( uint32_t ), mesh.indices.data(), GL_STATIC_DRAW );

   glEnableVertexAttribArray( 0 );
   glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* )offsetof( Vertex, position ) );

   glEnableVertexAttribArray( 1 );
   glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* )offsetof( Vertex, normal ) );

   glEnableVertexAttribArray( 2 );
   glVertexAttribPointer( 2, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* )offsetof( Vertex, uv ) );

   glEnableVertexAttribArray( 3 );
   glVertexAttribPointer( 3, 3, GL_FLOAT, GL_FALSE, sizeof( Vertex ), ( void* )offsetof( Vertex, tint ) );

   glBindVertexArray( 0 );

   e.indexCount = static_cast< uint32_t >( mesh.indices.size() );
   e.fEmpty     = false;
}
