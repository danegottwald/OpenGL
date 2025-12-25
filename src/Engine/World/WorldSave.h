#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <variant>

namespace World
{

struct WorldMeta
{
   uint32_t version { 1 };
   uint32_t seed { 0 };
   uint64_t tick { 0 };
};

struct PlayerSave
{
   glm::vec3 position { 0.0f };
   // TODO: inventory/stats
};

struct EntitySave
{
   uint64_t id { 0 }; // stable id for entity file naming
   // TODO: serialized components
};

struct ChunkCoord3
{
   int x { 0 };
   int y { 0 };
   int z { 0 };
};

class WorldSave
{
public:
   // Gets the root directory for the given world name
   static std::filesystem::path RootDir( const std::filesystem::path& worldName );

   // Meta
   static bool                       FSaveMeta( const std::filesystem::path& worldDir, const WorldMeta& meta );
   static std::optional< WorldMeta > LoadMeta( const std::filesystem::path& worldDir );

   static bool                        FSavePlayer( const std::filesystem::path& worldDir, const PlayerSave& player );
   static std::optional< PlayerSave > LoadPlayer( const std::filesystem::path& worldDir );

   static bool FSaveEntity( const std::filesystem::path& worldDir, const EntitySave& entity );
   static bool FDeleteEntityFile( const std::filesystem::path& worldDir, uint64_t entityId );

   static bool FSaveChunkBytes( const std::filesystem::path& worldDir, const ChunkCoord3& cc, std::span< const std::byte > bytes );
   static bool FLoadChunkBytes( const std::filesystem::path& worldDir, const ChunkCoord3& cc, std::vector< std::byte >& outBytes );
};

} // namespace World
