#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct CTransform
{
   glm::vec3 position { 0.0f, 0.0f, 0.0f };
   glm::vec3 prevPosition { 0.0f, 0.0f, 0.0f };

   glm::vec3 rotation { 0.0f, 0.0f, 0.0f }; // Euler angles (degrees)
   glm::vec3 prevRotation { 0.0f, 0.0f, 0.0f };

   glm::vec3 scale { 1.0f, 1.0f, 1.0f };

   CTransform() = default;
   CTransform( float x, float y, float z ) :
      position( x, y, z ),
      prevPosition( x, y, z )
   {}
   CTransform( const glm::vec3& pos ) :
      position( pos ),
      prevPosition( pos )
   {}

   void RecordPrev() noexcept
   {
      prevPosition = position;
      prevRotation = rotation;
   }
};
