#include "EntityCollisionSystem.h"

// Project Dependencies
#include <Engine/ECS/Components.h>

namespace Engine::Physics
{

static bool AabbOverlap( const glm::vec3& aMin, const glm::vec3& aMax, const glm::vec3& bMin, const glm::vec3& bMax )
{
   return ( aMin.x <= bMax.x && aMax.x >= bMin.x ) && ( aMin.y <= bMax.y && aMax.y >= bMin.y ) && ( aMin.z <= bMax.z && aMax.z >= bMin.z );
}

void CollectEntityAABBCollisions( Entity::Registry& registry, CollisionEventQueue& queue )
{
   queue.BeginCollect();

   // Naive O(n^2) broadphase. Good enough for small counts; can be replaced by spatial hash without changing consumers.
   std::vector< Entity::Entity > entities;
   entities.reserve( 256 );

   for( auto [ e, tran, phys ] : registry.ECView< CTransform, CPhysics >() )
      entities.push_back( e );

   for( size_t i = 0; i < entities.size(); ++i )
   {
      const Entity::Entity a  = entities[ i ];
      const CTransform*    at = registry.TryGet< CTransform >( a );
      const CPhysics*      ap = registry.TryGet< CPhysics >( a );
      if( !( at && ap ) )
         continue;

      const glm::vec3 aMin = at->position + ap->bbMin;
      const glm::vec3 aMax = at->position + ap->bbMax;
      for( size_t j = i + 1; j < entities.size(); ++j )
      {
         const Entity::Entity b  = entities[ j ];
         const CTransform*    bt = registry.TryGet< CTransform >( b );
         const CPhysics*      bp = registry.TryGet< CPhysics >( b );
         if( !( bt && bp ) )
            continue;

         const glm::vec3 bMin = bt->position + bp->bbMin;
         const glm::vec3 bMax = bt->position + bp->bbMax;
         if( AabbOverlap( aMin, aMax, bMin, bMax ) )
            queue.AddOverlapPair( a, b );
      }
   }

   queue.EndCollect();
}

} // namespace Engine::Physics
