#pragma once

namespace Entity
{
using Entity = uint64_t;
}

struct ParentComponent
{
   Entity::Entity parent { 0 }; //{ Entity::InvalidEntityID };
   ParentComponent() = default;
   ParentComponent( Entity::Entity parent ) :
      parent( parent )
   {}
};
