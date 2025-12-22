#pragma once

// Forward Declarations
class Level;


struct Ray
{
   glm::vec3 origin {};
   glm::vec3 direction {};
   float     maxDistance { 3.0f };
};


struct RaycastResult
{
   glm::ivec3 block {};
   glm::vec3  point {};
   glm::ivec3 faceNormal {};
   float      distance { 0.0f };
};


Ray                            CreateRay( const glm::vec3& origin, const glm::quat& rotation, float maxDistance );
std::optional< RaycastResult > TryRaycast( const Level& level, const Ray& ray );