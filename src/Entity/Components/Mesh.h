#pragma once

struct CMesh
{
   uint32_t vao { 0 };
   uint32_t indexCount { 0 };

   CMesh() = default;
   CMesh( uint32_t vao, uint32_t indexCount ) :
      vao( vao ),
      indexCount( indexCount )
   {}
}
