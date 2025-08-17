#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct CTransform
{
   glm::vec3 scale { 1.0f, 1.0f, 1.0f };
   glm::vec3 position { 0.0f, 0.0f, 0.0f };
   glm::quat rotation { 1.0f, 0.0f, 0.0f, 0.0f }; // w, x, y, z

   CTransform() = default;
   CTransform( float x, float y, float z ) :
      position( x, y, z )
   {}
   CTransform( const glm::vec3& pos ) :
      position( pos )
   {}
};
