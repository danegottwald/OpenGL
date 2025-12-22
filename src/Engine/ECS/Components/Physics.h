#pragma once

struct CPhysics
{
   glm::vec3 halfExtents { 0.5f, 0.5f, 0.5f }; // default half extents
   bool      fOnGround { false };
   float     bounciness { 0.0f }; // [0.0, 1.0] - 0 = no bounce, 1 = perfectly elastic
};