#include "Raycast.h"

// Local Dependencies
#include "Level.h"

// Project Dependencies
#include <Engine/ECS/Components/Camera.h>


Ray CreateRay( const glm::vec3& origin, const glm::vec3& rotation, float maxDistance )
{
   const float pitch = glm::radians( rotation.x );
   const float yaw   = glm::radians( rotation.y );

   glm::vec3 direction;
   direction.x = glm::cos( pitch ) * glm::sin( yaw );
   direction.y = -glm::sin( pitch );
   direction.z = -glm::cos( pitch ) * glm::cos( yaw );
   direction   = glm::normalize( direction );

   return Ray { origin, direction, maxDistance };
}


std::optional< RaycastResult > TryRaycast( const Level& level, const Ray& ray )
{
   glm::vec3 dir = glm::normalize( ray.direction );
   if( glm::dot( dir, dir ) < 1e-12f )
      return std::nullopt; // zero-length ray

   // Current voxel
   glm::ivec3 blockPos = glm::floor( ray.origin );

   // If we start inside a solid block, hit it immediately
   if( FSolid( level.GetBlock( WorldBlockPos { blockPos } ) ) )
      return RaycastResult { blockPos,
                             ray.origin, // exact start position
                             glm::ivec3( dir.x < 0.0f ? 1 : -1, dir.y < 0.0f ? 1 : -1, dir.z < 0.0f ? 1 : -1 ),
                             0.0f };

   // DDA setup
   glm::ivec3 step( dir.x < 0.0f ? -1 : 1, dir.y < 0.0f ? -1 : 1, dir.z < 0.0f ? -1 : 1 );
   glm::vec3  invDir( dir.x != 0.0f ? 1.0f / dir.x : std::numeric_limits< float >::infinity(),
                     dir.y != 0.0f ? 1.0f / dir.y : std::numeric_limits< float >::infinity(),
                     dir.z != 0.0f ? 1.0f / dir.z : std::numeric_limits< float >::infinity() );

   // Distance from origin to first voxel boundary on each axis
   glm::vec3 tMax {};
   for( int i = 0; i < 3; ++i )
      tMax[ i ] = ( static_cast< float >( blockPos[ i ] ) + ( step[ i ] > 0 ? 1.0f : 0.0f ) - ray.origin[ i ] ) * invDir[ i ];

   float      dist { 0.0f };
   glm::vec3  tDelta { glm::abs( invDir ) };
   glm::ivec3 hitNormal { 0 };
   while( dist <= ray.maxDistance )
   {
      if( FSolid( level.GetBlock( WorldBlockPos { blockPos } ) ) )
         return RaycastResult { blockPos, ray.origin + dir * dist, hitNormal, dist };

      // Step to next voxel
      int axis = 0;
      if( tMax.y < tMax.x )
         axis = 1;
      if( tMax.z < tMax[ axis ] )
         axis = 2;

      dist = tMax[ axis ];
      tMax[ axis ] += tDelta[ axis ];
      blockPos[ axis ] += step[ axis ];
      hitNormal         = glm::ivec3( 0 );
      hitNormal[ axis ] = -step[ axis ];
   }

   return std::nullopt;
}
