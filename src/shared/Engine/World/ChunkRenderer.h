#pragma once

#include <glad/glad.h>

#include <Engine/World/Level.h>

class ChunkRenderer
{
public:
   ChunkRenderer() noexcept = default;
   ~ChunkRenderer() { Clear(); }

   struct Vertex
   {
      glm::vec3 position {};
      glm::vec3 normal {};
      glm::vec3 uv {};  // xy = texture coords, z = layer index
      glm::vec3 tint {};
   };

   struct MeshData
   {
      std::vector< Vertex >   vertices {};
      std::vector< uint32_t > indices {};

      void Clear()
      {
         vertices.clear();
         indices.clear();
      }

      bool FEmpty() const noexcept { return vertices.empty() || indices.empty(); }
   };

   struct SectionEntry
   {
      GLuint   vao { 0 };
      GLuint   vbo { 0 };
      GLuint   ebo { 0 };
      uint32_t indexCount { 0 };

      uint64_t builtRevision { 0 };
      bool     fEmpty { true };
   };

   struct Entry
   {
      std::array< SectionEntry, SECTIONS_PER_CHUNK > sections {};
      uint64_t                                       lastSeenRevision { 0 };
   };

   void        Update( Level& level, const glm::vec3& playerPos, uint8_t viewRadius );
   const auto& GetEntries() const noexcept { return m_entries; }

private:
   NO_COPY_MOVE( ChunkRenderer )

   void        Clear();
   static void DestroySectionGL( SectionEntry& e );

   static bool                                  InView( const ChunkPos& cc, const ChunkPos& center, uint8_t viewRadius );
   static std::tuple< ChunkPos, LocalBlockPos > WorldToChunkPos( WorldBlockPos wpos );

   static void BuildSectionMesh( const Level& level, const Chunk& chunk, int sectionIndex, MeshData& out );
   static void Upload( SectionEntry& e, const MeshData& mesh );

   std::unordered_map< ChunkPos, Entry, ChunkPosHash > m_entries;
}; // class ChunkRenderer
