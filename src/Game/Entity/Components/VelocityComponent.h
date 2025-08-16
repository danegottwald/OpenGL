#pragma once
#include <glm/glm.hpp>

struct VelocityComponent
{
   glm::vec3 velocity { 0.0f, 0.0f, 0.0f };

   VelocityComponent() = default;
   VelocityComponent( float vx, float vy, float vz ) :
      velocity( vx, vy, vz )
   {}
   VelocityComponent( const glm::vec3& vel ) :
      velocity( vel )
   {}
};
