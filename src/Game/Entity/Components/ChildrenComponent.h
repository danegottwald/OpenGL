#pragma once

#include <vector>

namespace Entity
{
using Entity = uint64_t;
}

struct ChildrenComponent
{
   std::vector< Entity::Entity > children;
   ChildrenComponent() = default;
   ChildrenComponent( const std::vector< Entity::Entity >& childrenList ) :
      children( childrenList )
   {}
};
