#pragma once

struct CPhysics
{
   glm::vec3 bbMin { -0.5f, -0.5f, -0.5f }; // bounding box min
   glm::vec3 bbMax { 0.5f, 0.5f, 0.5f };    // bounding box max
   void      SetBoundingBox( float halfExtent ) { SetBoundingBox( glm::vec3( -halfExtent ), glm::vec3( halfExtent ) ); }
   void      SetBoundingBox( const glm::vec3& min, const glm::vec3& max )
   {
      bbMin = min;
      bbMax = max;
   }

   bool  fOnGround { false };
   float bounciness { 0.0f }; // [0.0, 1.0] : 0 = no bounce, 1 = perfectly elastic
};