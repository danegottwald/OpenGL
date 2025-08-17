#pragma once

#include <Renderer/Mesh.h>

struct CMesh
{
   std::shared_ptr< IMesh > mesh;
   CMesh( std::shared_ptr< IMesh > mesh ) :
      mesh( std::move( mesh ) )
   {}
};
