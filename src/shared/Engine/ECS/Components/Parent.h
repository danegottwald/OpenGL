#pragma once

namespace Entity
{
using Entity = uint64_t;
}

struct CParent
{
   Entity::Entity parent { 0 };                     //{ Entity::InvalidEntityID };
   glm::vec3      localOffset { 0.0f, 0.0f, 0.0f }; // position offset relative to parent
};
