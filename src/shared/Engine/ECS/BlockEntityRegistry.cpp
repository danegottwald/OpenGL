#include "BlockEntityRegistry.h"

namespace Engine::ECS
{

Entity::Entity BlockEntityRegistry::Find( WorldBlockPos pos ) const noexcept
{
   const BlockPos key { pos.x, pos.y, pos.z };
   if( auto it = m_map.find( key ); it != m_map.end() )
      return it->second;
   return Entity::NullEntity;
}

void BlockEntityRegistry::Bind( WorldBlockPos pos, Entity::Entity e )
{
   const BlockPos key { pos.x, pos.y, pos.z };
   m_map[ key ] = e;
}

void BlockEntityRegistry::Unbind( WorldBlockPos pos )
{
   const BlockPos key { pos.x, pos.y, pos.z };
   m_map.erase( key );
}

} // namespace Engine::ECS
