#pragma once

#include <glm/glm.hpp>

struct CVelocity
{
   glm::vec3 velocity { 0.0f, 0.0f, 0.0f };

   CVelocity() = default;
   CVelocity( float vx, float vy, float vz ) :
      velocity( vx, vy, vz )
   {}
   CVelocity( const glm::vec3& vel ) :
      velocity( vel )
   {}
};
