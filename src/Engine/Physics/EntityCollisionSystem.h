#pragma once

#include <Engine/Physics/CollisionEvents.h>

namespace Engine::Physics
{

// Very simple entity-entity overlap detection using existing CPhysics AABBs.
// Produces Enter/Stay/Exit via CollisionEventQueue.
void CollectEntityAABBCollisions( Entity::Registry& registry, CollisionEventQueue& queue );

} // namespace Engine::Physics
