#pragma once

#include <Engine/World/Level.h>

// Render-side cache for voxel chunks.
// Owns all OpenGL objects and any CPU-side mesh buffers used for upload.
//
// Design notes:
// - `Chunk` remains world-data only.
// - This cache can be created/destroyed depending on visibility.
// - Uses a revision counter to avoid boolean "needs remesh" threading issues.

class Shader;

class ChunkRenderer
{
public:
   ChunkRenderer() noexcept = default;
   ~ChunkRenderer() { Clear(); }

   struct ChunkVertex
   {
      glm::vec3 position;
      glm::vec3 normal;
      glm::vec2 uv;
      glm::vec3 tint;
   };

   struct ChunkMeshData
   {
      std::vector< ChunkVertex > vertices;
      std::vector< uint32_t >    indices;

      void Clear()
      {
         vertices.clear();
         indices.clear();
      }

      bool FEmpty() const noexcept { return vertices.empty() || indices.empty(); }
   };

   struct Entry
   {
      GLuint   vao        = 0;
      GLuint   vbo        = 0;
      GLuint   ebo        = 0;
      uint32_t indexCount = 0;

      uint64_t builtRevision = 0; // last `Chunk::MeshRevision()` that was uploaded
   };

   // Updates the set of visible chunk renders and rebuilds meshes as needed.
   void Update( Level& level, const glm::vec3& playerPos, uint8_t viewRadius );

   void Render( const Level& level, Shader& shader, const glm::mat4& viewProjection ) const;

   const auto& GetEntries() const noexcept { return m_entries; }


private:
   NO_COPY_MOVE( ChunkRenderer )

   void        Clear();
   static void DestroyEntryGL( Entry& e );
   static void BuildMesh( const Level& level, const Chunk& chunk, ChunkMeshData& out );
   static void Upload( Entry& e, const ChunkMeshData& mesh );
   static bool InView( const ChunkCoord& cc, const ChunkCoord& center, uint8_t viewRadius );

   std::unordered_map< ChunkCoord, Entry, ChunkCoordHash > m_entries;
};
