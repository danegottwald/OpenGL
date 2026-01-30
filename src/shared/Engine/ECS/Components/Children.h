#pragma once

#include "pch_shared.h"

namespace Entity
{
using Entity = uint64_t;
}

struct CChildren
{
   std::vector< Entity::Entity > children;
   CChildren() = default;
   CChildren( const std::vector< Entity::Entity >& childrenList ) :
      children( childrenList )
   {}
};
