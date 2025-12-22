#pragma once

namespace Entity
{
using Entity = uint64_t;
}

struct CParent
{
   Entity::Entity parent { 0 }; //{ Entity::InvalidEntityID };
   CParent() = default;
   CParent( Entity::Entity parent ) :
      parent( parent )
   {}
};
