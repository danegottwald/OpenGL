#pragma once

#include <Engine/ECS/Registry.h>
#include <Engine/World/Level.h>

namespace Engine::ECS
{

// World-side mapping from BlockPos -> ECS entity.
// The authoritative world owns and persists block entities. Clients can mirror this.
class BlockEntityRegistry
{
public:
   [[nodiscard]] Entity::Entity Find( WorldBlockPos pos ) const noexcept;
   void                   Bind( WorldBlockPos pos, Entity::Entity e );
   void                   Unbind( WorldBlockPos pos );

private:
   std::unordered_map< BlockPos, Entity::Entity, BlockPosHash > m_map;
};

} // namespace Engine::ECS
