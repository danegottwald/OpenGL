#pragma once

// Project dependencies
#include <Events/NetworkEvent.h>
#include <FastNoiseLite/FastNoiseLite.h>

using GLuint = unsigned int;

// Forward Declarations
class Camera;
class ChunkMesh;
class FastNoiseLite;
class IMesh;
namespace Entity
{
typedef uint64_t Entity;
class Registry;
class EntityHandle;
}

constexpr uint8_t CHUNK_SIZE = 16;
constexpr uint16_t CHUNK_HEIGHT = 1;

class World
{
public:
   explicit World( Entity::Registry& registry );
   ~World() = default;

   void Setup( Entity::Registry& registry );

   
   struct Chunk
   {
      Entity::Entity                                                         m_entity; // entity representing this chunk
      glm::ivec2                                                             chunkPos; // chunk coordinates in world
      std::shared_ptr< ChunkMesh >                                           mesh;     // for rendering
      std::array< std::array< int, CHUNK_SIZE >, CHUNK_HEIGHT * CHUNK_SIZE > blocks;   // block data
   };
   glm::ivec2                              m_playerChunk { INT_MIN, INT_MIN }; // current chunk player is in
   std::unordered_map< glm::ivec2, Chunk > m_activeChunks; // only active chunks

   struct Block
   {
      Entity::Entity entity;
      glm::ivec3     position;
   };
   std::vector< Entity::Entity > m_activeBlocks;
   std::vector< Block > m_activeBlocks2;

   void CreateChunkMesh( Entity::Registry& registry, _Inout_ std::shared_ptr< ChunkMesh >& mesh, const glm::ivec2& chunkPos, FastNoiseLite& noise );
   void UpdateActiveChunks( Entity::Registry& registry, const glm::vec3& playerPos );

   struct Chunk2
   {
      // Holds the block data
      enum class BlockID
      {
         Air       = 0,
         Dirt      = 1,
         Grass     = 2,
         Stone     = 3,
         // Add more block types as needed
      };
      BlockID blocks[ CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE ];
   };

private:
   // All relevant world data
   //      Generate terrain on construction?
   // Maybe list of all entities? then Tick them in Tick()?
   // std::vector< Entity > m_EntityList;

   FastNoiseLite m_mapNoise; // https://auburn.github.io/FastNoiseLite/

   std::vector< std::shared_ptr< Entity::EntityHandle > >                  m_entityHandles;
   std::unordered_map< uint64_t, std::shared_ptr< Entity::EntityHandle > > m_playerHandles; // Maps player ID to their entity handle

   Events::EventSubscriber m_eventSubscriber;
};
